#!/bin/bash
ARGS="-c=--no-iob -c=--soft-float -c=--hls-div -c=--registered-inputs=top -c=--panda-parameter=profile-top=1 -c=--device-name=TO_BE_OVERWRITTEN -c=--evaluation"
$(dirname $0)/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_lattice.xml" --tool=bambu -l$(dirname $0)/libm_list -olibm_altera $ARGS -t120m --table=libm_lattice.tex --benchmarks_root=$(dirname $0)  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
