#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/panda_regression_hls_release1.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
