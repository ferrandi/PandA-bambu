#!/bin/bash
#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
current_dir=$(pwd)
cd $root_dir/multi-keccak
./autogen.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
if test -d multi-keccak-build; then
   rm multi-keccak-build -rf
fi
mkdir -p multi-keccak-build
cd multi-keccak-build
$root_dir/multi-keccak/configure
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
make V=1
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
timeout 2h bambu --simulator=MODELSIM --use-raw --top-fname=keccak_coproc --channels-type=MEM_ACC_11 --device-name=LFE335EA8FN484C src/keccak --simulate --generate-tb=$root_dir/multi-keccak/test.xml --print-dot "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
