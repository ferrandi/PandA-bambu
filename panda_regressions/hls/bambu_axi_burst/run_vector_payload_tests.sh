#!/usr/bin/env bash
# ModelSim co-simulation for vector payload transport and conservative fallback.
set -Eeuo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../..")
if (($# != 2)); then
  echo "usage: $0 BAMBU FRESH_OUTPUT_DIR" >&2
  exit 2
fi
BAMBU_ARG=$1
OUT_ARG=$2
if [[ $BAMBU_ARG == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -z $BAMBU ]] || BAMBU=$(realpath "$BAMBU"); fi
[[ -n ${BAMBU:-} && -x $BAMBU ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r $SETTINGS ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $PLUGIN ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
[[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-} == 1 ]] || {
  echo "refusing ModelSim execution without BURST_MODELSIM_OUTSIDE_SANDBOX=1" >&2; exit 2;
}
for tool in python3 sha256sum timeout grep sed; do command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }; done
for tool in vsim vlog vlib vmap; do command -v "$tool" >/dev/null || { echo "ModelSim $tool is required in PATH" >&2; exit 2; }; done

OUT=$(realpath -m -- "$OUT_ARG")
[[ ! -e $OUT && ! -L $OUT_ARG ]] || { echo "output path must be fresh: $OUT_ARG" >&2; exit 2; }
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT"
SOURCE="$TESTS/bambu_axi_burst_vector_payload.cpp"
TB="$TESTS/bambu_axi_burst_vector_payload_tb.cpp"
RUNNER="$TESTS/run_vector_payload_tests.sh"
CHECKER="$TESTS/check_burst_results.py"
SIM=$(realpath "$(command -v vsim)")
VLOG=$(realpath "$(command -v vlog)")
MODELSIM_VERSION=$(vsim -version 2>&1 | sed -n '1p')
{
  printf 'bambu=%s\nsettings=%s\nplugin=%s\nsource=%s\ntestbench=%s\nchecker=%s\n' \
    "$BAMBU" "$SETTINGS" "$PLUGIN" "$SOURCE" "$TB" "$CHECKER"
  printf 'modelsim=Questa/ModelSim\nvsim=%s\nvlog=%s\nmodelsim_version=%s\n' \
    "$SIM" "$VLOG" "$MODELSIM_VERSION"
  sha256sum "$BAMBU" "$SETTINGS" "$PLUGIN" "$SOURCE" "$TB" "$CHECKER" "$RUNNER" "$SIM" "$VLOG"
} >"$OUT/provenance.txt"

run_case() {
  local name=$1 width=$2 inplace=$3 expected_engines=$4
  local dir="$OUT/$name" size_bytes status=0
  size_bytes=$((128 * width / 8))
  mkdir -p "$dir/tmp"
  local -a args=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 --clock-period=3.33 --AXI-burst-type=INCREMENTAL
    --memory-allocation-policy=NO_BRAM --tb-memory-mapping=SHARED --no-clean
    --simulate --simulator=MODELSIM --generate-vcd --generate-tb="$TB"
    --tb-param-size="src:$size_bytes" --tb-param-size="dst:$size_bytes"
    --bambu-parameter=m-axi-burst=1 --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
    "--extra-cc-options=-DVECTOR_BITS=$width" "--extra-cc-options=-DVECTOR_IN_PLACE=$inplace")
  {
    printf 'case=%s\nwidth=%s\nin_place=%s\nexpected_engines=%s\n' "$name" "$width" "$inplace" "$expected_engines"
    printf 'TMPDIR=%s\nargv=' "$dir/tmp"; printf '%q ' "$BAMBU" "$SOURCE" "${args[@]}"; printf '\n'
    sha256sum "$SOURCE" "$TB" "$RUNNER"
  } >"$dir/provenance.txt"
  if (cd "$dir" && TMPDIR="$dir/tmp" TMP="$dir/tmp" TEMP="$dir/tmp" \
      timeout 1800 "$BAMBU" "$SOURCE" "${args[@]}" >stdout.log 2>stderr.log); then
    status=0
  else
    status=$?
  fi
  if ((status != 0)); then
    tail -n 100 "$dir/stderr.log" "$dir/stdout.log" >&2 || true
    echo "$name ModelSim co-simulation failed (status=$status); see $dir" >&2
    return "$status"
  fi
  local tb_log="$dir/HLS_output/simulation/testbench.log"
  local marker="AXI_BURST_VECTOR_PAYLOAD_PASS WIDTH=$width MODE=$([[ $inplace == 1 ]] && echo in-place-overlap || echo copy) N=67"
  [[ -s $tb_log ]] && [[ $(grep -Fxc "$marker" "$tb_log" || true) == 1 ]] || {
    echo "$name lacks exact payload/canary marker: $marker" >&2; tail -n 80 "$tb_log" >&2 || true; return 1;
  }
  local rtl="$dir/kernel.v"
  [[ -s $rtl ]] || { echo "$name generated kernel.v is missing" >&2; return 1; }
  local read_count write_count
  read_count=$(grep -Ec 'MinimalAXI4MasterPipelined[[:space:]]*#' "$rtl" || true)
  write_count=$(grep -Ec 'MinimalAXI4MasterPipelinedWrite[[:space:]]*#' "$rtl" || true)
  if [[ $expected_engines == burst && ( $read_count != 1 || $write_count != 1 ) ]]; then
    echo "$name expected one read and one write burst engine; observed read=$read_count write=$write_count" >&2
    return 1
  fi
  if [[ $expected_engines == fallback && ( $read_count != 0 || $write_count != 0 ) ]]; then
    echo "$name expected conservative legacy fallback; observed read=$read_count write=$write_count" >&2
    return 1
  fi
  if [[ $expected_engines == burst ]]; then
    grep -Fq 'bundle=input read=recognized(' "$dir/stderr.log" &&
      grep -Fq 'bundle=output read=not-applicable-no-read-access' "$dir/stderr.log" &&
      grep -Fq 'write=recognized(' "$dir/stderr.log" &&
      grep -Fq '.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(64)' "$rtl" &&
      grep -Fq '.BITSIZE_m_axi_rdata(32)' "$rtl" &&
      grep -Fq '.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)' "$rtl" &&
      grep -Fq '.BITSIZE_m_axi_wdata(32)' "$rtl" || {
        echo "$name compiler log does not prove both vector directions were recognized" >&2; return 1;
      }
  elif [[ $expected_engines == fallback ]]; then
    grep -Fq 'bundle=input read=legacy-fallback(' "$dir/stderr.log" &&
      grep -Fq 'bundle=output' "$dir/stderr.log" &&
      grep -Fq 'write=legacy-fallback(' "$dir/stderr.log" &&
      [[ $(grep -Ec "Bitwidth[[:space:]]+: $width$" "$dir/stderr.log" || true) == 2 ]] &&
      [[ $(grep -Fc 'data width/alignment is not 32-bit/4-byte' "$dir/stderr.log" || true) == 2 ]] || {
        echo "$name compiler log does not prove conservative vector-width fallback" >&2; return 1;
      }
  elif [[ $expected_engines == shared-fallback ]]; then
    [[ $read_count == 0 && $write_count == 0 ]] &&
      grep -Fq 'bundle=shared read=legacy-fallback(' "$dir/stderr.log" &&
      grep -Fq 'write=legacy-fallback(' "$dir/stderr.log" &&
      grep -Fq 'duplex=preanalysis-candidate' "$dir/stderr.log" &&
      grep -Fq 'bundle=shared' "$dir/stderr.log" || {
        echo "$name compiler log does not prove conservative shared-bundle duplex fallback" >&2; return 1;
      }
  fi
  printf 'VECTOR_PAYLOAD_CASE_PASS CASE=%s WIDTH=%s MODE=%s engines=%s\n' \
    "$name" "$width" "$([[ $inplace == 1 ]] && echo in-place || echo copy)" "$expected_engines" \
    | tee -a "$OUT/aggregate.log"
}

run_case vector32-copy 32 0 burst
run_case vector64-copy 64 0 fallback
run_case vector128-copy 128 0 fallback
run_case vector32-overlap 32 1 shared-fallback
printf 'VECTOR_PAYLOAD_TESTS_PASS output=%s\n' "$OUT" | tee -a "$OUT/aggregate.log"
