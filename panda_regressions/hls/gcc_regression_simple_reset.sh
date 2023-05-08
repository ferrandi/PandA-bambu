#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu -c=--std=gnu89 \
   --args="--configuration-name=reset -lm --device-name=xc7z020,-1,clg484,VVD --simulate --experimental-setup=BAMBU --compiler=I386_GCC49 -O0 --reset-level=high" \
   -o output_reset_VVD -b$script_dir --table=output_reset_VVD.tex --name="GccRegressionResetZynqVVD" "$@"
exit $?
