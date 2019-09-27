#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p none
cd none
bambu $dir_script/dfdiv.c --simulate --clock-period=15 --hls-div=none
cd ..
mkdir -p nr1
cd nr1
bambu $dir_script/dfdiv.c --simulate --clock-period=15 --hls-div=nr1
cd ..
mkdir -p nr2
cd nr2
bambu $dir_script/dfdiv.c --simulate --clock-period=15 --hls-div=nr2
cd ..
mkdir -p NR
cd NR
bambu $dir_script/dfdiv.c --simulate --clock-period=15 --hls-div=NR
cd ..
mkdir -p as
cd as
bambu $dir_script/dfdiv.c --simulate --clock-period=15 --hls-div=as
cd ..
