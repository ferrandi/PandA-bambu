#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANG8_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG8" \
   --args="--configuration-name=CLANG8_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG8" \
   --args="--configuration-name=CLANG8_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG8" \
   --args="--configuration-name=CLANG8_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG8" \
   -o output_clang8_simple -b$(dirname $0) --table=output_clang8_simple.tex --name="Clang8RegressionSimple" $@
exit $?
