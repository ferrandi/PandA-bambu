#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--simulate" "-lm" "--reset-type=sync" "--experimental-setup=BAMBU")
OUT_SUFFIX="${compiler}_libm_tests"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${compiler}-soft-float -O0 -DFAITHFULLY_ROUNDED --max-ulp=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}-soft-float-std --libm-std-rounding --max-ulp=0 ${BATCH_ARGS[*]}" \
   -llibm-tests_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" $ARGS
