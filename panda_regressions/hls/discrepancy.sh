#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC7_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7 --discrepancy" \
   --args="--configuration-name=GCC7_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7 --discrepancy" \
   --args="--configuration-name=GCC7_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7 --discrepancy" \
   --args="--configuration-name=GCC7_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7 --discrepancy" \
   -ldiscrepancy_list -o output_discrepancy_test \
   -b$dir_script --table=output_discrepancy.tex --name="Discrepancy" $@
exit $?
