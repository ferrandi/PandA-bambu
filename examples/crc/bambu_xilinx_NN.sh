#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p icrc
cd icrc
echo "#synthesis of icrc"
timeout 2h bambu ../spec.c --top-fname=icrc --simulator=VERILATOR --simulate --generate-tb=../test_icrc.xml --channels-type=MEM_ACC_NN --experimental-setup=BAMBU --no-iob -v2 2>&1 | tee icrc.log
cd ..

mkdir -p main
cd main
echo "#synthesis of main"
timeout 2h bambu ../spec.c  --simulator=VERILATOR --simulate --generate-tb=../test.xml --channels-type=MEM_ACC_NN --experimental-setup=BAMBU --no-iob -v2 2>&1 | tee main.log


