#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p ludecomp
cd ludecomp
echo "#synthesis of fun"
bambu ../module.c --top-fname=fun --simulate -v2 --simulator=VERILATOR --generate-tb=../test.xml -v2 --print-dot --pretty-print=a.c 2>&1 | tee ludecomp.log
cd ..


