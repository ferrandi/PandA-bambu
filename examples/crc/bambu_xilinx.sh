#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p icrc
cd icrc
echo "#synthesis of icrc"
bambu ../spec.c --top-fname=icrc --simulator=VERILATOR --simulate --generate-tb=../test_icrc.xml --experimental-setup=BAMBU --no-iob -v2 2>&1 | tee icrc.log
cd ..

mkdir -p main
cd main
echo "#synthesis of main"
bambu ../spec.c  --simulator=VERILATOR --simulate --generate-tb=../test.xml --experimental-setup=BAMBU --no-iob -v2 2>&1 | tee main.log


