#!/usr/bin/env bash
# Generate (or reuse) Bambu RTL and exercise its full-width 6-bit RID map.
# Usage: run_generated_wrapper_rid_tests.sh BAMBU GENERATED_DIR FRESH_OUTPUT_DIR
set -euo pipefail

TESTS=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
BAMBU=${1:-$ROOT/install/bin/bambu}
if [[ "$BAMBU" == */* ]]; then
  BAMBU=$(realpath "$BAMBU")
else
  BAMBU=$(command -v "$BAMBU" || true)
  [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU")
fi
[[ -n "$BAMBU" && -x "$BAMBU" ]] || { echo "Bambu executable not found: ${1:-$BAMBU}" >&2; exit 2; }

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
source "$TESTS/modelsim_helpers.sh"
for tool in python3 sha256sum timeout grep tail sed tr; do
  command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }
done
modelsim_resolve
MODELSIM_VERSION=$(modelsim_version) || exit 2

if (($# != 3)); then
  echo "usage: $0 BAMBU GENERATED_DIR FRESH_OUTPUT_DIR" >&2
  exit 2
fi
if [[ -e "$3" || -L "$3" ]]; then
  echo "output directory must not already exist: $3" >&2; exit 2
fi
TASK_OUT=$(realpath -m "$3")
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
: >"$TASK_OUT/aggregate.log"

SOURCE="$ROOT/panda_regressions/hls/bambu_axi_burst/bambu_axi_burst_sum_o1.cpp"
GENERATION_ARGS=("$SOURCE" --top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
  --bambu-parameter=experimental-m-axi-burst=1 --mem-delay-read=64 --mem-delay-write=64
  --memory-allocation-policy=NO_BRAM --distram-threshold=0
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU --extra-cc-options=-DNOCACHE)
GEN=${2:-}
if [[ -n "$GEN" ]]; then
  GEN=$(realpath "$GEN")
  [[ -s "$GEN/kernel.v" && -s "$GEN/panda_libtech.v" ]] || {
    echo "existing generated RTL directory lacks kernel.v or panda_libtech.v: $GEN" >&2; exit 2;
  }
  GENERATION_MODE=shared-generated-tree
else
  GEN="$TASK_OUT/compiler"
  mkdir -p "$GEN"
  GENERATION_MODE=generate-here
fi
RTL="$GEN/kernel.v"
LIB="$GEN/panda_libtech.v"
TB="$TESTS/tb_generated_wrapper_rid.sv"
{ printf 'generation_mode=%s\nargv=' "$GENERATION_MODE"; printf '%q ' "$BAMBU" "${GENERATION_ARGS[@]}"; printf '\n'; } \
  >"$TASK_OUT/generation.command"
if [[ "$GENERATION_MODE" == generate-here ]]; then
  if ! (cd "$GEN" && "$BAMBU" "${GENERATION_ARGS[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
    tail -n 100 "$GEN/bambu.stderr.log" >&2 || true
    tail -n 100 "$GEN/bambu.stdout.log" >&2 || true
    echo "Bambu generation for wrapper RID test failed; see $GEN/bambu.*.log" >&2
    exit 1
  fi
fi
for artifact in "$RTL" "$LIB"; do
  [[ -s "$artifact" ]] || { echo "missing generated artifact: $artifact" >&2; exit 2; }
done
if [[ -s "$GEN/bambu.stderr.log" ]]; then
  grep -Eq 'Configured read burst regions for bundle src \(B=16\)' "$GEN/bambu.stderr.log" || {
    echo "Bambu did not configure the src burst region; refusing a legacy-path wrapper test" >&2; exit 2;
  }
fi
python3 - "$RTL" <<'PY'
from pathlib import Path
import re
import sys

normalized = re.sub(r"\s+", "", Path(sys.argv[1]).read_text())
expected = ("MinimalAXI4MasterPipelined#(.B_MAX(16),"
            ".MAX_OUTSTANDING(1),.FIFO_DEPTH(256)")
if expected not in normalized:
    raise SystemExit("generated wrapper burst parameters do not match fixture B=16/O=1/D=256")
for width_param in (".BITSIZE_m_axi_araddr(32)", ".BITSIZE_m_axi_rdata(32)", ".BITSIZE_in5(32)"):
    if width_param not in normalized:
        raise SystemExit(f"generated wrapper lacks width parameter {width_param}")
PY
grep -Eq '^module src_bambu_artificial_ParmMgr_modgen\(' "$RTL" || {
  echo "expected generated m_axi resource wrapper is absent" >&2; exit 2;
}
grep -Eq 'wire burst_rid = \|p_m_axi_src_rid;' "$RTL" || {
  echo "generated wrapper does not contain the expected full-width RID reduction" >&2; exit 2;
}

{
  printf 'root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
    "$ROOT" "$TASK_OUT" "$BAMBU" "$PREFIX" "$(realpath "$SETTINGS")" "$PLUGIN"
  printf 'generated_dir=%s\ngeneration_mode=%s\nsource=%s\ntestbench=%s\n' \
    "$GEN" "$GENERATION_MODE" "$SOURCE" "$TB"
  printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  printf 'modelsim_lookup_source=%s\nmodelsim_bin=%s\nvsim=%s\nvlib=%s\nvlog=%s\nvmap=%s\n' \
    "$MODELSIM_SOURCE" "$MODELSIM_BIN" "$MODELSIM_VSIM" "$MODELSIM_VLIB" \
    "$MODELSIM_VLOG" "$MODELSIM_VMAP"
  printf 'modelsim_version=%s\n' "$(printf '%s' "$MODELSIM_VERSION" | tr '\n' ' ')"
  sha256sum "$BAMBU" "$PLUGIN" "$SETTINGS" "$SOURCE" "$TB" "$RTL" "$LIB"
} >"$TASK_OUT/provenance.txt"

compile_and_run() {
  local name=$1 source_rtl=$2 source_lib=$3 runtime_arg=${4:-}
  local build="$TASK_OUT/$name/modelsim"
  local log="$TASK_OUT/$name/run.log"
  mkdir -p "$TASK_OUT/$name"
  if ! modelsim_compile "$build" tb_generated_wrapper_rid "$source_rtl" "$source_lib" "$TB"; then
    echo "ModelSim compile failed for $name; see $build/compile.log" >&2
    return 1
  fi
  local status=0
  if [[ -n "$runtime_arg" ]]; then
    modelsim_run "$build" tb_generated_wrapper_rid "$log" 60 -- "$runtime_arg" || status=$?
  else
    modelsim_run "$build" tb_generated_wrapper_rid "$log" 60 || status=$?
  fi
  modelsim_normalize_log "$log" "$log.normalized"
  if ((status != 0)) || grep -Eqi '^\*\* (Error|Fatal):|Segmentation fault|stack smashing|AddressSanitizer|Aborted' "$log.normalized"; then
    cat "$log" >&2
    echo "ModelSim wrapper RID simulation failed for $name (status=$status)" >&2
    return 1
  fi
  cat "$log.normalized" | tee -a "$TASK_OUT/aggregate.log"
}

compile_and_run positive "$RTL" "$LIB"
for rid in 0 1 2 32 63; do
  completion=0
  [[ "$rid" == 0 ]] && completion=1
  if [[ $(grep -Fxc "RID_CASE PASS rid=$rid completion=$completion" "$TASK_OUT/positive/run.log.normalized" || true) != 1 ]]; then
    cat "$TASK_OUT/positive/run.log" >&2
    echo "positive wrapper RID simulation lacks exact completion witness for RID=$rid" >&2
    exit 1
  fi
done
if [[ $(grep -Fxc 'GENERATED_WRAPPER_RID PASS' "$TASK_OUT/positive/run.log.normalized" || true) != 1 ]]; then
  echo "positive wrapper RID simulation lacks exact aggregate marker" >&2
  exit 1
fi

# A deliberately broken bit-0-only reduction is made only in a scratch copy
# of the generated file. Unlike the positive scoreboard, this run must finish:
# it demonstrates that both RID 2 and RID 32 are incorrectly accepted.
mkdir -p "$TASK_OUT/negative"
cp "$RTL" "$TASK_OUT/negative/renamed_sum.v"
python3 - "$TASK_OUT/negative/renamed_sum.v" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
correct = "wire burst_rid = |p_m_axi_src_rid;"
broken = "wire burst_rid = p_m_axi_src_rid[0];"
if text.count(correct) != 1:
    raise SystemExit("expected exactly one full-width generated RID reduction")
path.write_text(text.replace(correct, broken))
PY
compile_and_run negative "$TASK_OUT/negative/renamed_sum.v" "$LIB" +NEGATIVE_SENSITIVITY
for rid in 2 32; do
  if [[ $(grep -Fxc "RID_CASE PASS rid=$rid completion=1" "$TASK_OUT/negative/run.log.normalized" || true) != 1 ]]; then
    cat "$TASK_OUT/negative/run.log" >&2
    echo "mutated wrapper RID simulation lacks expected false completion for RID=$rid" >&2
    exit 1
  fi
done
if [[ $(grep -Fxc 'RID_NEGATIVE_SENSITIVITY PASS (bit-0-only reduction accepted 2 and 32)' \
    "$TASK_OUT/negative/run.log.normalized" || true) != 1 ]]; then
  cat "$TASK_OUT/negative/run.log" >&2
  echo "mutated wrapper RID simulation lacks exact negative-sensitivity marker" >&2
  exit 1
fi
echo "GENERATED_WRAPPER_RID_TESTS PASS: $TASK_OUT" | tee -a "$TASK_OUT/aggregate.log"
