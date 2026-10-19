#!/usr/bin/env bash
# Manual M-B1.1 compiler/RTL structure gate; deliberately not part of CI.
# Usage: run_bundle_preanalysis_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../..")
FIXTURE="$ROOT/panda_regressions/hls/bambu_axi_burst/compiler_cases/bundle_preanalysis/bundle_preanalysis.cpp"
DATAFLOW_FIXTURE="$ROOT/panda_regressions/hls/bambu_axi_burst/compiler_cases/bundle_preanalysis/dataflow_bundle_cases.cpp"
CHECKER="$ROOT/panda_regressions/hls/bambu_axi_burst/check_bundle_preanalysis_artifacts.py"
RUNNER="$TESTS/run_bundle_preanalysis_tests.sh"
CORE_SOURCE="$ROOT/src/frontend_analysis/IR_analysis/InterfaceInfer.cpp"
if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <bambu> [<fresh-output-directory; defaults to documentation/tmp/out_bundle_preanalysis>]" >&2
  exit 2
fi
BAMBU_ARG=$1
TMP_ROOT=$(realpath -m "$ROOT/documentation/tmp")
OUT_ARG=${2:-"$TMP_ROOT/out_bundle_preanalysis"}
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU"); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
if [[ -f "$SETTINGS" ]]; then
  # shellcheck disable=SC1090
  source "$SETTINGS"
  if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$PREFIX") ]]; then
    echo "settings.sh selected an unexpected BAMBU_HLS='${BAMBU_HLS:-<unset>}'" >&2; exit 2
  fi
fi
if [[ -z ${BAMBU_HLS:-} ]]; then echo "BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin" >&2; exit 2; fi
PLUGIN="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
if [[ ! -r "$PLUGIN" ]]; then echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; fi
command -v rg >/dev/null || { echo "required tool not found: rg" >&2; exit 2; }
[[ -f "$FIXTURE" && -f "$DATAFLOW_FIXTURE" && -f "$CHECKER" ]] || {
  echo "fixture/checker missing" >&2; exit 2;
}
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output path must not exist or be a symlink: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m "$OUT_ARG")
if [[ "$OUT" != "$TMP_ROOT" && "$OUT" != "$TMP_ROOT/"* ]]; then
  echo "warning: output directory is outside documentation/tmp: $OUT" >&2
fi
mkdir -p "$OUT/tmp"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1

{
  printf 'git_head=%s\nbambu_executable=%s\nbambu_prefix=%s\nBAMBU_HLS=%s\nplugin=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD)" "$BAMBU" "$PREFIX" "$(realpath "$BAMBU_HLS")" "$PLUGIN"
  printf 'fixture=%s\ndataflow_fixture=%s\nchecker=%s\noutput=%s\n' \
    "$FIXTURE" "$DATAFLOW_FIXTURE" "$CHECKER" "$OUT"
  sha256sum "$BAMBU" "$PLUGIN" "$CORE_SOURCE" "$FIXTURE" "$DATAFLOW_FIXTURE" "$CHECKER" "$RUNNER"
  if [[ -f "$SETTINGS" ]]; then sha256sum "$SETTINGS"; fi
} >"$OUT/provenance.txt"
"$BAMBU" --version >"$OUT/bambu_version.txt" 2>&1 || {
  echo "could not record Bambu version; see $OUT/bambu_version.txt" >&2; exit 1;
}

COMMON=(--compiler=I386_CLANG16 --generate-interface=INFER --device-name=xcu250,-2L,figd2104
  --clock-period=3.33 --bambu-parameter=experimental-m-axi-burst=1
  --bambu-parameter=print-ir-manager=1 --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based
  --print-dot --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

run_case() {
  local top=$1 expectation=$2 source=${3:-$FIXTURE} burst_type=${4:-INCREMENTAL}
  local out="$OUT/$1" rc=0
  if [[ -e "$out" || -L "$out" ]]; then
    echo "case output path must be fresh and not a symlink: $out" >&2; return 2;
  fi
  mkdir "$out"
  local args=("--top-fname=$top" "--AXI-burst-type=$burst_type" "${COMMON[@]}")
  printf '%q ' "$BAMBU" "$source" "${args[@]}" >"$out/argv.txt"
  printf '\n' >>"$out/argv.txt"
  {
    cat "$OUT/provenance.txt"
    printf 'case=%s\nexpectation=%s\nsource=%s\naxi_burst_type=%s\nargv=' \
      "$top" "$expectation" "$source" "$burst_type"
    cat "$out/argv.txt"
    sha256sum "$source"
  } >"$out/provenance.txt"
  if (cd "$out" && "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    rc=0
  else
    rc=$?
  fi
  if [[ $rc -ne 0 ]]; then echo "$top compilation failed with rc=$rc; see $out" >&2; return "$rc"; fi
  python3 "$CHECKER" "$expectation" --output "$out" --top "$top"
  printf 'BUNDLE_PREANALYSIS PASS CASE=%s\n' "$top"
}

run_case distinct_normal distinct
run_case distinct_reversed distinct
run_case nonqualifiable_read_normal write_direction_only
run_case nonqualifiable_read_reversed write_direction_only
run_case same_bundle_normal runtime_duplex
run_case same_bundle_reversed runtime_duplex
run_case inplace_pointwise inplace_duplex
run_case mixed_unsupported_store mixed_legacy
run_case unused_same_bundle_normal write_only
run_case unused_same_bundle_reversed write_only
run_case dataflow_forwarded_input dataflow_forwarding "$DATAFLOW_FIXTURE" FIXED

echo "BUNDLE_PREANALYSIS PASS: 11 end-to-end compiler cases; no simulation was run."
echo "BUNDLE_PREANALYSIS NOTE: ordinary/volatile global-read compatibility remains covered by run_frontend_compatibility_tests.sh."
echo "Output: $OUT"
