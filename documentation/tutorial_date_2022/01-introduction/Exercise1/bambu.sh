#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf  icrc1
mkdir -p icrc1
cd icrc1
echo "#synthesis of icrc1"
bambu ../icrc.c --top-fname=icrc1 \
   --generate-tb=../test_icrc1.xml --simulator=VERILATOR --simulate \
   -v2 --print-dot --pretty-print=a.c "$@" |& tee icrc1.log