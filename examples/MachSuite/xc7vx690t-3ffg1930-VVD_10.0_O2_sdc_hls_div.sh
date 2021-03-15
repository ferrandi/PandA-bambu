#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
ARGS="--compiler=I386_GCC49 --experimental-setup=BAMBU-PERFORMANCE-MP --top-fname=main --top-rtldesign-name=run_benchmark -DBAMBU_PROFILING --evaluation --no-iob -mx32 -fno-tree-vectorize --do-not-expose-globals --speculative-sdc-scheduling --hls-div --device-name=xc7vx690t,-3,ffg1930,VVD"
$root_dir/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu -l$root_dir/machsuite_bench_list --args="$ARGS"   --benchmarks_root=$root_dir -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@ -t720m
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
