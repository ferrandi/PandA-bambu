#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py -ldiscrepancy_list --tool=bambu \
   --args="--configuration-name=GCC49_O0 -O0 -lm --simulate --pretty-print=output.c --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=GCC49_O1 -O1 -lm --simulate --pretty-print=output.c --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=GCC49_O2 -O2 -lm --simulate --pretty-print=output.c --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC49" \
   --args="--configuration-name=GCC49_O3 -O3 -lm --simulate --pretty-print=output.c --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_GCC49" \
   -o output_gcc49_regression_simple_no_expose_globals_pretty_print -b$(dirname $0) --table=gcc49_regression_simple_no_expose_globals_pretty_print.tex --name="Gcc49RegressionSimpleDNEGPP" $@
exit $?
