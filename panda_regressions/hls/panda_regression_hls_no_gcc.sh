#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/libm-tests.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/libm-testsClang.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/libm-tests_single.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_experimental_setups.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_frontend.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_hlsdiv.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh -c="--bram-high-latency" -o output_CHStoneMemArch_high_latency_core1 --name="_BramHighLatency" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh -c="--bram-high-latency=4" -o output_CHStoneMemArch_high_latency_core14 --name="_BramHighLatency4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh -c="--reset-level=high" -o output_CHStoneMemArch_reset_high_core1 --name="_HighReset" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh -c="--reset-level=high" -c="--bram-high-latency" -o output_CHStoneMemArch_high_latency_reset_high_core1  --name="_BramHighLatency_HighLatency" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/CHStone_memarch_core1.sh -c="--reset-level=high" -c="--bram-high-latency=4" -o output_CHStoneMemArch_high_latency_reset_high_core14  --name="_BramHighLatency_HighLatency4" $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
if [ ! -d output_CHStoneMemArch_high_latency ]; then
   mkdir output_CHStoneMemArch_high_latency
fi
cd output_CHStoneMemArch_high_latency
$dir_script/CHStone_memarch.sh -c="--bram-high-latency" --name="_BramHighLatency" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d output_CHStoneMemArch_high_latency4 ]; then
   mkdir output_CHStoneMemArch_high_latency4
fi
cd output_CHStoneMemArch_high_latency4
$dir_script/CHStone_memarch.sh -c="--bram-high-latency=4" --name="_BramHighLatency4" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d output_CHStoneMemArch_reset_high ]; then
   mkdir output_CHStoneMemArch_reset_high
fi
cd output_CHStoneMemArch_reset_high
$dir_script/CHStone_memarch.sh -c="--reset-level=high" --name="_HighReset" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d output_CHStoneMemArch_high_latency_reset_high ]; then
   mkdir output_CHStoneMemArch_high_latency_reset_high
fi
cd output_CHStoneMemArch_high_latency_reset_high
$dir_script/CHStone_memarch.sh -c="--reset-level=high" -c="--bram-high-latency" --name="_BramHighLatency_HighLatency" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
cd ..
if [ ! -d output_CHStoneMemArch_high_latency_reset_high4 ]; then
   mkdir output_CHStoneMemArch_high_latency_reset_high4
fi
cd output_CHStoneMemArch_high_latency_reset_high4
$dir_script/CHStone_memarch.sh -c="--reset-level=high" -c="--bram-high-latency=4" --name="_BramHighLatency_HighLatency4" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
cd ..
$dir_script/CHStone_proxy.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test2.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test3.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test4.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test5.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/bambu_specific_test6.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/function_pointers.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/../../examples/file_simulate/all.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/../../examples/taste/taste.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
