#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--soft-float" "--registered-inputs=top" "--panda-parameter=profile-top=1" "--device-name=TO_BE_OVERWRITTEN" "--simulate" "-s")
OUT_SUFFIX="${compiler}_truefloat"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler}_wo-exc --softfloat-noexception   --max-ulp=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_wo-rnd --softfloat-norounding    --max-ulp=4 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m52 --fp-format=@*11*52*-1023 --max-ulp=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m45 --fp-format=@*11*45*-1023 --max-ulp=128 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m40 --fp-format=@*11*40*-1023 --max-ulp=4096 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m35 --fp-format=@*11*35*-1023 --max-ulp=131072 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e11m30 --fp-format=@*11*30*-1023 --max-ulp=4194304 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m23  --fp-format=@*8*23*-127  --max-ulp=536870912 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m17  --fp-format=@*8*17*-127  --max-ulp=34359738368 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m12  --fp-format=@*8*12*-127  --max-ulp=1099511627776 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${compiler}_e8m7   --fp-format=@*8*7*-127   --max-ulp=35184372088832 ${BATCH_ARGS[*]}"\
   -ltruefloat_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
