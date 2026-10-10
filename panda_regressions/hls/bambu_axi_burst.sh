#!/usr/bin/env bash
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(realpath "$script_dir/../..")
OUT_SUFFIX="bambu_axi_burst"
source "$script_dir/bambu_axi_burst/parallel_helpers.sh"

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  printf 'Usage: %s [Mantis options] [-o DIR|--output DIR] [-j N|--parallel N]\n' "${BASH_SOURCE[0]}"
  printf 'Default output: ./out_%s (must not already exist).\n' "$OUT_SUFFIX"
  printf 'Other options are passed through to Mantis; -j bounds parallel jobs (default: J or 1).\n'
  exit 0
fi

if ! normalize_regression_args "$@"; then
  exit 2
fi
parallelism=$BURST_PARALLELISM
mantis_args=("${BURST_NORMALIZED_ARGS[@]}")
out_root=$BURST_OUTPUT

# Fail closed before invoking any Bambu executable. -L also catches dangling
# symlinks, which `-e` alone would miss. With --restart the root must already
# exist; Mantis validates its own previous execution list.
if [[ -z ${BURST_RESTART:-} ]] && { [[ -e $out_root ]] || [[ -L $out_root ]]; }; then
  echo "output path already exists; refusing to overwrite: $out_root" >&2
  exit 2
fi

if ! bambu=$(command -v bambu); then
  echo "configured Bambu executable is missing from PATH" >&2
  exit 2
fi
bambu=$(realpath "$bambu")
bambu_prefix=$(dirname "$(dirname "$bambu")")
settings="$bambu_prefix/settings.sh"
if [[ ! -r $settings ]]; then
  echo "installed Bambu settings.sh is missing: $settings" >&2
  exit 2
fi
# shellcheck disable=SC1090
source "$settings"
IFS=$' \t\n'
if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$bambu_prefix") ]]; then
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $bambu_prefix" >&2
  exit 2
fi
scratch=$out_root
# Keep every artifact under the selected output root. Mantis requires its own
# -o directory to be fresh, so reserve a child for its outputs and use a
# sibling child for temporary files.
mkdir -p "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"
export BURST_TEST_TMPDIR="$scratch/tmp"
out="$scratch/mantis"

# Mantis cleanup would remove bambu_results.xml before the cycle gate runs.
# Retain cases internally, then apply Mantis-style cleanup after validation.
python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
  --args="--configuration-name=AXI-BURST --compiler=I386_CLANG16 --experimental-setup=BAMBU --generate-interface=INFER --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32 --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33 --generate-vcd --no-clean --extra-cc-options=-DNOCACHE --simulate --simulator=MODELSIM" \
  --no-clean --returnfail -l "$script_dir/bambu_axi_burst_list" -o "$out" -b "$script_dir/bambu_axi_burst" "${mantis_args[@]}"

if [[ ! -d $out || -L $out ]]; then
  echo "Mantis completed without creating its output directory: $out" >&2
  exit 1
fi

python3 -B "$script_dir/bambu_axi_burst/check_burst_results.py" \
  --results "$out" \
  --expectations "$script_dir/bambu_axi_burst/cycle_expectations.tsv"

# Mantis and the cycle checker have completed. Put all remaining helper output
# and temporary files beneath the selected out_ root.
mkdir -p "$scratch/jobs" "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"

# Offline checks for the cycle gate, latency-metadata runner, and exported-tree
# bootstraps do not need another Bambu invocation.
python3 -B "$script_dir/bambu_axi_burst/test_check_burst_results.py"
python3 -B "$script_dir/bambu_axi_burst/test_axi_latency_metadata.py"
python3 -B "$script_dir/bambu_axi_burst/test_simulation_logging.py"
bash "$script_dir/bambu_axi_burst/test_compiler_runner_export_root.sh"

run_compiler_suite() {
  local mode=$1 destination="$scratch/compiler-$1"
  bash "$script_dir/bambu_axi_burst/run_compiler_tests.sh" "$mode" "$bambu" "$destination"
}

run_post_mantis_job() {
  local job=$1
  case $job in
    compiler-pragma|compiler-burst-profile|compiler-xml-profile|compiler-latency-metadata|compiler-fallback-matrix|compiler-root-reinvocation|compiler-shared-resource)
      run_compiler_suite "${job#compiler-}"
      ;;
    virtual-dependencies)
      bash "$script_dir/bambu_axi_burst/run_virtual_dependency_tests.sh" \
        "$bambu" "$scratch/virtual-dependencies"
      ;;
    rtl-verilator)
      bash "$script_dir/bambu_axi_burst/rtl_tests/run_verilator.sh" "$scratch/rtl-verilator"
      ;;
    generated-kernel-wrapper)
      # These checks deliberately share generated RTL and must remain serial.
      local generated_out="$scratch/generated-kernel"
      bash "$script_dir/bambu_axi_burst/rtl_tests/run_generated_kernel_tests.sh" \
        "$bambu" "$generated_out" || return $?
      bash "$script_dir/bambu_axi_burst/rtl_tests/run_generated_wrapper_rid_tests.sh" \
        "$bambu" "$generated_out/generated" "$scratch/wrapper-rid" || return $?
      ;;
    *)
      echo "internal error: unknown burst job '$job'" >&2
      return 2
      ;;
  esac
}

# Mantis is a separate first phase and consumes at most the same parallelism
# internally. The independent helper jobs then share one bounded pool.
BURST_JOB_LOG_DIR="$scratch/jobs"
mkdir -p "$BURST_JOB_LOG_DIR"
if ! run_bounded_jobs "$parallelism" run_post_mantis_job \
  compiler-pragma \
  compiler-burst-profile \
  compiler-xml-profile \
  compiler-latency-metadata \
  compiler-fallback-matrix \
  compiler-root-reinvocation \
  compiler-shared-resource \
  virtual-dependencies \
  rtl-verilator \
  generated-kernel-wrapper; then
  echo "AXI burst helper suite failed; scratch outputs retained at $scratch" >&2
  exit 1
fi

# Mantis puts this configuration directly below out_<suffix>. Restrict the
# canonical-file walk to its case tree; the suite report is at the root.
cleanup_mantis_cases "$out/AXI-BURST"
rm -rf -- \
  "$scratch/jobs" "$scratch/tmp" \
  "$scratch/compiler-pragma" "$scratch/compiler-burst-profile" \
  "$scratch/compiler-xml-profile" "$scratch/compiler-latency-metadata" \
  "$scratch/compiler-fallback-matrix" "$scratch/compiler-root-reinvocation" \
  "$scratch/compiler-shared-resource" "$scratch/virtual-dependencies" \
  "$scratch/rtl-verilator" "$scratch/generated-kernel" "$scratch/wrapper-rid"
printf 'AXI burst regression PASS: %s\n' "$scratch"
