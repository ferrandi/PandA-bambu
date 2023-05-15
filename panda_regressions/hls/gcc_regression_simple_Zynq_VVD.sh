#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu -c=--std=gnu89 \
   --args="--configuration-name=11 -lm --device-name=xc7z020,-1,clg484,VVD --simulate --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=11-bhl -lm --device-name=xc7z020,-1,clg484,VVD --simulate --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=N1 -lm --device-name=xc7z020,-1,clg484,VVD --simulate --channels-type=MEM_ACC_N1 --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=N1-bhl -lm --device-name=xc7z020,-1,clg484,VVD --simulate --channels-type=MEM_ACC_N1 --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=NN -lm --device-name=xc7z020,-1,clg484,VVD --simulate --channels-type=MEM_ACC_NN --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=NN-bhl -lm --device-name=xc7z020,-1,clg484,VVD --simulate --channels-type=MEM_ACC_NN --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   -o output_Zynq_VVD -b$script_dir --table=output_Zynq_VVD.tex --name="GccRegressionSimpleZynqVVD" "$@"
exit $?
