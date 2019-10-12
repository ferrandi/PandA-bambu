#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/adpcm.c -O3 --simulate --speculative-sdc-scheduling -finline-limit=1000000
