#!/bin/bash
./scalarize.sh
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
bambu ../02_vecmul_b.scalarized.ll -v4 --compiler=I386_CLANG6 --print-dot --top-fname=fused_multiply --do-not-expose-globals --pretty-print=a.c --no-iob --device-name=EP4SGX530KH40C2 --evaluation=PERIOD,AREA,FREQUENCY,CLOCK_SLACK,REGISTERS,DSPS,BRAMS
