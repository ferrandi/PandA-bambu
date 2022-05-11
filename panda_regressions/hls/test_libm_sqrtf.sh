#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_libm_sqrtf/finished; then
   exit 0
fi
if [[ -z "$J" ]]; then
   J="1"
fi
echo "Parallel jobs: $J"
rm -fr output_test_libm_sqrtf
mkdir output_test_libm_sqrtf
cd output_test_libm_sqrtf
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/poly_sqrtf.c -DCHECK_SQRT_FUNCTION -lm -lmpfr -lgmp
export OMP_NUM_THREADS=$J
./a.out
return_value=$?
cd ..
if test $return_value != 0; then
   echo "C based test of softfloat sqrtf not passed."
   exit $return_value
fi
touch output_test_libm_sqrtf/finished
exit 0
