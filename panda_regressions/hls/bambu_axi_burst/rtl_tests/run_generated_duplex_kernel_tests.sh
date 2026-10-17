#!/usr/bin/env bash
# Generate and simulate the complete same-base duplex kernel with Verilator.
# Usage: BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]
set -euo pipefail
ulimit -c 0

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
FAMILY="$ROOT/panda_regressions/hls/bambu_axi_burst"
HELPERS="$FAMILY/parallel_helpers.sh"
SOURCE="$FAMILY/bambu_axi_burst_duplex.cpp"
TB="$TESTS/tb_generated_duplex_kernel.sv"
RUNNER="$TESTS/run_generated_duplex_kernel_tests.sh"
PROFILES=(symmetric asymmetric)
CASES=(COUNTS_0_1_15_16_17 COUNT38_SPLITS PAGE_BOUNDARY REINVOKE_DEPENDENT REINVOKE_BASE_COUNT LAST_B_WAIT RRESP_FAULT_RESET BRESP_FAULT_RESET)

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
for tool in verilator python3 rg sha256sum timeout nproc; do
  command -v "$tool" >/dev/null || { echo "required tool missing: $tool" >&2; exit 2; }
done
[[ -s $SOURCE && -s $TB && -s $HELPERS ]] || { echo "G2 kernel source, golden testbench, or helper is missing" >&2; exit 2; }

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

OUT_ARG=${2:-"$ROOT/documentation/tmp/out_generated_duplex_kernel"}
[[ ! -e $OUT_ARG && ! -L $OUT_ARG ]] || { echo "output directory must be fresh and not a symlink: $OUT_ARG" >&2; exit 2; }
OUT=$(realpath -m -- "$OUT_ARG")
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT" "$OUT/tmp" "$OUT/job_logs"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1

