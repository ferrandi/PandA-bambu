#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
     --args="--configuration-name=with --aligned -fwhole-program -fno-delete-null-pointer-checks --clock-period=5 --simulate --device=5SGXEA7N2F45C1 --fsm-encoding=binary --discrepancy-hw --compiler=I386_GCC8" \
     --args="--configuration-name=without --aligned -fwhole-program -fno-delete-null-pointer-checks --clock-period=5 --simulate --device=5SGXEA7N2F45C1 --fsm-encoding=binary --compiler=I386_GCC8" \
     -l$script_dir/CHStone_list -o output_hw_discrepancy -b$script_dir --table=table_hw_discrepancy.tex --name="hw_discrepancy" -t 120m "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
