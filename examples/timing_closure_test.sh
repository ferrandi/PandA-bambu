#!/bin/bash
ARGS="$@"
script=$(readlink -e $0)
root_dir=$(dirname $script)
$root_dir/softfloat/xc7z020-1clg484-VVD_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/softfloat/xc7z020-1clg484-VVD_05.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/xc7z020-1clg484-VVD_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/xc7z020-1clg484-VVD_06.6_OSF_sdc_no_proxy.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/xc7z020-1clg484-VVD_15.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/xc7z020-1clg484-VVD_10.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/softfloat/xc7vx690t-3ffg1930-VVD_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/softfloat/xc7vx690t-3ffg1930-VVD_02.5_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/xc7vx690t-3ffg1930-VVD_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/xc7vx690t-3ffg1930-VVD_05.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/xc7vx690t-3ffg1930-VVD_10.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/xc7vx690t-3ffg1930-VVD_05.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/softfloat/5CSEMA5F31C6_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/softfloat/5CSEMA5F31C6_06.6_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/5CSEMA5F31C6_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/5CSEMA5F31C6_07.5_OSF_sdc_no_proxy.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/5CSEMA5F31C6_15.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/5CSEMA5F31C6_10.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/softfloat/5SGXEA7N2F45C1_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/softfloat/5SGXEA7N2F45C1_03.3_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/5SGXEA7N2F45C1_10.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/libm/5SGXEA7N2F45C1_05.0_OSF_sdc.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/5SGXEA7N2F45C1_10.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$root_dir/CHStone/5SGXEA7N2F45C1_05.0_O2_sdc_aligned_access.sh $ARGS
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
