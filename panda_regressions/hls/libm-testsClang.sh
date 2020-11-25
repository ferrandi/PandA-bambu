#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu -llibm-tests_list -o output_libm-tests_Clang -b$(dirname $0) --table=libm-testsClang.tex --name="LibmTestsClang" \
   --args="--configuration-name=soft-floatC4 --compiler=I386_CLANG4 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC4-libm --compiler=I386_CLANG4 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC5 --compiler=I386_CLANG5 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC5-libm --compiler=I386_CLANG5 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC6 --compiler=I386_CLANG6 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC6-libm --compiler=I386_CLANG6 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC7 --compiler=I386_CLANG7 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC7-libm --compiler=I386_CLANG7 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC8 --compiler=I386_CLANG8 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC8-libm --compiler=I386_CLANG8 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC9 --compiler=I386_CLANG9 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC9-libm --compiler=I386_CLANG9 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
   --args="--configuration-name=soft-floatC10 --compiler=I386_CLANG10 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
   --args="--configuration-name=soft-floatC10-libm --compiler=I386_CLANG10 --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN --libm-std-rounding" \
  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
