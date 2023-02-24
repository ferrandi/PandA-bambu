#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf hls
mkdir hls
cd hls
echo "#simulation of qsort"
bambu $root_dir/test.c $root_dir/less.c $root_dir/qsort.c --top-fname=test \
   -Os --no-iob \
   --generate-tb=$root_dir/test.xml --simulate \
   -v2 --print-dot --pretty-print=a.c "$@" |& tee log.txt
