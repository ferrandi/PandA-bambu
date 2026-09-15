#!/usr/bin/env bash
SWD="$(dirname $(readlink -e $0))"

if ! command -v nxpython 2>&1 > /dev/null; then
  NANOXPLORE_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/nxpython" 2> /dev/null | sort -t- -k2,2V -k1,1V | tail -n1) 2> /dev/null`
  if [ -z "${NANOXPLORE_BIN}" ]; then
    echo "Nanoxplore tool not found"
    exit -1
  fi
  NANOXPLORE_ROOT=`dirname ${NANOXPLORE_BIN}`
  echo "Nanoxplore path: ${NANOXPLORE_ROOT}"
fi

(PATH="${NANOXPLORE_BIN}:${PATH}" nxpython "${SWD}/synthesis.py")
