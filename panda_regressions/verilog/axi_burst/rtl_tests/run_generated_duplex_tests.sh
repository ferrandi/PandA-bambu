#!/usr/bin/env bash
# Manual G1 test: exercise production duplex endpoints using generated RTL.
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
FAMILY="$ROOT/panda_regressions/hls/bambu_axi_burst"
CHECKER="$FAMILY/check_duplex_effect_analysis_artifacts.py"
HELPERS="$FAMILY/parallel_helpers.sh"
MODELSIM_HELPERS="$TESTS/modelsim_helpers.sh"
TB="$TESTS/tb_generated_duplex.sv"
SOURCE32="$FAMILY/compiler_cases/duplex_effect_family_cases.cpp"
SOURCE64="$FAMILY/compiler_cases/duplex_effect_family_cases.cpp"
CASES=(LOAD_WAIT_STORE STORE_WAIT_LOAD WRITE_STALL_READ_PROGRESS READ_STALL_WRITE_PROGRESS
  SIMULTANEOUS CONFIG_ISOLATION ZERO_READ ZERO_WRITE UNPAIRED_DESCRIPTORS INPLACE_POINTWISE
  BRESP_PEER_DRAIN RRESP_PEER_DRAIN FAULT_RESET_RESTART RESET_STALLED_CHANNELS)
PROFILES=(same_base_pointwise same_base_asymmetric_64)
RESET_VARIANTS=(sync-high sync-low async-high async-low)

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
[[ -s "$CHECKER" && -s "$HELPERS" && -s "$MODELSIM_HELPERS" && -s "$TB" && -s "$SOURCE32" && -s "$SOURCE64" ]] || {
  echo "G1 checker/helper/testbench/source missing" >&2; exit 2;
}
source "$HELPERS"
source "$MODELSIM_HELPERS"
modelsim_resolve

OUT_ARG=${2:-"$PWD/out_generated_duplex"}
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
  echo "output directory must be fresh and not a symlink: $OUT_ARG" >&2; exit 2
