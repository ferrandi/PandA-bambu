#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CC="$(bambu_results /application/sources@compiler)"
BEH_CFLAGS="-DXILINX_SIMULATOR"
export BEH_CFLAGS

source "${SWD}/settings64.sh"

(. ${SWD}/simulation_wrapper.sh "${SWD}/setup.sh" "$@")
