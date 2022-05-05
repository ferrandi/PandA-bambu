#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--soft-float" "--registered-inputs=top" "--panda-parameter=profile-top=1" "--simulate" "--fp-format-propagate")
OUT_SUFFIX="${compiler}_truefloat"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler}_exc-ovf --fp-exception-mode=overflow --max-ulp=1 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_rnd-trunc --fp-rounding-mode=truncate --max-ulp=4 --pretty-print=a.c ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m52 --fp-format=@*e11m52b-1023nih --max-ulp=1 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m45 --fp-format=@*e11m45b-1023nih --max-ulp=128 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m40 --fp-format=@*e11m40b-1023nih --max-ulp=4096 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m35 --fp-format=@*e11m35b-1023nih --max-ulp=131072 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m30 --fp-format=@*e11m30b-1023nih --max-ulp=4194304 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m23 --fp-format=@*e8m23b-127nih --max-ulp=536870912 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m17 --fp-format=@*e8m17b-127nih --max-ulp=34359738368 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m12 --fp-format=@*e8m12b-127nih --max-ulp=1099511627776 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m7  --fp-format=@*e8m7b-127nih  --max-ulp=35184372088832 ${BATCH_ARGS[*]}"\
   -ltruefloat_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
