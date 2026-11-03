#!/bin/bash
# Mantis per-case tool: assert Bambu's schedule, then run the selected simulations.
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

if [ "${LP_EVALUATION_ONLY:-0}" = "1" ]; then
   echo 'BAMBU_EVALUATION_EXECUTED'
   exit 0
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
if [ -n "${LP_EXPECT_CAPTURE_CAPACITY:-}" ]; then
   grep -A1 -h -E "^[[:space:]]*TIMED_CAPTURE[[:space:]]+#\(\.LATENCY\(${LP_EXPECT_CAPTURE_LATENCY}\)," "${rtl[@]}" |
      grep -qE "^[[:space:]]*\.CAPACITY\(${LP_EXPECT_CAPTURE_CAPACITY}\)," ||
      fail "generated RTL lacks TIMED_CAPTURE with LATENCY(${LP_EXPECT_CAPTURE_LATENCY}) and CAPACITY(${LP_EXPECT_CAPTURE_CAPACITY})"
fi
if [ -n "${LP_EXPECT_STALL:-}" ]; then
   grep -q 'selector_stallable_fus' "${rtl[@]}" || fail 'generated RTL lacks shared stall selector'
   grep -Eq '[.]stall_port[(][^)]*selector_stallable_fus[^)]*[)]' "${rtl[@]}" ||
      fail 'generated multiplier does not connect the shared stall selector'
fi
if [ -n "${LP_EXPECT_INPUT_HOLD:-}" ]; then
   grep -Eq "^[[:space:]]*INPUT_HOLD[[:space:]]+#\\(" "${rtl[@]}" ||
      fail 'generated RTL lacks INPUT_HOLD on pipelined FU inputs'
fi
if [ -n "${LP_EXPECT_MUL_II2_HOLD:-}" ] &&
   ! grep -q 'selector_input_hold_' "${rtl[@]}"; then
   fail 'generated RTL lacks input-hold selector for the II=2 multiplier'
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

# Resolve generated identifiers from the selected function datapath. Bambu's
# numeric suffixes are unstable across otherwise identical compilations.
dynamic_tb_defines=()
if [ -n "${LP_EXPECT_ARRAY_INTERFACE:-}" ]; then
   dynamic_tb_defines+=(+define+LP_ARRAY_INTERFACE)
fi
if [ -n "${LP_EXPECT_NO_MUL_CAPTURE:-}" ] || [ -n "${LP_EXPECT_MUL_II2_HOLD:-}" ] || [ -n "${LP_EXPECT_INPUT_HOLD:-}" ] || [ -n "${LP_EXPECT_PHASE_CAPTURE:-}" ]; then
   resolved="$(python3 - "${MANTIS_EXPECT_FUNCTION:?missing MANTIS_EXPECT_FUNCTION}" "${LP_EXPECT_NO_MUL_CAPTURE:-}" "${LP_EXPECT_MUL_II2_HOLD:-}" "${LP_EXPECT_INPUT_HOLD:-}" "${LP_EXPECT_PHASE_CAPTURE:-}" "${rtl[@]}" <<'PY'
import re, sys
function, no_capture, mul_ii2, input_hold, phase_capture, *paths = sys.argv[1:]
files = '\n'.join(open(path, encoding='utf-8', errors='replace').read() for path in paths)
module = 'datapath_' + function.lstrip('_')
matches = re.findall(r'(?ms)^module\s+' + re.escape(module) + r'\s*\(.*?^endmodule', files)
if len(matches) != 1:
    raise SystemExit(f'expected one {module}, found {len(matches)}')
body = matches[0]
fu_re = re.compile(r'(?ms)^\s*ui_mul_node_FU\s*#\(.*?\)\s*(fu_[A-Za-z0-9_$]+)\s*\((.*?)\);')
fus = fu_re.findall(body)
if len(fus) != 1:
    raise SystemExit(f'expected one specialized multiplier instance in {module}, found {len(fus)}')
fu, conns = fus[0]
def port(name):
    found = re.findall(r'\.(' + name + r')\s*\(', conns)
    if len(found) != 1:
        raise SystemExit(f'{fu}: expected one {name} operand connection, found {len(found)}')
    return found[0]
