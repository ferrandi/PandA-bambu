#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_softfloat_addsub32/finished; then
   exit 0
fi
rm -fr output_test_softfloat_addsub32
mkdir output_test_softfloat_addsub32
cd output_test_softfloat_addsub32
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ -I$dir_script/../../etc/libbambu/softfloat $dir_script/test_softfloat_addsub32.c
./a.out
cd ..
return_value=$?
if test $return_value != 0; then
   echo "C based test of softfloat addition not passed."
   exit $return_value
fi
touch output_test_softfloat_addsub32/finished
exit 0
