#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f $dir_script/testfloat/finished; then
   exit 0
fi
make -C $dir_script/testfloat/berkeley-softfloat/build/bambu clean
make -C $dir_script/testfloat/build/bambu clean
make -j -C $dir_script/testfloat/berkeley-softfloat/build/bambu
make -j -C $dir_script/testfloat/build/bambu
$dir_script/testfloat/build/bambu/testfloat -rnear_even -level 2 -all2 -skipSubnormals
return_value=$?
if test $return_value != 0; then
   echo "C based test of softfloat not passed."
   exit $return_value
fi
touch $dir_script/testfloat/finished
exit 0