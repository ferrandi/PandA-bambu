#!/bin/bash
# ModelSim-only scheduling and backpressure regression.
# Pass Bambu options after the launcher; use Mantis -a/-c options for runner
# controls, including whether to retain generated output.
set -u
script_dir="$(dirname "$(readlink -e "$0")")"
repo_root="$(readlink -e "$script_dir/../..")"

if [ -z "${LP_MOCK_MODE:-}" ]; then
   source "$repo_root/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
   if ! bambu_modelsim_resolve; then
      echo "ModelSim vsim could not be resolved through PATH or BAMBU_HLS_BACKEND_PATH" >&2
      exit 1
   fi
   export PATH="$BAMBU_MODELSIM_BIN${PATH:+:$PATH}"
   for tool in vsim vlib vlog vcom; do
      if [ ! -x "$BAMBU_MODELSIM_BIN/$tool" ]; then
         echo "ModelSim $tool is missing beside selected vsim: $BAMBU_MODELSIM_BIN/$tool" >&2
         exit 1
      fi
   done

   if ! python3 "$script_dir/bambu_lp_array_channel/test_timed_capture.py" "$@"; then
      exit 1
   fi
   if ! python3 -B "$script_dir/bambu_lp_array_channel/test_shared_fu.py"; then
      exit 1
   fi
   if ! python3 -B "$script_dir/bambu_lp_array_channel/test_stallable_multiplier.py" "$@"; then
      exit 1
   fi
   if ! python3 -B "$script_dir/bambu_lp_array_channel/test_global_pipeline_stall.py" "$@"; then
      exit 1
   fi
   if ! python3 -B "$script_dir/bambu_lp_array_channel/test_array_memory_model.py"; then
      exit 1
   fi
fi

exec python3 "$repo_root/etc/scripts/mantis.py" \
   --tool="$script_dir/bambu_lp_array_channel/run_case.sh" \
   --args="--configuration-name=LP-ARRAY-CHANNEL-MODELSIM --generate-interface=INFER --simulator=MODELSIM --simulate --device-name=xc7z020,-1,clg484 " \
   -l "$script_dir/bambu_lp_array_channel_list" \
   -b "$script_dir/bambu_lp_array_channel" \
   -o out_bambu_lp_array_channel_MODELSIM --returnfail "$@"
