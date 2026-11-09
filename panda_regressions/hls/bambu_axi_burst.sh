#!/usr/bin/env bash
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(realpath "$script_dir/../..")
OUT_SUFFIX="bambu_axi_burst"
source "$script_dir/bambu_axi_burst/parallel_helpers.sh"

group=all
group_seen=0
saw_separator=0
wrapper_args=()
for arg in "$@"; do
  if [[ $arg == -- && $saw_separator == 0 ]]; then
    saw_separator=1
  fi
  if (( ! saw_separator )) && [[ $arg == --group=* ]]; then
    if (( group_seen )); then
      echo "--group may be specified only once" >&2
      exit 2
    fi
    group_seen=1
    group=${arg#--group=}
    case $group in
      compiler|functional|all) ;;
      *) echo "invalid group '$group' (expected compiler, functional, or all)" >&2; exit 2 ;;
    esac
  else
    wrapper_args+=("$arg")
  fi
done
if [[ ${wrapper_args[0]:-} == --help || ${wrapper_args[0]:-} == -h ]]; then
  printf 'Usage: %s [--group=compiler|functional|all] [Mantis options] [-o DIR|--output DIR] [-j N|--parallel N]\n' "${BASH_SOURCE[0]}"
  printf 'Default output: ./out_%s (must not already exist).\n' "$OUT_SUFFIX"
  printf 'Default group: all (compiler checks and functional simulation).\n'
  printf 'Other options are passed through to Mantis; -j bounds parallel jobs (default: J or 1).\n'
  exit 0
fi
if [[ $group == compiler ]]; then
  export BURST_SIMULATE=0
fi

if ! normalize_regression_args "${wrapper_args[@]}"; then
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
if [[ $group != compiler ]]; then
  source "$repo_root/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
  if ! bambu_modelsim_resolve; then
    echo "ModelSim vsim could not be resolved through PATH or BAMBU_HLS_BACKEND_PATH" >&2
    exit 2
  fi
  export PATH="$BAMBU_MODELSIM_BIN${PATH:+:$PATH}"
  for modelsim_tool in vsim vlib vmap vlog; do
    if [[ ! -x "$BAMBU_MODELSIM_BIN/$modelsim_tool" ]]; then
      echo "ModelSim $modelsim_tool is missing beside selected vsim: $BAMBU_MODELSIM_BIN/$modelsim_tool" >&2
      exit 2
    fi
  done
  printf 'AXI_BURST_MODELSIM source=%s vsim=%s bin=%s mentor_root=%s\n' \
    "$BAMBU_MODELSIM_SOURCE" "$BAMBU_MODELSIM_PATH" "$BAMBU_MODELSIM_BIN" "$BAMBU_MODELSIM_ROOT"
fi
scratch=$out_root
if [[ $group == compiler && -n ${BURST_RESTART:-} ]]; then
  scratch=$(mktemp -d "$out_root/out_helpers_compiler.XXXXXX")
fi
# Keep every artifact under the selected output root. Mantis requires its own
# -o directory to be fresh, so reserve a child for its outputs and use a
# sibling child for temporary files.
mkdir -p "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"
export BURST_TEST_TMPDIR="$scratch"
out="$scratch/mantis"

# Mantis cleanup would remove bambu_results.xml before the cycle gate runs.
# Retain cases internally, then apply Mantis-style cleanup after validation.
if [[ $group != compiler ]]; then
  fixed_inputs="$scratch/fixed-burst-fallback-inputs"
  fixed_device="$fixed_inputs/xcu250-fixed.xml"
  fixed_list="$fixed_inputs/bambu_axi_burst_fixed.list"
  mkdir -p "$fixed_inputs"
  sed 's/<axi_burst_type value="1"\/>/<axi_burst_type value="0"\/>/' \
    "$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml" >"$fixed_device"
  if ! grep -Eq '<axi_burst_type value="0"/>' "$fixed_device"; then
    echo "could not prepare effective FIXED target from xcu250" >&2
    exit 2
  fi
  cat "$script_dir/bambu_axi_burst_list" >"$fixed_list"
  python3 -c 'import shlex, sys