fi
OUT=$(realpath -m -- "$OUT_ARG")
SIM_JOBS=${SIM_JOBS:-2}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "SIM_JOBS must be a positive integer" >&2; exit 2; }
SIM_JOBS=$((10#$SIM_JOBS)); (( SIM_JOBS <= 2 )) || { echo "SIM_JOBS must not exceed 2" >&2; exit 2; }
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
  printf 'sim_jobs=%s\nscenario_jobs=1\n' "$SIM_JOBS"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  modelsim_version
  sha256sum "$SOURCE32" "$SOURCE64"
} >"$OUT/provenance.txt"

COMMON=(--compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33
  --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
generate_profile() {
  local profile=$1 reset_mode=${2:-} reset_level=${3:-} source out artifact
  if [[ -n $reset_mode ]]; then
    out="$OUT/${profile}_reset_${reset_mode}-${reset_level}"
  else
    out="$OUT/$profile"
  fi
  mkdir "$out" "$out/tmp" "$out/generated"
  if [[ $profile == same_base_pointwise ]]; then source=$SOURCE32; else source=$SOURCE64; fi
  local -a args=("--top-fname=$profile" "${COMMON[@]}")
  if [[ $profile == same_base_pointwise ]]; then
    args+=(--extra-cc-options=-DAXI_DUPLEX_EFFECT_CASE_16)
  else
    args+=(--extra-cc-options=-DAXI_DUPLEX_EFFECT_CASE_15)
  fi
  if [[ -n $reset_mode ]]; then
    args+=("--reset-type=$reset_mode" "--reset-level=$reset_level")
  fi
  [[ $profile != same_base_asymmetric_64 ]] || args+=(--addr-bus-bitsize=64)
  printf '%q ' "$BAMBU" "$source" "${args[@]}" >"$out/compiler_argv.txt"; printf '\n' >>"$out/compiler_argv.txt"
  {
  printf 'profile=%s\nreset_mode=%s\nreset_level=%s\nsource=%s\nsource_sha256=%s\n' \
    "$profile" "${reset_mode:-default}" "${reset_level:-default}" "$source" "$(digest "$source")"
    cat "$out/compiler_argv.txt"
  } >"$out/provenance.txt"
  if ! (cd "$out/generated" && TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" \
      "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 100 "$out/generated/stderr.log" >&2 || true
    echo "Bambu generation failed for $profile; see $out/generated/" >&2; return 1
  fi
  local rtl="$out/generated/$profile.v" lib="$out/generated/panda_libtech.v"
  local architecture="$out/generated/panda-temp/architecture.xml"
  local stdout="$out/generated/stdout.log" stderr="$out/generated/stderr.log"
  for artifact in "$rtl" "$lib" "$architecture"; do
    [[ -s "$artifact" && ! -L "$artifact" ]] || {
      echo "missing generated functional artifact for $profile: $artifact" >&2; return 1;
    }
  done
  for artifact in "$stdout" "$stderr"; do
    [[ -f "$artifact" && ! -L "$artifact" ]] || {
      echo "missing compiler log for $profile: $artifact" >&2; return 1;
    }
  done
  {
    printf 'top=%s\nsource=%s\nsource_sha256=%s\nbambu=%s\nbambu_sha256=%s\n' \
      "$profile" "$source" "$(digest "$source")" "$BAMBU" "$(digest "$BAMBU")"
    printf 'plugin=%s\nplugin_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
      "$PLUGIN" "$(digest "$PLUGIN")" "$CHECKER" "$(digest "$CHECKER")" \
      "$TESTS/run_generated_duplex_tests.sh" "$(digest "$TESTS/run_generated_duplex_tests.sh")"
    printf 'rtl=%s\nrtl_sha256=%s\narchitecture=%s\narchitecture_sha256=%s\n' \
      "$rtl" "$(digest "$rtl")" "$architecture" "$(digest "$architecture")"
    printf 'stdout=%s\nstdout_sha256=%s\nstderr=%s\nstderr_sha256=%s\n' \
      "$stdout" "$(digest "$stdout")" "$stderr" "$(digest "$stderr")"
  } >"$out/generated/provenance.txt"
  python3 "$CHECKER" "$profile" --functional-only --output "$out/generated" --source "$source" --bambu "$BAMBU" \
    --plugin "$PLUGIN" --checker "$CHECKER" --runner "$TESTS/run_generated_duplex_tests.sh" || return $?
  sha256sum "$rtl" "$lib" >>"$out/provenance.txt"
  local -a overrides
  if [[ $profile == same_base_pointwise ]]; then
    overrides=(-gADDRESS_BITS=32 -gREAD_B_MAX=16 -gREAD_OUTSTANDING=1 -gREAD_FIFO_DEPTH=32
      -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  else
    overrides=(-gADDRESS_BITS=64 -gREAD_B_MAX=8 -gREAD_OUTSTANDING=2 -gREAD_FIFO_DEPTH=32
      -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  fi
  printf 'modelsim_elaboration_args=' >>"$out/provenance.txt"
  printf '%q ' "${overrides[@]}" >>"$out/provenance.txt"
  printf '\n' >>"$out/provenance.txt"
  printf 'reset_mode=%s reset_level=%s\n' "${reset_mode:-default}" "${reset_level:-default}" >>"$out/provenance.txt"
  if ! modelsim_compile "$out/sim" tb_generated_duplex "$rtl" "$lib" "$TB"; then
    echo "ModelSim build failed for $profile" >&2; return 1
  fi
  sha256sum "$TB" "$TESTS/run_generated_duplex_tests.sh" "$out/sim/compile.log" >>"$out/provenance.txt"
  printf 'GENERATED_DUPLEX_BUILD PASS PROFILE=%s RESET=%s LEVEL=%s\n' \
    "$profile" "${reset_mode:-default}" "${reset_level:-default}"
}
build_reset_profile() {
  local variant=$1 reset_mode=${1%%-*} reset_level=${1#*-}
  generate_profile same_base_pointwise "$reset_mode" "$reset_level"
  printf 'GENERATED_DUPLEX_RESET_BUILD PASS VARIANT=%s\n' "$variant"
}

# Two independent profile builds maximum; scenario runs start only after both binaries exist.
BURST_JOB_LOG_DIR="$OUT/job_logs/build"; mkdir "$BURST_JOB_LOG_DIR"
BURST_QUIET_JOB_STATUS=1
run_bounded_jobs "$SIM_JOBS" generate_profile "${PROFILES[@]}"
for profile in "${PROFILES[@]}"; do
  grep -Eq "^GENERATED_DUPLEX_BUILD PASS PROFILE=$profile RESET=default LEVEL=default$" \
    "$BURST_JOB_LOG_DIR/$profile.log" || {
    echo "missing build completion marker for $profile" >&2; exit 1;
  }
done
BURST_JOB_LOG_DIR="$OUT/job_logs/reset-build"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" build_reset_profile "${RESET_VARIANTS[@]}"
for variant in "${RESET_VARIANTS[@]}"; do
  grep -Fqx "GENERATED_DUPLEX_RESET_BUILD PASS VARIANT=$variant" "$BURST_JOB_LOG_DIR/$variant.log" || {
    echo "missing reset variant completion marker for $variant" >&2; exit 1;
  }
done

run_scenario() {
  local profile=$1 scenario=$2 reset_mode=${3:-} reset_level=${4:-} profile_out sim_build reset_active=0 reset_async=0 log raw rc
  if [[ -n $reset_mode ]]; then
    profile_out="$OUT/${profile}_reset_${reset_mode}-${reset_level}"
    [[ $reset_level != high ]] || reset_active=1
    [[ $reset_mode != async ]] || reset_async=1
  else
    profile_out="$OUT/$profile"
  fi
  sim_build="$profile_out/sim"
  log="$profile_out/scenarios/$scenario.log"
  mkdir -p "$(dirname "$log")"
  local simulation_tmp="$profile_out/scenarios/tmp-$scenario"; mkdir "$simulation_tmp"
  raw="$profile_out/scenarios/$scenario.raw.log"
  local -a overrides fault_arg=() expected_faults=() observed_faults=()
  if [[ $profile == same_base_pointwise ]]; then
    overrides=(-gADDRESS_BITS=32 -gREAD_B_MAX=16 -gREAD_OUTSTANDING=1 -gREAD_FIFO_DEPTH=32
      -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  else
    overrides=(-gADDRESS_BITS=64 -gREAD_B_MAX=8 -gREAD_OUTSTANDING=2 -gREAD_FIFO_DEPTH=32
      -gWRITE_B_MAX=16 -gWRITE_OUTSTANDING=1 -gWRITE_FIFO_DEPTH=16)
  fi
  if [[ -n $reset_mode ]]; then
    overrides+=("-gRESET_ACTIVE_LEVEL=$reset_active" "-gRESET_ASYNC=$reset_async")
  fi
  case $scenario in
    BRESP_PEER_DRAIN|RRESP_PEER_DRAIN|FAULT_RESET_RESTART) fault_arg=(+AXI_BURST_EXPECT_FAULT) ;;
  esac
  if (cd "$simulation_tmp" && TMPDIR="$simulation_tmp" TMP="$simulation_tmp" TEMP="$simulation_tmp" \
      modelsim_run "$sim_build" tb_generated_duplex "$raw" 180 "${overrides[@]}" -- "+CASE=$scenario" "+PROFILE=$profile" "${fault_arg[@]}"); then
    rc=0
  else rc=$?; fi
  [[ $rc -eq 0 ]] || { echo "scenario $profile/$scenario failed rc=$rc; see $raw" >&2; return "$rc"; }
  modelsim_normalize_log "$raw" "$log"
  grep -Eq "^GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile$" "$log" || {
    echo "missing exact success marker for $profile/$scenario; see $log" >&2; return 1;
  }
  case $scenario in
    BRESP_PEER_DRAIN)
      expected_faults=("AXI_BURST_FAULT engine=write reason=BRESP instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic" "AXI_BURST_PEER_FAULT engine=read reason=peer instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic" "AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic") ;;
    RRESP_PEER_DRAIN)
      expected_faults=("AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic" "AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic" "AXI_BURST_PEER_FAULT engine=read reason=peer instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic") ;;
    FAULT_RESET_RESTART)
      expected_faults=("AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic") ;;
  esac
  mapfile -t observed_faults < <(sed -E 's/^\*\* Fatal: //' "$log" | grep -E '^AXI_BURST_' || true)
  if ((${#observed_faults[@]} != ${#expected_faults[@]})); then
    echo "wrong AXI diagnostic count for $profile/$scenario: expected=${#expected_faults[@]} observed=${#observed_faults[@]}" >&2
    cat "$log" >&2; return 1
  fi
  local i record
  for i in "${!expected_faults[@]}"; do
    record=${observed_faults[i]}
    [[ $record == "${expected_faults[i]}"* ]] || { echo "wrong AXI diagnostic order/content for $profile/$scenario at $i: $record" >&2; return 1; }
    if [[ ${expected_faults[i]} == *engine=read* ]]; then
      [[ $record =~ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ resp=[01]{2}\ id=-?[0-9]+\ last=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed read diagnostic for $profile/$scenario" >&2; return 1; }
    else
      [[ $record =~ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ payload=[[:xdigit:]]+\ resp=[01]{2}\ id=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed write diagnostic for $profile/$scenario" >&2; return 1; }
    fi
  done
  if [[ $scenario == RESET_STALLED_CHANNELS ]]; then
    local channel
    for channel in AR R_RESPONSE_WAIT AW W B_RESPONSE_WAIT; do
      grep -Fqx "GENERATED_RESET_CHANNEL_PASS channel=$channel async=$reset_async active_level=$reset_active" "$log" || {
        echo "missing reset observation for $profile/$channel; see $log" >&2; return 1;
      }
    done
    grep -Fqx "GENERATED_RESET_B_RESPONSE_WAIT BVALID=0 BREADY=1" "$log" || {
      echo "missing legal pending-B-response observation for $profile; see $log" >&2; return 1;
    }
    grep -Eq '^GENERATED_RESET_R_RESPONSE_WAIT descriptor_count=[1-9][0-9]* credits=[0-9]+ RVALID=0 RREADY=1$' "$log" || {
      echo "missing legal pending-R-response observation for $profile; see $log" >&2; return 1;
    }
    grep -Fqx "GENERATED_CACHE_RESET_PASS async=$reset_async active_level=$reset_active" "$log" || {
      echo "missing setup/cache-reset observation for $profile; see $log" >&2; return 1;
    }
  fi
  printf 'GENERATED_DUPLEX_SCENARIO PASS PROFILE=%s CASE=%s\n' "$profile" "$scenario"
}
run_profile_scenarios() { local profile=$1 scenario; for scenario in "${CASES[@]}"; do run_scenario "$profile" "$scenario" || return $?; done; }
BURST_JOB_LOG_DIR="$OUT/job_logs/scenario"; mkdir "$BURST_JOB_LOG_DIR"
run_bounded_jobs "$SIM_JOBS" run_profile_scenarios "${PROFILES[@]}"
for variant in "${RESET_VARIANTS[@]}"; do
  reset_mode=${variant%%-*}; reset_level=${variant#*-}
  run_scenario same_base_pointwise RESET_STALLED_CHANNELS "$reset_mode" "$reset_level"
done

for profile in "${PROFILES[@]}"; do
  for scenario in "${CASES[@]}"; do
    log="$OUT/$profile/scenarios/$scenario.log"
    grep -Eq "^GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile$" "$log" || {
      echo "scenario omitted from completed evidence: $profile/$scenario" >&2; exit 1;
    }
    printf '%s\n' "$(digest "$log")  $profile/scenarios/$scenario.log" >>"$OUT/scenario-sha256.txt"
  done
done
for variant in "${RESET_VARIANTS[@]}"; do
  log="$OUT/same_base_pointwise_reset_${variant}/scenarios/RESET_STALLED_CHANNELS.log"
  grep -Fqx "GENERATED_DUPLEX PASS CASE=RESET_STALLED_CHANNELS PROFILE=same_base_pointwise" "$log" || {
    echo "reset variant omitted from completed evidence: $variant" >&2; exit 1;
  }
  printf '%s\n' "$(digest "$log")  same_base_pointwise_reset_${variant}/scenarios/RESET_STALLED_CHANNELS.log" \
    >>"$OUT/scenario-sha256.txt"
done
(cd "$OUT" && sha256sum --check scenario-sha256.txt)
printf 'GENERATED_DUPLEX PASS: %d scenarios x %d profiles plus %d reset variants\n' \
  "${#CASES[@]}" "${#PROFILES[@]}" "${#RESET_VARIANTS[@]}" | tee "$OUT/aggregate.log"
grep -Eq '^GENERATED_DUPLEX PASS: 14 scenarios x 2 profiles plus 4 reset variants$' "$OUT/aggregate.log" || exit 1
echo "Output: $OUT"
