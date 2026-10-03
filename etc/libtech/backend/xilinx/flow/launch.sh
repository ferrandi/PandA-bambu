#!/usr/bin/env bash
SWD="$(dirname $(readlink -e $0))"

source "${SWD}/settings64.sh"

vivado -mode batch -nojournal -nolog -source "${SWD}/vivado.tcl"
