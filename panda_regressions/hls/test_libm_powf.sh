#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_libm_powf/finished; then
   exit 0
fi
rm -fr output_test_libm_powf
mkdir output_test_libm_powf
cd output_test_libm_powf
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/wf_pow_opt.c -DCHECK_POW_FUNCTION -lm
./a.out
cd ..
return_value=$?
if test $return_value != 0; then
   echo "C based test of softfloat pow not passed."
   exit $return_value
fi
touch output_test_libm_powf/finished
exit 0
