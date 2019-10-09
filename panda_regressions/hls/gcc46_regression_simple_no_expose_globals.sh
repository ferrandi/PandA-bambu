#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC46_O0 -O0 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O1 -O1 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O2 -O2 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O3 -O3 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC46" \
   -o output_gcc46_simple_do_not_expose_globals -b$(dirname $0) --table=output_gcc46_simple_do_not_expose_globals.tex --name="Gcc46RegressionSimpleDNEG" $@
exit $?
