#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/mips.c --distram-threshold=1024 -fwhole-program --clock-period=15 --simulate -v3

