#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CFLAGS="-DMODEL_TECH"
export BEH_CFLAGS

if ! command -v vsim 2>&1 > /dev/null; then
  MENTOR_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/vsim" 2> /dev/null | head -n1) 2> /dev/null`
  echo "Siemens Modelsim path: ${MENTOR_BIN}"
  if [ -z "${MENTOR_BIN}" ]; then
    echo "Siemens Modelsim not found"
    exit -1
  fi
fi

(PATH="${MENTOR_BIN}:${PATH}" . ${SWD}/simulation_wrapper.sh "${SWD}/setup.sh" "$@")
