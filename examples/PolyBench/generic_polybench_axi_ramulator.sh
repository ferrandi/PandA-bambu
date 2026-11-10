#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# PolyBench AXI + Ramulator2 test driver (banked bus AXI, no source modifications)
#
# Tests PolyBench benchmarks using banked bus AXI protocol with Ramulator2
# realistic DRAM timing via TestbenchMEMAXIRamulatorIntegrated.
#
# Usage: ./generic_polybench_axi_ramulator.sh [mantis args]

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. ${script_dir}/../../panda_regressions/hls/generic_getopt.sh

DATASET_SIZE="mini"
DATASET_SIZE_U="$(echo $DATASET_SIZE|tr '[:lower:]' '[:upper:]')"
BATCH_ARGS=("-m64" "-D${DATASET_SIZE_U}_DATASET" "--std=gnu11")
BATCH_ARGS+=("--generate-tb=BENCHMARKS_ROOT/utilities/polybench.c")
BATCH_ARGS+=("--generate-interface=INFER" "--panda-parameter=simple-benchmark-name=1")
BATCH_ARGS+=("--max-sim-cycles=20000000" "--simulate")
BATCH_ARGS+=("-p=__float,expf" "--hls-fpdiv=SRT4U" "-s")
BATCH_ARGS+=("-DPOLYBENCH_USE_RESTRICT" "-DDATA_TYPE_IS_FLOAT")
BATCH_ARGS+=("--enable-ramulator-integrated" "--clock-period=10.0" "--memory-allocation-policy=NO_BRAM")

configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="_${configuration}_Polybench_AXI_Ramulator.${DATASET_SIZE}"
BENCHMARKS_ROOT="${script_dir}/PolyBenchC"

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-${script_dir}/../../ext/ramulator2-no-spdlog/install}"

python3 ${script_dir}/../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=${configuration}-axi-1ch --channels-number=1 --channels-type=MEM_ACC_11 -funroll-loops ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${configuration}-axi-2ch --channels-number=2 --channels-type=MEM_ACC_NN -funroll-loops ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${configuration}-axi-4ch --channels-number=4 --channels-type=MEM_ACC_NN -funroll-loops ${BATCH_ARGS[*]}" \
   -l${script_dir}/polybench_list_axi_ramulator \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"
