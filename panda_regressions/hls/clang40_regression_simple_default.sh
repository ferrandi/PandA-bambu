#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANG4_O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG4" \
   --args="--configuration-name=CLANG4_O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG4" \
   --args="--configuration-name=CLANG4_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG4" \
   --args="--configuration-name=CLANG4_O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_CLANG4" \
   -o output_clang4_simple -b$(dirname $0) --table=output_clang4_simple.tex --name="Clang4RegressionSimple" $@
exit $?
