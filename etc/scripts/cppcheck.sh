#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
cppcheck --enable=all --force --suppress=unusedFunction --suppress=unmatchedSuppression --inline-suppr ${dir_script}/../../src \
   -i${dir_script}/../../src/frontend_analysis/IR_analysis/lut_transformation.cpp \
   -I${dir_script}/../../src \
   -I${dir_script}/../../src/algorithms/clique_covering \
   -I${dir_script}/../../src/algorithms/dominance \
   -I${dir_script}/../../src/algorithms/graph_helpers \
   -I${dir_script}/../../src/constants \
   -I${dir_script}/../../src/graph \
   -I${dir_script}/../../src/HLS/binding/interconnection \
   -I${dir_script}/../../src/HLS/binding/module \
   -I${dir_script}/../../src/HLS/binding/register \
   -I${dir_script}/../../src/HLS/simulation \
   -I${dir_script}/../../src/HLS/virtual_components\
   -I${dir_script}/../../src/ilp \
   -I${dir_script}/../../src/polixml \
   -I${dir_script}/../../src/tree \
   -I${dir_script}/../../src/utility \
   "$@"
