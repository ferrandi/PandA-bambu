#!/bin/bash
bambu minmax.c --generate-tb=testbench.xml --simulate "$@" |& tee log.txt