script_dir, fixed_device = sys.argv[1:]
post_check = shlex.join(["python3", "-B", f"{script_dir}/bambu_axi_burst/check_fixed_burst_fallback.py",
                         "--output", ".", "--target-file", fixed_device])
fields = ["BENCHMARK_ENV=MANTIS_POST_CHECK=" + post_check,
          f"{script_dir}/bambu_axi_burst/bambu_axi_burst_sum_profile.cpp", "--top-fname=kernel",
          f"--generate-tb={script_dir}/bambu_axi_burst/bambu_axi_burst_tb.cpp", "--tb-arg=1024",
          "--tb-param-size=src:16384", "--extra-cc-options=-DBAMBU",
          "--extra-cc-options=-DBAMBU_AXI_SUM_O5", "--extra-cc-options=-DBURST_TEST_DEFAULT_N=1024",
          f"--target-file={fixed_device}",
          "--benchmark-name=fixed-burst-fallback"]
print(shlex.join(fields))' "$script_dir" "$fixed_device" >>"$fixed_list"
  bambu_args=$(python3 -c 'import shlex, sys
args = ["--configuration-name=AXI-BURST", "--compiler=I386_CLANG16",
        "--experimental-setup=BAMBU", "--generate-interface=INFER",
        "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL",
        "--max-sim-cycles=1000000",
        "--mem-delay-read=64", "--mem-delay-write=2", "--tb-queue-size=32",
        "--memory-allocation-policy=NO_BRAM", "--distram-threshold=0", "--clock-period=3.33",
        "--generate-vcd", "--no-clean", "--extra-cc-options=-DNOCACHE", "--simulate",
        "--simulator=MODELSIM", "--mentor-root=" + sys.argv[1]]
print(shlex.join(args))' "$BAMBU_MODELSIM_ROOT")
  python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
    --args="$bambu_args" \
    --no-clean --returnfail --timeout=30m -l "$fixed_list" \
    -o "$out" -b "$script_dir/bambu_axi_burst" "${mantis_args[@]}"

  if [[ ! -d $out || -L $out ]]; then
    echo "Mantis completed without creating its output directory: $out" >&2
    exit 1
  fi

  python3 -B "$script_dir/bambu_axi_burst/check_burst_results.py" \
    --results "$out" \
    --expectations "$script_dir/bambu_axi_burst/cycle_expectations.tsv" \
    --legacy-expectations "$script_dir/bambu_axi_burst/legacy_write_expectations.tsv" \
    --fixed-fallback-target "$fixed_device"
fi

# Put all helper output and temporary files beneath the selected output root.
if [[ $group != compiler && -n ${BURST_RESTART:-} ]]; then
  # Keep prior helper evidence intact while Mantis resumes its existing case tree.
  scratch=$(mktemp -d "$out_root/out_helpers_restart.XXXXXX")
  export BURST_TEST_TMPDIR="$scratch"
fi
mkdir -p "$scratch/jobs" "$scratch/tmp"
export TMPDIR="$scratch/tmp" TMP="$scratch/tmp" TEMP="$scratch/tmp"

# Offline checks for the cycle gate, latency-metadata runner, and exported-tree
# bootstraps do not need another Bambu invocation.
if [[ $group != compiler ]]; then
  python3 -B "$script_dir/bambu_axi_burst/test_check_burst_results.py"
fi
python3 -B "$script_dir/bambu_axi_burst/test_regression_restart.py"
python3 -B "$script_dir/bambu_axi_burst/test_compiler_test_support.py"
python3 -B "$script_dir/bambu_axi_burst/test_compiler_case_manifest.py"
python3 -B "$script_dir/bambu_axi_burst/test_write_compiler_artifacts.py"
python3 -B "$script_dir/bambu_axi_burst/test_read_compiler_artifacts.py"
if [[ $group != compiler ]]; then
  BURST_TEST_TMPDIR="$scratch/tmp" python3 -B "$script_dir/bambu_axi_burst/test_modelsim_tool_lookup.py"
fi
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_extract_write_engine.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_write_burst_modelsim_runner.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_read_burst_modelsim_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_axi_master_specialization.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_generated_read_modelsim_runner.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_generated_memaxi_response_runner.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/test_vlog_portability_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_axi_latency_metadata.py"
python3 -B "$script_dir/bambu_axi_burst/test_simulation_logging.py"
python3 -B "$script_dir/bambu_axi_burst/test_bundle_preanalysis_artifacts.py"
python3 -B "$script_dir/bambu_axi_burst/test_parallel_helpers.py"
python3 -B "$script_dir/bambu_axi_burst/test_duplex_effect_analysis_artifacts.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/test_generated_duplex_kernel_runner.py"
python3 -B "$repo_root/panda_regressions/verilog/axi_burst/test_generated_duplex_runner.py"
python3 -B "$script_dir/bambu_axi_burst/test_source_restrict_provenance.py"
bash "$script_dir/bambu_axi_burst/test_compiler_runner_export_root.sh"

run_compiler_suite() {
  local mode=$1 destination="$scratch/compiler-$1"
  bash "$script_dir/bambu_axi_burst/run_compiler_tests.sh" "$mode" "$bambu" "$destination"
}

run_post_mantis_job() {
  local job=$1 job_slots=${2:-1}
  case $job in
    compiler-pragma|compiler-burst-profile|compiler-xml-profile|compiler-latency-metadata|compiler-fallback-matrix|compiler-root-reinvocation|compiler-shared-resource)
      run_compiler_suite "${job#compiler-}"
      ;;
    compiler-write-all)
      bash "$script_dir/bambu_axi_burst/run_write_compiler_tests.sh" all "$bambu" "$scratch/compiler-write-all"
      ;;
    compiler-frontend-compatibility)
      bash "$script_dir/bambu_axi_burst/run_frontend_compatibility_tests.sh" "$bambu" "$scratch/compiler-frontend-compatibility"
      ;;
    compiler-default-burst)
      bash "$script_dir/bambu_axi_burst/run_default_burst_tests.sh" "$bambu" "$scratch/compiler-default-burst"
      ;;
    functional-float-read-write)
      BURST_WRITE_FLOAT_SIM_OUTSIDE_SANDBOX=1 \
        bash "$script_dir/bambu_axi_burst/run_write_compiler_tests.sh" float-sim "$bambu" "$scratch/functional-float-read-write"
      ;;
    rtl-burst-diagnostics)
      BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_burst_fault_diagnostics.sh" "$scratch/rtl-burst-diagnostics"
      ;;
    rtl-width-contract)
      SIM_JOBS=1 bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_axi_width_contract.sh" "$scratch/rtl-width-contract"
      ;;
    rtl-duplex-core)
      BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_duplex_core_tests.sh" "$scratch/rtl-duplex-core"
      ;;
    rtl-generated-duplex)
      BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$job_slots" \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_duplex_tests.sh" "$bambu" "$scratch/rtl-generated-duplex"
      ;;
    rtl-generated-duplex-kernel)
      BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$job_slots" DUPLEX_G2_JOBS="$job_slots" \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_duplex_kernel_tests.sh" "$bambu" "$scratch/rtl-generated-duplex-kernel"
      ;;
    rtl-default-duplex-reduction)
      BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX=1 \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_default_duplex_reduction_modelsims.sh" \
          "$bambu" "$scratch/rtl-default-duplex-reduction"
      ;;
    rtl-generated-write-kernels)
      local write_out="$scratch/rtl-generated-write-kernels"
      local outstanding
      for outstanding in 1 2 3; do
        BURST_G2_SIM_OUTSIDE_SANDBOX=1 WRITE_OUTSTANDING=$outstanding SIM_JOBS=1 \
          bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_write_kernel_tests.sh" \
            "$bambu" "$write_out/outstanding-$outstanding" || return $?
      done
      ;;
    rtl-duplex-alias)
      BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 DUPLEX_ALIAS_JOBS="$job_slots" DUPLEX_ALIAS_FLOAT_BITS=0 \
        bash "$script_dir/bambu_axi_burst/run_duplex_alias_tests.sh" "$bambu" "$scratch/rtl-duplex-alias-integer" || return $?
      BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 DUPLEX_ALIAS_JOBS="$job_slots" DUPLEX_ALIAS_FLOAT_BITS=1 \
        bash "$script_dir/bambu_axi_burst/run_duplex_alias_tests.sh" "$bambu" "$scratch/rtl-duplex-alias-float32"
      ;;
    rtl-duplex-fallback|rtl-duplex-independent)
      if [[ $job == rtl-duplex-fallback ]]; then
        BURST_DUPLEX_FALLBACK_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$job_slots" DUPLEX_FALLBACK_JOBS="$job_slots" \
          bash "$script_dir/bambu_axi_burst/run_duplex_fallback_tests.sh" "$bambu" "$scratch/$job"
      else
        BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$job_slots" DUPLEX_INDEPENDENT_JOBS="$job_slots" \
          bash "$script_dir/bambu_axi_burst/run_duplex_independent_tests.sh" "$bambu" "$scratch/$job"
      fi
      ;;
    rtl-adapter-size)
      BURST_ADAPTER_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_adapter_size_guard.sh" "$scratch/rtl-adapter-size"
      ;;
    rtl-mdpi-extent)
      MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1 MDPI_EXTENT_JOBS="$job_slots" SIM_JOBS="$job_slots" \
        bash "$script_dir/bambu_axi_burst/run_mdpi_extent_tests.sh" "$bambu" "$scratch/rtl-mdpi-extent"
      ;;
    rtl-testbench-memaxi-qualify)
      BURST_MODELSIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_testbench_memaxi_model.sh" qualify "$scratch/rtl-testbench-memaxi-qualify"
      ;;
    rtl-vector-payload)
      BURST_MODELSIM_OUTSIDE_SANDBOX=1 \
        bash "$script_dir/bambu_axi_burst/run_vector_payload_tests.sh" "$bambu" "$scratch/rtl-vector-payload"
      ;;
    rtl-generated-memaxi-response)
      BURST_MODELSIM_OUTSIDE_SANDBOX=1 SIM_JOBS="$job_slots" \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_memaxi_response_tests.sh" \
          "$bambu" "$scratch/rtl-generated-memaxi-response"
      ;;
    rtl-copy-comparison)
      BURST_COPY_COMPARISON_OUTSIDE_SANDBOX=1 COPY_COMPARISON_JOBS="$job_slots" \
        bash "$script_dir/bambu_axi_burst/run_copy_comparison_tests.sh" "$bambu" "$scratch/rtl-copy-comparison"
      ;;
    rtl-modelsim-transactions|rtl-modelsim-credits|rtl-modelsim-bounds|rtl-modelsim-configure)
      local suite=${job#rtl-modelsim-}
      bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_read_burst_modelsim.sh" \
        "$scratch/rtl-modelsim-$suite" "$suite"
      ;;
    rtl-write-engine)
      BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 \
        BURST_WRITER_GROUP_JOBS="$job_slots" \
        bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_write_burst_engine.sh" \
          "$scratch/rtl-write-engine"
      ;;
    rtl-vlog)
      bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_vlog_portability.sh" \
        "$scratch/out_axi_core_vlog_portability"
      ;;
    generated-kernel-wrapper)
      # These checks deliberately share generated RTL and must remain serial.
      local generated_out="$scratch/generated-kernel"
      bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_kernel_tests.sh" \
        "$bambu" "$generated_out" || return $?
      bash "$repo_root/panda_regressions/verilog/axi_burst/rtl_tests/run_generated_wrapper_rid_tests.sh" \
        "$bambu" "$generated_out/generated" "$scratch/wrapper-rid" || return $?
      ;;
    *)
      echo "internal error: unknown burst job '$job'" >&2
      return 2
      ;;
  esac
}

