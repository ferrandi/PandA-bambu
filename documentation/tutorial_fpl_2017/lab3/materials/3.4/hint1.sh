#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/mips.c --do-not-use-asynchronous-memories -fwhole-program --clock-period=15 --simulate -v3

