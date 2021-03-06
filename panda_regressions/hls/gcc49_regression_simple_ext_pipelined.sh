#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu -c="--memory-allocation-policy=EXT_PIPELINED_BRAM"\
   --args="--configuration-name=GCC49_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49 --channels-type=MEM_ACC_NN" \
   --args="--configuration-name=GCC49_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49 --channels-type=MEM_ACC_NN" \
   --args="--configuration-name=GCC49_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49 --channels-type=MEM_ACC_NN" \
   --args="--configuration-name=GCC49_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC49 --channels-type=MEM_ACC_NN" \
   -o output_gcc49_simple_ext_pipelined -b$(dirname $0) --table=output_gcc49_simple_ext_pipelined.tex --name="Gcc49RegressionSimple-ExtPipelined" $@
exit $?
