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

# --no-clean is forwarded to Mantis, but it must also hold back this script's own cleanup: the
# RTL, the intermediates, the auxiliary logs and the helper results it deletes are exactly the
# artifacts the caller asked to keep.
retain_artifacts=0
for mantis_arg in "${mantis_args[@]}"; do
  if [[ $mantis_arg == --no-clean ]]; then
    retain_artifacts=1
  fi
done
verilator_jobs=${BURST_VERILATOR_JOBS:-2}
if [[ ! $verilator_jobs =~ ^[1-9][0-9]*$ ]]; then
  echo "BURST_VERILATOR_JOBS must be a positive integer, got: $verilator_jobs" >&2
  exit 2
fi
writer_group_jobs=${BURST_WRITER_GROUP_JOBS:-8}
if [[ ! $writer_group_jobs =~ ^[1-9][0-9]*$ ]]; then
  echo "BURST_WRITER_GROUP_JOBS must be a positive integer, got: $writer_group_jobs" >&2
  exit 2
fi
if (( writer_group_jobs > parallelism )); then
  writer_group_jobs=$parallelism
fi

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
source "$repo_root/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
if ! bambu_modelsim_resolve; then
  echo "ModelSim vsim could not be resolved through PATH or BAMBU_HLS_BACKEND_PATH" >&2
  exit 2
fi
export PATH="$BAMBU_MODELSIM_BIN${PATH:+:$PATH}"
for modelsim_tool in vsim vlib vlog; do
  if [[ ! -x "$BAMBU_MODELSIM_BIN/$modelsim_tool" ]]; then
    echo "ModelSim $modelsim_tool is missing beside selected vsim: $BAMBU_MODELSIM_BIN/$modelsim_tool" >&2
    exit 2
  fi
done
printf 'AXI_BURST_MODELSIM source=%s vsim=%s bin=%s mentor_root=%s\n' \
  "$BAMBU_MODELSIM_SOURCE" "$BAMBU_MODELSIM_PATH" "$BAMBU_MODELSIM_BIN" "$BAMBU_MODELSIM_ROOT"
modelsim_root_arg=$(python3 -c 'import shlex, sys; print(shlex.quote(sys.argv[1]))' "$BAMBU_MODELSIM_ROOT")
scratch=$out_root
# Keep every artifact under the selected output root. Mantis requires its own
# -o directory to be fresh, so reserve a child for its outputs and use a
# sibling child for temporary files.
mkdir -p "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"
export BURST_TEST_TMPDIR="$scratch"
out="$scratch/mantis"

# Mantis cleanup would remove bambu_results.xml before the cycle gate runs.
# Retain cases internally, then apply Mantis-style cleanup after validation.
python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
  --args="--configuration-name=AXI-BURST --compiler=I386_CLANG16 --experimental-setup=BAMBU --generate-interface=INFER --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32 --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33 --generate-vcd --no-clean --extra-cc-options=-DNOCACHE --simulate --simulator=MODELSIM --mentor-root=$modelsim_root_arg" \
  --no-clean --returnfail -l "$script_dir/bambu_axi_burst_list" -o "$out" -b "$script_dir/bambu_axi_burst" "${mantis_args[@]}"

if [[ ! -d $out || -L $out ]]; then
  echo "Mantis completed without creating its output directory: $out" >&2
  exit 1
fi

python3 -B "$script_dir/bambu_axi_burst/check_burst_results.py" \
  --results "$out" \
  --expectations "$script_dir/bambu_axi_burst/cycle_expectations.tsv" \
  --legacy-expectations "$script_dir/bambu_axi_burst/legacy_write_expectations.tsv"

# Mantis and the cycle checker have completed. Put all remaining helper output
# and temporary files beneath the selected out_ root.
mkdir -p "$scratch/jobs" "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"

