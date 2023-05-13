#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ac_include_dir="${script_dir}/../ac_types/include"
tests_dir="${script_dir}/tests"

out_dir="$1"
shift

mkdir -p ${out_dir}
\time make --directory=${out_dir} -f ${script_dir}/tests/Makefile all AC_TYPES_INC=${ac_include_dir} TESTS_DIR=${tests_dir} "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
