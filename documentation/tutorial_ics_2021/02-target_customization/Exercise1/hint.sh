#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)

bambu $dir_script/hint.c "$@" |& tee log.txt
