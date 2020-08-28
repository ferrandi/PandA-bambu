#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/EP2C70F896C6-R_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/EP2C70F896C6-R_15.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_15.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_05.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx330t-1ffg1157_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_05.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_10.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/LFE335EA8FN484C_15.0_O2_sdc_aligned_access.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/EP2C70F896C6-R_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/EP2C70F896C6-R_15.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_15.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_05.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx330t-1ffg1157_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_05.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_10.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/LFE335EA8FN484C_15.0_O3_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessClang4.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessClang4_CSROA.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessClang5.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessClang6.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessClang7.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessGCC5.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessGCC6.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessGCC7.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_accessGCC8.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0


