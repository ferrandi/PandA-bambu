#!/usr/bin/env bash
SWD=`dirname $(readlink -e $0)`
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CFLAGS="-DMODEL_TECH"
export BEH_CFLAGS

source "$SWD/tool_lookup.sh"
if ! bambu_modelsim_resolve; then
  echo "Siemens Modelsim not found"
  exit -1
fi
MENTOR_BIN=$BAMBU_MODELSIM_BIN
echo "Siemens Modelsim path: ${MENTOR_BIN}"

(PATH="${MENTOR_BIN}${PATH:+:$PATH}" . "${SWD}/simulation_wrapper.sh" "${SWD}/setup.sh" "$@")
