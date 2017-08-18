#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
$dir_script/panda_regression_hls_release.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
if test -f $dir_script/panda_regression_hls_no_release.sh; then
   $dir_script/panda_regression_hls_no_release.sh $@
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
fi
$dir_script/../examples/example.sh -c="--evaluation=CYCLES" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
