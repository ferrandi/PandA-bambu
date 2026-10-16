#!/usr/bin/env bash
# Manual, analysis-only same-base duplex candidate gate; deliberately not part of CI.
# Usage: run_duplex_effect_analysis_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../..")
SOURCE="$ROOT/panda_regressions/hls/bambu_axi_burst/compiler_cases/duplex_effects/duplex_effects.cpp"
CHECKER="$ROOT/panda_regressions/hls/bambu_axi_burst/check_duplex_effect_analysis_artifacts.py"
RUNNER="$TESTS/run_duplex_effect_analysis_tests.sh"
CORE_SOURCE="$ROOT/src/frontend_analysis/IR_analysis/InterfaceInfer.cpp"

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <bambu> [<fresh-output-directory>]" >&2
  exit 2
fi
BAMBU_ARG=$1
OUT_ARG=${2:-"$ROOT/documentation/tmp/out_duplex_effect_analysis"}
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
command -v sha256sum >/dev/null || { echo "required tool not found: sha256sum" >&2; exit 2; }
[[ -f "$SOURCE" && -f "$CHECKER" && -f "$CORE_SOURCE" ]] || { echo "source/checker missing" >&2; exit 2; }
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output path must not exist or be a symlink: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp"
OUT=$(realpath "$OUT")
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp" PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum "$1" | awk '{print $1}'; }
{
  printf 'git_head=%s\nbambu=%s\nbambu_sha256=%s\nplugin=%s\nplugin_sha256=%s\n' \
    "$(git -C "$ROOT" rev-parse HEAD)" "$BAMBU" "$(digest "$BAMBU")" "$PLUGIN" "$(digest "$PLUGIN")"
  printf 'source=%s\nsource_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\ncore_source=%s\ncore_source_sha256=%s\noutput=%s\n' \
    "$SOURCE" "$(digest "$SOURCE")" "$CHECKER" "$(digest "$CHECKER")" "$RUNNER" "$(digest "$RUNNER")" \
    "$CORE_SOURCE" "$(digest "$CORE_SOURCE")" "$OUT"
  if [[ -f "$SETTINGS" ]]; then printf 'settings=%s\nsettings_sha256=%s\n' "$SETTINGS" "$(digest "$SETTINGS")"; fi
} >"$OUT/run_provenance.txt"
"$BAMBU" --version >"$OUT/bambu_version.txt" 2>&1 || { echo "could not record Bambu version" >&2; exit 1; }

COMMON=(--AXI-burst-type=INCREMENTAL --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33
  --bambu-parameter=experimental-m-axi-burst=1 --bambu-parameter=print-ir-manager=1
  --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based --print-dot --no-clean
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
CASES=(same_base_pointwise same_base_pointwise_reversed shifted_index stride_index
  same_base_cached conditional_store residual_store volatile_effect separate_phase distinct_bases)

run_case() {
  local top=$1 out="$OUT/$1" rc=0
  if [[ -e "$out" || -L "$out" ]]; then echo "case output path must be fresh and not a symlink: $out" >&2; return 2; fi
  mkdir "$out"
  local args=("--top-fname=$top" "${COMMON[@]}")
  {
    printf 'top=%s\nsource=%s\nsource_sha256=%s\nbambu=%s\nbambu_sha256=%s\nplugin=%s\nplugin_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
      "$top" "$SOURCE" "$(digest "$SOURCE")" "$BAMBU" "$(digest "$BAMBU")" \
      "$PLUGIN" "$(digest "$PLUGIN")" "$CHECKER" "$(digest "$CHECKER")" "$RUNNER" "$(digest "$RUNNER")"
  } >"$out/provenance.txt"
  printf '%q ' "$BAMBU" "$SOURCE" "${args[@]}" >"$out/argv.txt"
  printf '\n' >>"$out/argv.txt"
  if (cd "$out" && "$BAMBU" "$SOURCE" "${args[@]}" >stdout.log 2>stderr.log); then rc=0; else rc=$?; fi
  if [[ $rc -ne 0 ]]; then echo "$top compilation failed with rc=$rc; see $out" >&2; return "$rc"; fi
  local raw_ir="$out/panda-temp/before_Frontend::InterfaceInfer.raw"
  if [[ ! -s "$raw_ir" || -L "$raw_ir" ]]; then echo "$top did not retain a regular pre-InterfaceInfer raw IR dump" >&2; return 1; fi
  {
    printf 'raw_ir=%s\nraw_ir_sha256=%s\n' "$raw_ir" "$(digest "$raw_ir")"
  } >>"$out/provenance.txt"
  python3 "$CHECKER" "$top" --output "$out" --source "$SOURCE" --bambu "$BAMBU" \
    --plugin "$PLUGIN" --checker "$CHECKER" --runner "$RUNNER"
  printf 'DUPLEX_EFFECT_ANALYSIS PASS CASE=%s\n' "$top"
}

for top in "${CASES[@]}"; do run_case "$top"; done
printf 'DUPLEX_EFFECT_ANALYSIS PASS: %d compiler cases; candidate analysis only, no RTL simulation.\n' "${#CASES[@]}"
echo "Output: $OUT"
