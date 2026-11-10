#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# Test script for AXI + Ramulator2 integrated testbench
#
# Tests TestbenchMEMAXIRamulatorIntegrated: AXI4 protocol with Ramulator2 DRAM timing.
# Accepts channel count as argument to test multi-channel Ramulator DRAM.
#
# Usage:
#   ./test_axi_ramulator.sh <channels> [extra bambu flags]
#
# Examples:
#   ./test_axi_ramulator.sh 1          # single DRAM channel
#   ./test_axi_ramulator.sh 2          # 2 DRAM channels, banked bus, pipelined
#   ./test_axi_ramulator.sh 4          # 4 DRAM channels, banked bus, pipelined

set -eo pipefail

if [ -z "$1" ] || ! [[ "$1" =~ ^[0-9]+$ ]]; then
    echo "Usage: $0 <channels> [extra bambu flags]"
    echo "  channels: number of Ramulator DRAM channels (1, 2, 4, ...)"
    exit 1
fi

CHANNELS="$1"
shift

script=$(readlink -e "$0")
root_dir=$(dirname "$script")

BAMBU_BIN="${BAMBU_BIN:-$(command -v bambu || echo bambu)}"
if [ ! -x "$BAMBU_BIN" ] && ! command -v "$BAMBU_BIN" &>/dev/null; then
    echo "[ERROR] bambu not found at $BAMBU_BIN"
    exit 1
fi

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-$root_dir/../../../ext/ramulator2-no-spdlog/install}"

COMMON_FLAGS=(
    -m64
    --simulator=VERILATOR
    --compiler=I386_CLANG13
    --generate-interface=INFER
    "--generate-tb=$root_dir/test_vector_add.xml"
    --simulate
    --top-fname=vector_add
    --enable-ramulator-integrated
    --memory-allocation-policy=NO_BRAM
    --clock-period=10.0
    -v4
    --max-sim-cycles=100000
)

CHANNEL_FLAGS=()
if [ "$CHANNELS" -eq 1 ]; then
    CHANNEL_FLAGS=(--channels-type=MEM_ACC_11 --channels-number=1)
else
    CHANNEL_FLAGS=(--channels-type=MEM_ACC_NN "--channels-number=${CHANNELS}" --bus-pipelined)
fi

workdir="vector_add_axi_ram_${CHANNELS}ch/hls_output"
mkdir -p "$root_dir/$workdir"

echo "============================================================"
echo "[TEST] AXI + Ramulator2 — ${CHANNELS} DRAM channel(s)"
echo "[TEST] Source: vector_add with 8-element arrays"
echo "[TEST] Output: $root_dir/$workdir"
echo "============================================================"

cd "$root_dir/$workdir"

timeout 30m "$BAMBU_BIN" \
    "${COMMON_FLAGS[@]}" \
    "${CHANNEL_FLAGS[@]}" \
    "$root_dir/vector_add_axi.c" \
    "$@" \
    2>&1 | tee bambu_output.log

return_value=${PIPESTATUS[0]}

cd "$root_dir"

echo ""
echo "============================================================"
if [ "$return_value" -ne 0 ]; then
    echo "[TEST] FAIL — bambu exited with $return_value"
    exit "$return_value"
fi
if [ ! -f "$workdir/bambu_results.xml" ]; then
    echo "[TEST] FAIL — no bambu_results.xml found"
    exit 1
fi
cycles=$(python3 -c 'import sys, xml.etree.ElementTree as ET; r=ET.parse(sys.argv[1]); print(next(e.attrib["CYCLES"] for e in r.iter() if "CYCLES" in e.attrib))' "$workdir/bambu_results.xml")
echo "[TEST] ${CHANNELS}ch: ${cycles} cycles"
echo "[TEST] FUNCTIONAL: PASS"

# Check Ramulator artifacts
for f in ramulator_profile.csv ramulator_debug.log; do
    if [ -f "$workdir/$f" ]; then
        lines=$(wc -l < "$workdir/$f")
        echo "[TEST] Found $f ($lines lines)"
    fi
done

# Check generated Verilog for Ramulator AXI module instantiation
tb_verilog="$workdir/HLS_output/simulation/bambu_testbench.v"
if [ -f "$tb_verilog" ]; then
    axi_ram_count=$(grep -c "TestbenchMEMAXIRamulatorIntegrated #" "$tb_verilog" 2>/dev/null || echo "0")
    echo "[TEST] TestbenchMEMAXIRamulatorIntegrated instances: $axi_ram_count"
fi

echo "[TEST] DONE"
exit 0
