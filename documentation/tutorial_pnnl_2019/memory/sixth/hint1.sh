#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/mips.c --base-address=1024 --memory-allocation-policy=LSS --clock-period=15 --simulate -v3

