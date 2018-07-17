#!/bin/bash
$(dirname $0)/test_softfloat_addsub32.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_softfloat_div32.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_libm_sinecosine.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_libm_logf.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_libm_expf.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_libm_sqrtf.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/test_libm_powf.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
             --args="--configuration-name=softfloat-tests-GCC49  --soft-float --compiler=I386_GCC49 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-tests -b$(dirname $0) --table=softfloat-tests.tex --name="softfloat-tests" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
