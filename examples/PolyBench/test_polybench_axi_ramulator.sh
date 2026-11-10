#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# PolyBench + AXI + Ramulator2 standalone test
#
# Runs a single PolyBench benchmark with per-parameter m_axi pragmas
# and Ramulator2 realistic DRAM timing.
#
# Usage:
#   ./test_polybench_axi_ramulator.sh <channels> <benchmark>
#
# Examples:
#   ./test_polybench_axi_ramulator.sh 1 gemm
#   ./test_polybench_axi_ramulator.sh 2 atax
#   ./test_polybench_axi_ramulator.sh 4 jacobi-1d
#
# Available benchmarks:
#   gemm, atax, bicg, mvt, trisolv, jacobi-1d, jacobi-2d, floyd-warshall

set -eo pipefail

# --- Benchmark registry: name -> source path, top function ---
declare -A BENCH_SRC BENCH_TOP BENCH_TB
BENCH_SRC[gemm]="linear-algebra/blas/gemm/gemm.c"
BENCH_TOP[gemm]="kernel_gemm"
BENCH_TB[gemm]="linear-algebra/blas/gemm/tb_gemm.c"

BENCH_SRC[atax]="linear-algebra/kernels/atax/atax.c"
BENCH_TOP[atax]="kernel_atax"
BENCH_TB[atax]="linear-algebra/kernels/atax/tb_atax.c"

BENCH_SRC[bicg]="linear-algebra/kernels/bicg/bicg.c"
BENCH_TOP[bicg]="kernel_bicg"
BENCH_TB[bicg]="linear-algebra/kernels/bicg/tb_bicg.c"

BENCH_SRC[mvt]="linear-algebra/kernels/mvt/mvt.c"
BENCH_TOP[mvt]="kernel_mvt"
BENCH_TB[mvt]="linear-algebra/kernels/mvt/tb_mvt.c"

BENCH_SRC[trisolv]="linear-algebra/solvers/trisolv/trisolv.c"
BENCH_TOP[trisolv]="kernel_trisolv"
BENCH_TB[trisolv]="linear-algebra/solvers/trisolv/tb_trisolv.c"

BENCH_SRC[jacobi-1d]="stencils/jacobi-1d/jacobi-1d.c"
BENCH_TOP[jacobi-1d]="kernel_jacobi_1d"
BENCH_TB[jacobi-1d]="stencils/jacobi-1d/tb_jacobi-1d.c"

BENCH_SRC[jacobi-2d]="stencils/jacobi-2d/jacobi-2d.c"
BENCH_TOP[jacobi-2d]="kernel_jacobi_2d"
BENCH_TB[jacobi-2d]="stencils/jacobi-2d/tb_jacobi-2d.c"

BENCH_SRC[floyd-warshall]="medley/floyd-warshall/floyd-warshall.c"
BENCH_TOP[floyd-warshall]="kernel_floyd_warshall"
BENCH_TB[floyd-warshall]="medley/floyd-warshall/tb_floyd-warshall.c"

ALL_BENCHMARKS=$(echo "${!BENCH_SRC[@]}" | tr ' ' '\n' | sort | tr '\n' ' ')

# --- Parse arguments ---
if [ -z "$1" ] || ! [[ "$1" =~ ^[0-9]+$ ]] || [ -z "$2" ]; then
    echo "Usage: $0 <channels> <benchmark> [extra bambu flags]"
    echo ""
    echo "  channels:  number of Ramulator DRAM channels (1, 2, 4, ...)"
    echo "  benchmark: one of: $ALL_BENCHMARKS"
    exit 1
fi

CHANNELS="$1"
BENCH="$2"
shift 2

if [ -z "${BENCH_SRC[$BENCH]}" ]; then
    echo "[ERROR] Unknown benchmark: $BENCH"
    echo "Available: $ALL_BENCHMARKS"
    exit 1
fi

script=$(readlink -e "$0")
root_dir=$(dirname "$script")
POLYBENCH_AXI="${root_dir}/PolyBenchC_AXI"

if [ ! -d "$POLYBENCH_AXI" ]; then
    echo "[ERROR] PolyBenchC_AXI directory not found at $POLYBENCH_AXI"
    echo "        Generate it first: cp -r PolyBenchC PolyBenchC_AXI && python3 add_axi_pragmas.py"
    exit 1
fi

BAMBU_BIN="${BAMBU_BIN:-$(command -v bambu || echo bambu)}"
if [ ! -x "$BAMBU_BIN" ] && ! command -v "$BAMBU_BIN" &>/dev/null; then
    echo "[ERROR] bambu not found at $BAMBU_BIN"
    exit 1
fi

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-$root_dir/../../ext/ramulator2-no-spdlog/install}"

# --- Channel flags ---
CHANNEL_FLAGS=()
if [ "$CHANNELS" -eq 1 ]; then
    CHANNEL_FLAGS=(--channels-type=MEM_ACC_11 --channels-number=1)
else
    CHANNEL_FLAGS=(--channels-type=MEM_ACC_NN "--channels-number=${CHANNELS}" --bus-pipelined)
fi

# --- Paths ---
src_file="${POLYBENCH_AXI}/${BENCH_SRC[$BENCH]}"
tb_file="${POLYBENCH_AXI}/${BENCH_TB[$BENCH]}"
top_fn="${BENCH_TOP[$BENCH]}"

workdir="${root_dir}/polybench_axi_ram_${BENCH}_${CHANNELS}ch/hls_output"
mkdir -p "$workdir"

echo "============================================================"
echo "[TEST] PolyBench AXI + Ramulator2"
echo "[TEST] Benchmark : $BENCH ($top_fn)"
echo "[TEST] Channels  : $CHANNELS"
echo "[TEST] Source     : $src_file"
echo "[TEST] Testbench : $tb_file"
echo "[TEST] Output    : $workdir"
echo "============================================================"

cd "$workdir"

timeout 30m "$BAMBU_BIN" \
    --simulator=VERILATOR \
    --compiler=I386_CLANG13 \
    -m64 \
    --generate-interface=INFER \
    "--generate-tb=$tb_file" \
    --simulate \
    "--top-fname=$top_fn" \
    --enable-ramulator-integrated \
    --memory-allocation-policy=NO_BRAM \
    --clock-period=10.0 \
    -v4 \
    --max-sim-cycles=20000000 \
    -DMINI_DATASET \
    --std=gnu11 \
    -DPOLYBENCH_USE_RESTRICT \
    -DDATA_TYPE_IS_FLOAT \
    "-p=__float,expf" \
    --hls-fpdiv=SRT4U \
    -s \
    "-I${POLYBENCH_AXI}/utilities" \
    "${POLYBENCH_AXI}/utilities/polybench.c" \
    "${CHANNEL_FLAGS[@]}" \
    "$src_file" \
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
