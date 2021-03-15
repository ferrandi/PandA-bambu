#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC7_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7" \
   --args="--configuration-name=GCC7_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7" \
   --args="--configuration-name=GCC7_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7" \
   --args="--configuration-name=GCC7_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7" \
   -o output_gcc7_simple -b$(dirname $0) --table=output_gcc7_simple.tex --name="Gcc7RegressionSimple" $@
exit $?
