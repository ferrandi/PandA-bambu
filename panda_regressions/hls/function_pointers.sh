#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
python3 $script_dir/../../etc/scripts/test_panda.py -l function_pointers_list_bsearch --tool=bambu \
   --args="-v4 --configuration-name=default --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49 --top-fname=test --experimental-setup=BAMBU -O3 " \
   --args="-v4 --configuration-name=default-NN --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49 --top-fname=test --experimental-setup=BAMBU -O3 --channels-type=MEM_ACC_NN" \
   --args="-v4 --configuration-name=default-hl --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49 --top-fname=test --experimental-setup=BAMBU -O3 --bram-high-latency" \
   --args="-v4 --configuration-name=default-NN-hl --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49 --top-fname=test --experimental-setup=BAMBU -O3 --channels-type=MEM_ACC_NN --bram-high-latency" \
   -o output_function_pointers_bsearch -b$script_dir --table=function_pointers_bsearch_output.tex --name="FunctionPointerBSearch" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
python3 $script_dir/../../etc/scripts/test_panda.py -l function_pointers_list_qsort --tool=bambu \
   --args="-v4 --configuration-name=default --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49  -O3 -DNDEBUG --top-fname=test --experimental-setup=BAMBU" \
   --args="-v4 --configuration-name=default-NN --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49  -O3 -DNDEBUG --top-fname=test --experimental-setup=BAMBU --channels-type=MEM_ACC_NN" \
   --args="-v4 --configuration-name=default-hl --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49  -O3 -DNDEBUG --top-fname=test --experimental-setup=BAMBU --bram-high-latency" \
   --args="-v4 --configuration-name=default-NN-hl --evaluation=CYCLES,TOTAL_CYCLES --compiler=I386_GCC49  -O3 -DNDEBUG --top-fname=test --experimental-setup=BAMBU --channels-type=MEM_ACC_NN --bram-high-latency" \
   -o output_function_pointers_qsort -b$script_dir --table=function_pointers_qsort_output.tex --name="FunctionPointerQSort" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

