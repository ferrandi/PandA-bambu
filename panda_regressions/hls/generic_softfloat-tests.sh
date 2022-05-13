#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="${compiler}_softfloat-tests"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${compiler} ${BATCH_ARGS[*]}"\
   -lsoftfloat-tests_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" $ARGS
