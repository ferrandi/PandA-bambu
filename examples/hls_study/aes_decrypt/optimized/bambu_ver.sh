#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "#synthesis"
timeout 2h bambu -v3 --print-dot ../aes.c --evaluation --generate-tb=../decrypt.xml --no-iob --device-name=xc7vx690t,-3,ffg1930,VVD --clock-period=5.3 --top-fname=main --top-rtldesign-name=decrypt -ftree-vectorize -finline-limit=10000 --simulator=VERILATOR 2>&1 | tee aes_decrypt.log
cd ..


