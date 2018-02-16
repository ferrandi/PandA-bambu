#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p Virtex4
cd Virtex4
bambu $dir_script/module.c --device-name=xc4vlx100-10ff1513 --clock-period=15 --simulate
cd ..
mkdir -p StratixV
cd StratixV
bambu $dir_script/module.c --device-name=5SGXEA7N2F45C1 --clock-period=5 --simulate
cd ..
mkdir -p Virtex7_100
cd Virtex7_100
bambu $dir_script/module.c --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 --simulate
cd ..
mkdir -p Virtex7_333
cd Virtex7_333
bambu $dir_script/module.c --device-name=xc7vx690t-3ffg1930-VVD --clock-period=3.3 --simulate
cd ..
mkdir -p Virtex7_400
cd Virtex7_400
bambu $dir_script/module.c --device-name=xc7vx690t-3ffg1930-VVD --clock-period=2.5 --simulate
cd ..
