#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
mkdir -p Gcc
cd Gcc
\time make -f $root_dir/tests/Makefile all AC_TYPES_INC=$root_dir/../ac_types/include CXX=g++ TESTS_DIR=$root_dir/tests
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0

