#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
FAMILY=$TESTS
ROOT=$(realpath "$TESTS/../../..")
RTL_TESTS="$FAMILY/rtl_tests"
G2_RUNNER="$RTL_TESTS/run_generated_duplex_kernel_tests.sh"
MAPPING_CHECKER="$FAMILY/check_duplex_iteration_results.py"
TB="$RTL_TESTS/tb_generated_duplex_kernel.sv"
LOAD_OBSERVER="$RTL_TESTS/duplex_load_observer.sv"
ITERATION_OBSERVER="$RTL_TESTS/duplex_iteration_observer.sv"
ITERATION_UNIT_TB="$RTL_TESTS/tb_duplex_iteration_observer.sv"
HELPERS="$FAMILY/parallel_helpers.sh"
PROFILES=(symmetric asymmetric)

if (($# > 2)); then
  echo "usage: DUPLEX_G3_SIM_OUTSIDE_SANDBOX=1 $0 [BAMBU] [FRESH_OUTPUT_DIR]" >&2
  exit 2
fi
[[ ${DUPLEX_G3_SIM_OUTSIDE_SANDBOX:-} == 1 ]] || {
  echo "refusing compiler/build/simulation unless DUPLEX_G3_SIM_OUTSIDE_SANDBOX=1 is set" >&2
  exit 2
}
BAMBU_ARG=${1:-"$ROOT/install/bin/bambu"}
if [[ $BAMBU_ARG == */* ]]; then BAMBU=$(realpath -- "$BAMBU_ARG"); else
  BAMBU=$(command -v -- "$BAMBU_ARG" || true)
  [[ -n $BAMBU ]] && BAMBU=$(realpath -- "$BAMBU")
fi
[[ -n ${BAMBU:-} && -x $BAMBU ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
for tool in verilator python3 rg sha256sum timeout; do
  command -v "$tool" >/dev/null || { echo "required tool missing: $tool" >&2; exit 2; }
done
[[ -s $G2_RUNNER && -s $MAPPING_CHECKER && -s $TB && -s $LOAD_OBSERVER && -s $ITERATION_OBSERVER && -s $ITERATION_UNIT_TB && -s $HELPERS ]] || {
  echo "G3.3 runner input is missing" >&2; exit 2;
}
SIM_JOBS=${SIM_JOBS:-2}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "SIM_JOBS must be a positive integer" >&2; exit 2; }
SIM_JOBS=$((10#$SIM_JOBS))
TMP_ROOT="$ROOT/documentation/tmp"
OUT_ARG=${2:-"$TMP_ROOT/out_duplex_iteration"}
mkdir -p "$TMP_ROOT"
OUT_PARENT=$(realpath -m -- "$(dirname -- "$OUT_ARG")")
case "$OUT_PARENT/" in "$TMP_ROOT/"*) ;; *)
  echo "output directory must be under $TMP_ROOT: $OUT_ARG" >&2; exit 2 ;;
esac
[[ ! -e $OUT_ARG && ! -L $OUT_ARG ]] || { echo "output directory must be fresh: $OUT_ARG" >&2; exit 2; }
OUT=$(realpath -m -- "$OUT_ARG")
mkdir -p "$ROOT/documentation/tmp"
export TMPDIR="$ROOT/documentation/tmp" TMP="$ROOT/documentation/tmp" TEMP="$ROOT/documentation/tmp" PYTHONDONTWRITEBYTECODE=1
mkdir "$OUT" "$OUT/tmp" "$OUT/job_logs"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r $SETTINGS ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}

digest() { sha256sum -- "$1" | awk '{print $1}'; }
{
  printf 'git_head=%s\nroot=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nsettings_sha256=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)" "$ROOT" "$OUT" "$BAMBU" "$(digest "$BAMBU")" \
    "$SETTINGS" "$(digest "$SETTINGS")"
  printf 'runner=%s\nrunner_sha256=%s\nmapping_checker=%s\nmapping_checker_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\n' \
    "$TESTS/run_duplex_iteration_tests.sh" "$(digest "$TESTS/run_duplex_iteration_tests.sh")" \
    "$MAPPING_CHECKER" "$(digest "$MAPPING_CHECKER")" "$TB" "$(digest "$TB")"
  printf 'sim_jobs=%s\n' "$SIM_JOBS"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  verilator --version
} >"$OUT/provenance.txt"

G2_OUT="$OUT/g2-prerequisite"
BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 DUPLEX_G3_ITERATION_DUMPS=1 SIM_JOBS="$SIM_JOBS" \
  bash "$G2_RUNNER" "$BAMBU" "$G2_OUT" >"$OUT/g2-prerequisite.log" 2>&1 || {
    tail -n 120 "$OUT/g2-prerequisite.log" >&2
    echo "G2/G3.1 prerequisite runner failed; see $OUT/g2-prerequisite.log" >&2
    exit 1
  }
rg -Fq 'GENERATED_DUPLEX_KERNEL_G2 PASS' "$OUT/g2-prerequisite.log" || {
  echo "G2 prerequisite omitted its exact aggregate marker" >&2; exit 1;
}

generate_binding() {
  local profile=$1 generated="$G2_OUT/$1_copy/generated" out="$OUT/$1"
  mkdir "$out" "$out/tmp" "$out/obj-parent" "$out/events"
  python3 -B "$MAPPING_CHECKER" "$generated" --profile "$profile" --report "$out/mapping.json" \
    >"$out/mapping.log" 2>&1 || { cat "$out/mapping.log" >&2; return 1; }
  python3 - "$out/mapping.json" "$out/g3_iteration_binding.svh" <<'PY'
import json
import re
import sys
from pathlib import Path

mapping = json.loads(Path(sys.argv[1]).read_text())
controller = mapping["controller"]
event = mapping["event_definition"]
state_path = event["state_signal"]
if not state_path.startswith("kernel."):
    raise SystemExit("mapped state signal is not relative to generated top kernel")
state_path = "dut." + state_path[len("kernel."):]
controller_scope = state_path.rsplit(".", 1)[0]
datapath_path = controller_scope.rsplit(".", 1)[0] + "." + controller["datapath_instance"]
index_signal = datapath_path + "." + event["iteration_index_binding"]
address_signal = datapath_path + "." + event["source_address_signal"]
for value in (state_path, index_signal, address_signal, *event["state_parameters"]):
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_.]*", value):
        raise SystemExit(f"unsafe generated mapping token: {value!r}")
state_cases = " : ".join(
    f'({state_path} == {controller_scope}.{state}) ? "{state}"'
    for state in event["state_parameters"])
state_cases += ' : ""'
body = "\n".join((
    f'`define G3_MAPPING_ID "{mapping["mapping_id"]}"',
    f"`define G3_STATE_SAMPLE ({state_cases})",
    f"`define G3_INDEX_SAMPLE {index_signal}",
    f"`define G3_SOURCE_ADDRESS_SAMPLE {address_signal}",
    ""))
Path(sys.argv[2]).write_text(body)
PY
}

build_one() {
  local profile=$1 id=1 out="$OUT/$1" generated="$G2_OUT/$1_copy/generated"
  [[ $profile == asymmetric ]] && id=2
  local -a overrides=(-GREAD_B_MAX=16 -GREAD_OUTSTANDING=1 -GREAD_FIFO_DEPTH=64
    -GWRITE_B_MAX=16 -GWRITE_OUTSTANDING=1 -GWRITE_FIFO_DEPTH=32)
  if [[ $profile == asymmetric ]]; then
    overrides=(-GREAD_B_MAX=16 -GREAD_OUTSTANDING=2 -GREAD_FIFO_DEPTH=64
      -GWRITE_B_MAX=8 -GWRITE_OUTSTANDING=1 -GWRITE_FIFO_DEPTH=16)
  fi
  mkdir "$out/g3-obj"
  if ! verilator --binary --timing --assert -Wno-fatal -Werror-IMPLICIT -Werror-UNDRIVEN -j 1 \
      --top-module tb_generated_duplex_kernel --Mdir "$out/g3-obj" -I"$out" \
      "${overrides[@]}" -DGENERATED_TOP=kernel -DDUPLEX_PROFILE_ID="$id" -DCOPY_MODE \
      -DG3_ITERATION_OBSERVER "$generated/kernel.v" "$generated/panda_libtech.v" \
      "$TB" "$LOAD_OBSERVER" "$ITERATION_OBSERVER" >"$out/verilator-build.log" 2>&1; then
    tail -n 120 "$out/verilator-build.log" >&2
    echo "G3 Verilator build failed for $profile" >&2; return 1
  fi
}

run_one() {
  local profile=$1 out="$OUT/$1" generated="$G2_OUT/$1_copy/generated"
  local log="$out/events/scenario.log"
  if ! (cd "$out/events" && TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" \
      timeout 180s "$out/g3-obj/Vtb_generated_duplex_kernel" \
      +CASE=COPY_PARALLELISM_256 +G3_METRICS >"$log" 2>&1); then
    tail -n 120 "$log" >&2
    echo "G3 iteration simulation failed for $profile" >&2; return 1
  fi
  rg -Fqx 'DUPLEX_ITERATION_OBSERVER PASS EVENT_COUNT=256' "$log" || {
    cat "$log" >&2; echo "missing exact iteration event marker for $profile" >&2; return 1;
  }
  rg -Fqx "DUPLEX_KERNEL_COPY_G2 PASS CASE=COPY_PARALLELISM_256 PROFILE=$profile" "$log" || {
    cat "$log" >&2; echo "missing preserved G2 payload/canary marker for $profile" >&2; return 1;
  }
  python3 -B "$MAPPING_CHECKER" "$generated" --profile "$profile" \
    --events "$out/events/iteration-events.json" \
    --load-log "$G2_OUT/${profile}_copy/scenarios/COPY_PARALLELISM_256.log" \
    --report "$out/iteration-report.json" >"$out/iteration-check.log" 2>&1 || {
      cat "$out/iteration-check.log" >&2; return 1;
    }
  cat "$out/iteration-check.log"
  sha256sum "$out/mapping.json" "$out/g3_iteration_binding.svh" "$out/events/iteration-events.json" \
    "$out/events/scenario.log" "$out/iteration-report.json" >"$out/artifact-sha256.txt"
}

source "$HELPERS"
BURST_QUIET_JOB_STATUS=1
UNIT_OUT="$OUT/observer-unit"
mkdir "$UNIT_OUT" "$UNIT_OUT/tmp" "$UNIT_OUT/obj"
if ! verilator --binary --timing --assert -Wno-fatal -Werror-IMPLICIT -Werror-UNDRIVEN -j 1 \
    --top-module tb_duplex_iteration_observer --Mdir "$UNIT_OUT/obj" \
    "$ITERATION_OBSERVER" "$ITERATION_UNIT_TB" >"$UNIT_OUT/verilator-build.log" 2>&1; then
  tail -n 100 "$UNIT_OUT/verilator-build.log" >&2
  echo "iteration observer unit build failed" >&2; exit 1
fi
if ! (cd "$UNIT_OUT" && TMPDIR="$UNIT_OUT/tmp" TMP="$UNIT_OUT/tmp" TEMP="$UNIT_OUT/tmp" \
    ./obj/Vtb_duplex_iteration_observer >unit.log 2>&1); then
  cat "$UNIT_OUT/unit.log" >&2; echo "iteration observer unit simulation failed" >&2; exit 1
fi
rg -Fqx 'DUPLEX_ITERATION_OBSERVER_UNIT PASS COUNT=3 STATE=S_7' "$UNIT_OUT/unit.log" || {
  cat "$UNIT_OUT/unit.log" >&2; echo "iteration observer unit marker missing" >&2; exit 1;
}
BURST_JOB_LOG_DIR="$OUT/job_logs/mapping"; mkdir "$BURST_JOB_LOG_DIR"
map_profile() { generate_binding "$1"; }
run_bounded_jobs "$SIM_JOBS" map_profile "${PROFILES[@]}"
BURST_JOB_LOG_DIR="$OUT/job_logs/verilator"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" build_one "${PROFILES[@]}"
BURST_JOB_LOG_DIR="$OUT/job_logs/scenario"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" run_one "${PROFILES[@]}"
for profile in "${PROFILES[@]}"; do
  rg -Fq 'DUPLEX ITERATION MAPPING PASS' "$OUT/$profile/mapping.log"
  rg -Fq 'DUPLEX EFFECTIVE II' "$OUT/$profile/iteration-check.log"
  rg -Fq 'DUPLEX LOAD INTERVAL' "$OUT/$profile/iteration-check.log"
done
sha256sum "$ITERATION_OBSERVER" "$ITERATION_UNIT_TB" "$TB" "$MAPPING_CHECKER" >>"$OUT/provenance.txt"
echo "DUPLEX_ITERATION_G3 PASS PROFILES=2 EVENTS_PER_PROFILE=256 SCHEDULED_II_AND_RUNTIME_INTERVALS=REPORTED" \
  | tee "$OUT/aggregate.log"
echo "Output: $OUT"
