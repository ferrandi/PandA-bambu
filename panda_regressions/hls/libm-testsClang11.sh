#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu -llibm-tests_list -o output_libm-tests_Clang11 -b$(dirname $0) --table=libm-testsClang11.tex --name="LibmTestsClang" \
   --args="--configuration-name=soft-floatC11 --compiler=I386_CLANG11 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC11-libm --compiler=I386_CLANG11 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
