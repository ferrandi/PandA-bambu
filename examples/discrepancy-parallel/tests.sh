#!/bin/bash
# Add --no-clean to measure the file sizes.
$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
	--args="-v3 --configuration-name=baseline --simulate --std=c11 --compiler=I386_GCC6 --disable-function-proxy " \
	--args="-v3 --configuration-name=vcd --simulate --generate-vcd --std=c11 --compiler=I386_GCC6 --disable-function-proxy " \
	--args="-v3 --configuration-name=discrepancy --simulate --std=c11 --discrepancy --discrepancy-no-load-pointers --discrepancy-force-uninitialized --compiler=I386_GCC6 --disable-function-proxy" \
	-ltest_list --ulimit="-f 10000000 -v 15000000 -s 16384" -t 120m -j2 -o output_dir -b$(dirname $0) --table=results.tex $@
return_value=$?
if test $return_value != 0; then
	exit $return_value
fi
exit 0
