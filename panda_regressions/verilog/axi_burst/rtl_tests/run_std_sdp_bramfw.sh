#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
source "$SCRIPT_DIR/modelsim_helpers.sh"
if (($# != 1)); then
    echo "usage: $0 FRESH_OUTPUT_DIRECTORY" >&2
    exit 2
fi
OUT=$(realpath -m -- "$1")
[[ ! -e $OUT && ! -L $OUT ]] || { echo "output must be fresh: $OUT" >&2; exit 2; }
modelsim_resolve
mkdir -p "$OUT"
python3 -B "$SCRIPT_DIR/extract_write_engine.py" --xml "$ROOT/etc/libtech/NC_MEM_IPs.xml" \
    --target STD_SDP_BRAMFW --out "$OUT/memory.sv"
modelsim_compile "$OUT/build" tb_std_sdp_bramfw "$OUT/memory.sv" \
    "$SCRIPT_DIR/std_sdp_bramfw_reference.sv" "$SCRIPT_DIR/tb_std_sdp_bramfw.sv"
profiles=()
for latency in 0 1 2; do
    for readonly in 0 1; do
        profiles+=("$latency,$readonly,4,8,8,2,2,zero")
    done
    for readonly in 0 1; do
        profiles+=("$latency,$readonly,4,8,8,2,2,file")
    done
done
for latency in 0 1 2; do
    for readonly in 0 1; do
        for shape in 1,1,1,1,1 3,8,4,2,3 4,4,8,3,2; do
            profiles+=("$latency,$readonly,$shape,zero")
        done
    done
done
# Check both sides of the forwarding threshold and the first depth above it.
for latency in 0 1 2; do
    for readonly in 0 1; do
        for depth in 511 512 513; do
            profiles+=("$latency,$readonly,$depth,8,8,10,10,zero")
        done
    done
done
printf '00010001\n10101010\n01010101\n11100111\n' > "$OUT/initial.mem"
for profile in "${profiles[@]}"; do
    IFS=, read -r latency readonly depth input_bits output_bits read_bits write_bits init <<< "$profile"
    label="h$latency-ro$readonly-d$depth-i$input_bits-o$output_bits-r$read_bits-w$write_bits-$init"
    args=(-gHIGH_LATENCY="$latency" -gREAD_ONLY_MEMORY="$readonly" -gDEPTH="$depth"
          -gINPUT_BITS="$input_bits" -gOUTPUT_BITS="$output_bits"
          -gREAD_ADDRESS_BITS="$read_bits" -gWRITE_ADDRESS_BITS="$write_bits")
    if [[ $init == file ]]; then
        args+=(-gMEMORY_INIT_file="$OUT/initial.mem")
    fi
    modelsim_run "$OUT/build" tb_std_sdp_bramfw "$OUT/$label.log" 60 "${args[@]}"
    grep -q 'STD_SDP_BRAMFW_PASS' "$OUT/$label.log"
done
echo "STD_SDP_BRAMFW_SIM_PASS profiles=${#profiles[@]}: $OUT"