ports = ['lhs', 'rhs'] if '.lhs(' in conns else ['in1', 'in2']
for p in ports:
    if not re.search(r'\.' + p + r'\s*\(', conns):
        raise SystemExit(f'{fu}: missing expected operand connection {p}')
fu_suffix = fu[len('fu_'):]
capture = 'timed_capture_' + fu_suffix
if no_capture:
    if re.search(r'(?m)^\s*TIMED_CAPTURE\s*#.*?\b' + re.escape(capture) + r'\s*\(', body, re.S):
        raise SystemExit(f'{module}: generated RTL still captures selected multiplier result {capture}')
defs = [f'LP_MUL_INSTANCE={fu}', f'LP_MUL_PORT_A={ports[0]}', f'LP_MUL_PORT_B={ports[1]}']
if mul_ii2 or input_hold:
    selector_kind = 'selector_input_hold_' if mul_ii2 else 'selector_timed_capture_' if 'selector_timed_capture_' in body else 'selector_input_hold_'
    selectors = re.findall(r'\b(' + re.escape(selector_kind) + r'[A-Za-z0-9_$]+)\b', body)
    selectors = list(dict.fromkeys(selectors))
    if selector_kind == 'selector_timed_capture_':
        selectors = [s for s in selectors if s.endswith(fu_suffix)]
    if len(selectors) != 1:
        raise SystemExit(f'{module}: expected one multiplier launch selector of kind {selector_kind}, found {selectors}')
    defs.append(f'LP_MUL_LAUNCH={selectors[0]}')
if input_hold:
    writers = re.findall(r'(?ms)^\s*out_bambu_artificial_ParmMgr_Write_fifo_modgen\s*#\(.*?\)\s*(fu_[A-Za-z0-9_$]+)\s*\((.*?)\);', body)
    writers = [(name, pins) for name, pins in writers if re.search(r'\.p_out_write\s*\(\s*p_out_write\s*\)', pins)]
    if len(writers) != 1:
        raise SystemExit(f'{module}: expected one output FIFO writer connected to p_out_write, found {len(writers)}')
    _, pins = writers[0]
    starts = re.findall(r'\.start_port\s*\(\s*\{\s*(selector_IN_UNBOUNDED_[A-Za-z0-9_$]+)\s*\}\s*\)', pins)
    if len(starts) != 1:
        raise SystemExit(f'{module}: expected one unbounded start selector on output writer, found {starts}')
    done_names = re.findall(r'(?m)^\s*assign\s+(OUT_UNBOUNDED_[A-Za-z0-9_$]+)\s*=\s*[^;]+;', body)
    done_names = [n for n in done_names if n.endswith(starts[0][len('selector_IN_UNBOUNDED_'):])]
    if len(done_names) != 1:
        raise SystemExit(f'{module}: expected unique matching unbounded done signal, found {done_names}')
    defs.extend([f'LP_UNBOUNDED_LAUNCH={starts[0]}', f'LP_UNBOUNDED_DONE={done_names[0]}'])
if phase_capture:
    selectors = list(dict.fromkeys(re.findall(r'\b(selector_timed_capture_' + re.escape(fu_suffix) + r'(?:_1)?)\b', body)))
    if len(selectors) != 2 or not any(s.endswith('_1') for s in selectors):
        raise SystemExit(f'{module}: expected launch and consume selectors for {capture}, found {selectors}')
    writers = re.findall(r'(?ms)^\s*out_bambu_artificial_ParmMgr_Write_fifo_modgen\s*#\(.*?\)\s*fu_[A-Za-z0-9_$]+\s*\((.*?)\);', body)
    writers = [pins for pins in writers if re.search(r'\.p_out_write\s*\(\s*p_out_write\s*\)', pins)]
    if len(writers) != 1:
        raise SystemExit(f'{module}: expected one output FIFO writer connected to p_out_write, found {len(writers)}')
    starts = re.findall(r'\.start_port\s*\(\s*\{\s*(selector_IN_UNBOUNDED_[A-Za-z0-9_$]+)\s*\}\s*\)', writers[0])
    if len(starts) != 1:
        raise SystemExit(f'{module}: expected one unbounded output launch selector, found {starts}')
    if not re.search(r'(?m)^\s*TIMED_CAPTURE\s*#.*?\b' + re.escape(capture) + r'\s*\(', body, re.S):
        raise SystemExit(f'{module}: expected timed capture instance {capture}')
    defs.extend([f'LP_MUL_LAUNCH={next(s for s in selectors if not s.endswith("_1"))}',
                 f'LP_MUL_CONSUME={next(s for s in selectors if s.endswith("_1"))}',
                 f'LP_MUL_CAPTURE={capture}', f'LP_UNBOUNDED_LAUNCH={starts[0]}'])
