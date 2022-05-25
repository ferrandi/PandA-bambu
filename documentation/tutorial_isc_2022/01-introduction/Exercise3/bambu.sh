#!/bin/bash
bambu matmul.ll --top-fname=main_kernel --generate-tb=test.xml --simulate --simulator=VERILATOR --compiler=I386_CLANG12 "$@" |& tee log.txt
