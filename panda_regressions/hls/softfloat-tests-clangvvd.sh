#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-CLANGVVD  --soft-float --compiler=I386_CLANGVVD --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-testsCLANGVVD -b$(dirname $0) --table=softfloat-testsCLANGVVD.tex --name="softfloat-testsCLANGVVD" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
