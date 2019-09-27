#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p O0
cd O0
bambu $dir_script/adpcm.c -O0 --simulate
cd ..
mkdir -p O1
cd O1
bambu $dir_script/adpcm.c -O1 --simulate
cd ..
mkdir -p O2
cd O2
bambu $dir_script/adpcm.c -O2 --simulate
cd ..
mkdir -p O3
cd O3
bambu $dir_script/adpcm.c -O3 --simulate
cd ..
mkdir -p O3inline
cd O3inline
bambu $dir_script/adpcm.c -O3 --simulate -finline-limit=1000000
cd ..
mkdir -p O3vectorize
cd O3vectorize
bambu $dir_script/adpcm.c -O3 --simulate -ftree-vectorize
cd ..
