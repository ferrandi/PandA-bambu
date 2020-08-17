#!/bin/bash
set -e
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:../../../../panda_dist/clangallrelease/bin:$PATH

BAMBU_OPTIONTS="--compiler=I386_GCC7 --no-iob -lm --softfloat-subnormal --speculative-sdc-scheduling --experimental-setup=BAMBU-PERFORMANCE-MP"

function ulp_test_compile_and_run {
   mkdir -p ulp_$1
   cd ulp_$1
   echo "Compiling $1 function with PandA Bambu ..."
   bambu $BAMBU_OPTIONTS $2 --pretty-print=$1.c &> bambu_log
   echo "$4" >> $1.c
   echo "Compiling environment for $1 ulp test ..."
   gcc $root_dir/ulp_test.c -c -fopenmp -O3 $3 -o ulp_test.o 
   gcc $1.c -c -O3 --include stdlib.h --include stdint.h --include math.h -o $1.o
   gcc ulp_test.o $1.o -fopenmp -O3 -lm -o $1
   echo "Running $1 ulp test ..."
   ./$1
   test_result=$?
   if test $test_result != 0; then
      exit $test_result
   fi
   echo "Test successful"
   cd ..
   rm -r ulp_$1
}

function ulp_func_test {
   ulp_test_compile_and_run $1_$5 "$root_dir/src/$2 --mask=@*$4" "-DFLOAT64_CONFIGURATION -DULP=$5 -DTEST_FUNC_NAME=$1 -DTEST_FUNC_EQ=$1_eq" "double $1_eq(double a){return $3;}"
}

function ulp_func_two_test {
   ulp_test_compile_and_run $1_$5 "$root_dir/src/$2 --mask=@*$4" "-DFLOAT64_CONFIGURATION -DULP=$5 -DTEST_FUNC_TWO_NAME=$1 -DTEST_FUNC_TWO_EQ=$1_eq" "double $1_eq(double a,double b){return $3;}"
}

function ulp_func_testf {
   ulp_test_compile_and_run $1_$5 "$root_dir/src/$2 --mask=@*$4" "-DFLOAT32_CONFIGURATION -DULP=$5 -DTEST_FUNC_NAME=$1 -DTEST_FUNC_EQ=$1_eq" "float $1_eq(float a){return $3;}"
}

function ulp_func_two_testf {
   ulp_test_compile_and_run $1_$5 "$root_dir/src/$2 --mask=@*$4" "-DFLOAT32_CONFIGURATION -DULP=$5 -DTEST_FUNC_TWO_NAME=$1 -DTEST_FUNC_TWO_EQ=$1_eq" "float $1_eq(float a,float b){return $3;}"
}

# Script start
if [ $# -lt 2 ] ; then
   echo "usage: ulp_test.sh <test_func> <significand_bitwidth>"
   echo "       ./ulp_test.sh f64exp  43"
   echo "       ./ulp_test.sh fsum    18"
   exit 1
fi

is_number='^[0-9]+$'
if ! [[ $2 =~ $is_number ]] ; then
   echo "error: Second argument should be an integer number" >&2; exit -1
fi

echo "Test required for function $1 with significand bitwidth of $2 bits"

case $1 in 
   f64sum   )  ulp_func_two_test "double_prec_addition" "f64sum.c" "a+b" "U*-1023*1024*$2" "$2"
               ;;
   fsum     )  ulp_func_two_testf "single_prec_addition" "fsum.c" "a+b" "U*-127*128*$2" "$2"
               ;;
   f64sub   )  ulp_func_two_test "double_prec_subtraction" "f64sub.c" "a-b" "U*-1023*1024*$2" "$2"
               ;;
   fsub     )  ulp_func_two_testf "single_prec_subtraction" "fsub.c" "a-b" "U*-127*128*$2" "$2"
               ;;
   f64div   )  ulp_func_two_test "double_prec_division" "f64div.c" "a/b" "U*-1023*1024*$2" "$2"
               ;;
   fdiv     )  ulp_func_two_testf "single_prec_division" "fdiv.c" "a/b" "U*-127*128*$2" "$2"
               ;;
   f64mul   )  ulp_func_two_test "double_prec_multiplication" "f64mul.c" "a*b" "U*-1023*1024*$2" "$2"
               ;;
   fmul     )  ulp_func_two_testf "single_prec_multiplication" "fmul.c" "a*b" "U*-127*128*$2" "$2"
               ;;
   f64pow   )  ulp_func_two_test "double_prec_pow" "f64pow.c" "pow(a,b)" "U*-1023*1024*$2" "$2"
               ;;
   fpow     )  ulp_func_two_testf "single_prec_pow" "fpow.c" "powf(a,b)" "U*-127*128*$2" "$2"
               ;;
   f64exp   )  ulp_func_test "double_prec_exp" "f64exp.c" "exp(a)" "U*-1023*1024*$2" "$2"
               ;;
   fexp     )  ulp_func_testf "single_prec_exp" "fexp.c" "expf(a)" "U*-127*128*$2" "$2"
               ;;
   f64sqrt  )  ulp_func_test "double_prec_sqrt" "f64sqrt.c" "sqrt(a)" "U*-1023*1024*$2" "$2"
               ;;
   fsqrt    )  ulp_func_testf "single_prec_sqrt" "fsqrt.c" "sqrtf(a)" "U*-127*128*$2" "$2"
               ;;
   *        )  echo "Requested operation not present"
               exit -2
               ;;
esac
