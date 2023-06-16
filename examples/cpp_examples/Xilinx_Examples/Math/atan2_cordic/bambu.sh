#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)
shared_flags="$root_dir/cordic_atan2.cpp"
shared_flags+="--top-fname=top_atan2"
shared_flags+="--generate-tb=$root_dir/cordic_test.cpp"
shared_flags+="-DTEST_COUNT=200"
shared_flags+="-DCUSTOM_VERIFICATION"
shared_flags+="--file-input-data=$root_dir/input_data.txt,$root_dir/ref_results.txt"
shared_flags+="--compiler=I386_CLANG6"
shared_flags+="--generate-interface=INFER"
shared_flags+="--evaluation"
shared_flags+="--simulator=MODELSIM"
shared_flags+="--device-name=xc7vx690t-3ffg1930-VVD"
shared_flags+="--clock-period=2.5"
shared_flags+="--print-dot"

mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#synthesis and simulation CORDIC"
timeout 2h bambu ${shared_flags} -DDB_CORDIC -DBIT_ACCURATE "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir

mkdir -p $basename_dir/hls2
cd $basename_dir/hls2
echo "#synthesis and simulation CORDIC"
timeout 2h bambu ${shared_flags} -DDB_CORDIC "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir

mkdir -p $basename_dir/hls3
cd $basename_dir/hls3
echo "#synthesis and simulation + single precision"
timeout 2h bambu ${shared_flags} -DDB_SINGLE_PRECISION -lm "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir

mkdir -p $basename_dir/hls4
cd $basename_dir/hls4
echo "#synthesis and simulation double precision"
timeout 2h bambu ${shared_flags} -DDB_DOUBLE_PRECISION -lm "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
