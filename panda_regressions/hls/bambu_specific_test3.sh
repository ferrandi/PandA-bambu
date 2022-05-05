#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--expose-globals" "--compiler=I386_GCC49")
OUT_SUFFIX="bambu_specific_test3"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O3-none --hls-div=none -O3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3-as   --hls-div=as   -O3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3-nr1  --hls-div=nr1  -O3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3-nr2  --hls-div=nr2  -O3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3-NR   --hls-div=NR   -O3 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test3_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
