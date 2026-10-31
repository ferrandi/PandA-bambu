#!/bin/bash
# Mantis per-case tool: synthesize, assert the expected schedule, then exercise
# output backpressure with Questa/ModelSim.
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

expected_ii="${MANTIS_EXPECT_LOOP_II:?missing MANTIS_EXPECT_LOOP_II}"
if [ "$expected_ii" -eq 0 ]; then
   grep -q 'Loop pipelining not possible' bambu.log || fail 'expected loop pipelining to be impossible'
else
   grep -q "II=${expected_ii} solution found for loop (BB2)" bambu.log || fail "missing BB2 II=${expected_ii} scheduling summary"
   grep -qE "Initiation Interval \\(II\\) for function [^ ]*:BB2 = ${expected_ii}$" bambu.log || fail "missing BB2 II=${expected_ii} metric"
   if [ -n "${LP_LATENCY:-}" ]; then
      grep -qE "Iteration latency for function [^ ]*:BB2 = ${LP_LATENCY}$" bambu.log ||
         fail "missing expected BB2 iteration latency ${LP_LATENCY}"
   fi
   if grep -q 'Loop pipelining not possible' bambu.log; then fail 'loop pipelining reported impossible'; fi
fi

if [ -n "${LP_EXPECT_FU_II:-}" ]; then
   python3 "$regression_dir/check_fu_model.py" || fail 'selected FU timing/assignment does not match the expected model'
fi

