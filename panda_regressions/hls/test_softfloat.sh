#!/bin/bash
dir_script="$(dirname $(readlink -e $0))"

NUM_JOBS="${J:-1}"
echo "Parallel jobs: ${NUM_JOBS}"
make -C "${dir_script}/testfloat/berkeley-softfloat/build/bambu" clean
make -C "${dir_script}/testfloat/build/bambu" clean
make -j${NUM_JOBS} -C "${dir_script}/testfloat/berkeley-softfloat/build/bambu"
make -j${NUM_JOBS} -C "${dir_script}/testfloat/build/bambu"

FP_FUNCS=()
FP_FUNCS+=("-all2")
FP_FUNCS+=("ui32_to_f32" "ui64_to_f32" "i32_to_f32" "i64_to_f32")
FP_FUNCS+=("ui32_to_f64" "ui64_to_f64" "i32_to_f64" "i64_to_f64")
FP_FUNCS+=("f32_to_ui32_rx_minMag" "f32_to_ui64_rx_minMag" "f32_to_i32_rx_minMag" "f32_to_i64_rx_minMag")
FP_FUNCS+=("f64_to_ui32_rx_minMag" "f64_to_ui64_rx_minMag" "f64_to_i32_rx_minMag" "f64_to_i64_rx_minMag")

for func in ${FP_FUNCS[*]}
do
   ${dir_script}/testfloat/build/bambu/testfloat -rnear_even -level 2 ${func} -skipSubnormals
   return_value="$?"
   if test ${return_value} != 0; then
      echo "C based test of softfloat not passed."
      exit ${return_value}
   fi
done
