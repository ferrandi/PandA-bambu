#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC49" \
   -lbambu_specific_test_list -o output_BST -b$(dirname $0) --table=BSToutput.tex --name="BambuSpecificTest" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
#   --args="--configuration-name=O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC49" 

