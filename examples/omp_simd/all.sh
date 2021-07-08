#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/5SGXEA7N2F45C1_10.0_O2.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_10.0_O2.sh "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

