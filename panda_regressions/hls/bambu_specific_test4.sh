#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=O2-INFER --generate-interface=INFER -O2 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --compiler=I386_CLANG6" \
   -lbambu_specific_test4_list -o output_BST4 -b$(dirname $0) --table=BST4output.tex --name="BambuSpecificTest4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
