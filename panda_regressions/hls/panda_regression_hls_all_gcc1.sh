#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
$script_dir/gcc_regression_simple1.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$script_dir/softfloat-tests1.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
if [[ "$@" != *"-wH"* ]] && [[ "$@" != *"--speculative-sdc-scheduling"* ]] ;  then
   $script_dir/discrepancy1.sh "$@"
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
fi
#$script_dir/wb4_interface.sh "$@"
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
exit 0
