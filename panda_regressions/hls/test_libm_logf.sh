#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_libm_logf/finished; then
   exit 0
fi
rm -fr output_test_libm_logf
mkdir output_test_libm_logf
cd output_test_libm_logf
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/hotbm_logf.c -DCHECK_LOG_FUNCTION -lm -lmpfr -lgmp
./a.out
return_value=$?
cd ..
if test $return_value != 0; then
   echo "C based test of softfloat logf not passed."
   exit $return_value
fi
touch output_test_libm_logf/finished
exit 0
