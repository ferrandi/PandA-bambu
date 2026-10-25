#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <bambu> <fresh-output-directory>" >&2
  exit 2
fi
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
bambu=$1
out_dir=$2
if [[ "$bambu" == */* ]]; then bambu=$(realpath "$bambu"); else bambu=$(command -v "$bambu"); fi
[[ -x "$bambu" ]] || { echo "FAIL: missing Bambu: $1" >&2; exit 2; }
prefix=$(dirname "$(dirname "$bambu")")
if [[ -f "$prefix/settings.sh" ]]; then
  source "$prefix/settings.sh"
  [[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$prefix") ]] || {
    echo "FAIL: settings selected an unexpected BAMBU_HLS" >&2; exit 2;
  }
fi
[[ -n ${BAMBU_HLS:-} && -r "$BAMBU_HLS/lib/panda/clang-16/ASTAnalyzer.so" ]] || {
  echo "FAIL: selected ASTAnalyzer plugin is unavailable" >&2; exit 2;
}
[[ ! -e "$out_dir" && ! -L "$out_dir" ]] || { echo "FAIL: output exists: $out_dir" >&2; exit 2; }
mkdir -p "$out_dir/tmp"
out_dir=$(realpath "$out_dir")
export TMPDIR="$out_dir/tmp" TMP="$out_dir/tmp" TEMP="$out_dir/tmp"
fixture="$script_dir/write_compiler_cases/write_cases.cpp"
set +e
(cd "$out_dir" && "$bambu" "$fixture" --top-fname=copy_distinct_buffers \
  --compiler=I386_CLANG16 --generate-interface=INFER --device-name=xcu250,-2L,figd2104 \
  --clock-period=3.33 --AXI-burst-type=INCREMENTAL --bambu-parameter=experimental-m-axi-burst=1 \
  --bambu-parameter=print-ir-manager=1 --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based \
  --print-dot --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU) >"$out_dir/bambu.stdout.log" 2>"$out_dir/bambu.stderr.log"
status=$?
set -e
python3 "$script_dir/check_copy_handoff.py" "$out_dir"
if [[ $status -ne 0 ]]; then echo "FAIL: Bambu returned $status after emitting checked artifacts" >&2; exit "$status"; fi
echo "COPY_HANDOFF_PASS output=$out_dir"
