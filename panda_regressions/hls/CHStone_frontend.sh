#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=GCC45-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC45 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC46-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC46 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC47-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC47 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC48-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC48 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC5-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC5 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC6-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC6 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC7-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC7 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC8 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG4-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG4 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG5-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG5 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG6-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG6 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG7-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG7 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG8 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG9 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG10 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANG11 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=CLANG8-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_CLANGVVD --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O0-wp-NN --simulate -O0 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O1-wp-NN --simulate -O1 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O2-wp-NN --simulate -O2 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O3-wp-NN --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O4-wp-NN --simulate -O4 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-O5-wp-NN --simulate -O5 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             --args="--configuration-name=GCC49-Os-wp-NN --simulate -Os -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --experimental-setup=BAMBU"\
             -lCHStone_list -o output_CHStone_frontend -b$(dirname $0) --table=CHStone_frontend.tex --name="CHStone" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
