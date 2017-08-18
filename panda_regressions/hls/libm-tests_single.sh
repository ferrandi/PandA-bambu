#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py --tool=bambu -llibm-tests_list_single -o output_libm-tests_single -b`dirname $0` --table=libm-tests_single.tex --name="LibmTestsSingle" \
   --args="--configuration-name=soft-float --soft-float --experimental-setup=BAMBU -lm -O0" \
  $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
