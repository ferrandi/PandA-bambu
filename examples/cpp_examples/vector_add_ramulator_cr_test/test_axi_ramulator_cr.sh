#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# AXI + Ramulator2 integrated testbench smoke test for the *candidate-release* build.
#
# Self-contained: all artifacts stay under this directory so nothing mixes with the
# private-PandA-bambu examples or the candidate-release vanilla build.
#
# Exercises TestbenchMEMAXIRamulatorIntegrated: AXI4 protocol driven by Ramulator2
# DRAM timing.  Uses bambu verbosity -v5 so the MDPI driver is compiled with
# __M_OUT_LVL=5, which enables the Ramulator2 if_debug()/debug() statements in
# etc/libmdpi/mdpi_driver.cpp (they are #if __M_OUT_LVL > 4).  Those prints are how
# we confirm the Ramulator code path is actually being driven.
#
# Usage:
#   ./test_axi_ramulator_cr.sh <channels> [extra bambu flags]
#
# Examples:
#   ./test_axi_ramulator_cr.sh 1          # single DRAM channel
#   ./test_axi_ramulator_cr.sh 2          # 2 DRAM channels, banked bus, pipelined
#   ./test_axi_ramulator_cr.sh 4          # 4 DRAM channels, banked bus, pipelined

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

# Bambu build with this repo's Ramulator2 integration (override with BAMBU_BIN=...
# if bambu is not already on PATH)
BAMBU_BIN="${BAMBU_BIN:-$(command -v bambu || echo bambu)}"
if [ ! -x "$BAMBU_BIN" ]; then
    echo "[ERROR] bambu not found at $BAMBU_BIN"
    echo "        Build it with scripts/external/setup-bambu-candidate-release-ramulator.sh"
    echo "        or set BAMBU_BIN to your candidate-release+Ramulator install."
    exit 1
fi

# The candidate-release build refuses to run unless BAMBU_HLS and
# BAMBU_HLS_BACKEND_PATH are set; source the install's settings.sh.
BAMBU_INSTALL_DIR="$(dirname "$(dirname "$BAMBU_BIN")")"
if [ -f "$BAMBU_INSTALL_DIR/settings.sh" ]; then
    source "$BAMBU_INSTALL_DIR/settings.sh"
    unset IFS  # settings.sh leaves IFS=":" set; restore default word splitting
else
    echo "[ERROR] settings.sh not found at $BAMBU_INSTALL_DIR/settings.sh"
    echo "        (needed to set BAMBU_HLS / BAMBU_HLS_BACKEND_PATH)"
    exit 1
fi

# 32-bit libramulator.so must be on the loader path for the co-sim driver.
export RAMULATOR2_DIR="${RAMULATOR2_DIR:-$root_dir/../../../ext/ramulator2-no-spdlog/install}"

# Point the MDPI driver at the local Ramulator config (otherwise it searches cwd
# and falls back to a generated minimal DDR4 config in /tmp).
export RAMULATOR2_CONFIG="$root_dir/ramulator2_config.yaml"

COMMON_FLAGS=(
    -m64
    --simulator=VERILATOR
    --compiler=I386_CLANG13
    --generate-interface=INFER
    "--generate-tb=$root_dir/test_vector_add.xml"
    --simulate
    --top-fname=vector_add
    --enable-ramulator
    --memory-allocation-policy=NO_BRAM
    --clock-period=10.0
    -v5
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
echo "[TEST] candidate-release: AXI + Ramulator2 — ${CHANNELS} DRAM channel(s)"
echo "[TEST] bambu  : $BAMBU_BIN"
echo "[TEST] config : $RAMULATOR2_CONFIG"
echo "[TEST] output : $root_dir/$workdir"
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

# ---- functional correctness -------------------------------------------------
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

# ---- proof the Ramulator2 path was exercised --------------------------------
# These debug lines only appear when the MDPI driver was compiled with
# __M_OUT_LVL > 4 (i.e. bambu -v5) AND -DHAVE_RAMULATOR2 AND the Ramulator code
# path actually ran.  If we see them, Ramulator timing really drove the memory.
log="$workdir/bambu_output.log"
echo ""
echo "[TEST] --- Ramulator2 path evidence (from $log) ---"
ram_hits=0
for marker in \
    "Issued Ramulator2 read request" \
    "Read completed for addr" \
    "Advanced Ramulator2 by" \
    "Issued Ramulator2 write request" \
    "Ramulator2 read completion"; do
    n=$(grep -c "$marker" "$log" 2>/dev/null || echo 0)
    printf "[TEST]   %-34s : %s\n" "$marker" "$n"
    ram_hits=$((ram_hits + n))
done

if [ "$ram_hits" -gt 0 ]; then
    echo "[TEST] RAMULATOR PATH: CONFIRMED ($ram_hits debug hits)"
else
    echo "[TEST] RAMULATOR PATH: NOT CONFIRMED — no Ramulator debug lines found."
    echo "[TEST]   Check that -v5 took effect and HAVE_RAMULATOR2 was defined"
    echo "[TEST]   (look for '-DHAVE_RAMULATOR2' in the driver compile line above)."
fi

# Show whether the driver was even compiled with Ramulator support this run.
if grep -q "DHAVE_RAMULATOR2" "$log" 2>/dev/null; then
    echo "[TEST] Driver compiled WITH -DHAVE_RAMULATOR2."
else
    echo "[TEST] WARNING: -DHAVE_RAMULATOR2 not seen in the build log."
fi

# ---- generated artifacts ----------------------------------------------------
for f in ramulator_profile.csv ramulator_debug.log; do
    if [ -f "$workdir/$f" ]; then
        lines=$(wc -l < "$workdir/$f")
        echo "[TEST] Found $f ($lines lines)"
    fi
done

tb_verilog="$workdir/HLS_output/simulation/bambu_testbench.v"
if [ -f "$tb_verilog" ]; then
    axi_ram_count=$(grep -c "TestbenchMEMAXIRamulatorIntegrated #" "$tb_verilog" 2>/dev/null || echo "0")
    echo "[TEST] TestbenchMEMAXIRamulatorIntegrated instances in Verilog: $axi_ram_count"
fi

echo "[TEST] DONE"
exit 0
