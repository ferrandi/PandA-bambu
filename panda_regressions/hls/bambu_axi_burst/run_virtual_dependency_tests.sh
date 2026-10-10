#!/usr/bin/env bash
set -uo pipefail

usage() {
  echo "usage: $0 <bambu> <output-directory>" >&2
}

if [[ $# -ne 2 ]]; then
  usage
  exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(realpath "$script_dir/../../..") || exit 2
bambu=$1
out_dir=$2
if [[ "$bambu" == */* ]]; then
  bambu=$(realpath "$bambu") || exit 2
else
  bambu=$(command -v "$bambu" || true)
  [[ -n "$bambu" ]] && bambu=$(realpath "$bambu")
fi
if [[ -z "$bambu" || ! -x "$bambu" ]]; then
  echo "FAIL: Bambu executable is missing or not executable: $1" >&2
  exit 2
fi
BAMBU_PREFIX=$(dirname "$(dirname "$bambu")")
BAMBU_SETTINGS="$BAMBU_PREFIX/settings.sh"
SETTINGS_USED=none
if [[ -f "$BAMBU_SETTINGS" ]]; then
  # shellcheck disable=SC1090
  if ! source "$BAMBU_SETTINGS"; then
    echo "FAIL: could not source Bambu settings: $BAMBU_SETTINGS" >&2
    exit 2
  fi
  SETTINGS_USED=$(realpath "$BAMBU_SETTINGS") || exit 2
  if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$BAMBU_PREFIX") ]]; then
    echo "FAIL: settings selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $BAMBU_PREFIX" >&2
    exit 2
  fi
else
  if [[ -z ${BAMBU_HLS:-} ]]; then
    echo "FAIL: no sibling settings.sh and BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin" >&2
    exit 2
  fi
  BAMBU_HLS=$(realpath "$BAMBU_HLS" 2>/dev/null) || {
    echo "FAIL: caller BAMBU_HLS does not resolve to an existing prefix: ${BAMBU_HLS}" >&2
    exit 2
  }
  export BAMBU_HLS
  if [[ ! -r "$BAMBU_HLS/lib/panda/clang-16/ASTAnalyzer.so" ]]; then
    echo "FAIL: caller ASTAnalyzer plugin is missing: $BAMBU_HLS/lib/panda/clang-16/ASTAnalyzer.so" >&2
    exit 2
  fi
fi

if [[ -e "$out_dir" || -L "$out_dir" ]]; then
  echo "FAIL: output path already exists; refusing stale or user data: $out_dir" >&2
  exit 2
fi
out_dir=$(realpath -m "$out_dir") || exit 2
mkdir -p "$out_dir/tmp"
export TMPDIR="$out_dir/tmp"
export TMP="$TMPDIR"
export TEMP="$TMPDIR"
fixture="$script_dir/compiler_cases/recognized_loops.cpp"

set +e
(
  cd "$out_dir" || exit 2
  "$bambu" "$fixture" \
    --top-fname=renamed_sum \
    --compiler=I386_CLANG16 \
    --generate-interface=INFER \
    --device-name=xcu250,-2L,figd2104 \
    --clock-period=3.33 \
    --AXI-burst-type=INCREMENTAL \
    --bambu-parameter=experimental-m-axi-burst=1 \
    --bambu-parameter=print-ir-manager=1 \
    --extra-cc-options=-DNDEBUG \
    --extra-cc-options=-DBAMBU \
    --debug-classes=InterfaceInfer,BuildVirtualPhi \
    --no-clean
) >"$out_dir/bambu.stdout.log" 2>"$out_dir/bambu.stderr.log"
bambu_status=$?
set -e

echo "Bambu exit status: $bambu_status (stdout/stderr: $out_dir/bambu.*.log)"
set +e
python3 "$script_dir/check_virtual_dependencies.py" "$out_dir"
check_status=$?
set -e
if [[ $check_status -ne 0 ]]; then
  echo "FAIL: virtual-dependency dump validation failed; Bambu status=$bambu_status" >&2
  exit 1
fi
if [[ $bambu_status -ne 0 ]]; then
  echo "FAIL: dump artifacts were complete and validated, but Bambu exited $bambu_status; see logs" >&2
  exit "$bambu_status"
fi
echo "PASS: Bambu exited successfully and both emitted IR stages passed the dependency checks"
