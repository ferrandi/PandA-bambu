#!/usr/bin/env bash
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$HERE/../../..")
if [[ ${1:-} == -h || ${1:-} == --help ]]; then
  echo "Usage: $0 BAMBU OUTPUT_DIR (OUTPUT_DIR must not exist)"
  exit 0
fi
BAMBU_ARG=${1:?Bambu executable required}
OUT_ARG=${2:?fresh output directory required}
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output directory must not exist: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
FIXTURE="$HERE/compiler_cases/duplex_effects/source_restrict_provenance.cpp"
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
python3 -B "$HERE/check_source_restrict_provenance.py" --output "$OUT"
