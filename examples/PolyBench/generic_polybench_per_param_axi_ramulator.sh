#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# PolyBench per-parameter m_axi + Ramulator2 test driver
#
# Tests PolyBench benchmarks with per-parameter #pragma HLS interface m_axi
# pragmas (each array gets its own AXI port) combined with Ramulator2.
# Uses the PolyBenchC_AXI/ copies which have the m_axi pragmas inserted.
#
# Usage: ./generic_polybench_per_param_axi_ramulator.sh [mantis args]

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. ${script_dir}/../../panda_regressions/hls/generic_getopt.sh

DATASET_SIZE="mini"
DATASET_SIZE_U="$(echo $DATASET_SIZE|tr '[:lower:]' '[:upper:]')"
BATCH_ARGS=("-m64" "-D${DATASET_SIZE_U}_DATASET" "--std=gnu11")
BATCH_ARGS+=("--generate-interface=INFER" "--panda-parameter=simple-benchmark-name=1")
BATCH_ARGS+=("--max-sim-cycles=20000000" "--simulate")
BATCH_ARGS+=("-p=__float,expf" "--hls-fpdiv=SRT4U" "-s")
BATCH_ARGS+=("-DPOLYBENCH_USE_RESTRICT" "-DDATA_TYPE_IS_FLOAT")
BATCH_ARGS+=("--enable-ramulator" "--clock-period=10.0" "--memory-allocation-policy=NO_BRAM")

configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="_${configuration}_Polybench_PerParam_AXI_Ramulator.${DATASET_SIZE}"

# Use PolyBenchC_AXI for sources (with m_axi pragmas) but original for testbenches
BENCHMARKS_ROOT="${script_dir}/PolyBenchC_AXI"

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-${script_dir}/../../ext/ramulator2-no-spdlog/install}"

python3 ${script_dir}/../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=${configuration}-perparam-axi --generate-tb=BENCHMARKS_ROOT/utilities/polybench.c ${BATCH_ARGS[*]}" \
   -l${script_dir}/polybench_list_per_param_axi \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"
