#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--generate-interface=INFER --expose-globals --compiler=I386_CLANG6 --std=c99")
OUT_SUFFIX="bambu_axi_test"

# Needed to open values.txt file, relative path seems not to work
sed -i "s+__placeholder__+$script_dir/bambu_axi_test/values.txt+g" $script_dir/bambu_axi_test/mm_axi_tb.sv

. /opt/Xilinx/Vivado/$(ls /opt/Xilinx/Vivado | sort -t - | tail -1)/settings64.sh

mkdir out_$OUT_SUFFIX
while read line; do
  c_file=${line%% *}
  tcl_file=${c_file/.c/.tcl}
  mkdir out_$OUT_SUFFIX/$(basename $c_file .c)
  cd out_$OUT_SUFFIX/$(basename $c_file .c)
  bambu $script_dir/$(eval echo $line) $BATCH_ARGS
  mv HLS_output/simulation/values.txt $script_dir/bambu_axi_test/values.txt
  export OUT_DIR="$(pwd)"
  cd $script_dir
  echo "executing $tcl_file"
  vivado -mode tcl -source $script_dir/$tcl_file
  retval=$?
  if [ $retval != 0 ]; then
    echo "$script_dir/$tcl_file failed with error $retval"
    exit $retval
  fi
done < $script_dir/bambu_axi_test_list

exit 0
