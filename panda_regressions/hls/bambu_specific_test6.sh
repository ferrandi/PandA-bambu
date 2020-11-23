#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=CLANG11_O0 -O0 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG11_O1 -O1 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG11_O2 -O2 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG10_O0 -O0 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG10" \
   --args="--configuration-name=CLANG10_O1 -O1 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG10" \
   --args="--configuration-name=CLANG10_O2 -O2 --simulate --experimental-setup=BAMBU --compiler=I386_CLANG10" \
   -lbambu_specific_test6_list -o output_BST6 -b$(dirname $0) --table=BST6output.tex --name="BambuSpecificTest6" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