burst_job_slots() {
  local capacity
  case $1 in
    rtl-write-engine) capacity=49 ;;
    rtl-generated-duplex) capacity=2 ;;
    rtl-generated-duplex-kernel) capacity=6 ;;
    rtl-duplex-alias) capacity=12 ;;
    rtl-duplex-fallback) capacity=4 ;;
    rtl-duplex-independent) capacity=2 ;;
    rtl-mdpi-extent) capacity=13 ;;
    rtl-copy-comparison) capacity=2 ;;
    rtl-generated-memaxi-response) capacity=6 ;;
    *) capacity=1 ;;
  esac
  if (( capacity > parallelism )); then capacity=$parallelism; fi
  echo "$capacity"
}

# In functional/all modes Mantis is a separate first phase and consumes at most
# the same parallelism internally. The selected helper jobs then share one pool. The
# raw-IR virtual-dependency diagnostic is developer-only: Release builds do not
# support --debug-classes, so it must not be scheduled here.
BURST_JOB_LOG_DIR="$scratch/jobs"
mkdir -p "$BURST_JOB_LOG_DIR"
compiler_jobs=(compiler-pragma compiler-burst-profile compiler-xml-profile compiler-latency-metadata
  compiler-fallback-matrix compiler-root-reinvocation compiler-shared-resource compiler-write-all
  compiler-frontend-compatibility compiler-default-burst)
