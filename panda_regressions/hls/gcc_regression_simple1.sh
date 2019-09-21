#!/bin/bash
$(dirname $0)/gcc49_regression_simple_ext_pipelined.sh -c=--reset-type=sync $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc49_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc49_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