print(' '.join(defs))
PY
)" || fail 'could not resolve unique FU/capture/handshake identifiers from selected datapath'
   for define in $resolved; do dynamic_tb_defines+=("+define+$define"); done
fi
if [ -n "${LP_EXPECT_CAPTURE_COUNT:-}" ]; then
   capture_count="$(grep -hE '^[[:space:]]*TIMED_CAPTURE[[:space:]]+#' "${rtl[@]}" | awk 'END { print NR + 0 }')"
   [ "$capture_count" -eq "$LP_EXPECT_CAPTURE_COUNT" ] ||
      fail "expected ${LP_EXPECT_CAPTURE_COUNT} TIMED_CAPTURE instances, found ${capture_count}"
fi

source "$script_dir/../../../etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
bambu_modelsim_resolve || fail 'ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH'
bindir="$BAMBU_MODELSIM_BIN"
export PATH="$bindir:$PATH"
for tool in vlib vlog vsim; do command -v "$tool" >/dev/null || fail "ModelSim tool missing: $tool"; done

for bp in 0 1 2; do
   lib="$case_dir/questa_lib_BP${bp}"
   vlib "$lib" >"vlib_BP${bp}.log" 2>&1 || fail "vlib failed for BP=$bp"
   vlog -quiet "${dynamic_tb_defines[@]}" -work "$lib" "$tb" "${rtl[@]}" >"vlog_BP${bp}.log" 2>&1 ||
      fail "vlog failed for BP=$bp; see vlog_BP${bp}.log"
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
         grep -Eq 'UNBOUNDED_HANDSHAKE launches=16 inflight_launches=[1-9][0-9]* done_same_cycle=[0-9]+ done_later=0 done_before_ii=[1-9][0-9]* pending=0 handshake_errors=0 accepted_writes=16' "vsim_BP${bp}.log" ||
            fail 'unbounded operation lacked an in-flight overlap or completion before FU II expiry at BP=0'
      else
         grep -Eq 'UNBOUNDED_HANDSHAKE launches=16 inflight_launches=[1-9][0-9]* done_same_cycle=[0-9]+ done_later=[1-9][0-9]* done_before_ii=[0-9]+ pending=0 handshake_errors=0 accepted_writes=16' "vsim_BP${bp}.log" ||
            fail "unbounded requests did not complete after a stalled in-flight overlap for BP=$bp"
         grep -Eq 'FU_II2_MONITOR .*stalled_hold_checks=[1-9][0-9]*' "vsim_BP${bp}.log" ||
            fail "input hold was not checked during a stalled interval for BP=$bp"
      fi
   fi
   if [ -n "${LP_EXPECT_CAPTURE_QUEUE_MIN:-}" ] && [ "$bp" -eq 2 ]; then
      grep -Eq 'CAPTURE_STALL_ORACLE mul_peak=([2-9]|[1-9][0-9]+) required=2' "vsim_BP${bp}.log" ||
         fail "fewer than two TIMED_CAPTURE results were pending during one unbounded wait at BP=$bp"
   fi
   if [ -n "${LP_EXPECT_STALL:-}" ] && [ "$bp" -ne 0 ]; then
      grep -Eq 'STALLABLE_STALL_ORACLE stall_cycles=[1-9][0-9]*' "vsim_BP${bp}.log" ||
         fail "stallable multiplier did not freeze during backpressure at BP=$bp"
   fi
   if [ -n "${LP_EXPECT_ARRAY_INTERFACE:-}" ]; then
      grep -Eq 'ARRAY_INTERFACE_MONITOR a_reads=16 b_reads=16 ce_errors=0 address_errors=0 q_errors=0 stall_cycles=[0-9]+' \
         "vsim_BP${bp}.log" || fail "array interface hold/request oracle failed for BP=$bp"
      if [ "$bp" -ne 0 ]; then
         grep -Eq 'ARRAY_INTERFACE_MONITOR .*stall_cycles=[1-9][0-9]*' "vsim_BP${bp}.log" ||
            fail "array interface was not monitored during a stalled interval at BP=$bp"
      fi
   fi
   if [ -n "${LP_EXPECT_MUL_II2_HOLD:-}" ]; then
      if [ "$bp" -eq 0 ]; then
         grep -Eq 'STALLABLE_II2_MONITOR launches=16 hold_checks=[1-9][0-9]* stable_errors=0 operand_changes=[1-9][0-9]* spacing_min=([2-9]|[1-9][0-9]+) spacing_errors=0 stalled_hold_checks=0 stall_cycles=0' \
            "vsim_BP${bp}.log" || fail 'stallable multiplier II=2 input hold/spacing check failed without backpressure'
      else
         grep -Eq 'STALLABLE_II2_MONITOR launches=16 hold_checks=[1-9][0-9]* stable_errors=0 operand_changes=[1-9][0-9]* spacing_min=([2-9]|[1-9][0-9]+) spacing_errors=0 stalled_hold_checks=[1-9][0-9]* stall_cycles=[1-9][0-9]*' \
            "vsim_BP${bp}.log" || fail "stallable multiplier II=2 hold/stall check failed at BP=$bp"
      fi
   fi
   echo "PASS [BP=$bp]"
