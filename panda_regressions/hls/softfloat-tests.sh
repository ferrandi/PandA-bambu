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
             --args="--configuration-name=softfloat-tests-GCC45  --soft-float --compiler=I386_GCC45 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC46  --soft-float --compiler=I386_GCC46 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC47  --soft-float --compiler=I386_GCC47 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC48  --soft-float --compiler=I386_GCC48 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC49  --soft-float --compiler=I386_GCC49 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC5  --soft-float --compiler=I386_GCC5 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC6  --soft-float --compiler=I386_GCC6 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC7  --soft-float --compiler=I386_GCC7 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             --args="--configuration-name=softfloat-tests-GCC8  --soft-float --compiler=I386_GCC8 --max-ulp=0 --experimental-setup=BAMBU-PERFORMANCE-MP"\
             -lsoftfloat-tests_list -o output_softfloat-tests -b$(dirname $0) --table=softfloat-tests.tex --name="softfloat-tests" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang4.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang5.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang6.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang7.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang8.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang9.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang10.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests-clang11.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

