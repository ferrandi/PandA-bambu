#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("-lm" "-fopenmp" "--generate-interface=INFER" "--mem-delay-read=32" "--mem-delay-write=32" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" )
OUT_SUFFIX="${compiler}_gapbs"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=1co_1cs    -DNUM_ACCELS=1   --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_1cs    -DNUM_ACCELS=2   --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_1cs    -DNUM_ACCELS=4   --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_1cs    -DNUM_ACCELS=8   --channels-number=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_1cs   -DNUM_ACCELS=16  --channels-number=16  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_1cs   -DNUM_ACCELS=32  --channels-number=32  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_2cs    -DNUM_ACCELS=2   --channels-number=1  --context_switch=2 --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_2cs    -DNUM_ACCELS=4   --channels-number=2  --context_switch=2 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_2cs    -DNUM_ACCELS=8   --channels-number=4  --context_switch=2 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_2cs    -DNUM_ACCELS=16   --channels-number=8 --context_switch=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_2cs   -DNUM_ACCELS=32  --channels-number=16 --context_switch=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_2cs   -DNUM_ACCELS=64  --channels-number=32 --context_switch=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_4cs    -DNUM_ACCELS=4   --channels-number=1  --context_switch=4 --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_4cs    -DNUM_ACCELS=8   --channels-number=2  --context_switch=4 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_4cs    -DNUM_ACCELS=16   --channels-number=4  --context_switch=4 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_4cs    -DNUM_ACCELS=32   --channels-number=8 --context_switch=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_4cs   -DNUM_ACCELS=64  --channels-number=16 --context_switch=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_4cs   -DNUM_ACCELS=128  --channels-number=32 --context_switch=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_8cs    -DNUM_ACCELS=8   --channels-number=1  --context_switch=8 --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_8cs    -DNUM_ACCELS=16   --channels-number=2  --context_switch=8 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_8cs    -DNUM_ACCELS=32   --channels-number=4  --context_switch=8 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_8cs    -DNUM_ACCELS=64   --channels-number=8 --context_switch=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_8cs   -DNUM_ACCELS=128  --channels-number=16 --context_switch=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_8cs   -DNUM_ACCELS=256  --channels-number=32 --context_switch=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   -lllist \
   --timeout=300m \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
exit $?
   