#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC6_O0 -O0 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC6" \
   --args="--configuration-name=GCC6_O1 -O1 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC6" \
   --args="--configuration-name=GCC6_O2 -O2 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC6" \
   --args="--configuration-name=GCC6_O3 -O3 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC6" \
   -o output_gcc6_simple_do_not_expose_globals -b$(dirname $0) --table=output_gcc6_simple_do_not_expose_globals.tex --name="Gcc6RegressionSimpleDNEG" $@
exit $?