functional_jobs=(rtl-modelsim-transactions rtl-modelsim-credits rtl-modelsim-bounds rtl-modelsim-configure
  rtl-write-engine rtl-vlog generated-kernel-wrapper rtl-burst-diagnostics rtl-width-contract
  rtl-duplex-core rtl-generated-duplex rtl-generated-duplex-kernel rtl-generated-write-kernels
  rtl-default-duplex-reduction rtl-duplex-alias rtl-duplex-fallback rtl-duplex-independent rtl-adapter-size rtl-mdpi-extent
  functional-float-read-write rtl-testbench-memaxi-qualify rtl-vector-payload
  rtl-generated-memaxi-response rtl-copy-comparison)
jobs=()
case $group in
  compiler) jobs=("${compiler_jobs[@]}") ;;
  functional) jobs=("${functional_jobs[@]}") ;;
  all) jobs=("${compiler_jobs[@]}" "${functional_jobs[@]}") ;;
esac
if ! run_bounded_jobs "$parallelism" run_post_mantis_job --slots=burst_job_slots "${jobs[@]}"; then
  echo "AXI burst helper suite failed; scratch outputs retained at $scratch" >&2
  exit 1
fi

# Mantis puts this configuration directly below out_<suffix>. Restrict its
# canonical-file walk to the case tree; the suite report is at the root.
if (( retain_artifacts )); then
  printf 'AXI_BURST_RETAIN_ARTIFACTS: --no-clean given, keeping output trees and helper scratch\n'
