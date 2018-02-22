#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
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
