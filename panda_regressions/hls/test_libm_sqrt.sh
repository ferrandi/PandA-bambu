#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_libm_sqrt/finished; then
   exit 0
fi
rm -fr output_test_libm_sqrt
mkdir output_test_libm_sqrt
cd output_test_libm_sqrt
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/poly_sqrt.c -DCHECK_SQRT_FUNCTION -lm 
./a.out
return_value=$?
cd ..
if test $return_value != 0; then
   echo "C based test of softfloat sqrt not passed."
   exit $return_value
fi
touch output_test_libm_sqrt/finished
exit 0
