#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-CLANG7  --soft-float --compiler=I386_CLANG7 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-testsCLANG7 -b$(dirname $0) --table=softfloat-testsCLANG7.tex --name="softfloat-testsCLANG7" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
