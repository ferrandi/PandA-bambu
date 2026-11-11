#!/usr/bin/env bash
# Compile and simulate default-profile duplex copy with a live-out reduction.
# Usage: BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../..")
FAMILY="$TESTS"
TESTS="$ROOT/panda_regressions/verilog/axi_burst/rtl_tests"
SOURCE="$FAMILY/compiler_cases/axi_default_profile_cases.cpp"
TB="$TESTS/tb_default_duplex_reduction.sv"
HELPERS="$TESTS/modelsim_helpers.sh"

if (($# != 2)); then
  echo "usage: BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR" >&2
  exit 2
fi
[[ ${BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX:-} == 1 ]] || {
  echo "refusing compiler/simulation unless BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX=1 is set" >&2
  exit 2
}
BAMBU=$(realpath -- "$1")
[[ -x $BAMBU && -s $SOURCE && -s $TB && -s $HELPERS ]] || {
  echo "Bambu or default duplex reduction inputs are missing" >&2; exit 2;
}
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $SETTINGS && -s $PLUGIN ]] || { echo "installed settings or plugin is missing below $PREFIX" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}
source "$HELPERS"
modelsim_resolve

OUT=$2
[[ ! -e $OUT && ! -L $OUT ]] || { echo "output directory must be fresh: $OUT" >&2; exit 2; }
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1
mkdir -p "$TMPDIR"

digest() { sha256sum -- "$1" | awk '{print $1}'; }
{
  printf 'root=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nplugin=%s\n' \
    "$ROOT" "$OUT" "$BAMBU" "$(digest "$BAMBU")" "$SETTINGS" "$PLUGIN"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\n' \
    "$SOURCE" "$(digest "$SOURCE")" "$TB" "$(digest "$TB")"
  "$BAMBU" --version 2>&1 | sed -n '1,2p'
  printf 'modelsim_source=%s\nmodelsim_vsim=%s\n' "$MODELSIM_SOURCE" "$MODELSIM_VSIM"
  modelsim_version
} >"$OUT/provenance.txt"

run_profile() {
  local profile=$1 read_out=$2 read_fifo=$3 explicit=$4
  local profile_kind=default
  [[ $explicit == 0 ]] || profile_kind=explicit
  local dir="$OUT/$profile"
  mkdir -p "$dir/generated" "$dir/tmp"
  local -a args=("$SOURCE" --top-fname=duplex_copy --compiler=I386_CLANG16
    --generate-interface=INFER --device-name=xcu250,-2L,figd2104 --no-clean
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
    --extra-cc-options=-DAXI_DEFAULT_CASE_DUPLEX)
  [[ $explicit == 0 ]] || args+=(--extra-cc-options=-DAXI_DEFAULT_DUPLEX_EXPLICIT_O1)
  {
    printf 'profile=%s\nread_outstanding=%s\nread_fifo=%s\n' "$profile" "$read_out" "$read_fifo"
    printf 'argv='; printf '%q ' "$BAMBU" "${args[@]}"; printf '\n'
  } >"$dir/provenance.txt"
  if ! (cd "$dir/generated" && TMPDIR="$dir/tmp" TMP="$dir/tmp" TEMP="$dir/tmp" \
      "$BAMBU" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$dir/generated/stderr.log" >&2 || true
    echo "Bambu failed for $profile; see $dir/generated" >&2; return 1
  fi
  local rtl="$dir/generated/duplex_copy.v" lib="$dir/generated/panda_libtech.v"
  [[ -s $rtl && -s $lib ]] || { echo "missing generated RTL for $profile" >&2; return 1; }
  grep -Fq "read_max=16($profile_kind) read_outstanding=$read_out($profile_kind) read_fifo=$read_fifo(" \
    "$dir/generated/stderr.log" || {
    echo "Bambu did not report the expected read profile for $profile" >&2; return 1;
  }
  grep -Fq 'Activated distinct-base restricted pointwise duplex m_axi bundle shared' \
    "$dir/generated/stderr.log" || { echo "duplex lowering was not activated for $profile" >&2; return 1; }
  python3 - "$rtl" "$lib" "$read_out" "$read_fifo" <<'PY'
from pathlib import Path
import re
import sys

rtl, lib = (Path(p).read_text(errors="replace") for p in sys.argv[1:3])
expected_read = {"B_MAX": "16", "MAX_OUTSTANDING": sys.argv[3], "FIFO_DEPTH": sys.argv[4]}
expected_write = {"B_MAX": "16", "MAX_OUTSTANDING": sys.argv[3], "FIFO_DEPTH": "32"}
for module, expected in (("MinimalAXI4MasterPipelined", expected_read),
                         ("MinimalAXI4MasterPipelinedWrite", expected_write)):
    if not re.search(rf"(?m)^\s*module\s+{module}\s*\(", lib):
        raise SystemExit(f"generated library lacks {module}")
    matches = list(re.finditer(rf"\b{module}\s*#\s*\((.*?)\)\s*\w+\s*\(", rtl, re.S))
    if len(matches) != 1:
        raise SystemExit(f"expected one {module} instance, got {len(matches)}")
    params = dict(re.findall(r"\.(\w+)\s*\(\s*(\d+)\s*\)", matches[0].group(1)))
    wrong = {name: (params.get(name), value) for name, value in expected.items()
             if params.get(name) != value}
    if wrong:
        raise SystemExit(f"{module} profile mismatch: {wrong}")
if not re.search(r"(?m)^\s*module\s+duplex_copy\s*\(", rtl):
    raise SystemExit("generated duplex_copy top is missing")
PY
  local sim="$dir/modelsim"
  modelsim_compile "$sim" tb_default_duplex_reduction "$rtl" "$lib" "$TB"
  local log="$dir/simulation.log" normalized="$dir/simulation.normalized.log"
  modelsim_run "$sim" tb_default_duplex_reduction "$log" 180 -gREAD_B_MAX=16 -gWRITE_B_MAX=16
  modelsim_normalize_log "$log" "$normalized"
  for count in 0 1 2 17 33 4096; do
    [[ $(grep -Ec "^DEFAULT_DUPLEX_REDUCTION PASS n=$count([[:space:]]|$)" "$normalized" || true) == 1 ]] || {
      cat "$log" >&2; echo "missing exact ModelSim witness for n=$count ($profile)" >&2; return 1;
    }
  done
  grep -Fxq 'DEFAULT_DUPLEX_REDUCTION ALL_CASES PASS' "$normalized" || {
    cat "$log" >&2; echo "missing ModelSim aggregate marker ($profile)" >&2; return 1;
  }
  printf 'DEFAULT_DUPLEX_REDUCTION_MODELSIM PASS PROFILE=%s\n' "$profile"
}

run_profile default_o16 16 256 0
run_profile explicit_o1 1 32 1
echo "DEFAULT_DUPLEX_REDUCTION_MODELSIM ALL_CASES PASS output=$OUT"
