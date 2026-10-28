#!/usr/bin/env bash
# Generate and simulate in-place and distinct-owner duplex kernels with ModelSim.
# Usage: BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]
set -euo pipefail
ulimit -c 0

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
FAMILY="$ROOT/panda_regressions/hls/bambu_axi_burst"
HELPERS="$FAMILY/parallel_helpers.sh"
MODELSIM_HELPERS="$TESTS/modelsim_helpers.sh"
SOURCE="$FAMILY/bambu_axi_burst_duplex.cpp"
COPY_SOURCE="$FAMILY/bambu_axi_burst_duplex_copy.cpp"
COPY_TB="$FAMILY/bambu_axi_burst_duplex_copy_tb.cpp"
PREFIX_SOURCE="$FAMILY/bambu_axi_burst_duplex_prefix.cpp"
TB="$TESTS/tb_generated_duplex_kernel.sv"
OBSERVER="$TESTS/duplex_load_observer.sv"
OBSERVER_UNIT_TB="$TESTS/tb_duplex_load_observer.sv"
PARALLELISM_CHECKER="$FAMILY/check_duplex_parallelism_results.py"
RUNNER="$TESTS/run_generated_duplex_kernel_tests.sh"
PROFILES=(symmetric asymmetric)
KINDS=(inplace copy)
CASES=(COUNTS_0_1_15_16_17 COUNT38_SPLITS PAGE_BOUNDARY REINVOKE_DEPENDENT REINVOKE_BASE_COUNT LAST_B_WAIT RRESP_FAULT_RESET BRESP_FAULT_RESET)
PREFIX_CASES=(READ_LONGER WRITE_LONGER ZERO_READ_WRITE_TAIL READ_ONLY ZERO_ZERO BOUNDS_CLAMP PAGE_SPLITS)

