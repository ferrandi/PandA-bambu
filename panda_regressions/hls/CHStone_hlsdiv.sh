#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=GCC49-O3-hls-div --simulate -O3 -fwhole-program --compiler=I386_GCC49 --clock-period=15 -D'printf(fmt, ...)=' --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 --experimental-setup=BAMBU --hls-div"\
             -lCHStone_hlsdiv_list -o output_CHStoneHLSDiv -b$script_dir --table=CHStoneMemArch.tex --name="CHStoneHLSDiv" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
