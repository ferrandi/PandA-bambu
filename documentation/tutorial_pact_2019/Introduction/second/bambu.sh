#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p pop
cd pop
echo "#synthesis of search function"
bambu ../module.c --top-fname=search --simulate -v2 -DNDEBUG --generate-tb=../module.c -v2 --print-dot --pretty-print=a.c 2>&1 | tee pop.log
cd ..



