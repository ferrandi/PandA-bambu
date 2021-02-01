#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py -lgcc_regression_simple_verilator --tool=bambu -c="--simulator=VERILATOR"\
   --args="--configuration-name=CLANG11_O0 -O0 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG11_O1 -O1 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG11_O2 -O2 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   --args="--configuration-name=CLANG11_O3 -O3 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANG11" \
   -o output_clang11_simple_do_not_expose_globals_verilator -b$(dirname $0) --table=output_clang11_simple_do_not_expose_globals_verilator.tex --name="Clang11RegressionSimpleDNEG" $@
exit $?
