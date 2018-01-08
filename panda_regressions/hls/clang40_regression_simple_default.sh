#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANG40_O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG40" \
   --args="--configuration-name=CLANG40_O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG40" \
   --args="--configuration-name=CLANG40_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG40" \
   --args="--configuration-name=CLANG40_O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG40" \
   -o output_clang40_simple -b$(dirname $0) --table=output_clang40_simple.tex --name="Clang40RegressionSimple" $@
exit $?
