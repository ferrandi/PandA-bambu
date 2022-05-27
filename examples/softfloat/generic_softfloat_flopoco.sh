#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--flopoco" "--registered-inputs=top" "--panda-parameter=profile-top=1" "--simulate" "--max-ulp=0")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_softfloat_flopoco"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${configuration} ${BATCH_ARGS[*]}"\
   -lsoftfloat_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
