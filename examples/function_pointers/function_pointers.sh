#!/bin/bash
#   --args="--configuration-name=default-ALLBRAM-treevectorize --evaluation --compiler=I386_GCC49 --memory-allocation-policy=ALL_BRAM -ftree-vectorize" \
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py -l list --tool=bambu \
   --args="--configuration-name=default --evaluation --compiler=I386_GCC49 --soft-float" \
   --args="--configuration-name=default-ALLBRAM --evaluation --compiler=I386_GCC49 --memory-allocation-policy=ALL_BRAM --soft-float" \
   --args="--configuration-name=default-ALLBRAM-notreevectorize --evaluation --compiler=I386_GCC49 --memory-allocation-policy=ALL_BRAM -fno-tree-vectorize --soft-float" \
   -c=--simulator=MODELSIM \
   -o output_FunctionPointer -b$script_dir --table=output.tex --stop --name="FunctionPointer" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

