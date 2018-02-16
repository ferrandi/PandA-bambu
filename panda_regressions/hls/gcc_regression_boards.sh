#!/bin/bash
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
exit 0
