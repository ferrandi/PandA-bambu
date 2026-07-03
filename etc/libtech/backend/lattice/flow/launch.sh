#!/usr/bin/env bash
SWD="$(dirname $(readlink -e $0))"

. "${SWD}/setup.sh"

(
  export TEMP="/tmp"
  export LSC_INI_PATH=""
  export LSC_DIAMOND="true"
  export TCL_LIBRARY="${LATTICE_ROOT}/tcltk/lib/tcl8.5"
  export FOUNDRY="${LATTICE_ROOT}/ispfpga"
  export PATH="${FOUNDRY}/${LATTICE_BIN/$LATTICE_ROOT/}:${LATTICE_BIN}:${PATH}"
  diamondc "${SWD}/lattice.tcl"
)
