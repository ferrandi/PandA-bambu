#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
$root_dir/5SGXEA7N2F45C1_custom.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/xc7vx690t-3ffg1930-VVD_custom.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/5SGXEA7N2F45C1_02.5_donotexploseglobals_alignedaccess.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/xc7vx690t-3ffg1930-VVD_02.5_donotexploseglobals_alignedaccess.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
