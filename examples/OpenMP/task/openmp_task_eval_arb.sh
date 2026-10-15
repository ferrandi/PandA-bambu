#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1

BATCH_ARGS=("--std=c++17" "-lm" "-fopenmp" "-DUSE_AXI" "--compiler=I386_CLANG19" "--simulate" "--bambu-parameter=enable-task=1" "--generate-interface=INFER" "--max-sim-cycles=100000000" "--print-dot" "-s" "--pipelining=__float" "--hls-fpdiv=SRT4U" "--device-name=xcu280-2Lfsvh2892")
OUT_SUFFIX="openmp_task_eval_arb"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--memory-allocation-policy=NO_BRAM --bus-pipelined --clock-period=5 --mem-delay-write=32 --mem-delay-read=32 --tb-queue-size=32 ${BATCH_ARGS[*]}"\
   --args="--memory-allocation-policy=NO_BRAM --bus-pipelined --clock-period=4 --mem-delay-write=40 --mem-delay-read=40 --tb-queue-size=40 ${BATCH_ARGS[*]}"\
   -ltask_list_eval_arb \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
