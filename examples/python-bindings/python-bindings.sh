#!/bin/bash
timeout 2h bambu main.c --simulator=MODELSIM --C-python-no-parse=sum.c sum.xml --simulate --testbench-extra-gcc-flags="-I /usr/include/i386-linux-gnu/python2.7/ -I /usr/include/python2.7/ -lpython2.7 "