if (($# < 1 || $# > 2)); then
  echo "usage: BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]" >&2
  exit 2
fi
[[ ${BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX:-} == 1 ]] || {
  echo "refusing compiler/build/simulation unless BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 is set" >&2
  exit 2
}

BAMBU_ARG=$1
if [[ $BAMBU_ARG == */* ]]; then BAMBU=$(realpath -- "$BAMBU_ARG"); else
  BAMBU=$(command -v -- "$BAMBU_ARG" || true)
  [[ -n $BAMBU ]] && BAMBU=$(realpath -- "$BAMBU")
fi
[[ -n ${BAMBU:-} && -x $BAMBU ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
for tool in python3 sha256sum timeout; do
  command -v "$tool" >/dev/null || { echo "required tool missing: $tool" >&2; exit 2; }
done
[[ -s $SOURCE && -s $COPY_SOURCE && -s $COPY_TB && -s $PREFIX_SOURCE && -s $TB && -s $OBSERVER && -s $OBSERVER_UNIT_TB && -s $PARALLELISM_CHECKER && -s $HELPERS && -s $MODELSIM_HELPERS ]] || { echo "G2/G3 kernel source, observer, unit testbench, checker, or helper is missing" >&2; exit 2; }

PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r $SETTINGS ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -s $PLUGIN ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
source "$MODELSIM_HELPERS"
modelsim_resolve

OUT_ARG=${2:-"$PWD/out_generated_duplex_kernel"}
[[ ! -e $OUT_ARG && ! -L $OUT_ARG ]] || { echo "output directory must be fresh and not a symlink: $OUT_ARG" >&2; exit 2; }
OUT=$(realpath -m -- "$OUT_ARG")
SIM_JOBS=${SIM_JOBS:-2}
DUPLEX_G2_JOBS=${DUPLEX_G2_JOBS:-8}
G3_ITERATION_DUMPS=${DUPLEX_G3_ITERATION_DUMPS:-0}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ && $DUPLEX_G2_JOBS =~ ^[1-9][0-9]*$ ]] || {
  echo "SIM_JOBS and DUPLEX_G2_JOBS must be positive integers" >&2; exit 2;
}
[[ $G3_ITERATION_DUMPS == 0 || $G3_ITERATION_DUMPS == 1 ]] || {
  echo "DUPLEX_G3_ITERATION_DUMPS must be 0 or 1" >&2; exit 2;
}
SIM_JOBS=$((10#$SIM_JOBS)); DUPLEX_G2_JOBS=$((10#$DUPLEX_G2_JOBS))
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT" "$OUT/tmp" "$OUT/job_logs"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum -- "$1" | awk '{print $1}'; }
{
  printf 'git_head=%s\nroot=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nsettings_sha256=%s\nplugin=%s\nplugin_sha256=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)" "$ROOT" "$OUT" "$BAMBU" "$(digest "$BAMBU")" \
    "$SETTINGS" "$(digest "$SETTINGS")" "$PLUGIN" "$(digest "$PLUGIN")"
  printf 'inplace_source=%s\ninplace_source_sha256=%s\ncopy_source=%s\ncopy_source_sha256=%s\ncopy_golden=%s\ncopy_golden_sha256=%s\nprefix_source=%s\nprefix_source_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nobserver=%s\nobserver_sha256=%s\nobserver_unit_testbench=%s\nobserver_unit_testbench_sha256=%s\nparallelism_checker=%s\nparallelism_checker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$SOURCE" "$(digest "$SOURCE")" "$COPY_SOURCE" "$(digest "$COPY_SOURCE")" \
    "$COPY_TB" "$(digest "$COPY_TB")" "$PREFIX_SOURCE" "$(digest "$PREFIX_SOURCE")" \
    "$TB" "$(digest "$TB")" "$OBSERVER" "$(digest "$OBSERVER")" \
    "$OBSERVER_UNIT_TB" "$(digest "$OBSERVER_UNIT_TB")" \
    "$PARALLELISM_CHECKER" "$(digest "$PARALLELISM_CHECKER")" "$RUNNER" "$(digest "$RUNNER")"
  printf 'sim_jobs=%s\nscenario_jobs=%s\n' "$SIM_JOBS" "$DUPLEX_G2_JOBS"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  printf 'simulator=MODELSIM\nsimulator_source=%s\nsimulator_invocation=%s\n' "$MODELSIM_SOURCE" "$MODELSIM_VSIM"
  modelsim_version
} >"$OUT/provenance.txt"

COMMON=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --no-clean
  --extra-cc-options=-DNDEBUG
  --extra-cc-options=-DBAMBU)
profile_dir() {
  case $2 in
    copy) printf '%s/%s_copy' "$OUT" "$1" ;;
    prefix) printf '%s/%s_prefix' "$OUT" "$1" ;;
    *) printf '%s/%s' "$OUT" "$1" ;;
  esac
}

generate_one() {
  local profile=$1 kind=$2 profile_id=1 source_out source
  source=$SOURCE
  [[ $kind == copy ]] && source=$COPY_SOURCE
  [[ $kind == prefix ]] && source=$PREFIX_SOURCE
  source_out=$(profile_dir "$profile" "$kind")
  [[ $profile == symmetric ]] || profile_id=2
  mkdir "$source_out" "$source_out/tmp" "$source_out/generated" "$source_out/scenarios"
  local -a args=("${COMMON[@]}" "--extra-cc-options=-DDUPLEX_PROFILE=$profile_id")
  if [[ $kind == copy && $G3_ITERATION_DUMPS == 1 ]]; then
    args+=(--bambu-parameter=print-ir-manager=1
      --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based --print-dot)
  fi
  local read_b=16 read_o=1 read_fifo=64 write_b=16 write_o=1 write_fifo=32
  if [[ $profile == asymmetric ]]; then
    read_o=2; write_b=8; write_fifo=16
  fi
  {
    printf 'profile=%s\nkind=%s\nprofile_id=%s\nsource=%s\nsource_sha256=%s\n' "$profile" "$kind" "$profile_id" "$source" "$(digest "$source")"
    printf 'read_profile=B:%s,O:%s,FIFO:%s\nwrite_profile=B:%s,O:%s,FIFO:%s\n' "$read_b" "$read_o" "$read_fifo" "$write_b" "$write_o" "$write_fifo"
  printf 'argv='; printf '%q ' "$BAMBU" "$source" "${args[@]}"; printf '\n'
  } >"$source_out/compiler_argv.txt"
  if ! (cd "$source_out/generated" && TMPDIR="$source_out/tmp" TMP="$source_out/tmp" TEMP="$source_out/tmp" \
      "$BAMBU" "$source" "${args[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
    tail -n 100 "$source_out/generated/bambu.stderr.log" >&2 || true
    echo "Bambu generation failed for $profile; see $source_out/generated/" >&2; return 1
  fi
  local rtl="$source_out/generated/kernel.v" lib="$source_out/generated/panda_libtech.v"
  [[ -s $rtl && -s $lib ]] || { echo "missing generated kernel RTL/library for $profile" >&2; return 1; }
  python3 - "$rtl" "$lib" "$read_b" "$read_o" "$read_fifo" "$write_b" "$write_o" "$write_fifo" "$kind" <<'PY'
from pathlib import Path
import re
import sys

rtl_path, lib_path = map(Path, sys.argv[1:3])
rtl, lib = rtl_path.read_text(errors="replace"), lib_path.read_text(errors="replace")
if not re.search(r"(?m)^\s*module\s+kernel\s*\(", rtl):
    raise SystemExit("generated RTL has no kernel top module")
profiles = {
    "MinimalAXI4MasterPipelined": dict(B_MAX=sys.argv[3], MAX_OUTSTANDING=sys.argv[4], FIFO_DEPTH=sys.argv[5]),
    "MinimalAXI4MasterPipelinedWrite": dict(B_MAX=sys.argv[6], MAX_OUTSTANDING=sys.argv[7], FIFO_DEPTH=sys.argv[8]),
}
for module, expected in profiles.items():
    if not re.search(rf"(?m)^\s*module\s+{module}\s*\(", lib):
        raise SystemExit(f"generated library lacks {module}")
    matches = list(re.finditer(rf"\b{module}\s*#\s*\((.*?)\)\s*\w+\s*\(", rtl, re.S))
    if len(matches) != 1:
        raise SystemExit(f"kernel must instantiate exactly one {module}; found {len(matches)}")
    params = dict(re.findall(r"\.(\w+)\s*\(\s*(\d+)\s*\)", matches[0].group(1)))
    wrong = {key: (params.get(key), value) for key, value in expected.items() if params.get(key) != value}
    if wrong:
        raise SystemExit(f"{module} profile mismatch (actual, expected): {wrong}")
if "p_m_axi_shared_" not in rtl:
    raise SystemExit("generated kernel does not connect both engines to the shared AXI bundle")
if sys.argv[9] == "copy":
    top = re.search(r"(?ms)^module\s+kernel\s*\((.*?)\);(.*?)^endmodule", rtl)
    if not top or not re.search(r"\bsrc\b", top.group(1)) or not re.search(r"\bdst\b", top.group(1)):
        raise SystemExit("copy carrier top lacks distinct src and dst owners")
elif sys.argv[9] == "prefix":
    top = re.search(r"(?ms)^module\s+kernel\s*\((.*?)\);(.*?)^endmodule", rtl)
    if not top or not all(re.search(rf"\b{name}\b", top.group(1)) for name in
                          ("src", "dst", "read_bound", "write_bound", "n")):
        raise SystemExit("prefix carrier top lacks distinct owners and per-direction bounds")
PY
  printf 'GENERATED_DUPLEX_KERNEL_BUILD PASS PROFILE=%s KIND=%s\n' "$profile" "$kind"
}
generate_profile() {
  local pair=$1 profile=${1%%__*} kind=${1#*__}
  generate_one "$profile" "$kind"
}

source "$HELPERS"
BURST_QUIET_JOB_STATUS=1
BURST_JOB_LOG_DIR="$OUT/job_logs/build"; mkdir "$BURST_JOB_LOG_DIR"
declare -a profile_jobs=()
for profile in "${PROFILES[@]}"; do for kind in "${KINDS[@]}"; do profile_jobs+=("${profile}__${kind}"); done; done
for profile in "${PROFILES[@]}"; do profile_jobs+=("${profile}__prefix"); done
run_bounded_jobs "$SIM_JOBS" generate_profile "${profile_jobs[@]}"
for pair in "${profile_jobs[@]}"; do
  profile=${pair%%__*}; kind=${pair#*__}
  grep -Eq "^GENERATED_DUPLEX_KERNEL_BUILD PASS PROFILE=$profile KIND=$kind$" "$BURST_JOB_LOG_DIR/$pair.log" || {
    echo "missing generation completion marker for $profile/$kind" >&2; exit 1;
  }
done

build_one() {
  local profile=$1 kind=$2 out
  out=$(profile_dir "$profile" "$kind")
  local rtl="$out/generated/kernel.v" lib="$out/generated/panda_libtech.v"
  local -a overrides=(-gREAD_B_MAX=16 -gREAD_OUTSTANDING=1 -gREAD_FIFO_DEPTH=64
    -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=32)
  if [[ $profile == asymmetric ]]; then
    overrides=(-gREAD_B_MAX=16 -gREAD_OUTSTANDING=2 -gREAD_FIFO_DEPTH=64
      -gWRITE_B_MAX=8 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  fi
  local -a kind_def=()
  [[ $kind == copy ]] && kind_def=(+define+COPY_MODE)
  [[ $kind == prefix ]] && kind_def=(+define+PREFIX_MODE)
  if ! modelsim_compile "$out/sim" tb_generated_duplex_kernel "$rtl" "$lib" "$TB" "$OBSERVER" -- \
      -mfcu -cuname duplex_load_observer_unit_cu "${kind_def[@]}"; then
    echo "ModelSim compile failed for $profile/$kind" >&2; return 1
  fi
  printf 'modelsim_elaboration_args=' >>"$out/compiler_argv.txt"
  printf '%q ' "${overrides[@]}" >>"$out/compiler_argv.txt"
  printf '\n' >>"$out/compiler_argv.txt"
  sha256sum "$rtl" "$lib" "$out/sim/compile.log" >>"$OUT/provenance.txt"
}
build_profile() {
  local pair=$1 profile=${1%%__*} kind=${1#*__}
  build_one "$profile" "$kind"
}

# Each profile/kind gets an isolated ModelSim work library.
BURST_JOB_LOG_DIR="$OUT/job_logs/modelsim-build"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" build_profile "${profile_jobs[@]}"

OBSERVER_UNIT_OUT="$OUT/observer-unit"
mkdir "$OBSERVER_UNIT_OUT" "$OBSERVER_UNIT_OUT/tmp"
modelsim_compile "$OBSERVER_UNIT_OUT/sim" tb_duplex_load_observer "$OBSERVER_UNIT_TB" "$OBSERVER" -- \
  -mfcu -cuname duplex_load_observer_unit_cu || {
  echo "ModelSim compile failed for duplex load observer unit test" >&2; exit 1;
}
if ! modelsim_run "$OBSERVER_UNIT_OUT/sim" tb_duplex_load_observer "$OBSERVER_UNIT_OUT/observer-unit.raw.log" 60 -voptargs=+acc -- +G3_METRICS +CASE=COPY_PARALLELISM_256; then
  cat "$OBSERVER_UNIT_OUT/observer-unit.raw.log" >&2
  echo "duplex load observer unit test failed; see $OBSERVER_UNIT_OUT/observer-unit.raw.log" >&2; exit 1
fi
modelsim_normalize_log "$OBSERVER_UNIT_OUT/observer-unit.raw.log" "$OBSERVER_UNIT_OUT/observer-unit.log"
grep -Fqx 'DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2' \
  "$OBSERVER_UNIT_OUT/observer-unit.log" || {
  cat "$OBSERVER_UNIT_OUT/observer-unit.log" >&2
  echo "duplex load observer unit test omitted its exact PASS marker" >&2; exit 1;
}

run_scenario() {
  local profile=$1 kind=$2 scenario=$3 out scenario_tmp raw log rc=0 marker
  out=$(profile_dir "$profile" "$kind")
  marker=DUPLEX_KERNEL_G2
  [[ $kind == copy ]] && marker=DUPLEX_KERNEL_COPY_G2
  [[ $kind == prefix ]] && marker=DUPLEX_KERNEL_PREFIX_G2
  local scenario_tmp="$out/scenarios/tmp-$scenario"
  mkdir "$scenario_tmp"
  log="$out/scenarios/$scenario.log"
  raw="$out/scenarios/$scenario.raw.log"
  local -a plusargs=("+CASE=$scenario") expected_faults=() observed_faults=()
  local -a overrides=(-gREAD_B_MAX=16 -gREAD_OUTSTANDING=1 -gREAD_FIFO_DEPTH=64
    -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=32)
  if [[ $profile == asymmetric ]]; then
    overrides=(-gREAD_B_MAX=16 -gREAD_OUTSTANDING=2 -gREAD_FIFO_DEPTH=64
      -gWRITE_B_MAX=8 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  fi
  [[ $scenario == COPY_PARALLELISM_256 ]] && plusargs+=(+G3_METRICS)
  case $scenario in
    RRESP_FAULT_RESET|BRESP_FAULT_RESET) plusargs+=(+AXI_BURST_EXPECT_FAULT) ;;
  esac
  if (cd "$scenario_tmp" && TMPDIR="$scenario_tmp" TMP="$scenario_tmp" TEMP="$scenario_tmp" \
      modelsim_run "$out/sim" tb_generated_duplex_kernel "$raw" 180 "${overrides[@]}" -- "${plusargs[@]}"); then
    rc=0
  else rc=$?; fi
  [[ $rc -eq 0 ]] || { echo "$profile/$kind/$scenario failed rc=$rc; see $raw" >&2; return "$rc"; }
  modelsim_normalize_log "$raw" "$log"
  grep -Fqx "$marker PASS CASE=$scenario PROFILE=$profile" "$log" || {
    echo "missing exact success marker for $profile/$kind/$scenario; see $log" >&2; return 1;
  }
  case $scenario in
    RRESP_FAULT_RESET)
      expected_faults=("AXI_BURST_FAULT engine=read reason=RRESP " "AXI_BURST_PEER_FAULT engine=write reason=peer " "AXI_BURST_PEER_FAULT engine=read reason=peer ") ;;
    BRESP_FAULT_RESET)
      expected_faults=("AXI_BURST_FAULT engine=write reason=BRESP " "AXI_BURST_PEER_FAULT engine=read reason=peer " "AXI_BURST_PEER_FAULT engine=write reason=peer ") ;;
  esac
  mapfile -t observed_faults < <(sed -E 's/^\*\* Fatal: //' "$log" | grep -E '^AXI_BURST_' || true)
  if ((${#observed_faults[@]} != ${#expected_faults[@]})); then
    echo "wrong AXI diagnostic count for $profile/$kind/$scenario: expected=${#expected_faults[@]} observed=${#observed_faults[@]}" >&2
    cat "$log" >&2; return 1
  fi
  local i record
  for i in "${!expected_faults[@]}"; do
    record=${observed_faults[i]}
    [[ $record == "${expected_faults[i]}"* ]] || { echo "wrong AXI diagnostic order/content for $profile/$kind/$scenario at $i: $record" >&2; return 1; }
    if [[ ${expected_faults[i]} == *engine=read* ]]; then
      [[ $record =~ instance=[^[:space:]]+\.report_axi_read_diagnostic\ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ resp=[01]{2}\ id=-?[0-9]+\ last=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed or unqualified read diagnostic instance for $profile/$kind/$scenario" >&2; return 1; }
    else
      [[ $record =~ instance=[^[:space:]]+\.report_axi_write_diagnostic\ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ payload=[[:xdigit:]]+\ resp=[01]{2}\ id=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed or unqualified write diagnostic instance for $profile/$kind/$scenario" >&2; return 1; }
    fi
  done
  if [[ $kind == prefix ]]; then
    local expected_read expected_write expected_ar expected_aw
    case $scenario in
      READ_LONGER) expected_read=38; expected_write=17; expected_ar=3; expected_aw=2 ;;
      WRITE_LONGER) expected_read=17; expected_write=38; expected_ar=2; expected_aw=3 ;;
      ZERO_READ_WRITE_TAIL) expected_read=0; expected_write=17; expected_ar=0; expected_aw=2 ;;
      READ_ONLY) expected_read=17; expected_write=0; expected_ar=2; expected_aw=0 ;;
      ZERO_ZERO) expected_read=0; expected_write=0; expected_ar=0; expected_aw=0 ;;
      BOUNDS_CLAMP) expected_read=17; expected_write=17; expected_ar=2; expected_aw=2 ;;
      PAGE_SPLITS) expected_read=23; expected_write=35; expected_ar=3; expected_aw=3 ;;
      *) echo "unknown required prefix scenario: $scenario" >&2; return 1 ;;
    esac
    if [[ $profile == asymmetric ]]; then
      case $scenario in
        READ_LONGER|ZERO_READ_WRITE_TAIL|BOUNDS_CLAMP) expected_aw=3 ;;
        WRITE_LONGER|PAGE_SPLITS) expected_aw=5 ;;
      esac
    fi
    grep -Fqx "DUPLEX_KERNEL_PREFIX_COUNTS CASE=$scenario PROFILE=$profile AR_BURSTS=$expected_ar R_BEATS=$expected_read AW_BURSTS=$expected_aw W_BEATS=$expected_write B_RESPONSES=$expected_aw" "$log" || {
      echo "missing or incorrect prefix transaction-count witness for $profile/$scenario; see $log" >&2; return 1;
    }
  fi
  printf 'GENERATED_DUPLEX_KERNEL_SCENARIO PASS PROFILE=%s KIND=%s CASE=%s\n' "$profile" "$kind" "$scenario"
}
run_scenario_group() {
  local pair=$1 profile=${1%%__*} kind=${1#*__} scenario
  if [[ $kind == prefix ]]; then
    for scenario in "${PREFIX_CASES[@]}"; do run_scenario "$profile" "$kind" "$scenario" || return $?; done
  else
    for scenario in "${CASES[@]}"; do run_scenario "$profile" "$kind" "$scenario" || return $?; done
    if [[ $kind == copy ]]; then run_scenario "$profile" "$kind" COPY_PARALLELISM_256 || return $?; fi
  fi
}
declare -a scenario_groups=()
for profile in "${PROFILES[@]}"; do
  for kind in "${KINDS[@]}"; do scenario_groups+=("${profile}__${kind}"); done
  scenario_groups+=("${profile}__prefix")
done
BURST_JOB_LOG_DIR="$OUT/job_logs/scenario"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$DUPLEX_G2_JOBS" run_scenario_group "${scenario_groups[@]}"

(cd "$OUT" && python3 -B "$PARALLELISM_CHECKER" "$OUT") >"$OUT/parallelism-check.log" 2>&1 || {
  cat "$OUT/parallelism-check.log" >&2
  echo "duplex parallelism result check failed; see $OUT/parallelism-check.log" >&2
  exit 1
}
for profile in "${PROFILES[@]}"; do
  grep -Fqx "DUPLEX PARALLELISM PASS PROFILE=$profile" "$OUT/parallelism-check.log" || {
    echo "parallelism checker omitted profile $profile" >&2; exit 1;
  }
done

: >"$OUT/scenario-sha256.txt"
for profile in "${PROFILES[@]}"; do
  for kind in "${KINDS[@]}"; do
    marker=DUPLEX_KERNEL_G2
    [[ $kind == copy ]] && marker=DUPLEX_KERNEL_COPY_G2
    out=$(profile_dir "$profile" "$kind")
    local_scenarios=("${CASES[@]}")
    [[ $kind == copy ]] && local_scenarios+=(COPY_PARALLELISM_256)
    for scenario in "${local_scenarios[@]}"; do
      log="$out/scenarios/$scenario.log"
      grep -Fqx "$marker PASS CASE=$scenario PROFILE=$profile" "$log" || {
        echo "scenario omitted from evidence: $profile/$kind/$scenario" >&2; exit 1;
      }
      (cd "$OUT" && sha256sum "${out#"$OUT/"}/scenarios/$scenario.log") >>"$OUT/scenario-sha256.txt"
    done
  done
  out=$(profile_dir "$profile" prefix)
  for scenario in "${PREFIX_CASES[@]}"; do
    log="$out/scenarios/$scenario.log"
    grep -Fqx "DUPLEX_KERNEL_PREFIX_G2 PASS CASE=$scenario PROFILE=$profile" "$log" || {
      echo "prefix scenario omitted from evidence: $profile/$scenario" >&2; exit 1;
    }
    (cd "$OUT" && sha256sum "${out#"$OUT/"}/scenarios/$scenario.log") >>"$OUT/scenario-sha256.txt"
  done
done
(cd "$OUT" && sha256sum parallelism-check.log) >>"$OUT/scenario-sha256.txt"
(cd "$OUT" && sha256sum --check scenario-sha256.txt)
artifact_paths=()
for profile in "${PROFILES[@]}"; do
  for kind in "${KINDS[@]}"; do
    out=$(profile_dir "$profile" "$kind")
    out=${out#"$OUT/"}
    artifact_paths+=(
      "$out/generated/kernel.v" "$out/generated/panda_libtech.v"
      "$out/generated/bambu.stdout.log" "$out/generated/bambu.stderr.log"
      "$out/sim/compile.log"
    )
    for scenario in "${CASES[@]}"; do artifact_paths+=("$out/scenarios/$scenario.log" "$out/scenarios/$scenario.raw.log"); done
    [[ $kind == copy ]] && artifact_paths+=("$out/scenarios/COPY_PARALLELISM_256.log" "$out/scenarios/COPY_PARALLELISM_256.raw.log")
  done
  out=$(profile_dir "$profile" prefix); out=${out#"$OUT/"}
  artifact_paths+=(
    "$out/generated/kernel.v" "$out/generated/panda_libtech.v"
    "$out/generated/bambu.stdout.log" "$out/generated/bambu.stderr.log"
    "$out/sim/compile.log"
  )
  for scenario in "${PREFIX_CASES[@]}"; do artifact_paths+=("$out/scenarios/$scenario.log" "$out/scenarios/$scenario.raw.log"); done
done
{
  printf 'parallelism_check_log=%s\nparallelism_check_log_sha256=%s\n' \
    "$OUT/parallelism-check.log" "$(digest "$OUT/parallelism-check.log")"
  for profile in "${PROFILES[@]}"; do
    log=$(profile_dir "$profile" copy)/scenarios/COPY_PARALLELISM_256.log
    printf 'parallelism_scenario_%s=%s\nparallelism_scenario_%s_sha256=%s\n' \
      "$profile" "$log" "$profile" "$(digest "$log")"
  done
  sha256sum -- "$OBSERVER" "$OBSERVER_UNIT_TB" "$PARALLELISM_CHECKER"
} >>"$OUT/provenance.txt"
artifact_paths+=(parallelism-check.log observer-unit/observer-unit.log observer-unit/observer-unit.raw.log observer-unit/sim/compile.log)
(cd "$OUT" && sha256sum "${artifact_paths[@]}") >"$OUT/artifact-sha256.txt"
(cd "$OUT" && sha256sum -- "$OBSERVER" "$OBSERVER_UNIT_TB" "$PARALLELISM_CHECKER") >>"$OUT/artifact-sha256.txt"
(cd "$OUT" && sha256sum --check artifact-sha256.txt)
echo "GENERATED_DUPLEX_KERNEL_G2 PASS: $(( ${#CASES[@]} * ${#PROFILES[@]} * ${#KINDS[@]} + ${#PREFIX_CASES[@]} * ${#PROFILES[@]} + ${#PROFILES[@]} )) scenarios; includes two G3.1 parallelism scenarios" | tee "$OUT/aggregate.log"
echo "Output: $OUT"
