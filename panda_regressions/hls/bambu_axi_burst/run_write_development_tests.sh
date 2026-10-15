#!/usr/bin/env bash
# Manual development-only AXI write-burst tests; deliberately not part of CI.
# Usage: run_write_development_tests.sh {offline|compiler|g2|all} BAMBU OUTPUT_DIR
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

usage() {
  sed -n '2,3p' "${BASH_SOURCE[0]}"
}

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  usage
  exit 0
fi
if (($# != 3)); then
  usage >&2
  exit 2
fi

mode=$1
bambu_arg=$2
out_arg=$3
case "$mode" in offline|compiler|g2|all) ;; *) echo "invalid mode: $mode" >&2; exit 2 ;; esac

if [[ $bambu_arg == */* ]]; then
  bambu=$(realpath "$bambu_arg" 2>/dev/null || true)
else
  bambu=$(command -v "$bambu_arg" || true)
  [[ -z $bambu ]] || bambu=$(realpath "$bambu")
fi
if [[ -z ${bambu:-} || ! -x $bambu ]]; then
  echo "Bambu executable not found: $bambu_arg" >&2
  exit 2
fi
if [[ $mode == g2 || $mode == all ]] && [[ ${BURST_G2_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "g2/all requires BURST_G2_SIM_OUTSIDE_SANDBOX=1 and must run outside the sandbox" >&2
  exit 2
fi
if [[ $mode == g2 || $mode == all ]]; then
  export BURST_G2_SIM_OUTSIDE_SANDBOX=1
fi
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2
  exit 2
fi

out_root=$(realpath -m -- "$out_arg")
mkdir -p "$out_root/tmp"
export TMPDIR="$out_root/tmp" TMP="$out_root/tmp" TEMP="$out_root/tmp"

run_offline() {
  local offline_dir="$out_root/offline"
  mkdir -p "$offline_dir/tmp"
  export TMPDIR="$offline_dir/tmp" TMP="$offline_dir/tmp" TEMP="$offline_dir/tmp"
  python3 -B "$script_dir/test_write_compiler_artifacts.py"
  python3 -B "$script_dir/rtl_tests/test_extract_write_engine.py"
}

run_compiler() {
  local compiler_dir="$out_root/compiler"
  mkdir -p "$compiler_dir/tmp"
  export TMPDIR="$compiler_dir/tmp" TMP="$compiler_dir/tmp" TEMP="$compiler_dir/tmp"
  bash "$script_dir/run_write_compiler_tests.sh" all "$bambu" "$compiler_dir/write"
  bash "$script_dir/run_frontend_compatibility_tests.sh" "$bambu" "$compiler_dir/frontend-compatibility"
}

run_g2() {
  local g2_dir="$out_root/g2"
  mkdir -p "$g2_dir/tmp"
  export TMPDIR="$g2_dir/tmp" TMP="$g2_dir/tmp" TEMP="$g2_dir/tmp"
  local top
  for top in write_count_seed write_with_unused_same_bundle write_with_unused_same_bundle_reversed; do
    bash "$script_dir/rtl_tests/run_generated_write_kernel_tests.sh" \
      "$bambu" "$g2_dir/$top" "$top"
  done
}

case "$mode" in
  offline) run_offline ;;
  compiler) run_compiler ;;
  g2) run_g2 ;;
  all)
    run_offline
    run_compiler
    run_g2
    ;;
esac
printf 'WRITE DEVELOPMENT TESTS PASS: mode=%s output=%s\n' "$mode" "$out_root"
