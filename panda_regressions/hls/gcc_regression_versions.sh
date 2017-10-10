#!/bin/bash
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
$(dirname $0)/gcc6_regression_simple_ext_pipelined.sh $@
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
exit 0
