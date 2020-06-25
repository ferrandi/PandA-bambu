#!/bin/bash
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
timeout 2h bambu main.c --C-python-no-parse=sum.c sum.xml --simulate --testbench-extra-gcc-flags="-I /usr/include/i386-linux-gnu/python2.7/ -I /usr/include/python2.7/ -lpython2.7 "
