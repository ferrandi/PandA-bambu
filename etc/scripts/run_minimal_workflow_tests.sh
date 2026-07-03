#!/usr/bin/env bash
set -euo pipefail

script_dir="$(dirname "$(readlink -e "$0")")"
repo_root="$(readlink -e "${script_dir}/../..")"

all_groups=(
   build
   cppcheck
   acmath
   specific
   interface
   axi
   gcc-simple
   gcc-pretty
   gcc-vhdl
   discrepancy
   softfloat-preliminary
   softfloat
   libm-preliminary
   libm
   chstone
   openroad
   memarch
   openmp
   panda-bench
   make-checks
)

build_dir="${repo_root}/build"
report_root="${repo_root}/test-reports/minimal"
cppcheck_dir="${repo_root}/cppcheck-report"
compile_db=""
dist_dir=""
settings_file=""
dry_run=0
strict_prereqs=0
list_groups=0
restart_mode=0
only_groups_csv=""
skip_groups_csv=""
machine_threads="$(nproc 2>/dev/null || echo 1)"
host_jobs="${J:-${machine_threads}}"
host_threads="${OMP_NUM_THREADS:-${host_jobs}}"
parallel_backend=2
state_file=""
last_run_shell_skipped=0

declare -A selected_groups=()
declare -A completed_steps=()

usage() {
   cat <<EOF
Usage: $(basename "$0") [options]

Replicates the locally runnable test/analysis commands from .github/workflows/minimal.yml.
This script intentionally skips CI-only wrappers such as artifact upload/download,
GitHub path filters, reusable workflow dispatch, and perf-check result publication.

Options:
  --build-dir DIR        CMake build directory to reuse (default: ${build_dir})
  --report-dir DIR       Root directory for junit/perf outputs (default: ${report_root})
  --cppcheck-dir DIR     Output directory for cppcheck reports (default: ${cppcheck_dir})
  --compile-db PATH      Compilation database for cppcheck (default: <build-dir>/compile_commands.json)
  --dist-dir DIR         Distribution/install directory containing settings.sh
  --settings FILE        settings.sh to source before distribution-dependent tests
  --jobs N|all, -j N|all Parallel jobs to export as J (default: ${host_jobs}, capped at ${machine_threads})
  --threads N|all, -t N|all
                         Parallel threads to export as OMP_NUM_THREADS (default: ${host_threads}, capped at ${machine_threads})
  --parallel-backend N|all
                         Parallel backend workers for the workflow groups that use it (default: ${parallel_backend}, capped at ${machine_threads})
  --only g1,g2           Run only the selected groups
  --skip g1,g2           Skip the selected groups
  --restart              Resume from the saved state file and forward --restart to wrapped mantis-based scripts
  --strict-prereqs       Fail instead of skipping groups with missing prerequisites
  --dry-run              Print commands without executing them
  --list-groups          List available groups and exit
  -h, --help             Show this help text

Groups:
  ${all_groups[*]}
EOF
}

list_available_groups() {
   printf '%s\n' "${all_groups[@]}"
}

normalize_parallel_value() {
   local name="$1"
   local raw="$2"
   local normalized

   if [[ "${raw}" == "all" ]]; then
      printf '%s' "${machine_threads}"
      return 0
   fi

   if [[ ! "${raw}" =~ ^[0-9]+$ ]]; then
      printf '[minimal] ERROR invalid %s value: %s\n' "${name}" "${raw}" >&2
      exit 1
   fi

   if (( raw < 1 )); then
      printf '[minimal] ERROR %s must be >= 1 (got %s)\n' "${name}" "${raw}" >&2
      exit 1
   fi

   normalized="${raw}"
   if (( normalized > machine_threads )); then
      printf '[minimal] WARN  capping %s from %s to %s (local machine limit)\n' "${name}" "${raw}" "${machine_threads}" >&2
      normalized="${machine_threads}"
   fi
   printf '%s' "${normalized}"
}

