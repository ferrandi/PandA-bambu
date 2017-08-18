#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=O0 -O0 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=O1 -O1 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=O2 -O2 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=O3 -O3 -lm --evaluation=TOTAL_CYCLES,CYCLES --experimental-setup=BAMBU --compiler=I386_GCC49" \
   -lbambu_specific_test2_list -o output_BST2 -b`dirname $0` --table=BST2output.tex --name="BambuSpecificTest2" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
