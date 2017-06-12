#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
$dir_script/../../etc/scripts/test_panda.py --tool=bambu \
     --args="--debug-classes=HWDiscrepancyAnalysis --generate-vcd --configuration-name=with --aligned -fwhole-program -fno-delete-null-pointer-checks --no-iob --clock-period=5 --evaluation --device=5SGXEA7N2F45C1 --fsm-encoding=binary --discrepancy-hw" \
     --args="--generate-vcd --configuration-name=without --aligned -fwhole-program -fno-delete-null-pointer-checks --no-iob --clock-period=5 --evaluation --device=5SGXEA7N2F45C1 --fsm-encoding=binary" \
     -l$dir_script/CHStone_list -o output_hw_discrepancy -b$dir_script --table=table_hw_discrepancy.tex --name="hw_discrepancy" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
