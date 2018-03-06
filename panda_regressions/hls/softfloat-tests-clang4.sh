#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-CLANG4  --soft-float --compiler=I386_CLANG4 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-CLANG5  --soft-float --compiler=I386_CLANG5 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-testsCLANG4 -b$(dirname $0) --table=softfloat-testsCLANG4.tex --name="softfloat-testsCLANG4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
