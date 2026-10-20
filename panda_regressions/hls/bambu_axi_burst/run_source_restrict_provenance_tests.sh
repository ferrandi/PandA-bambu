#!/usr/bin/env bash
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$HERE/../../..")
if [[ ${1:-} == -h || ${1:-} == --help ]]; then
  echo "Usage: $0 BAMBU [OUTPUT_DIR] (OUTPUT_DIR must not exist; default is ./out_source_restrict_provenance)"
  exit 0
fi
if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 BAMBU [OUTPUT_DIR]" >&2
  exit 2
fi
BAMBU_ARG=${1:?Bambu executable required}
OUT_ARG=${2:-"$PWD/out_source_restrict_provenance"}
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n ${BAMBU:-} ]] && BAMBU=$(realpath "$BAMBU"); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output directory must not exist: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m -- "$OUT_ARG")
mkdir -p -- "$(dirname -- "$OUT")"
mkdir "$OUT"
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
FIXTURE="$HERE/compiler_cases/duplex_effects/source_restrict_provenance.cpp"
CHECKER="$HERE/check_source_restrict_provenance.py"
RUNNER="$HERE/run_source_restrict_provenance_tests.sh"
CORE_SOURCE="$ROOT/src/frontend_analysis/IR_analysis/InterfaceInfer.cpp"
PARALLEL_HELPERS="$HERE/parallel_helpers.sh"
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
if [[ -f "$SETTINGS" ]]; then
  # shellcheck disable=SC1090
  source "$SETTINGS"
fi
[[ -n ${BAMBU_HLS:-} ]] || { echo "BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin" >&2; exit 2; }
PLUGIN="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r "$PLUGIN" ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
digest() { sha256sum "$1" | awk '{print $1}'; }
GIT_HEAD=$(git -C "$ROOT" rev-parse HEAD)
{
  printf 'git_head=%s\n' "$GIT_HEAD"
  for entry in "bambu=$BAMBU" "plugin=$PLUGIN" "core_source=$CORE_SOURCE" \
      "fixture=$FIXTURE" "checker=$CHECKER" "runner=$RUNNER" "parallel_helpers=$PARALLEL_HELPERS"; do
    name=${entry%%=*}
    path=${entry#*=}
    printf '%s=%s\n%s_sha256=%s\n' "$name" "$path" "$name" "$(digest "$path")"
  done
} >"$OUT/provenance.txt"
for top in restrict_both restrict_typedef restrict_absent restrict_one; do
  case_dir="$OUT/$top"
  mkdir -p "$case_dir"
  args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 --no-clean --extra-cc-options=-DNDEBUG
    --extra-cc-options=-DBAMBU)
  printf '%q ' "$BAMBU" "$FIXTURE" "${args[@]}" >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  (cd "$case_dir" && "$BAMBU" "$FIXTURE" "${args[@]}" >stdout.log 2>stderr.log)
  [[ -s "$case_dir/panda-temp/architecture.xml" ]] || {
    echo "$top did not emit nonempty panda-temp/architecture.xml" >&2; exit 1;
  }
done
if ! python3 -B "$HERE/check_source_restrict_provenance.py" --output "$OUT" >"$OUT/aggregate.log" 2>&1; then
  cat "$OUT/aggregate.log" >&2
  exit 1
fi
cat "$OUT/aggregate.log"
printf 'SOURCE_RESTRICT_PROVENANCE OUT=%s\n' "$OUT" | tee -a "$OUT/aggregate.log"
