#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CC="$(bambu_results /application/sources@compiler)"
BEH_CFLAGS="-DVCS"
export BEH_CFLAGS

if ! command -v vcs 2>&1 > /dev/null; then
  VCS_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/vcs" 2> /dev/null | head -n1) 2> /dev/null`
  VCS_HOME=`dirname ${VCS_BIN}`
  echo "Synopsys VCS path: ${VSC_HOME}"
  if [ -z "${VCS_HOME}" ]; then
    echo "Synopsys VCS not found"
    exit -1
  fi
  export VSC_HOME
fi

(PATH="${VCS_BIN}:${PATH}" . ${SWD}/simulation_wrapper.sh "${SWD}/setup.sh" "$@")
