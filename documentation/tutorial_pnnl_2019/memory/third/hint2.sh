#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/mips.c --bram-high-latency=4 --channels-type=MEM_ACC_NN --clock-period=15 --simulate -v3

