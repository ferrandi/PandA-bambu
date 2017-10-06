#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if [ ! -d sdc_test ]; then
   mkdir sdc_test
fi
cd sdc_test
$dir_script/panda_regression_hls1.sh $@ -c="--speculative-sdc-scheduling"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d lb_test ]; then
   mkdir lb_test
fi
cd lb_test
$dir_script/panda_regression_hls1.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d vhdl_test ]; then
   mkdir vhdl_test
fi
cd vhdl_test
$dir_script/panda_regression_hls1.sh $@ -c="-wH" --name="_VHDL"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
