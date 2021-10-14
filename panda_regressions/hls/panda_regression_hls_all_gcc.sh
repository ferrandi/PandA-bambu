#!/bin/bash
$(dirname $0)/gcc_regression_simple.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/softfloat-tests.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
if [[ "$@" != *"-wH"* ]] && [[ "$@" != *"--speculative-sdc-scheduling"* ]] ;  then
   $(dirname $0)/discrepancy.sh "$@"
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
fi
#$(dirname $0)/wb4_interface.sh "$@"
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
exit 0
