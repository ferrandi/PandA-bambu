#!/bin/bash
# Mantis per-case tool: synthesize, assert the issue #434 schedule, then
# exercise both required output backpressure patterns with Questa/ModelSim.
set -u
script_dir="$(dirname "$(readlink -e "$0")")"
regression_dir="$script_dir"
case_dir="$PWD"
mkdir -p "${TMPDIR:-$script_dir/../../../documentation/tmp}"
export TMPDIR="${TMPDIR:-$script_dir/../../../documentation/tmp}" TMP="${TMP:-$TMPDIR}" TEMP="${TEMP:-$TMPDIR}"

fail() { echo "FAIL: $*" >&2; exit 1; }
timeout 900 bambu "$@" >bambu.log 2>&1
status=$?
[ "$status" -eq 0 ] || fail "bambu exited with status $status; see bambu.log"

grep -q 'II=1 solution found for loop (BB2)' bambu.log || fail 'missing BB2 II=1 scheduling summary'
grep -qE 'Initiation Interval \(II\) for function [^ ]*:BB2 = 1$' bambu.log || fail 'missing BB2 II=1 metric'
grep -qE "Iteration latency for function [^ ]*:BB2 = ${LP_LATENCY:-2}$" bambu.log || fail 'missing expected BB2 iteration latency'
if grep -q 'Loop pipelining not possible' bambu.log; then fail 'loop pipelining reported impossible'; fi

tb="$regression_dir/${LP_TB:?missing LP_TB}"
[ -s "$tb" ] || fail "missing testbench: $tb"
rtl=("$case_dir"/*.v)
[ -e "${rtl[0]}" ] || fail 'missing generated RTL'
selected_tb="$(awk -v top="$LP_TB_TOP" '
   $1 == "module" { in_top = ($2 ~ ("^" top "([;(]|$)")) }
   in_top { print }
   in_top && $1 == "endmodule" { found = 1; exit }
   END { if (!found) exit 1 }
' "$tb")" || fail "testbench has no module $LP_TB_TOP"
duts="$(printf '%s\n' "$selected_tb" | sed -n 's/^[[:space:]]*\([A-Za-z_][A-Za-z0-9_]*\)[[:space:]]\+dut[[:space:]]*(.*/\1/p')"
[ -n "$duts" ] || fail "testbench module $LP_TB_TOP contains no DUT instance"
for dut in $duts; do grep -q "^module ${dut}(" "${rtl[@]}" || fail "generated RTL lacks DUT module $dut"; done

resolve_bin_dir() {
   local name="$1" found root
   found="$(command -v "$name" 2>/dev/null)" && { dirname "$found"; return 0; }
   IFS=: read -r -a roots <<<"${BAMBU_HLS_BACKEND_PATH:-}"
   for root in "${roots[@]}"; do
      found="$(find -L "$root" -type f -path "*/bin/$name" -print -quit 2>/dev/null)"
      [ -z "$found" ] || { dirname "$found"; return 0; }
   done
   return 1
}
bindir="$(resolve_bin_dir vsim)" || fail 'ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH'
export PATH="$bindir:$PATH"
for tool in vlib vlog vsim; do command -v "$tool" >/dev/null || fail "ModelSim tool missing: $tool"; done

for bp in 0 2; do
   lib="$case_dir/questa_lib_BP${bp}"
   vlib "$lib" >"vlib_BP${bp}.log" 2>&1 || fail "vlib failed for BP=$bp"
   vlog -quiet -work "$lib" "$tb" "${rtl[@]}" >"vlog_BP${bp}.log" 2>&1 || fail "vlog failed for BP=$bp; see vlog_BP${bp}.log"
   timeout 600 vsim -c -L "$lib" -gBP_MODE="$bp" "questa_lib_BP${bp}.${LP_TB_TOP:?missing LP_TB_TOP}" \
      -do 'run -all; quit -f' >"vsim_BP${bp}.log" 2>&1
   status=$?
   [ "$status" -eq 0 ] || fail "vsim failed with status $status for BP=$bp; see vsim_BP${bp}.log"
   grep -qE "(^|# )PASS \\[BP=${bp}\\]" "vsim_BP${bp}.log" || fail "missing PASS verdict for BP=$bp"
   if grep -qE '(^|# )(FAIL|INCONCLUSIVE)' "vsim_BP${bp}.log"; then fail "negative verdict present for BP=$bp"; fi
   echo "PASS [BP=$bp]"
done
