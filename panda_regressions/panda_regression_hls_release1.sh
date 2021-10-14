#!/bin/bash
$(dirname $0)/hls/panda_regression_hls_all_gcc1.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/hls/panda_regression_hls_no_gcc1.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