# Offline checks for the cycle gate, latency-metadata runner, and exported-tree
# bootstraps do not need another Bambu invocation.
python3 -B "$script_dir/bambu_axi_burst/test_check_burst_results.py"
python3 -B "$script_dir/bambu_axi_burst/test_write_compiler_artifacts.py"
python3 -B "$script_dir/bambu_axi_burst/test_read_compiler_artifacts.py"
BURST_TEST_TMPDIR="$scratch/tmp" python3 -B "$script_dir/bambu_axi_burst/test_modelsim_tool_lookup.py"
python3 -B "$script_dir/bambu_axi_burst/rtl_tests/test_extract_write_engine.py"
python3 -B "$script_dir/bambu_axi_burst/rtl_tests/test_vlog_portability_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_axi_latency_metadata.py"
python3 -B "$script_dir/bambu_axi_burst/test_simulation_logging.py"
python3 -B "$script_dir/bambu_axi_burst/test_bundle_preanalysis_artifacts.py"
python3 -B "$script_dir/bambu_axi_burst/test_parallel_helpers.py"
python3 -B "$script_dir/bambu_axi_burst/test_duplex_effect_analysis_artifacts.py"
python3 -B "$script_dir/bambu_axi_burst/test_generated_duplex_kernel_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_generated_duplex_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_source_restrict_provenance.py"
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
    rtl-verilator-transactions|rtl-verilator-credits|rtl-verilator-bounds|rtl-verilator-configure)
      local suite=${job#rtl-verilator-}
      SIM_JOBS="$verilator_jobs" bash "$script_dir/bambu_axi_burst/rtl_tests/run_verilator.sh" \
        "$scratch/rtl-verilator-$suite" "$suite"
      ;;
    rtl-write-engine)
      BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$verilator_jobs" \
        BURST_WRITER_GROUP_JOBS="$writer_group_jobs" \
        bash "$script_dir/bambu_axi_burst/rtl_tests/run_write_burst_engine.sh" \
          "$scratch/rtl-write-engine"
      ;;
    rtl-vlog)
      bash "$script_dir/bambu_axi_burst/rtl_tests/run_vlog_portability.sh" \
        "$scratch/out_axi_core_vlog_portability"
      ;;
    generated-kernel-wrapper)
      # These checks deliberately share generated RTL and must remain serial.
      local generated_out="$scratch/generated-kernel"
      SIM_JOBS="$verilator_jobs" bash "$script_dir/bambu_axi_burst/rtl_tests/run_generated_kernel_tests.sh" \
        "$bambu" "$generated_out" || return $?
      SIM_JOBS="$verilator_jobs" bash "$script_dir/bambu_axi_burst/rtl_tests/run_generated_wrapper_rid_tests.sh" \
        "$bambu" "$generated_out/generated" "$scratch/wrapper-rid" || return $?
      ;;
    *)
      echo "internal error: unknown burst job '$job'" >&2
      return 2
      ;;
  esac
}

# Mantis is a separate first phase and consumes at most the same parallelism
# internally. The independent helper jobs then share one bounded pool. The
# raw-IR virtual-dependency diagnostic is developer-only: Release builds do not
# support --debug-classes, so it must not be scheduled here.
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
  rtl-verilator-transactions \
  rtl-verilator-credits \
  rtl-verilator-bounds \
  rtl-verilator-configure \
  rtl-write-engine \
  rtl-vlog \
  generated-kernel-wrapper; then
  echo "AXI burst helper suite failed; scratch outputs retained at $scratch" >&2
  exit 1
fi

# Mantis puts this configuration directly below out_<suffix>. Restrict the
# canonical-file walk to its case tree; the suite report is at the root.
if (( retain_artifacts )); then
  printf 'AXI_BURST_RETAIN_ARTIFACTS: --no-clean given, keeping Mantis case trees and helper scratch\n'
else
cleanup_mantis_cases "$out/AXI-BURST"
rm -rf -- \
  "$scratch/jobs" "$scratch/tmp" \
  "$scratch/compiler-pragma" "$scratch/compiler-burst-profile" \
  "$scratch/compiler-xml-profile" "$scratch/compiler-latency-metadata" \
  "$scratch/compiler-fallback-matrix" "$scratch/compiler-root-reinvocation" \
  "$scratch/compiler-shared-resource" \
  "$scratch/rtl-verilator-transactions" "$scratch/rtl-verilator-credits" \
  "$scratch/rtl-verilator-bounds" "$scratch/rtl-verilator-configure" \
  "$scratch/rtl-write-engine" \
  "$scratch/out_axi_core_vlog_portability" \
  "$scratch/generated-kernel" "$scratch/wrapper-rid"
fi
printf 'AXI burst regression PASS: %s\n' "$scratch"
