#!/bin/bash
export PATH=/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "# integrating IP simulation"
bambu --print-dot ../top.c --generate-tb=../main_test.c --C-no-parse=../module1.c,../module2.c,../printer1.c,../printer2.c --file-input-data=../module1.v,../module2.v,../printer1.v,../printer2.v ../module_lib.xml -v4  --experimental-setup=BAMBU --top-fname=my_ip --simulate ../constraints_STD.xml --memory-allocation-policy=ALL_BRAM -O3  2>&1 | tee integration.log
cd ..

