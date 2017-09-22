#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/module.c --device-name=xc4vlx100-10ff1513 --clock-period=15 --simulate

