#!/bin/bash
ARGS="--clock-period=10 --speculative-sdc-scheduling --experimental-setup=BAMBU-PERFORMANCE-MP --device=5CSEMA5F31C6 --compiler=I386_GCC49 --evaluation -fwhole-program -fno-delete-null-pointer-checks --no-iob"
script=`readlink -e $0`
root_dir=`dirname $script`
NAME=`basename $0 .sh`
DIRNAME=${root_dir##*/}
`dirname $0`/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_altera.xml" --tool=bambu -l`dirname $0`/5CSEMA5F31C6_O3_list -ooutput_${DIRNAME}_$NAME --args="$ARGS" -t60m --table=${DIRNAME}_$NAME.tex --benchmarks_root=`dirname $0` --name=${DIRNAME}_$NAME $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
