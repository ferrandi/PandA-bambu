#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. ${script_dir}/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--simulate" "--experimental-setup=BAMBU")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_function_pointers"

python3 ${script_dir}/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=base" \
   --args="--configuration-name=allbram --memory-allocation-policy=ALL_BRAM" \
   --args="--configuration-name=allbram_ntv --memory-allocation-policy=ALL_BRAM -fno-tree-vectorize" \
   -l${script_dir}/function_pointers_list \
   -o "out${OUT_SUFFIX}" -b${script_dir} \
   --name="${OUT_SUFFIX}" "$@"
