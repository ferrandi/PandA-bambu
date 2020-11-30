#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=CLANG4  --evaluation=TOTAL_CYCLES,CYCLES --compiler=I386_CLANG4" \
   --args="--configuration-name=CLANG10  --evaluation=TOTAL_CYCLES,CYCLES --compiler=I386_CLANG10" \
   -lbambu_specific_test5_list -o output_BST5 -b$(dirname $0) --table=BST5output.tex --name="BambuSpecificTest5" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
