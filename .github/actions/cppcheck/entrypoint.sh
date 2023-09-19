#!/bin/bash
set -e

workspace_dir="$PWD"
report_dir="$1"
shift

echo "Parallel jobs: $J"
mkdir -p "$report_dir"
cppcheck -j$J $@ src \
   --xml --xml-version=2 --output-file="$report_dir/cppcheck.xml" \
   -isrc/frontend_analysis/IR_analysis/lut_transformation.cpp \
   -Isrc \
   -Isrc/algorithms/clique_covering \
   -Isrc/algorithms/dominance \
   -Isrc/algorithms/graph_helpers \
   -Isrc/constants \
   -Isrc/graph \
   -Isrc/HLS/binding/interconnection \
   -Isrc/HLS/binding/module \
   -Isrc/HLS/binding/register \
   -Isrc/HLS/simulation \
   -Isrc/HLS/virtual_components\
   -Isrc/ilp \
   -Isrc/polixml \
   -Isrc/tree \
   -Isrc/utility

cppcheck-htmlreport --source-dir=. --title=Bambu --file="$report_dir/cppcheck.xml" --report-dir="$report_dir"

echo "error-count=$(grep -o -i severity=\"error\" $report_dir/cppcheck.xml | wc -l)" >> $GITHUB_OUTPUT
echo "report-dir=$report_dir" >> $GITHUB_OUTPUT
