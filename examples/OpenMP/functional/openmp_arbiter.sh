#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1

BATCH_ARGS=("-lm" "-fopenmp" "--simulate")
OUT_SUFFIX="openmp_arbiter"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=A_P --memory-allocation-policy=NO_BRAM --bus-pipelined ${BATCH_ARGS[*]}"\
   --args="--configuration-name=A_RP --memory-allocation-policy=GLSS --bus-arbiter-type=RP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=A_LP --memory-allocation-policy=GLSS --bus-arbiter-type=LP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=A_P_RP --memory-allocation-policy=NO_BRAM --bus-pipelined --bus-arbiter-type=RP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=A_P_LP --memory-allocation-policy=NO_BRAM --bus-pipelined --bus-arbiter-type=LP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC --memory-allocation-policy=GLSS --bus-architecture=GROUPED_CHANNEL ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC_P --memory-allocation-policy=NO_BRAM --bus-architecture=GROUPED_CHANNEL ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC_RP --memory-allocation-policy=GLSS --bus-architecture=GROUPED_CHANNEL --bus-arbiter-type=RP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC_LP --memory-allocation-policy=GLSS --bus-architecture=GROUPED_CHANNEL --bus-arbiter-type=LP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC_P_RP --memory-allocation-policy=NO_BRAM --bus-architecture=GROUPED_CHANNEL --bus-arbiter-type=RP ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GC_P_LP --memory-allocation-policy=NO_BRAM --bus-architecture=GROUPED_CHANNEL --bus-arbiter-type=LP ${BATCH_ARGS[*]}"\
   -larbiter_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
