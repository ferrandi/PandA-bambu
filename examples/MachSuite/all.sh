#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
$dir_script/xc7vx690t-3ffg1930-VVD_10.0_O2_sdc_hls_div.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

