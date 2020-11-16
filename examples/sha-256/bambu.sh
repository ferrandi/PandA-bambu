#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:$root_dir/../../../../bin/:/opt/panda/bin:$PATH

mkdir -p bambu_test_sha
cd bambu_test_sha
timeout 2h bambu -v5 --print-dot $root_dir/sha-256.c --generate-tb=$root_dir/test.xml --no-iob --simulate -s --top-fname=calc_sha_256
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