else
if [[ $group != compiler ]]; then cleanup_mantis_cases "$out/AXI-BURST"; fi
rm -rf -- \
  "$scratch/jobs" "$scratch/tmp" \
  "$scratch/compiler-pragma" "$scratch/compiler-burst-profile" \
  "$scratch/compiler-xml-profile" "$scratch/compiler-latency-metadata" \
  "$scratch/compiler-fallback-matrix" "$scratch/compiler-root-reinvocation" \
  "$scratch/compiler-shared-resource" \
  "$scratch/compiler-write-all" \
  "$scratch/compiler-frontend-compatibility" \
  "$scratch/compiler-default-burst" \
  "$scratch/rtl-modelsim-transactions" "$scratch/rtl-modelsim-credits" \
  "$scratch/rtl-modelsim-bounds" "$scratch/rtl-modelsim-configure" \
  "$scratch/rtl-write-engine" \
  "$scratch/out_axi_core_vlog_portability" \
  "$scratch/generated-kernel" "$scratch/wrapper-rid" \
  "$scratch/rtl-burst-diagnostics" "$scratch/rtl-width-contract" \
  "$scratch/rtl-duplex-core" "$scratch/rtl-generated-duplex" \
  "$scratch/rtl-generated-duplex-kernel" "$scratch/rtl-generated-write-kernels" \
  "$scratch/rtl-default-duplex-reduction" \
  "$scratch/rtl-duplex-alias-integer" "$scratch/rtl-duplex-alias-float32" \
  "$scratch/rtl-duplex-fallback" "$scratch/rtl-duplex-independent" \
  "$scratch/rtl-adapter-size" "$scratch/rtl-mdpi-extent" \
  "$scratch/functional-float-read-write" "$scratch/rtl-testbench-memaxi-qualify" \
  "$scratch/rtl-vector-payload" "$scratch/rtl-generated-memaxi-response" "$scratch/rtl-copy-comparison"
fi
printf 'AXI burst regression PASS: %s\n' "$scratch"