done

# Compile a separate corrupted-model library so the positive verdict checks
# cannot accidentally be satisfied by the mutation run.
mut_lib="$case_dir/questa_lib_MUT"
vlib "$mut_lib" >vlib_MUT.log 2>&1 || fail 'vlib failed for MUT'
tb_defines=(+define+MUT "${dynamic_tb_defines[@]}")
vlog -quiet "${tb_defines[@]}" -work "$mut_lib" "$tb" "${rtl[@]}" >vlog_MUT.log 2>&1 ||
   fail 'vlog failed for MUT; see vlog_MUT.log'
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
   vlog -quiet +define+HOLD_MUT "${dynamic_tb_defines[@]}" -work "$hold_mut_lib" "$tb" "${rtl[@]}" >vlog_HOLD_MUT.log 2>&1 ||
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

if [ -n "${LP_EXPECT_MUL_II2_HOLD:-}" ]; then
   hold_mut_lib="$case_dir/questa_lib_MUL_HOLD_MUT"
   vlib "$hold_mut_lib" >vlib_MUL_HOLD_MUT.log 2>&1 || fail 'vlib failed for MUL_HOLD_MUT'
   vlog -quiet +define+HOLD_MUT "${dynamic_tb_defines[@]}" -work "$hold_mut_lib" "$tb" "${rtl[@]}" \
      >vlog_MUL_HOLD_MUT.log 2>&1 || fail 'vlog failed for MUL_HOLD_MUT; see vlog_MUL_HOLD_MUT.log'
   timeout 600 vsim -c -L "$hold_mut_lib" -gBP_MODE=0 "questa_lib_MUL_HOLD_MUT.${LP_TB_TOP:?missing LP_TB_TOP}" \
      -do 'run -all; quit -f' >vsim_MUL_HOLD_MUT.log 2>&1
   status=$?
   [ "$status" -eq 0 ] || fail "vsim failed with status $status for MUL_HOLD_MUT; see vsim_MUL_HOLD_MUT.log"
   grep -qE '(^|# )FAIL \[BP=0\]' vsim_MUL_HOLD_MUT.log || fail 'MUL_HOLD_MUT did not trigger the input stability oracle'
   grep -q 'STALLABLE_HOLD_MUT_DETECTED' vsim_MUL_HOLD_MUT.log || fail 'MUL_HOLD_MUT did not identify corrupted operands'
   if grep -qE '(^|# )(PASS|INCONCLUSIVE)' vsim_MUL_HOLD_MUT.log; then
      fail 'MUL_HOLD_MUT run had an invalid verdict'
   fi
   echo 'PASS [stallable multiplier II=2 input hold oracle sensitivity]'
fi
