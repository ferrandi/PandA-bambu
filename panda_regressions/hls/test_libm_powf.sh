#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
if test -f output_test_libm_powf/finished; then
   exit 0
fi
if [[ -z "$J" ]]; then
   J="1"
fi
echo "Parallel jobs: $J"
rm -fr output_test_libm_powf
mkdir output_test_libm_powf
cd output_test_libm_powf
gcc -fopenmp -O3 -I$dir_script/../../etc/libbambu/ $dir_script/../../etc/libbambu/libm/wf_pow_opt.c -DCHECK_POW_FUNCTION -lm
export OMP_NUM_THREADS=$J
./a.out
return_value=$?
cd ..
if test $return_value != 0; then
   echo "C based test of softfloat pow not passed."
   exit $return_value
fi
touch output_test_libm_powf/finished
exit 0
