#!/usr/bin/env bash
# Manual G1 test: exercise production duplex endpoints using generated RTL.
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
FAMILY="$ROOT/panda_regressions/hls/bambu_axi_burst"
CHECKER="$FAMILY/check_duplex_effect_analysis_artifacts.py"
HELPERS="$FAMILY/parallel_helpers.sh"
TB="$TESTS/tb_generated_duplex.sv"
SOURCE32="$FAMILY/compiler_cases/duplex_effects/same_base_pointwise.cpp"
SOURCE64="$FAMILY/compiler_cases/duplex_effects/same_base_asymmetric_64.cpp"
CASES=(LOAD_WAIT_STORE STORE_WAIT_LOAD WRITE_STALL_READ_PROGRESS READ_STALL_WRITE_PROGRESS
  SIMULTANEOUS CONFIG_ISOLATION ZERO_READ ZERO_WRITE UNPAIRED_DESCRIPTORS INPLACE_POINTWISE
  BRESP_PEER_DRAIN RRESP_PEER_DRAIN FAULT_RESET_RESTART)
PROFILES=(same_base_pointwise same_base_asymmetric_64)

if (($# < 1 || $# > 2)); then
  echo "usage: $0 BAMBU [FRESH_OUTPUT]" >&2
  exit 2
fi
if [[ ${BURST_DUPLEX_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/build/simulation unless BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 is set" >&2
  exit 2
fi
BAMBU_ARG=$1
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath -- "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath -- "$BAMBU"); fi
[[ -n ${BAMBU:-} && -x "$BAMBU" ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
command -v verilator >/dev/null || { echo "verilator is required" >&2; exit 2; }
for tool in sha256sum python3 timeout; do command -v "$tool" >/dev/null || { echo "required tool missing: $tool" >&2; exit 2; }; done

PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r "$SETTINGS" ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r "$PLUGIN" ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
[[ -s "$CHECKER" && -s "$HELPERS" && -s "$TB" && -s "$SOURCE32" && -s "$SOURCE64" ]] || {
  echo "G1 checker/helper/testbench/source missing" >&2; exit 2;
}

OUT_ARG=${2:-"$PWD/out_generated_duplex"}
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
  echo "output directory must be fresh and not a symlink: $OUT_ARG" >&2; exit 2
fi
OUT=$(realpath -m -- "$OUT_ARG")
SIM_JOBS=${SIM_JOBS:-2}
DUPLEX_G1_JOBS=${DUPLEX_G1_JOBS:-8}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "SIM_JOBS must be a positive integer" >&2; exit 2; }
[[ $DUPLEX_G1_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "DUPLEX_G1_JOBS must be a positive integer" >&2; exit 2; }
SIM_JOBS=$((10#$SIM_JOBS)); DUPLEX_G1_JOBS=$((10#$DUPLEX_G1_JOBS))
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT" "$OUT/tmp" "$OUT/job_logs"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum -- "$1" | awk '{print $1}'; }
{
  printf 'git_head=%s\nroot=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nsettings_sha256=%s\nplugin=%s\nplugin_sha256=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)" "$ROOT" "$OUT" "$BAMBU" "$(digest "$BAMBU")" \
    "$SETTINGS" "$(digest "$SETTINGS")" "$PLUGIN" "$(digest "$PLUGIN")"
  printf 'testbench=%s\ntestbench_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$TB" "$(digest "$TB")" "$CHECKER" "$(digest "$CHECKER")" "$TESTS/run_generated_duplex_tests.sh" \
    "$(digest "$TESTS/run_generated_duplex_tests.sh")"
  printf 'sim_jobs=%s\nscenario_jobs=%s\n' "$SIM_JOBS" "$DUPLEX_G1_JOBS"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  verilator --version
  sha256sum "$SOURCE32" "$SOURCE64"
} >"$OUT/provenance.txt"

COMMON=(--AXI-burst-type=INCREMENTAL --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33 --bambu-parameter=experimental-m-axi-burst=1
  --bambu-parameter=print-ir-manager=1 --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based
  --print-dot --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
generate_profile() {
  local profile=$1 source out args_text
  out="$OUT/$profile"; mkdir "$out" "$out/tmp" "$out/generated"
  if [[ $profile == same_base_pointwise ]]; then source=$SOURCE32; else source=$SOURCE64; fi
  local -a args=("--top-fname=$profile" "${COMMON[@]}")
  [[ $profile != same_base_asymmetric_64 ]] || args+=(--addr-bus-bitsize=64)
  printf '%q ' "$BAMBU" "$source" "${args[@]}" >"$out/compiler_argv.txt"; printf '\n' >>"$out/compiler_argv.txt"
  {
    printf 'profile=%s\nsource=%s\nsource_sha256=%s\n' "$profile" "$source" "$(digest "$source")"
    cat "$out/compiler_argv.txt"
  } >"$out/provenance.txt"
  if ! (cd "$out/generated" && TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" \
      "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 100 "$out/generated/stderr.log" >&2 || true
    echo "Bambu generation failed for $profile; see $out/generated/" >&2; return 1
  fi
  local raw_ir="$out/generated/panda-temp/before_Frontend::InterfaceInfer.raw"
  [[ -s "$raw_ir" && ! -L "$raw_ir" ]] || {
    echo "missing retained pre-InterfaceInfer raw IR for $profile: $raw_ir" >&2; return 1;
  }
  local post_interface_raw="$out/generated/panda-temp/after_Frontend::InterfaceInfer.raw"
  [[ -s "$post_interface_raw" && ! -L "$post_interface_raw" ]] || {
    echo "missing retained post-InterfaceInfer raw IR for $profile: $post_interface_raw" >&2; return 1;
  }
  {
    printf 'top=%s\nsource=%s\nsource_sha256=%s\nbambu=%s\nbambu_sha256=%s\n' \
      "$profile" "$source" "$(digest "$source")" "$BAMBU" "$(digest "$BAMBU")"
    printf 'plugin=%s\nplugin_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
      "$PLUGIN" "$(digest "$PLUGIN")" "$CHECKER" "$(digest "$CHECKER")" \
      "$TESTS/run_generated_duplex_tests.sh" "$(digest "$TESTS/run_generated_duplex_tests.sh")"
    printf 'raw_ir=%s\nraw_ir_sha256=%s\n' "$raw_ir" "$(digest "$raw_ir")"
    printf 'post_interface_raw=%s\npost_interface_raw_sha256=%s\n' \
      "$post_interface_raw" "$(digest "$post_interface_raw")"
  } >"$out/generated/provenance.txt"
  python3 "$CHECKER" "$profile" --output "$out/generated" --source "$source" --bambu "$BAMBU" \
    --plugin "$PLUGIN" --checker "$CHECKER" --runner "$TESTS/run_generated_duplex_tests.sh" || return $?
  local rtl="$out/generated/$profile.v" lib="$out/generated/panda_libtech.v"
  [[ -s "$rtl" && -s "$lib" ]] || { echo "missing generated RTL/library for $profile" >&2; return 1; }
  sha256sum "$rtl" "$lib" >>"$out/provenance.txt"
  local -a overrides
  if [[ $profile == same_base_pointwise ]]; then
    overrides=(-GADDRESS_BITS=32 -GREAD_B_MAX=16 -GREAD_OUTSTANDING=1 -GREAD_FIFO_DEPTH=32
      -GWRITE_B_MAX=16 -GWRITE_OUTSTANDING=1 -GWRITE_FIFO_DEPTH=16)
  else
    overrides=(-GADDRESS_BITS=64 -GREAD_B_MAX=8 -GREAD_OUTSTANDING=2 -GREAD_FIFO_DEPTH=32
      -GWRITE_B_MAX=16 -GWRITE_OUTSTANDING=1 -GWRITE_FIFO_DEPTH=16)
  fi
  printf 'verilator_argv=' >>"$out/provenance.txt"
  printf '%q ' verilator --binary --timing --assert -Wno-fatal -Werror-IMPLICIT -Werror-UNDRIVEN \
    -j 1 --top-module tb_generated_duplex \
    --Mdir "$out/obj" "${overrides[@]}" "$rtl" "$lib" "$TB" >>"$out/provenance.txt"
  printf '\n' >>"$out/provenance.txt"
  if ! verilator --binary --timing --assert -Wno-fatal -Werror-IMPLICIT -Werror-UNDRIVEN \
      -j 1 --top-module tb_generated_duplex \
      --Mdir "$out/obj" "${overrides[@]}" "$rtl" "$lib" "$TB" >"$out/verilator.log" 2>&1; then
    tail -n 120 "$out/verilator.log" >&2; echo "Verilator build failed for $profile" >&2; return 1
  fi
  sha256sum "$TB" "$TESTS/run_generated_duplex_tests.sh" "$out/verilator.log" >>"$out/provenance.txt"
  printf 'GENERATED_DUPLEX_BUILD PASS PROFILE=%s\n' "$profile"
}

# Two independent profile builds maximum; scenario runs start only after both binaries exist.
BURST_JOB_LOG_DIR="$OUT/job_logs/build"; mkdir "$BURST_JOB_LOG_DIR"
BURST_QUIET_JOB_STATUS=1; source "$HELPERS"
run_bounded_jobs "$SIM_JOBS" generate_profile "${PROFILES[@]}"
for profile in "${PROFILES[@]}"; do
  grep -Eq "^GENERATED_DUPLEX_BUILD PASS PROFILE=$profile$" "$BURST_JOB_LOG_DIR/$profile.log" || {
    echo "missing build completion marker for $profile" >&2; exit 1;
  }
done

run_scenario() {
  local profile=$1 scenario=$2 log rc
  log="$OUT/$profile/scenarios/$scenario.log"
  mkdir -p "$(dirname "$log")"
  local simulation_tmp="$OUT/$profile/scenarios/tmp-$scenario"; mkdir "$simulation_tmp"
  if (cd "$simulation_tmp" && TMPDIR="$simulation_tmp" TMP="$simulation_tmp" TEMP="$simulation_tmp" \
      timeout 180s "$OUT/$profile/obj/Vtb_generated_duplex" "+CASE=$scenario" "+PROFILE=$profile" >"$log" 2>&1); then
    rc=0
  else rc=$?; fi
  [[ $rc -eq 0 ]] || { echo "scenario $profile/$scenario failed rc=$rc; see $log" >&2; return "$rc"; }
  grep -Eq "^GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile$" "$log" || {
    echo "missing exact success marker for $profile/$scenario; see $log" >&2; return 1;
  }
  printf 'GENERATED_DUPLEX_SCENARIO PASS PROFILE=%s CASE=%s\n' "$profile" "$scenario"
}
SCENARIO_JOBS=()
for profile in "${PROFILES[@]}"; do for scenario in "${CASES[@]}"; do SCENARIO_JOBS+=("${profile}__${scenario}"); done; done
dispatch_scenario() { local pair=$1; run_scenario "${pair%%__*}" "${pair#*__}"; }
BURST_JOB_LOG_DIR="$OUT/job_logs/scenario"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$DUPLEX_G1_JOBS" dispatch_scenario "${SCENARIO_JOBS[@]}"

for profile in "${PROFILES[@]}"; do
  for scenario in "${CASES[@]}"; do
    log="$OUT/$profile/scenarios/$scenario.log"
    grep -Eq "^GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile$" "$log" || {
      echo "scenario omitted from completed evidence: $profile/$scenario" >&2; exit 1;
    }
    printf '%s\n' "$(digest "$log")  $profile/scenarios/$scenario.log" >>"$OUT/scenario-sha256.txt"
  done
done
(cd "$OUT" && sha256sum --check scenario-sha256.txt)
printf 'GENERATED_DUPLEX PASS: %d scenarios x %d profiles\n' "${#CASES[@]}" "${#PROFILES[@]}" | tee "$OUT/aggregate.log"
grep -Eq '^GENERATED_DUPLEX PASS: 13 scenarios x 2 profiles$' "$OUT/aggregate.log" || exit 1
echo "Output: $OUT"
