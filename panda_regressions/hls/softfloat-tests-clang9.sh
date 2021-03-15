#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-CLANG9  --soft-float --compiler=I386_CLANG9 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-testsCLANG9 -b$(dirname $0) --table=softfloat-testsCLANG9.tex --name="softfloat-testsCLANG9" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
