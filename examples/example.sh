#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
$dir_script/softfloat/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/libm/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/MachSuite/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/hls_study/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
