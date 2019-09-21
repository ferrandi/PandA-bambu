#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=CLANG-O0-wp-NN --simulate -O0 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-O1-wp-NN --simulate -O1 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-O2-wp-NN --simulate -O2 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-O4-wp-NN --simulate -O4 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-O5-wp-NN --simulate -O5 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG-Os-wp-NN --simulate -Os -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             -lCHStone_list -o output_CHStone_frontendCLANG4 -b$(dirname $0) --table=CHStone_frontendCLANG4.tex --name="CHStoneCLANG4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
