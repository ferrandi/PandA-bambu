#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# GAPBS + AXI + Ramulator2 test driver
#
# Tests GAPBS graph benchmarks (BFS, CC, PageRank, Triangle Count) using
# AXI protocol with Ramulator2 realistic DRAM timing.
# Reduced configuration set for reasonable test time.
#
# Usage: ./generic_GAPBS_axi_ramulator.sh [mantis args]

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../../panda_regressions/hls/generic_getopt.sh

BENCHMARKS_ROOT="${script_dir}/gapbs"
BATCH_ARGS=("-m64" "-lm" "-fopenmp" "--generate-interface=INFER")
BATCH_ARGS+=("--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "--tb-queue-size=20")
BATCH_ARGS+=("--enable-ramulator-integrated" "--clock-period=10.0")
OUT_SUFFIX="${compiler}_gapbs_axi_ramulator"

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-${script_dir}/../../../ext/ramulator2-no-spdlog/install}"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=1co_1cs    -DTHREAD_NUMBER=1                       --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=2co_1cs    -DTHREAD_NUMBER=2                       --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=4co_1cs    -DTHREAD_NUMBER=4                       --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=2co_2cs    -DTHREAD_NUMBER=4    --context_switch=2 --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}" \
   -l${script_dir}/gapbs_list \
   --timeout=3000m \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"
exit $?
