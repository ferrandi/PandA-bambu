#!/usr/bin/env bash
# Manual development-only AXI write-burst tests; deliberately not part of CI.
# Usage: run_write_development_tests.sh {offline|compiler|mdpi-extent|g2|all} BAMBU OUTPUT_DIR
#        run_write_development_tests.sh handoff BAMBU BASELINE_RUN1 BASELINE_RUN2 OUTPUT_DIR
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

usage() {
  sed -n '2,3p' "${BASH_SOURCE[0]}"
}

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  usage
  exit 0
fi
if [[ ${1:-} == handoff ]]; then
  [[ $# == 5 ]] || { usage >&2; echo "handoff requires BAMBU, two validated baseline runs, and a fresh output directory" >&2; exit 2; }
else
  (($# == 3)) || { usage >&2; exit 2; }
fi

mode=$1
if [[ $mode == handoff ]]; then
  bambu_arg=$2
  baseline_run1=$3
  baseline_run2=$4
  out_arg=$5
else
  mode=$1
  bambu_arg=$2
  out_arg=$3
fi
case "$mode" in offline|compiler|mdpi-extent|g2|all|handoff) ;; *) echo "invalid mode: $mode" >&2; exit 2 ;; esac

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

if [[ $mode == handoff ]]; then
  for baseline in "$baseline_run1" "$baseline_run2"; do
    [[ -d $baseline && ! -L $baseline ]] || { echo "handoff baseline directory missing or symlink: $baseline" >&2; exit 2; }
  done
  baseline_run1=$(realpath -- "$baseline_run1")
  baseline_run2=$(realpath -- "$baseline_run2")
  [[ $baseline_run1 != "$baseline_run2" ]] || {
    echo "handoff requires two distinct baseline run directories: $baseline_run1" >&2; exit 2;
  }
fi

if [[ $mode == handoff ]]; then
  [[ ! -e $out_arg && ! -L $out_arg ]] || { echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2; }
  out_root=$(realpath -m -- "$out_arg")
  mkdir -p "$out_root/tmp"
  export TMPDIR="$out_root/tmp" TMP="$out_root/tmp" TEMP="$out_root/tmp"
  baseline_checker="$script_dir/check_copy_baseline_results.py"
  baseline_source="$script_dir/copy_baseline.cpp"
  baseline_tb="$script_dir/copy_baseline_tb.cpp"
  expectations="$script_dir/copy_cycle_expectations.tsv"
  prefix=$(dirname "$(dirname "$bambu")")
  settings="$prefix/settings.sh"
  plugin="$prefix/lib/panda/clang-16/ASTAnalyzer.so"
  [[ -r $settings && -r $plugin ]] || { echo "handoff Bambu settings/plugin missing below $prefix" >&2; exit 2; }
  [[ $(realpath "$settings") == "$prefix/settings.sh" ]] || { echo "unexpected Bambu settings path" >&2; exit 2; }
  for baseline in "$baseline_run1" "$baseline_run2"; do
    python3 -B "$baseline_checker" --results "$baseline" --expectations "$expectations" \
      --source "$baseline_source" --testbench "$baseline_tb" --bambu "$bambu" \
      --settings "$settings" --plugin "$plugin"
  done
  baseline_cases=(copy-burst-two-bundles copy-legacy-two-bundles copy-legacy-shared-bundle)
  for baseline in "$baseline_run1" "$baseline_run2"; do
    [[ -f $baseline/provenance.txt && ! -L $baseline/provenance.txt ]] || {
      echo "handoff baseline provenance missing or not a regular file: $baseline/provenance.txt" >&2; exit 2;
    }
    for case_name in "${baseline_cases[@]}"; do
      manifest="$baseline/$case_name/artifact-sha256.txt"
      [[ -f $manifest && ! -L $manifest ]] || {
        echo "handoff baseline artifact manifest missing or not a regular file: $manifest" >&2; exit 2;
      }
    done
  done
  handoff_out="$out_root/handoff"
  [[ ! -e $handoff_out && ! -L $handoff_out ]] || { echo "handoff output must be fresh: $handoff_out" >&2; exit 2; }
  {
    printf 'mode=handoff\nbambu=%s\nsettings=%s\nplugin=%s\n' "$bambu" "$settings" "$plugin"
    printf 'source_git_head=%s\n' "$(git -C "$script_dir/../../.." rev-parse HEAD)"
    printf 'baseline_run1=%s\nbaseline_run2=%s\n' "$baseline_run1" "$baseline_run2"
    sha256sum "$baseline_run1/provenance.txt" "$baseline_run2/provenance.txt"
    for baseline in "$baseline_run1" "$baseline_run2"; do
      for case_name in "${baseline_cases[@]}"; do
        sha256sum "$baseline/$case_name/artifact-sha256.txt"
      done
    done
    printf 'fixture=%s\nrunner=%s\nchecker=%s\n' \
      "$script_dir/write_compiler_cases/write_cases.cpp" "$script_dir/run_copy_handoff_tests.sh" "$script_dir/check_copy_handoff.py"
    printf 'argv='; printf '%q ' "$bambu" "$script_dir/write_compiler_cases/write_cases.cpp" \
      --top-fname=copy_distinct_buffers --compiler=I386_CLANG16 --generate-interface=INFER \
      --device-name=xcu250,-2L,figd2104 --clock-period=3.33 --AXI-burst-type=INCREMENTAL \
      --bambu-parameter=experimental-m-axi-burst=1 --bambu-parameter=print-ir-manager=1 \
      --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based --print-dot --no-clean \
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU; printf '\n'
    printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
    printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 || true; printf 'bambu_version_end\n'
    sha256sum "$bambu" "$settings" "$plugin" "$script_dir/write_compiler_cases/write_cases.cpp" \
      "$script_dir/run_copy_handoff_tests.sh" "$script_dir/check_copy_handoff.py" \
      "$script_dir/check_copy_baseline_results.py" "$script_dir/copy_baseline.cpp" \
      "$script_dir/copy_baseline_tb.cpp" "$script_dir/copy_cycle_expectations.tsv"
  } >"$out_root/handoff-provenance.txt"
  mkdir -p "$out_root/tmp"
  export TMPDIR="$out_root/tmp" TMP="$out_root/tmp" TEMP="$out_root/tmp"
  bash "$script_dir/run_copy_handoff_tests.sh" "$bambu" "$handoff_out"
  printf 'WRITE DEVELOPMENT TESTS PASS: mode=handoff output=%s\n' "$out_root"
  exit 0
fi

if (($# != 3)); then
  usage >&2
  exit 2
fi

if [[ $mode == g2 || $mode == all ]] && [[ ${BURST_G2_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "g2/all requires BURST_G2_SIM_OUTSIDE_SANDBOX=1 and must run outside the sandbox" >&2
  exit 2
fi
if [[ $mode == g2 || $mode == all ]]; then
  export BURST_G2_SIM_OUTSIDE_SANDBOX=1
fi
if [[ $mode == mdpi-extent ]]; then
  if [[ ${MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "mdpi-extent requires MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1 and must run outside the sandbox" >&2
    exit 2
  fi
  export MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1
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
  export BURST_TEST_TMPDIR="$offline_dir"
  python3 -B "$script_dir/test_write_compiler_artifacts.py"
  python3 -B "$script_dir/../../verilog/axi_burst/rtl_tests/test_extract_write_engine.py"
}

run_compiler() {
  local compiler_dir="$out_root/compiler"
  mkdir -p "$compiler_dir/tmp"
  export TMPDIR="$compiler_dir/tmp" TMP="$compiler_dir/tmp" TEMP="$compiler_dir/tmp"
  bash "$script_dir/run_write_compiler_tests.sh" all "$bambu" "$compiler_dir/write"
  bash "$script_dir/run_frontend_compatibility_tests.sh" "$bambu" "$compiler_dir/frontend-compatibility"
  bash "$script_dir/run_source_restrict_provenance_tests.sh" "$bambu" "$compiler_dir/source-restrict-provenance"
  bash "$script_dir/run_duplex_fast_store_ack_tests.sh" "$bambu" "$compiler_dir/duplex-fast-store-ack"
}

run_mdpi_extent() {
  local extent_dir="$out_root/mdpi-extent"
  mkdir -p "$extent_dir/tmp"
  export TMPDIR="$extent_dir/tmp" TMP="$extent_dir/tmp" TEMP="$extent_dir/tmp"
  bash "$script_dir/run_mdpi_extent_tests.sh" "$bambu" "$extent_dir/results"
}

run_g2() {
  local g2_dir="$out_root/g2"
  mkdir -p "$g2_dir/tmp"
  export TMPDIR="$g2_dir/tmp" TMP="$g2_dir/tmp" TEMP="$g2_dir/tmp"
  local top
  for top in write_count_seed write_with_unused_same_bundle write_with_unused_same_bundle_reversed; do
    bash "$script_dir/../../verilog/axi_burst/rtl_tests/run_generated_write_kernel_tests.sh" \
      "$bambu" "$g2_dir/$top" "$top"
  done
}

case "$mode" in
  offline) run_offline ;;
  compiler) run_compiler ;;
  mdpi-extent) run_mdpi_extent ;;
  g2) run_g2 ;;
  all)
    run_offline
    run_compiler
    run_g2
    ;;
esac
printf 'WRITE DEVELOPMENT TESTS PASS: mode=%s output=%s\n' "$mode" "$out_root"
