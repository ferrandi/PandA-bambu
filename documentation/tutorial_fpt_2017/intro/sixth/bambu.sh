#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "#simulation of qsort"
bambu --top-fname=test --generate-tb=../test.xml -Os --no-iob -v4 --simulate ../test.c ../less.c ../qsort.c -v2 --print-dot --pretty-print=a.c 2>&1 | tee qsort.log
cd ..



