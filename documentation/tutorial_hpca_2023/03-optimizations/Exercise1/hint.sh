#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/adpcm.c -O0 --simulate "$@" |& tee log.txt
