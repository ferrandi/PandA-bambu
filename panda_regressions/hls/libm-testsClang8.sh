#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu -llibm-tests_list -o output_libm-tests_Clang8 -b$(dirname $0) --table=libm-testsClang8.tex --name="LibmTestsClang" \
   --args="--configuration-name=soft-floatC8 --compiler=I386_CLANG8 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC8-libm --compiler=I386_CLANG8 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
