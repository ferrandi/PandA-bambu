#!/bin/bash
`dirname $0`/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC46_O0 -O0 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O1 -O1 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O2 -O2 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC46" \
   --args="--configuration-name=GCC46_O3 -O3 -lm --simulate --experimental-setup=BAMBU --compiler=I386_GCC46" \
   -o output_gcc46_simple -b`dirname $0` --table=output_gcc46_simple.tex --name="Gcc46RegressionSimple" $@
exit $?