SIM_JOBS=${SIM_JOBS:-2}
DUPLEX_G2_JOBS=${DUPLEX_G2_JOBS:-8}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ && $DUPLEX_G2_JOBS =~ ^[1-9][0-9]*$ ]] || {
  echo "SIM_JOBS and DUPLEX_G2_JOBS must be positive integers" >&2; exit 2;
}
SIM_JOBS=$((10#$SIM_JOBS)); DUPLEX_G2_JOBS=$((10#$DUPLEX_G2_JOBS))
(( SIM_JOBS <= 2 )) || SIM_JOBS=2
(( DUPLEX_G2_JOBS <= 8 )) || DUPLEX_G2_JOBS=8

digest() { sha256sum -- "$1" | awk '{print $1}'; }
{
  printf 'git_head=%s\nroot=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nsettings_sha256=%s\nplugin=%s\nplugin_sha256=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)" "$ROOT" "$OUT" "$BAMBU" "$(digest "$BAMBU")" \
    "$SETTINGS" "$(digest "$SETTINGS")" "$PLUGIN" "$(digest "$PLUGIN")"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$SOURCE" "$(digest "$SOURCE")" "$TB" "$(digest "$TB")" "$RUNNER" "$(digest "$RUNNER")"
  printf 'sim_jobs=%s\nscenario_jobs=%s\n' "$SIM_JOBS" "$DUPLEX_G2_JOBS"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  verilator --version
} >"$OUT/provenance.txt"

COMMON=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
  --bambu-parameter=experimental-m-axi-burst=1 --extra-cc-options=-DNDEBUG
  --extra-cc-options=-DBAMBU)
generate_profile() {
  local profile=$1 profile_id=1 source_out="$OUT/$1"
  [[ $profile == symmetric ]] || profile_id=2
  mkdir "$source_out" "$source_out/tmp" "$source_out/generated" "$source_out/scenarios"
  local -a args=("${COMMON[@]}" "--extra-cc-options=-DDUPLEX_PROFILE=$profile_id")
  local read_b=16 read_o=1 read_fifo=64 write_b=16 write_o=1 write_fifo=32
  if [[ $profile == asymmetric ]]; then
    read_o=2; write_b=8; write_fifo=16
  fi
  {
    printf 'profile=%s\nprofile_id=%s\nsource=%s\nsource_sha256=%s\n' "$profile" "$profile_id" "$SOURCE" "$(digest "$SOURCE")"
    printf 'read_profile=B:%s,O:%s,FIFO:%s\nwrite_profile=B:%s,O:%s,FIFO:%s\n' "$read_b" "$read_o" "$read_fifo" "$write_b" "$write_o" "$write_fifo"
    printf 'argv='; printf '%q ' "$BAMBU" "$SOURCE" "${args[@]}"; printf '\n'
  } >"$source_out/compiler_argv.txt"
  if ! (cd "$source_out/generated" && TMPDIR="$source_out/tmp" TMP="$source_out/tmp" TEMP="$source_out/tmp" \
      "$BAMBU" "$SOURCE" "${args[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
    tail -n 100 "$source_out/generated/bambu.stderr.log" >&2 || true
    echo "Bambu generation failed for $profile; see $source_out/generated/" >&2; return 1
  fi
  local rtl="$source_out/generated/kernel.v" lib="$source_out/generated/panda_libtech.v"
  [[ -s $rtl && -s $lib ]] || { echo "missing generated kernel RTL/library for $profile" >&2; return 1; }
  python3 - "$rtl" "$lib" "$read_b" "$read_o" "$read_fifo" "$write_b" "$write_o" "$write_fifo" <<'PY'
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
PY
  printf 'GENERATED_DUPLEX_KERNEL_BUILD PASS PROFILE=%s\n' "$profile"
}

source "$HELPERS"
BURST_QUIET_JOB_STATUS=1
BURST_JOB_LOG_DIR="$OUT/job_logs/build"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" generate_profile "${PROFILES[@]}"
for profile in "${PROFILES[@]}"; do
  rg -q "^GENERATED_DUPLEX_KERNEL_BUILD PASS PROFILE=$profile$" "$BURST_JOB_LOG_DIR/$profile.log" || {
    echo "missing generation completion marker for $profile" >&2; exit 1;
  }
done

build_profile() {
  local profile=$1 profile_id=1 out="$OUT/$1"
  [[ $profile == symmetric ]] || profile_id=2
  local rtl="$out/generated/kernel.v" lib="$out/generated/panda_libtech.v"
  local -a overrides=(-GREAD_B_MAX=16 -GREAD_OUTSTANDING=1
    -GWRITE_B_MAX=16 -GWRITE_OUTSTANDING=1)
  if [[ $profile == asymmetric ]]; then
    overrides=(-GREAD_B_MAX=16 -GREAD_OUTSTANDING=2
      -GWRITE_B_MAX=8 -GWRITE_OUTSTANDING=1)
  fi
  if ! verilator --binary --timing --assert -Wno-fatal -Werror-IMPLICIT -Werror-UNDRIVEN -j 1 \
      --top-module tb_generated_duplex_kernel --Mdir "$out/obj" "${overrides[@]}" \
      -DGENERATED_TOP=kernel "-DDUPLEX_PROFILE_ID=$profile_id" "$rtl" "$lib" "$TB" \
      >"$out/verilator-build.log" 2>&1; then
    tail -n 120 "$out/verilator-build.log" >&2
    echo "Verilator build failed for $profile" >&2; return 1
  fi
  sha256sum "$rtl" "$lib" "$out/verilator-build.log" >>"$OUT/provenance.txt"
}

# Generate each profile exactly once; two independent Verilator builds maximum.
BURST_JOB_LOG_DIR="$OUT/job_logs/verilator"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" build_profile "${PROFILES[@]}"

run_scenario() {
  local pair=$1 profile=${1%%__*} scenario=${1#*__} log rc=0
  local scenario_tmp="$OUT/$profile/scenarios/tmp-$scenario"
  mkdir "$scenario_tmp"
  log="$OUT/$profile/scenarios/$scenario.log"
  if (cd "$scenario_tmp" && TMPDIR="$scenario_tmp" TMP="$scenario_tmp" TEMP="$scenario_tmp" \
      timeout 180s "$OUT/$profile/obj/Vtb_generated_duplex_kernel" "+CASE=$scenario" >"$log" 2>&1); then
    rc=0
  else rc=$?; fi
  [[ $rc -eq 0 ]] || { echo "$profile/$scenario failed rc=$rc; see $log" >&2; return "$rc"; }
  rg -Fqx "DUPLEX_KERNEL_G2 PASS CASE=$scenario PROFILE=$profile" "$log" || {
    echo "missing exact success marker for $profile/$scenario; see $log" >&2; return 1;
  }
  printf 'GENERATED_DUPLEX_KERNEL_SCENARIO PASS PROFILE=%s CASE=%s\n' "$profile" "$scenario"
}
declare -a scenario_jobs=()
for profile in "${PROFILES[@]}"; do
  for scenario in "${CASES[@]}"; do scenario_jobs+=("${profile}__${scenario}"); done
done
run_scenario_job() { run_scenario "$1"; }
BURST_JOB_LOG_DIR="$OUT/job_logs/scenario"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$DUPLEX_G2_JOBS" run_scenario_job "${scenario_jobs[@]}"

: >"$OUT/scenario-sha256.txt"
for profile in "${PROFILES[@]}"; do
  for scenario in "${CASES[@]}"; do
    log="$OUT/$profile/scenarios/$scenario.log"
    rg -Fqx "DUPLEX_KERNEL_G2 PASS CASE=$scenario PROFILE=$profile" "$log" || {
      echo "scenario omitted from evidence: $profile/$scenario" >&2; exit 1;
    }
    (cd "$OUT" && sha256sum "$profile/scenarios/$scenario.log") >>"$OUT/scenario-sha256.txt"
  done
done
(cd "$OUT" && sha256sum --check scenario-sha256.txt)
artifact_paths=()
for profile in "${PROFILES[@]}"; do
  artifact_paths+=(
    "$profile/generated/kernel.v" "$profile/generated/panda_libtech.v"
    "$profile/generated/bambu.stdout.log" "$profile/generated/bambu.stderr.log"
    "$profile/verilator-build.log"
  )
  for scenario in "${CASES[@]}"; do artifact_paths+=("$profile/scenarios/$scenario.log"); done
done
(cd "$OUT" && sha256sum "${artifact_paths[@]}") >"$OUT/artifact-sha256.txt"
(cd "$OUT" && sha256sum --check artifact-sha256.txt)
echo "GENERATED_DUPLEX_KERNEL_G2 PASS: ${#CASES[@]} scenarios x ${#PROFILES[@]} profiles" | tee "$OUT/aggregate.log"
echo "Output: $OUT"
