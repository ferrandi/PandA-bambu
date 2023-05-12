#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
set -e

CXX=g++

${CXX} ${script_dir}/tb.cpp ${script_dir}/sum3numbers.cpp -O2 -m32 \
   -I${script_dir}/../../../etc/libbambu/ac_types/include -DEXPORT_TB -o tb_exporter
./tb_exporter

bambu ${script_dir}/sum3numbers.cpp --top-fname=sum3numbers \
   --generate-tb=test.sum3numbers.xml --simulate \
   --generate-interface=INFER --compiler=I386_CLANG12
