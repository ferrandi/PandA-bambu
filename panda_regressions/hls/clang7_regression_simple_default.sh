#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANG7_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG7" \
   --args="--configuration-name=CLANG7_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG7" \
   --args="--configuration-name=CLANG7_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG7" \
   --args="--configuration-name=CLANG7_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANG7" \
   -o output_clang7_simple -b$(dirname $0) --table=output_clang7_simple.tex --name="Clang7RegressionSimple" $@
exit $?
