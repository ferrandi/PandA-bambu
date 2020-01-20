#!/bin/bash
ARGS="-c=--compiler=I386_GCC49 -c=--device-name=TO_BE_OVERWRITTEN -c=--evaluation -c=-fwhole-program -c=-fno-delete-null-pointer-checks -c=--no-iob"
NAME=$(basename $0 .sh)
$(dirname $0)/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_altera.xml" --tool=bambu -ooutput_$NAME $ARGS -t300m --table=$NAME.tex --benchmarks_root=$(dirname $0) $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

