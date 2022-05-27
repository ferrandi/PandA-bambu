#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "-fwhole-program" "-fno-delete-null-pointer-checks" "--simulate" "-s")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_CHStone"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${configuration} ${BATCH_ARGS[*]}"\
   -lchstone_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
