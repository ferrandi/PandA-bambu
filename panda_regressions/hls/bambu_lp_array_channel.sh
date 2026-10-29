#!/bin/bash
# ModelSim-only issue #434 scheduling and backpressure regression.
# Pass Bambu options after the launcher; use Mantis -a/-c options for runner
# controls. The former LP_* simulator/latency/worker controls are unsupported.
set -u
script_dir="$(dirname "$(readlink -e "$0")")"
repo_root="$(readlink -e "$script_dir/../..")"
mkdir -p "$repo_root/documentation/tmp"
export TMPDIR="$repo_root/documentation/tmp" TMP="$repo_root/documentation/tmp" TEMP="$repo_root/documentation/tmp"

exec python3 "$repo_root/etc/scripts/mantis.py" \
   --tool="$script_dir/bambu_lp_array_channel/run_case.sh" \
   --args="--configuration-name=LP-ARRAY-CHANNEL-MODELSIM --generate-interface=INFER" \
   -l "$script_dir/bambu_lp_array_channel_list" \
   -b "$script_dir/bambu_lp_array_channel" \
   -o out_bambu_lp_array_channel_MODELSIM --returnfail --no-clean "$@"
