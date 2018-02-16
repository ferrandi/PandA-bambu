#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANG5_O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG5" \
   --args="--configuration-name=CLANG5_O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG5" \
   --args="--configuration-name=CLANG5_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG5" \
   --args="--configuration-name=CLANG5_O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG5" \
   -o output_clang5_simple -b$(dirname $0) --table=output_clang5_simple.tex --name="Clang5RegressionSimple" $@
exit $?
