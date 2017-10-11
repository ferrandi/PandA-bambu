#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC5_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC5" \
   -ldiscrepancy_list -o output_discrepancy_test --ulimit="3-f 6000000 -v 10000000 -s 16384" -j3 \
   -b$dir_script --table=output_discrepancy.tex --name="Discrepancy" $@
exit $?
