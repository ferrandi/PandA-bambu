#!/bin/bash
#   --args="--configuration-name=default-ALLBRAM-treevectorize --evaluation --compiler=I386_GCC49 --memory-allocation-policy=ALL_BRAM -ftree-vectorize" \
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py -l list_pipeline --tool=bambu \
   --args="--configuration-name=default --compiler=I386_GCC49 --experimental-setup=BAMBU" \
   -c=--simulator=MODELSIM \
   -o output_filtering_pipeline -b$script_dir --table=filtering_pipeline.tex --stop --name="FilteringPipeline" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

