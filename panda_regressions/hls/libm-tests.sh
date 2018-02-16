#!/bin/bash
#   --args="--configuration-name=flopoco --flopoco --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0" \
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu -llibm-tests_list -o output_libm-tests -b$(dirname $0) --table=libm-tests.tex --name="LibmTests" \
   --args="--configuration-name=soft-float-libm --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 --libm-std-rounding" \
   --args="--configuration-name=soft-float --soft-float --simulate --experimental-setup=BAMBU -lm --reset-type=sync -DNO_MAIN -O0 -DFAITHFULLY_ROUNDED" \
  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
