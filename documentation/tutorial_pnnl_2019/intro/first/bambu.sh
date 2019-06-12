#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p icrc1
cd icrc1
echo "#synthesis of icrc1"
bambu ../module.c --top-fname=icrc1 --simulator=VERILATOR --simulate --generate-tb=../test_icrc1.xml -v2 --print-dot --pretty-print=a.c 2>&1 | tee icrc1.log
cd ..



