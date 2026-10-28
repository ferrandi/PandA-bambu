#!/usr/bin/env bash

if ! command -v diamondc 2>&1 > /dev/null; then
  LATTICE_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/lin/diamondc" 2> /dev/null | sort -V | tail -n1) 2> /dev/null`
  if [ -z "${LATTICE_BIN}" ]; then
    LATTICE_BIN=`dirname $(find -L ${BAMBU_HLS_BACKEND_PATH//:/ } \( -type f -o -type l \) -path "*/bin/lin64/diamondc" 2> /dev/null | sort -V | tail -n1) 2> /dev/null`
    if [ -z "${LATTICE_BIN}" ]; then
      echo "Lattice Diamond tool not found"
      exit -1
    fi
  fi
  LATTICE_ROOT=`readlink -e ${LATTICE_BIN}/../..`
  echo "Lattice Diamond root: ${LATTICE_ROOT}"
fi
