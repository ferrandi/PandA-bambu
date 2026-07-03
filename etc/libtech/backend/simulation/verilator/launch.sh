#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

if command -v verilator 2>&1 > /dev/null; then
  VERILATOR_ROOT=`dirname $(dirname $(command -v verilator))`
else
   VERILATOR_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/verilator" 2> /dev/null | head -n1) 2> /dev/null`
   VERILATOR_ROOT=`dirname ${VERILATOR_BIN}`
fi
if [ -z "${VERILATOR_ROOT}" ]; then
   echo "Verilator executable not found"
   exit -1
fi
echo "Verilator root path: ${VERILATOR_ROOT}"

BEH_CC="$(bambu_results /application/sources@compiler)"
BEH_CFLAGS="-DVERILATOR -isystem ${VERILATOR_ROOT}/share/verilator/include/vltstd"
export BEH_CFLAGS

(PATH="${VERILATOR_BIN}:${PATH}" . ${SWD}/simulation_wrapper.sh "${SWD}/setup.sh" "$@")
