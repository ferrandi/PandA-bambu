#!/usr/bin/env bash
SWD=$(dirname "$(readlink -e -- "$0")")
OUT_LVL="$(bambu_results /application@verbosity)"

BEH_CFLAGS="-DMODEL_TECH"
export BEH_CFLAGS

source "$SWD/tool_lookup.sh"
if ! bambu_modelsim_resolve; then
  echo "Siemens Modelsim not found"
  exit -1
fi
MENTOR_BIN=$BAMBU_MODELSIM_BIN
if [[ $BAMBU_MODELSIM_SOURCE == BAMBU_HLS_BACKEND_PATH ]]; then
  echo "Siemens Modelsim path: ${MENTOR_BIN}"
fi

(PATH="${MENTOR_BIN}:${PATH:-}" . "$SWD/simulation_wrapper.sh" "$SWD/setup.sh" "$@")
