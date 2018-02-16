#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC5_O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC5" \
   --args="--configuration-name=GCC5_O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC5" \
   --args="--configuration-name=GCC5_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC5" \
   --args="--configuration-name=GCC5_O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC5" \
   -o output_gcc5_simple -b$(dirname $0) --table=output_gcc5_simple.tex --name="Gcc5RegressionSimple" $@
exit $?
