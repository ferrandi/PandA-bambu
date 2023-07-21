#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--generate-interface=INFER" "-mx32" "-fno-tree-vectorize" "-s" "--hls-div" "--experimental-setup=BAMBU-PERFORMANCE-MP" "-DCUSTOM_VERIFICATION" "--simulator=VERILATOR" "--simulate")
OUT_SUFFIX="${compiler}_MachSuite"
BENCHMARKS_ROOT="${script_dir}/MachSuite"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler} -I${BENCHMARKS_ROOT}/common --generate-tb=${BENCHMARKS_ROOT}/common/harness.c --generate-tb=${BENCHMARKS_ROOT}/common/support.c ${BATCH_ARGS[*]}"\
   -l${script_dir}/machsuite_list \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   --name="${OUT_SUFFIX}" "$@"