load_completed_steps() {
   [[ -f "${state_file}" ]] || return 0
   local line
   while IFS= read -r line; do
      [[ -n "${line}" ]] || continue
      completed_steps["${line}"]=1
   done < "${state_file}"
}

mark_step_done() {
   local label="$1"
   (( dry_run )) && return 0
   completed_steps["${label}"]=1
   printf '%s\n' "${label}" >> "${state_file}"
}

step_completed() {
   [[ "${completed_steps[$1]:-0}" -eq 1 ]]
}

trim_spaces() {
   local value="$1"
   value="${value#"${value%%[![:space:]]*}"}"
   value="${value%"${value##*[![:space:]]}"}"
   printf '%s' "${value}"
}

split_csv() {
   local csv="$1"
   local -n out_ref=$2
   out_ref=()
   [[ -z "${csv}" ]] && return 0
   local item
   IFS=',' read -r -a out_ref <<< "${csv}"
   for item in "${!out_ref[@]}"; do
      out_ref[$item]="$(trim_spaces "${out_ref[$item]}")"
   done
}

init_selected_groups() {
   local group
   for group in "${all_groups[@]}"; do
      selected_groups["${group}"]=1
   done

   if [[ -n "${only_groups_csv}" ]]; then
      for group in "${all_groups[@]}"; do
         selected_groups["${group}"]=0
      done
      local only_groups=()
      split_csv "${only_groups_csv}" only_groups
      for group in "${only_groups[@]}"; do
         [[ -n "${group}" ]] && selected_groups["${group}"]=1
      done
   fi

   if [[ -n "${skip_groups_csv}" ]]; then
      local skip_groups=()
      split_csv "${skip_groups_csv}" skip_groups
      for group in "${skip_groups[@]}"; do
         [[ -n "${group}" ]] && selected_groups["${group}"]=0
      done
   fi
}

is_selected() {
   [[ "${selected_groups[$1]:-0}" -eq 1 ]]
}

log() {
   printf '[minimal] %s\n' "$*"
}

skip_or_fail() {
   local group="$1"
   local reason="$2"
   if (( strict_prereqs )); then
      printf '[minimal] ERROR [%s] %s\n' "${group}" "${reason}" >&2
      exit 1
   fi
   printf '[minimal] SKIP  [%s] %s\n' "${group}" "${reason}" >&2
   return 1
}

require_command() {
   local group="$1"
   local cmd="$2"
   command -v "${cmd}" >/dev/null 2>&1 || skip_or_fail "${group}" "missing command '${cmd}'"
}

run_shell() {
   local label="$1"
   shift
   local command="$1"
   if (( restart_mode )) && step_completed "${label}"; then
      last_run_shell_skipped=1
      log "SKIP [restart] ${label}"
      return 0
   fi
   last_run_shell_skipped=0
   log ">>> ${label}"
   if (( dry_run )); then
      printf '%s\n' "${command}"
      return 0
   fi
   set +e
   (
      cd "${repo_root}"
      eval "${command}"
   )
   local rc=$?
   set -e
   if (( rc != 0 )); then
      printf '[minimal] ERROR [%s] command failed with exit code %d\n' "${label}" "${rc}" >&2
      return "${rc}"
   fi
   mark_step_done "${label}"
}

discover_mantis_outdir() {
   local script_path="$1"
   shift
   local wrapper_args="$*"
   local script_abs
   script_abs="$(readlink -e "${repo_root}/${script_path}")"
   [[ -n "${script_abs}" ]] || return 1

   local tmpdir capture
   tmpdir="$(mktemp -d)"
   capture="${tmpdir}/mantis-argv.bin"
   cat > "${tmpdir}/python3" <<'EOF'
#!/usr/bin/env bash
if [[ $# -gt 0 && "$(basename "$1")" == "mantis.py" && -n "${COPILOT_MANTIS_CAPTURE:-}" ]]; then
   printf '%s\0' "$@" > "${COPILOT_MANTIS_CAPTURE}"
fi
exit 0
EOF
   chmod +x "${tmpdir}/python3"

   set +e
   (
      cd "${repo_root}"
      eval "env PATH=\"${tmpdir}:\$PATH\" COPILOT_MANTIS_CAPTURE=\"${capture}\" bash \"${script_abs}\" ${wrapper_args}" \
         >/dev/null 2>&1
   )
   set -e

   if [[ ! -s "${capture}" ]]; then
      rm -rf "${tmpdir}"
      return 1
   fi

   local outdir
   outdir="$(python3 - "${capture}" <<'PY'
from pathlib import Path
import sys

argv = [item.decode(errors="ignore") for item in Path(sys.argv[1]).read_bytes().split(b"\0") if item]
outdir = None
for idx, arg in enumerate(argv):
    if arg in ("-o", "--output") and idx + 1 < len(argv):
        outdir = argv[idx + 1]
        break
    if arg.startswith("--output="):
        outdir = arg.split("=", 1)[1]
        break
    if arg.startswith("-o") and len(arg) > 2:
        outdir = arg[2:]
        break
if outdir:
    print(outdir)
PY
)"
   rm -rf "${tmpdir}"
   [[ -n "${outdir}" ]] || return 1
   if [[ "${outdir}" = /* ]]; then
      printf '%s' "${outdir}"
   else
      printf '%s' "${repo_root}/${outdir}"
   fi
}

build_dist_restart_args() {
   local label="$1"
   local script_path="$2"
   shift 2
   local extra_args="$*"
   local outdir=""
   outdir="$(discover_mantis_outdir "${script_path}" ${extra_args})" || {
      printf '[minimal] INFO  [restart] %s: unable to detect mantis output directory, rerunning fresh\n' "${label}" >&2
      printf ''
      return 0
   }

   if (( ! restart_mode )); then
      if [[ -e "${outdir}" ]]; then
         printf '[minimal] INFO  [fresh] %s: removing generated output %s\n' "${label}" "${outdir}" >&2
         (( dry_run )) || rm -rf "${outdir}"
      fi
      printf ''
      return 0
   fi

   local exec_list="${outdir}/exec_list.json"
   if [[ -f "${exec_list}" ]]; then
      printf ' --restart'
      return 0
   fi

   if [[ -e "${outdir}" ]]; then
      printf '[minimal] WARN  [restart] %s: removing stale generated output %s\n' "${label}" "${outdir}" >&2
      (( dry_run )) || rm -rf "${outdir}"
   else
      printf '[minimal] INFO  [restart] %s: no previous mantis state found, rerunning fresh\n' "${label}" >&2
   fi
   printf ''
}

prepare_output_roots() {
   state_file="${report_root}/.minimal-state"
   if (( restart_mode )); then
      (( dry_run )) || mkdir -p "${report_root}"
      load_completed_steps
      return 0
   fi
   if (( dry_run )); then
      return 0
   fi
   rm -rf "${report_root}"
   mkdir -p "${report_root}"
   rm -f "${state_file}"
   if is_selected cppcheck; then
      rm -rf "${cppcheck_dir}"
   fi
}

cppcheck_error_summary() {
   python3 - <<PY
from pathlib import Path
import xml.etree.ElementTree as ET
p = Path(r"${cppcheck_dir}/cppcheck.xml")
root = ET.parse(p).getroot()
seen = 0
for err in root.findall('.//error'):
    if err.attrib.get('severity') != 'error':
        continue
    seen += 1
    msg = err.attrib.get('msg', '')
    err_id = err.attrib.get('id', '')
    loc = err.find('location')
    file_name = loc.attrib.get('file', '') if loc is not None else ''
    print(f"[minimal]   {seen}. {err_id}: {msg}")
    if file_name:
        print(f"[minimal]      file: {file_name}")
    if seen == 10:
        break
PY
}

filter_cppcheck_db() {
   local db_path="$1"
   local build_path="$2"
   local ext_path="$3"
   local out_path="$4"
   python3 - "$db_path" "$build_path" "$ext_path" "$out_path" <<'PY'
import json
import sys
from pathlib import Path

db_path = Path(sys.argv[1]).resolve()
build_path = Path(sys.argv[2]).resolve()
ext_path = Path(sys.argv[3]).resolve()
out_path = Path(sys.argv[4]).resolve()

db = json.loads(db_path.read_text())
filtered = []
for obj in db:
    file_name = obj.get("file", "")
    try:
        file_path = Path(file_name).resolve()
    except Exception:
        continue
    if str(file_path).startswith(str(build_path)):
        continue
    if str(file_path).startswith(str(ext_path)):
        continue
    filtered.append(obj)

out_path.write_text(json.dumps(filtered, indent=2))
PY
}

build_report_args() {
   local subdir="$1"
   local report_dir="${report_root}/${subdir}"
   mkdir -p "${report_dir}"
   printf -- '--returnfail --junitdir="%s" --summary="%s/perf.xml"' "${report_dir}" "${report_dir}"
}

discover_settings_file() {
   if [[ -n "${settings_file}" ]]; then
      return 0
   fi

   if [[ -n "${dist_dir}" && -f "${dist_dir}/settings.sh" ]]; then
      settings_file="${dist_dir}/settings.sh"
      return 0
   fi

   if [[ -f "${build_dir}/CMakeCache.txt" ]]; then
      local install_prefix
      install_prefix="$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' "${build_dir}/CMakeCache.txt" | tail -n 1)"
      if [[ -n "${install_prefix}" && -f "${install_prefix}/settings.sh" ]]; then
         settings_file="${install_prefix}/settings.sh"
         return 0
      fi
   fi

   if [[ -f "${repo_root}/local_install_dir/settings.sh" ]]; then
      settings_file="${repo_root}/local_install_dir/settings.sh"
      return 0
   fi

   if [[ -n "${BAMBU_HLS:-}" && -f "${BAMBU_HLS}/settings.sh" ]]; then
      settings_file="${BAMBU_HLS}/settings.sh"
      return 0
   fi

   return 1
}

dist_prefix() {
   discover_settings_file || return 1
   printf 'source "%s"; export J="%s"; export OMP_NUM_THREADS="%s"' \
      "${settings_file}" "${host_jobs}" "${host_threads}"
}

run_dist_script() {
   local label="$1"
   local script_path="$2"
   local subdir="$3"
   shift 3
   local extra_args="$*"
   local prefix
   prefix="$(dist_prefix)" || {
      skip_or_fail "${label}" "missing settings.sh (use --settings or --dist-dir, or build+install first)"
      return 0
   }
   local report_args
   report_args="$(build_report_args "${subdir}")"
   local restart_args=""
   restart_args="$(build_dist_restart_args "${label}" "${script_path}" ${extra_args})"
   run_shell "${label}" "${prefix}; ${script_path} ${report_args}${restart_args} ${extra_args}"
}

run_build_group() {
   local group="build"
   require_command "${group}" cmake || return 0
   [[ -d "${build_dir}" ]] || skip_or_fail "${group}" "build directory '${build_dir}' does not exist"
   run_shell "${group}" "cmake --build \"${build_dir}\" -j \"${host_jobs}\" && cmake --install \"${build_dir}\""
   discover_settings_file || true
}

run_cppcheck_group() {
   local group="cppcheck"
   require_command "${group}" python3 || return 0
   require_command "${group}" cppcheck || return 0
   require_command "${group}" cppcheck-htmlreport || return 0
   local db="${compile_db:-${build_dir}/compile_commands.json}"
   [[ -f "${db}" ]] || skip_or_fail "${group}" "compilation database '${db}' does not exist"
   mkdir -p "${cppcheck_dir}"
   local filtered="${build_dir}/compile_commands.filtered.json"
   run_shell "${group}" "filter_cppcheck_db \"${db}\" \"${build_dir}\" \"${repo_root}/ext\" \"${filtered}\" && cppcheck --enable=all --force --suppress=missingIncludeSystem --suppress=unknownMacro --inline-suppr -i ext --project=\"${filtered}\" --xml --xml-version=2 --output-file=\"${cppcheck_dir}/cppcheck.xml\" -j${host_jobs} && cppcheck-htmlreport --source-dir=. --title=Bambu --file=\"${cppcheck_dir}/cppcheck.xml\" --report-dir=\"${cppcheck_dir}\""
   (( last_run_shell_skipped )) && return 0
   (( dry_run )) && return 0
   local error_count
   error_count="$(python3 - <<PY
from pathlib import Path
text = Path(r"${cppcheck_dir}/cppcheck.xml").read_text(errors="ignore").lower()
print(text.count('severity="error"'))
PY
   )"
   if [[ "${error_count}" != "0" ]]; then
      printf '[minimal] ERROR [%s] cppcheck detected %s error(s). See %s/index.html\n' \
         "${group}" "${error_count}" "${cppcheck_dir}" >&2
      cppcheck_error_summary >&2
      return 1
   fi
   log "[${group}] cppcheck completed with no severity=error entries"
}

run_acmath_group() {
   local group="acmath"
   local -a cases=(
      "g++|"
      "g++|-std=c++17"
      "clang++|"
      "clang++|-std=c++17"
      "clang++-16|-std=c++17"
   )
   local entry compiler flags out_dir
   for entry in "${cases[@]}"; do
      IFS='|' read -r compiler flags <<< "${entry}"
      command -v "${compiler}" >/dev/null 2>&1 || {
         skip_or_fail "${group}" "compiler '${compiler}' not available"
         continue
      }
      out_dir="$(mktemp -d "/tmp/ac_math_${compiler//+/x}_XXX")"
      run_shell "${group}:${compiler}${flags:+ ${flags}}" "env MAKEFLAGS='-j${host_jobs}' CXX='${compiler}' CXXUSERFLAGS='${flags}' ./etc/ac_math/ac_math_test.sh \"${out_dir}\""
   done
}

run_specific_group() {
   local ids=("" "2" "3" "4" "5" "6")
   local id
   for id in "${ids[@]}"; do
      run_dist_script "specific${id:-0}" "./panda_regressions/hls/bambu_specific_test${id}.sh" "specific${id:-0}" "-c=--simulator=VERILATOR -c=--parallel-backend=${parallel_backend}"
   done
}

run_interface_group() {
   run_dist_script "interface:MODELSIM" "./panda_regressions/hls/bambu_interface_test.sh" "interface-modelsim" "-c=--simulator=MODELSIM"
   run_dist_script "interface:VERILATOR" "./panda_regressions/hls/bambu_interface_test.sh" "interface-verilator" "-c=--simulator=VERILATOR"
}

run_axi_group() {
   local prefix
   prefix="$(dist_prefix)" || {
      skip_or_fail "axi" "missing settings.sh (use --settings or --dist-dir, or build+install first)"
      return 0
   }
   run_shell "axi" "${prefix}; ./panda_regressions/hls/bambu_axi_verification.sh"
}

run_gcc_simple_group() {
   local -a cases=(
      "clang-4.0|generic_gcc_regression_simple_eg.sh|-c=--compiler=I386_CLANG4 -c=--simulator=MODELSIM"
      "clang-6.0|generic_gcc_regression_simple_bambu.sh|-c=--compiler=I386_CLANG6 -c=--simulator=VERILATOR"
      "clang-7|generic_gcc_regression_simple_bambu.sh|-c=--compiler=I386_CLANG7 -c=--simulator=VERILATOR"
      "clang-11|generic_gcc_regression_simple_bambu.sh|-c=--compiler=I386_CLANG11 -c=--simulator=VERILATOR"
      "clang-13|generic_gcc_regression_simple_eg_ext_pipelined.sh|-c=--compiler=I386_CLANG13 -c=--simulator=VERILATOR"
      "clang-16|generic_gcc_regression_simple_eg_ext_pipelined.sh|-c=--compiler=I386_CLANG16 -c=--simulator=VERILATOR -c=-Wno-int-conversion"
      "clang-19|generic_gcc_regression_simple_eg_ext_pipelined.sh|-c=--compiler=I386_CLANG19 -c=--simulator=VERILATOR -c=-Wno-int-conversion"
   )
   local entry setup script args
   for entry in "${cases[@]}"; do
      IFS='|' read -r setup script args <<< "${entry}"
      run_dist_script "gcc-simple:${setup}" "./panda_regressions/hls/${script}" "gcc-simple-${setup}" "${args}"
   done
}

run_gcc_pretty_group() {
   run_dist_script "gcc-pretty" "./panda_regressions/hls/generic_gcc_regression_simple_bambu_pretty_print.sh" \
      "gcc-pretty" "-c=--compiler=I386_CLANG16 -c=--simulator=VERILATOR -c=--parallel-backend=${parallel_backend}"
}

run_gcc_vhdl_group() {
   run_dist_script "gcc-vhdl" "./panda_regressions/hls/generic_gcc_regression_simple_bambu_vhdl.sh" \
      "gcc-vhdl" "-c=--compiler=I386_CLANG16 -c=--simulator=MODELSIM"
}

run_discrepancy_group() {
   run_dist_script "discrepancy" "./panda_regressions/hls/generic_discrepancy_eg_bambu.sh" \
      "discrepancy" "-c=--compiler=I386_CLANG16"
}

run_softfloat_preliminary_group() {
   run_shell "softfloat-preliminary" "./panda_regressions/hls/test_softfloat.sh"
}

run_softfloat_group() {
   run_dist_script "softfloat" "./panda_regressions/hls/multi_softfloat-tests.sh" \
      "softfloat" "-c=--simulator=VERILATOR -c=--parallel-backend=${parallel_backend}"
}

run_libm_preliminary_group() {
   run_shell "libm-preliminary" "./panda_regressions/hls/test_libm.sh"
}

run_libm_group() {
   local -a cases=(
      "clang-4.0|-c=--compiler=I386_CLANG4 -c=--simulator=VERILATOR"
      "clang-6.0|-c=--compiler=I386_CLANG6 -c=--simulator=VERILATOR"
      "clang-7.0|-c=--compiler=I386_CLANG7 -c=--simulator=VERILATOR"
      "clang-11|-c=--compiler=I386_CLANG11 -c=--simulator=MODELSIM"
      "clang-13|-c=--compiler=I386_CLANG13 -c=--simulator=VERILATOR"
      "clang-16|-c=--compiler=I386_CLANG16 -c=--simulator=VERILATOR"
      "clang-19|-c=--compiler=I386_CLANG19 -c=--simulator=MODELSIM"
   )
   local entry setup args
   for entry in "${cases[@]}"; do
      IFS='|' read -r setup args <<< "${entry}"
      run_dist_script "libm:${setup}" "./panda_regressions/hls/generic_libm-tests.sh" \
         "libm-${setup}" "${args} -c=--speculative-sdc-scheduling"
   done
}

run_chstone_group() {
   run_dist_script "chstone" "./panda_regressions/hls/multi_CHStone-frontend.sh" \
      "chstone" "-c=--simulator=MODELSIM"
}

run_openroad_group() {
   require_command "openroad" docker || return 0
   local prefix
   prefix="$(dist_prefix)" || {
      skip_or_fail "openroad" "missing settings.sh (use --settings or --dist-dir, or build+install first)"
      return 0
   }
   local report_args
   report_args="$(build_report_args "openroad")"
   run_shell "openroad" "${prefix}; OPENROAD_LAUNCH=\"\$(find \"\${BAMBU_HLS}\" -type f -path '*/libtech/backend/openroad/flow/launch.sh' | head -n 1)\"; [[ -n \"\${OPENROAD_LAUNCH}\" ]]; OR_IMAGE_DEFAULT=\"\$(sed -n 's/.*local docker_image=\"\${OR_IMAGE:-\\([^\"]\\+\\)}\".*/\\1/p' \"\${OPENROAD_LAUNCH}\" | head -n 1)\"; [[ -n \"\${OR_IMAGE_DEFAULT}\" ]]; unset OPENROAD_FLOW_DATA_INSTALLDIR; export OPENROAD_USE_DOCKER=1; export OR_IMAGE=\"\${OR_IMAGE_DEFAULT}\"; docker image inspect \"\${OR_IMAGE}\" >/dev/null 2>&1 || docker pull \"\${OR_IMAGE}\"; ./examples/panda_bench_openroad.sh ${report_args} -c=--evaluation"
}

run_memarch_group() {
   local -a cases=(
      "clang-16|generic_CHStone-memarch1.sh|-c=--compiler=I386_CLANG16"
      "clang-16-vhdl|generic_CHStone-memarch1.sh|-c=--compiler=I386_CLANG16 -c=-wH -c=--simulator=MODELSIM"
      "clang-13|generic_CHStone-memarch2.sh|-c=--compiler=I386_CLANG13"
      "clang-13-vhdl|generic_CHStone-memarch2.sh|-c=--compiler=I386_CLANG13 -c=-wH -c=--simulator=MODELSIM"
      "clang-11|generic_gcc-memarch3.sh|-c=--compiler=I386_CLANG11"
      "clang-11-vhdl|generic_gcc-memarch3.sh|-c=--compiler=I386_CLANG11 -c=-wH -c=--simulator=MODELSIM"
   )
   local entry label script args
   for entry in "${cases[@]}"; do
      IFS='|' read -r label script args <<< "${entry}"
      run_dist_script "memarch:${label}" "./panda_regressions/hls/${script}" "memarch-${label}" "${args}"
   done
}

run_openmp_group() {
   local common="-c=--simulator=MODELSIM"
   local -a cases=(
      "functional|./examples/OpenMP/functional/openmp_functional.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--compiler=I386_CLANG13"
      "arbiter|./examples/OpenMP/functional/openmp_arbiter.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--compiler=I386_CLANG13"
      "banked|./examples/OpenMP/banked-memory/openmp_banked.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--compiler=I386_CLANG13"
      "banked_profiling|./examples/OpenMP/banked-memory/openmp_banked.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--noc-profiling -c=--compiler=I386_CLANG13"
      "banked_cache|./examples/OpenMP/banked-memory/openmp_banked_cache.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--compiler=I386_CLANG13"
      "banked_allocation|./examples/OpenMP/banked-memory/openmp_banked_allocation.sh|-c=--clock-period=10 -c=--device-name=xc7vx690t-3ffg1930 -c=--assert-debug -c=--compiler=I386_CLANG13"
   )
   local entry label script args
   for entry in "${cases[@]}"; do
      IFS='|' read -r label script args <<< "${entry}"
      run_dist_script "openmp:${label}" "${script}" "openmp-${label}" "${common} ${args}"
   done
}

run_panda_bench_group() {
   local -a cases=(
      "altera|./examples/panda_bench_altera.sh|-c=--simulate"
      "hw|./examples/panda_bench_hw.sh|-c=--evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS"
      "lattice|./examples/panda_bench_lattice.sh|-c=--simulate"
      "nanoxplore|./examples/panda_bench_nanoxplore.sh|-c=--simulate"
      "sim|./examples/panda_bench_sim.sh|-c=--simulate"
      "xilinx_vvd|./examples/panda_bench_xilinx_vvd.sh|-c=--evaluation"
   )
   local entry label script args
   for entry in "${cases[@]}"; do
      IFS='|' read -r label script args <<< "${entry}"
      run_dist_script "panda-bench:${label}" "${script}" "panda-bench-${label}" "${args}"
   done
}

run_make_checks_group() {
   local prefix
   prefix="$(dist_prefix)" || {
      skip_or_fail "make-checks" "missing settings.sh (use --settings or --dist-dir, or build+install first)"
      return 0
   }
   run_shell "make-checks" "${prefix}; ./examples/cpp_examples/fir_filter/bambu.sh && ./examples/cpp_examples/gcd_example/bambu.sh && ./examples/crypto_designs/multi.sh"
}

while (($#)); do
   case "$1" in
      --build-dir)
         build_dir="$(readlink -m "$2")"
         shift 2
         ;;
      --report-dir)
         report_root="$(readlink -m "$2")"
         shift 2
         ;;
      --cppcheck-dir)
         cppcheck_dir="$(readlink -m "$2")"
         shift 2
         ;;
      --compile-db)
         compile_db="$(readlink -m "$2")"
         shift 2
         ;;
      --dist-dir)
         dist_dir="$(readlink -m "$2")"
         shift 2
         ;;
      --settings)
         settings_file="$(readlink -m "$2")"
         shift 2
         ;;
      --jobs|-j)
         host_jobs="$2"
         shift 2
         ;;
      --threads|-t)
         host_threads="$2"
         shift 2
         ;;
      --parallel-backend)
         parallel_backend="$2"
         shift 2
         ;;
      --only)
         only_groups_csv="$2"
         shift 2
         ;;
      --skip)
         skip_groups_csv="$2"
         shift 2
         ;;
      --restart)
         restart_mode=1
         shift
         ;;
      --strict-prereqs)
         strict_prereqs=1
         shift
         ;;
      --dry-run)
         dry_run=1
         shift
         ;;
      --list-groups)
         list_groups=1
         shift
         ;;
      -h|--help)
         usage
         exit 0
         ;;
      *)
         printf 'Unknown option: %s\n' "$1" >&2
         usage >&2
         exit 1
         ;;
   esac
