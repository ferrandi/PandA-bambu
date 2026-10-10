#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("-lm" "-fopenmp" "--generate-interface=INFER" "--channels-type=MEM_ACC_NN" "--simulate" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "-DUSE_CACHE=1")
OUT_SUFFIX="banked_cache"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=banked_cache_64b_16d  -DCACHE_64b_16d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_64b_32d  -DCACHE_64b_32d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_64b_64d  -DCACHE_64b_64d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_128b_16d -DCACHE_128b_16d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_128b_32d -DCACHE_128b_32d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_128b_64d -DCACHE_128b_64d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_256b_16d -DCACHE_256b_16d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_256b_32d -DCACHE_256b_32d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_256b_64d -DCACHE_256b_64d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_512b_16d -DCACHE_512b_16d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_512b_32d -DCACHE_512b_32d=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_cache_512b_64d -DCACHE_512b_64d=1 ${BATCH_ARGS[*]}" \
   -lbanked_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