tb="$regression_dir/${LP_TB:?missing LP_TB}"
[ -s "$tb" ] || fail "missing testbench: $tb"
rtl=("$case_dir"/*.v)
[ -e "${rtl[0]}" ] || fail 'missing generated RTL'
if [ -n "${LP_EXPECT_PIPE_PARAMETER:-}" ]; then
   grep -Fq ".PIPE_PARAMETER(${LP_EXPECT_PIPE_PARAMETER})" "${rtl[@]}" ||
      fail "generated RTL lacks PIPE_PARAMETER(${LP_EXPECT_PIPE_PARAMETER})"
fi
if [ -n "${LP_EXPECT_CAPTURE_LATENCY:-}" ]; then
   grep -Eq "^[[:space:]]*TIMED_CAPTURE[[:space:]]+#\(\.LATENCY\(${LP_EXPECT_CAPTURE_LATENCY}\)," "${rtl[@]}" ||
      fail "generated RTL lacks TIMED_CAPTURE instance with LATENCY(${LP_EXPECT_CAPTURE_LATENCY})"
fi
if [ -n "${LP_EXPECT_INPUT_HOLD:-}" ]; then
   grep -Eq "^[[:space:]]*INPUT_HOLD[[:space:]]+#\\(" "${rtl[@]}" ||
      fail 'generated RTL lacks INPUT_HOLD on pipelined FU inputs'
fi
if [ -n "${LP_EXPECT_UNIT:-}" ]; then
   grep -Eq "^[[:space:]]*${LP_EXPECT_UNIT}([[:space:]]+#|[[:space:]]+[[:alnum:]_]+[[:space:]]*\\()" "${rtl[@]}" ||
      fail "generated RTL lacks instance of expected unit ${LP_EXPECT_UNIT}"
fi
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

for bp in 0 1 2; do
   lib="$case_dir/questa_lib_BP${bp}"
   vlib "$lib" >"vlib_BP${bp}.log" 2>&1 || fail "vlib failed for BP=$bp"
   vlog -quiet -work "$lib" "$tb" "${rtl[@]}" >"vlog_BP${bp}.log" 2>&1 || fail "vlog failed for BP=$bp; see vlog_BP${bp}.log"
   timeout 600 vsim -c -L "$lib" -gBP_MODE="$bp" "questa_lib_BP${bp}.${LP_TB_TOP:?missing LP_TB_TOP}" \
      -do 'run -all; quit -f' >"vsim_BP${bp}.log" 2>&1
   status=$?
   [ "$status" -eq 0 ] || fail "vsim failed with status $status for BP=$bp; see vsim_BP${bp}.log"
   grep -qE "(^|# )PASS \\[BP=${bp}\\]" "vsim_BP${bp}.log" || fail "missing PASS verdict for BP=$bp"
   if grep -qE '(^|# )(FAIL|INCONCLUSIVE)' "vsim_BP${bp}.log"; then fail "negative verdict present for BP=$bp"; fi
   if [ -n "${LP_EXPECT_INPUT_HOLD:-}" ]; then
      grep -Eq 'FU_II2_MONITOR launches=[1-9][0-9]* hold_checks=[1-9][0-9]* stable_errors=0 operand_changes=[1-9][0-9]* spacing_min=([2-9]|[1-9][0-9]+) spacing_errors=0 stalled_hold_checks=[0-9]+' "vsim_BP${bp}.log" ||
         fail "II=2 input stability or launch spacing failed for BP=$bp"
      if [ "$bp" -eq 0 ]; then
         grep -Eq 'UNBOUNDED_HANDSHAKE co_launches=([1-9][0-9]*) done_same_cycle=\1 done_later=0 pending=0 handshake_errors=0' "vsim_BP${bp}.log" ||
            fail 'unbounded operation did not complete in the co-launch cycle at BP=0'
      else
         grep -Eq 'UNBOUNDED_HANDSHAKE co_launches=[1-9][0-9]* done_same_cycle=[0-9]+ done_later=[1-9][0-9]* pending=0 handshake_errors=0' "vsim_BP${bp}.log" ||
            fail "unbounded stalls did not complete after co-launch for BP=$bp"
         grep -Eq 'FU_II2_MONITOR .*stalled_hold_checks=[1-9][0-9]*' "vsim_BP${bp}.log" ||
            fail "input hold was not checked during a stalled interval for BP=$bp"
      fi
   fi
   echo "PASS [BP=$bp]"
done

# Compile a separate corrupted-model library so the positive verdict checks
# cannot accidentally be satisfied by the mutation run.
mut_lib="$case_dir/questa_lib_MUT"
vlib "$mut_lib" >vlib_MUT.log 2>&1 || fail 'vlib failed for MUT'
vlog -quiet +define+MUT -work "$mut_lib" "$tb" "${rtl[@]}" >vlog_MUT.log 2>&1 || fail 'vlog failed for MUT; see vlog_MUT.log'
timeout 600 vsim -c -L "$mut_lib" -gBP_MODE=0 "questa_lib_MUT.${LP_TB_TOP:?missing LP_TB_TOP}" \
   -do 'run -all; quit -f' >vsim_MUT.log 2>&1
status=$?
[ "$status" -eq 0 ] || fail "vsim failed with status $status for MUT; see vsim_MUT.log"
grep -qE '(^|# )FAIL \[BP=0\]' vsim_MUT.log || fail 'MUT run did not detect corrupted data'
if grep -qE '(^|# )(PASS|INCONCLUSIVE)' vsim_MUT.log; then fail 'MUT run had an invalid verdict'; fi
echo 'PASS [MUT oracle sensitivity]'

if [ -n "${LP_EXPECT_INPUT_HOLD:-}" ]; then
   hold_mut_lib="$case_dir/questa_lib_HOLD_MUT"
   vlib "$hold_mut_lib" >vlib_HOLD_MUT.log 2>&1 || fail 'vlib failed for HOLD_MUT'
   vlog -quiet +define+HOLD_MUT -work "$hold_mut_lib" "$tb" "${rtl[@]}" >vlog_HOLD_MUT.log 2>&1 ||
      fail 'vlog failed for HOLD_MUT; see vlog_HOLD_MUT.log'
   timeout 600 vsim -c -L "$hold_mut_lib" -gBP_MODE=0 "questa_lib_HOLD_MUT.${LP_TB_TOP:?missing LP_TB_TOP}" \
      -do 'run -all; quit -f' >vsim_HOLD_MUT.log 2>&1
   status=$?
   [ "$status" -eq 0 ] || fail "vsim failed with status $status for HOLD_MUT; see vsim_HOLD_MUT.log"
   grep -qE '(^|# )FAIL \[BP=0\]' vsim_HOLD_MUT.log || fail 'HOLD_MUT did not trigger the input stability oracle'
   grep -q 'HOLD_MUT_DETECTED.*return=4280' vsim_HOLD_MUT.log ||
      fail 'HOLD_MUT did not prove the pin oracle fails with the functional checksum intact'
   if grep -qE '(^|# )(PASS|INCONCLUSIVE)' vsim_HOLD_MUT.log; then fail 'HOLD_MUT run had an invalid verdict'; fi
   echo 'PASS [input hold oracle sensitivity]'
fi
