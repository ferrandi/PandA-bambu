#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/EP2C70F896C6-R_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_06.6_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5CSEMA5F31C6_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_03.3_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/5SGXEA7N2F45C1_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx330t-1ffg1157_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_02.5_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7vx690t-3ffg1930-VVD_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_05.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/xc7z020-1clg484-VVD_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$dir_script/LFE335EA8FN484C_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
#$dir_script/EP2C70F896C6-R_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/5CSEMA5F31C6_06.6_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/5CSEMA5F31C6_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/5SGXEA7N2F45C1_03.3_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/5SGXEA7N2F45C1_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/xc7vx330t-1ffg1157_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/xc7vx690t-3ffg1930-VVD_02.5_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/xc7vx690t-3ffg1930-VVD_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/xc7z020-1clg484-VVD_05.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/xc7z020-1clg484-VVD_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/LFE335EA8FN484C_10.0_OSF_sdc_flopoco.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
exit 0
