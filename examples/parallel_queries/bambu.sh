#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p parallel_queries_sim
cd parallel_queries_sim

#!/bin/bash
timeout 2h bambu --compiler=I386_GCC49 --std=c99 --experimental-set=BAMBU -O3 -v3 -fno-delete-null-pointer-checks -fopenmp --pragma-parse --mem-delay-read=20 --mem-delay-write=20 --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM --no-iob --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 -DMAX_VERTEX_NUMBER=26455 -DMAX_EDGE_NUMBER=100573 --configuration-name=02W-04CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=4 ${root_dir}/common/atominIncrement.c ${root_dir}/common/data.c -I${root_dir}/common/ --benchmark-name=tq1-1DB  --generate-tb=${root_dir}/trinityq3/test-1.xml  ${root_dir}/trinityq3/lubm_trinityq3.c --top-fname=search --simulate
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

