#!/bin/bash
$(dirname $0)/gcc45_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc46_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc47_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc48_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc49_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc5_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc6_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc7_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc8_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang4_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang5_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang6_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang7_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang8_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang9_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang10_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang11_regression_simple_no_expose_globals.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc45_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc46_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc47_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc48_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc49_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc5_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc6_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc7_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc8_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang4_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang5_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang6_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang7_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang8_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang9_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang10_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang11_regression_simple_ext_pipelined.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc45_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc46_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc47_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc48_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc49_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc5_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc6_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc7_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc8_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang4_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang5_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang6_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang7_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang8_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang9_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang10_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/clang11_regression_simple_default.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_Zynq_VVD.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_Zynq.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_CII.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_CIIR.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_ECP3.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
$(dirname $0)/gcc_regression_simple_reset.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
