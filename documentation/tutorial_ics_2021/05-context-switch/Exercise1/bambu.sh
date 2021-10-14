#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
bambu --compiler=I386_GCC49 --std=c99 --experimental-set=BAMBU -O3 -v3 -fno-delete-null-pointer-checks --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 -DMAX_VERTEX_NUMBER=26455 -DMAX_EDGE_NUMBER=100573 -DN_THREADS=1  ${root_dir}/common/atominIncrement.c ${root_dir}/common/data.c -I${root_dir}/common/ ${root_dir}/trinityq4/lubm_trinityq4.c --simulator=VERILATOR

