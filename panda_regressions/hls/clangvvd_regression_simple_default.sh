#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANGVVD_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_CLANGVVD" \
   -o output_clangvvd_simple -b$(dirname $0) --table=output_clangvvd_simple.tex --name="ClangVVDRegressionSimple" $@
exit $?