done

if (( list_groups )); then
   list_available_groups
   exit 0
fi

host_jobs="$(normalize_parallel_value jobs "${host_jobs}")"
host_threads="$(normalize_parallel_value threads "${host_threads}")"
parallel_backend="$(normalize_parallel_value parallel-backend "${parallel_backend}")"

init_selected_groups
prepare_output_roots

for group in "${all_groups[@]}"; do
   is_selected "${group}" || continue
   case "${group}" in
      build) run_build_group ;;
      cppcheck) run_cppcheck_group ;;
      acmath) run_acmath_group ;;
      specific) run_specific_group ;;
      interface) run_interface_group ;;
      axi) run_axi_group ;;
      gcc-simple) run_gcc_simple_group ;;
      gcc-pretty) run_gcc_pretty_group ;;
      gcc-vhdl) run_gcc_vhdl_group ;;
      discrepancy) run_discrepancy_group ;;
      softfloat-preliminary) run_softfloat_preliminary_group ;;
      softfloat) run_softfloat_group ;;
      libm-preliminary) run_libm_preliminary_group ;;
      libm) run_libm_group ;;
      chstone) run_chstone_group ;;
      openroad) run_openroad_group ;;
      memarch) run_memarch_group ;;
      openmp) run_openmp_group ;;
      panda-bench) run_panda_bench_group ;;
      make-checks) run_make_checks_group ;;
      *)
         printf 'Internal error: unsupported group %s\n' "${group}" >&2
         exit 1
         ;;
   esac
done

log "Completed selected minimal workflow groups."
