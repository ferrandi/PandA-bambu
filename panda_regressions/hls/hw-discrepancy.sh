+#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=without --clock-period=5 --evaluation --device=5SGXEA7N2F45C1 --fsm-encoding=binary" \
     --args="--configuration-name=with --clock-period=5 --evaluation --device=5SGXEA7N2F45C1 --fsm-encoding=binary --discrepancy-hw" \
     -lCHStone_list -o output_hw_discrepancy -b`dirname $0` --table=table_hw_discrepancy.tex --name="hw_discrepancy" $@ \
     --spider-style="$root_dir/latex_format_bambu_results_altera_synth.xml"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

