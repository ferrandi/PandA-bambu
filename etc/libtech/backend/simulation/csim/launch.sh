#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CFLAGS="-DBAMBU_CSIM"
export BEH_CFLAGS

(. ${SWD}/simulation_wrapper.sh "${SWD}/setup.sh" "$@")
