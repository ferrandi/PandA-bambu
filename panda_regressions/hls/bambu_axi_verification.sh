#!/bin/bash
set -e
script_dir="$(dirname $(readlink -e $0))"
tcl_file="${script_dir}/bambu_axi_verification/axi_vip.tcl"
BATCH_ARGS=("--generate-interface=INFER" "--expose-globals" "--compiler=I386_CLANG6" "--std=c99" "--simulate" "--simulator=XSIM" "-m64")
OUT_SUFFIX="bambu_axi_verification"

. /opt/Xilinx/Vivado/$(ls /opt/Xilinx/Vivado | sort -t - | tail -1)/settings64.sh

while read line; do
  line="$(sed -E 's/([^#]*)#.*/\1/' <<< ${line})"
  if [ -z "${line}" ]; then
    continue
  fi
  benchmark_name="$(sed -n 's/.*--benchmark-name=\([^ ]*\).*/\1/p' <<< ${line})"
  top_fname="$(sed -n 's/.*--top-fname=\([^ ]*\).*/\1/p' <<< ${line})"
  line="$(sed "s/BENCHMARKS_ROOT/${script_dir//'/'/'\/'}/g" <<< ${line})"
  rm -rf out_${OUT_SUFFIX}/${benchmark_name}
  mkdir -p out_${OUT_SUFFIX}/${benchmark_name}
  cd out_${OUT_SUFFIX}/${benchmark_name}
  bambu ${script_dir}/${line} ${BATCH_ARGS[*]}
  export TOP_FNAME="${top_fname}"
  export OUT_DIR="$PWD"
  # Replace top-level module with Vivado IP integrator design
  sed -i "s/${top_fname} top/${top_fname}_VIP top/" HLS_output/simulation/bambu_testbench.v
  # Set 20 cycle reset pulse to fulfill Xilinx IP requirement of 16 cycles of the slowest AXI clock
  sed -i 's/RESET_CYCLES([0-9]*)/RESET_CYCLES(100)/' HLS_output/simulation/bambu_testbench.v
  echo "Launch verification for ${benchmark_name}"
  vivado -mode tcl -source ${tcl_file} -log vivado.log
  retval=$?
  if [ $retval != 0 ]; then
    echo "${benchmark_name} failed with error $retval"
    exit $retval
  fi
  if [ ! -z "$(cat vivado.log | grep -i '^ERROR:')" ]; then 
    echo "${benchmark_name} failed with following errors:"
    cat vivado.log | grep --color=never -i '^ERROR:'
    exit 1
  fi
done < ${script_dir}/bambu_axi_verification_list

exit 0
