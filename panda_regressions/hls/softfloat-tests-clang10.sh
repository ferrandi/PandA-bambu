#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-CLANG10  --soft-float --compiler=I386_CLANG10 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-testsCLANG10 -b$(dirname $0) --table=softfloat-testsCLANG10.tex --name="softfloat-testsCLANG10" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
