#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf search
mkdir -p search
cd search
echo "#simulation of search function"
bambu $root_dir/tree.c --top-fname=search \
   -DNDEBUG \
   -O3 --experimental-setup=BAMBU \
   --generate-tb=$root_dir/tree.c  --simulator=VERILATOR --simulate \
   --print-dot -v2 "$@" |& tee log.txt