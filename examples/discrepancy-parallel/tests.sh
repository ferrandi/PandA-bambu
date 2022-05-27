#!/bin/bash
# Add --no-clean to measure the file sizes.
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
	--args="--configuration-name=baseline --simulate --std=c11 --compiler=I386_GCC6 --disable-function-proxy " \
	--args="--configuration-name=vcd --simulate --generate-vcd --std=c11 --compiler=I386_GCC6 --disable-function-proxy " \
	--args="--configuration-name=discrepancy --simulate --std=c11 --discrepancy --discrepancy-no-load-pointers --discrepancy-force-uninitialized --compiler=I386_GCC6 --disable-function-proxy" \
	-c=--simulator=MODELSIM \
	-ltest_list --ulimit="-f 10000000 -v 15000000 -s 16384" -t 120m -j2 -o output_dir -b$script_dir --table=results.tex "$@"
return_value=$?
if test $return_value != 0; then
	exit $return_value
fi
exit 0
