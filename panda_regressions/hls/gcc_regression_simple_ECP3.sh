#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=11 -lm --device-name=LFE335EA8FN484C --simulate --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=11-bhl -lm --device-name=LFE335EA8FN484C --simulate --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=N1 -lm --device-name=LFE335EA8FN484C --simulate --channels-type=MEM_ACC_N1 --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=N1-bhl -lm --device-name=LFE335EA8FN484C --simulate --channels-type=MEM_ACC_N1 --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=NN -lm --device-name=LFE335EA8FN484C --channels-type=MEM_ACC_NN --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
   --args="--configuration-name=NN-bhl -lm --device-name=LFE335EA8FN484C --simulate --channels-type=MEM_ACC_NN --bram-high-latency --experimental-setup=BAMBU --compiler=I386_GCC49 -O0" \
  -o output_ECP3 -b$script_dir --table=output_ECP3.tex  --name="GccRegressionSimpleECP3" "$@"
exit $?
