#! /bin/bash
`dirname $0`/../../etc/scripts/test_panda.py gcc_regression_simple \
	--args="-lm --device-name=xc7z020,-1,clg484,VVD --configuration-name=base --simulate -v2 --experimental-setup=BAMBU --generate-interface=WB4 -O0"  \
	--args="-lm --device-name=xc7z020,-1,clg484,VVD --configuration-name=hl --simulate -v2 --experimental-setup=BAMBU --bram-high-latency --generate-interface=WB4 -O0"  \
	--args="-lm --device-name=xc7z020,-1,clg484,VVD --configuration-name=N1 --simulate -v2 --experimental-setup=BAMBU --channels-type=MEM_ACC_N1 --generate-interface=WB4 -O0" \
	--args="-lm --device-name=xc7z020,-1,clg484,VVD --configuration-name=N1hl --simulate -v2 --experimental-setup=BAMBU --channels-type=MEM_ACC_N1 --bram-high-latency --generate-interface=WB4 -O0" \
	-o output_Zynq_VVDWB4 -b`dirname $0` --table=output_Zynq_VVD_WB4.tex --name="GccRegressionSimpleZynqVVDWB4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
