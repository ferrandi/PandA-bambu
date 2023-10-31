#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BENCHMARKS_ROOT="${script_dir}/MachSuite"
BATCH_ARGS=("-I${BENCHMARKS_ROOT}/common" "--generate-tb=${BENCHMARKS_ROOT}/common/harness.c" "--generate-tb=${BENCHMARKS_ROOT}/common/support.c")
BATCH_ARGS+=("-mx32" "-fno-tree-vectorize")
BATCH_ARGS+=("--generate-interface=INFER" "-s" "--hls-div=NR")
BATCH_ARGS+=("-DCUSTOM_VERIFICATION" "--simulator=VERILATOR" "--simulate")
OUT_SUFFIX="${compiler}_MachSuite"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler}_BALANCED_MP --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_PERFORMANCE_MP --experimental-setup=BAMBU-PERFORMANCE-MP ${BATCH_ARGS[*]}" \
   -l${script_dir}/machsuite_list \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   --name="${OUT_SUFFIX}" "$@"
