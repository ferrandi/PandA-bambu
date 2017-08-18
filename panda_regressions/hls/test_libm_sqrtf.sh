#!/bin/bash
abs_script=`readlink -e $0`
dir_script=`dirname $abs_script`
if test -f output_test_libm_sqrtf/finished; then
   exit 0
fi
rm -fr output_test_libm_sqrtf
mkdir output_test_libm_sqrtf
cd output_test_libm_sqrtf
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/poly_sqrtf.c -DCHECK_SQRT_FUNCTION -lm -lmpfr -lgmp
./a.out
cd ..
return_value=$?
if test $return_value != 0; then
   echo "C based test of softfloat sqrtf not passed."
   exit $return_value
fi
touch output_test_libm_sqrtf/finished
exit 0
