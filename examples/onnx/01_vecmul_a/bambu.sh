#!/bin/bash
./scalarize.sh
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
/opt/panda/bin/bambu ../01_vecmul_a.scalarized.ll -v4 --compiler=I386_CLANG6 --print-dot --top-fname=fused_multiply --do-not-expose-globals --pretty-print=a.c --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS
