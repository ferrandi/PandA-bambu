#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf search
mkdir -p search
cd search
echo "#simulation of search function"
bambu $root_dir/tree.c --top-fname=main \
   -DNDEBUG -DBAMBU_PROFILING \
   -O3 --experimental-setup=BAMBU \
   --generate-tb=$root_dir/test_search.xml --simulator=ICARUS --simulate \
   --print-dot -v2 "$@" |& tee log.txt