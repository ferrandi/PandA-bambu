#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--simulate" "-O3" "-fwhole-program" "--experimental-setup=BAMBU" "--clock-period=15" "-D'printf(fmt, ...)='")
OUT_SUFFIX="${COMPILER}_CHStone-memarch"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER}-O3-wp-11-D00-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D00-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D00-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D10-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D10-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D10-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D11-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D11-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D11-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D21-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D21-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D21-EXT --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D00-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D00-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D00-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D10-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D10-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D10-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D10 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D11-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D11-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D11-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-11-D21-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-N1-D21-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_N1 --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-NN-D21-ALL --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D21 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-VVD-NN-D00-LSS --memory-allocation-policy=LSS --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-VVD-NN-D00-GSS --memory-allocation-policy=GSS --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${COMPILER}-O3-wp-VVD-NN-D00-NO-BRAM --memory-allocation-policy=NO_BRAM --channels-type=MEM_ACC_NN --memory-ctrl-type=D00 ${BATCH_ARGS[*]}"\
   -lCHStone_memarch_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
