#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
bambu -v2 -O2 $root_dir/module.c --generate-tb=$root_dir/test.xml --evaluation=CYCLES --simulator=VERILATOR --pretty-print=a.c --experimental-setup=BAMBU --target-file=$root_dir/xc7z045-2ffg900-VVD.xml

