#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--clock-period=5" "--compiler=I386_CLANG13")
OUT_SUFFIX="pb_openroad"
OPENROAD_ARGS=()

append_openroad_configuration() {
   case "$1" in
      nangate45)
         OPENROAD_ARGS+=("--args=--configuration-name=nangate45 --device-name=nangate45 BENCHMARK_ENV=DIE_AREA='0 0 2020 1980',CORE_AREA='10 12 2010 1971' ${BATCH_ARGS[*]}")
         ;;
      asap7-TC)
         OPENROAD_ARGS+=("--args=--configuration-name=asap7-TC --device-name=asap7-TC BENCHMARK_ENV=DIE_AREA='0 0 100 100',CORE_AREA='5.08 5.08 99 99' ${BATCH_ARGS[*]}")
         ;;
      *)
         echo "Unsupported OPENROAD_CONFIGURATION: $1" >&2
         exit 1
         ;;
   esac
}

if [[ -n "${OPENROAD_CONFIGURATION:-}" ]]; then
   OUT_SUFFIX+="_${OPENROAD_CONFIGURATION}"
   append_openroad_configuration "${OPENROAD_CONFIGURATION}"
else
   append_openroad_configuration "nangate45"
   append_openroad_configuration "asap7-TC"
fi

OPENROAD_BENCH_LIST="${OPENROAD_BENCH_LIST:-panda_bench_openroad_list}"

python3 $script_dir/../etc/scripts/mantis.py --tool=bambu \
   "${OPENROAD_ARGS[@]}" \
   -l"${OPENROAD_BENCH_LIST}" \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
