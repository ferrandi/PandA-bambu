#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--registered-inputs=top" "--panda-parameter=profile-top=1" "--simulate" "--fp-format-propagate" "-DCUSTOM_VERIFICATION")
BATCH_ARGS+=("--generate-tb=${script_dir}/common/harness.c" "--generate-tb=${script_dir}/common/support.c" "--generate-tb=$(readlink -e ${script_dir}/../../etc/libbambu/softfloat/softfloat.c)")
BATCH_ARGS+=("-I${script_dir}/common" "-I$(readlink -e ${script_dir}/../../etc/libbambu)" "-I$(readlink -e ${script_dir}/../../etc/libbambu/softfloat)")
OUT_SUFFIX="${compiler}_truefloat"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler}_exc-ovf --fp-exception-mode=overflow  -DTEST_EXC=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_rnd-trunc --fp-rounding-mode=truncate -DTEST_RND=0 -DMAX_ULP=4 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m52 --fp-format=@*e11m52b-1023nih -DTEST_FRAC_BITS=52 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m45 --fp-format=@*e11m45b-1023nih -DTEST_FRAC_BITS=45 -DMAX_ULP=128 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m40 --fp-format=@*e11m40b-1023nih -DTEST_FRAC_BITS=40 -DMAX_ULP=4096 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m35 --fp-format=@*e11m35b-1023nih -DTEST_FRAC_BITS=35 -DMAX_ULP=131072 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m30 --fp-format=@*e11m30b-1023nih -DTEST_FRAC_BITS=30 -DMAX_ULP=4194304 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m23 --fp-format=@*e8m23b-127nih -DTEST_EXP_BITS=8 -DTEST_EXP_BIAS=-127 -DTEST_FRAC_BITS=23 -DMAX_ULP=536870912 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m17 --fp-format=@*e8m17b-127nih -DTEST_EXP_BITS=8 -DTEST_EXP_BIAS=-127 -DTEST_FRAC_BITS=17 -DMAX_ULP=34359738368 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m12 --fp-format=@*e8m12b-127nih -DTEST_EXP_BITS=8 -DTEST_EXP_BIAS=-127 -DTEST_FRAC_BITS=12 -DMAX_ULP=1099511627776 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m7  --fp-format=@*e8m7b-127nih  -DTEST_EXP_BITS=8 -DTEST_EXP_BIAS=-127 -DTEST_FRAC_BITS=7  -DMAX_ULP=35184372088832 ${BATCH_ARGS[*]}"\
   -ltruefloat_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
