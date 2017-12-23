#!/bin/bash
export PATH=/opt/panda/bin:$PATH
./autogen.sh
if test -d multi-keccak-build; then
   rm multi-keccak-build -rf
fi
mkdir -p multi-keccak-build
cd multi-keccak-build
../configure
relative_tree_panda_gcc=$(which tree-panda-gcc)
tree_panda_gcc=$(readlink -e $relative_tree_panda_gcc)
make CC=$tree_panda_gcc V=1
bambu --use-raw --top-fname=keccak_coproc -v4 --channels-type=MEM_ACC_11 --simulate --generate-tb=../test.xml --print-dot -v3 src/keccak  2>&1 | tee multi.log
cd ..

