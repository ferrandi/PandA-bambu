#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
python3 $dir_script/../../test_panda.py --tool=bambu --bambu=bambu --spider=spider \
   --args="--configuration-name=GCC49 --compiler=I386_GCC49" \
   -c=--simulate -b$dir_script -l$dir_script/list "$@"
