#!/bin/bash
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

# OpenMP Functional + AXI + Ramulator2 test driver
#
# Tests OpenMP functional benchmarks using AXI protocol with Ramulator2
# realistic DRAM timing via TestbenchMEMAXIRamulatorIntegrated.
# Includes both MEM_ACC_11 (1 channel) and MEM_ACC_NN (2 channels) configs.
#
# Usage: ./openmp_functional_axi_ramulator.sh [mantis args]

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("-m64" "-lm" "-fopenmp" "--simulate")
BATCH_ARGS+=("--enable-ramulator-integrated" "--clock-period=10.0" "--memory-allocation-policy=NO_BRAM")
BATCH_ARGS+=("--bus-pipelined")
OUT_SUFFIX="openmp_functional_axi_ramulator"

export RAMULATOR2_DIR="${RAMULATOR2_DIR:-${script_dir}/../../../ext/ramulator2-no-spdlog/install}"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=axi_ramulator ${BATCH_ARGS[*]}" \
   -lfunctional_list_axi_ramulator \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
