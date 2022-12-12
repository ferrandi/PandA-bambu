#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
bambu ${root_dir}/histogram.c \
   --compiler=I386_GCC49 --experimental-setup=BAMBU-BALANCED-MP \
   --device-name=5SGXEA7N2F45C1 --clock-period=10 \
   -fwhole-program -fno-delete-null-pointer-checks -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000 \
   --simulate \
   "$@" |& tee log.txt
