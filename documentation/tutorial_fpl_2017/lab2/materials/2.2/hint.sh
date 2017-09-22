#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/module.c --generate-tb=$dir_script/hint.xml --simulate -v4
