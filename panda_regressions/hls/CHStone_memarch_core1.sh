#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=GCC49-O3-wp-VVD-NN-D00-LSS --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU --device-name=xc7z020,-1,clg484,VVD --memory-allocation-policy=LSS"\
             --args="--configuration-name=GCC49-O3-wp-VVD-NN-D00-GSS --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU --device-name=xc7z020,-1,clg484,VVD --memory-allocation-policy=GSS"\
             --args="--configuration-name=GCC49-O3-wp-VVD-NN-D00-NO-BRAM --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU --device-name=xc7z020,-1,clg484,VVD --memory-allocation-policy=NO_BRAM"\
             -lCHStone_memarch_list -o output_CHStoneMemArch_core1 -b`dirname $0` --table=CHStoneMemArch.tex --name="CHStoneMemArch_AllocationPolicy" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
