#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
TESTS="$SCRIPT_DIR"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
if (($# != 1)); then echo "usage: $0 fresh-output-directory" >&2; exit 2; fi
if [[ -e "$1" || -L "$1" ]]; then
    echo "output directory must not already exist: $1" >&2; exit 2
fi
TASK_OUT=$(realpath -m "$1")
mkdir -p "$TASK_OUT"
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp"
export TMP="$TASK_OUT/tmp"
export TEMP="$TASK_OUT/tmp"

python3 - "$XML" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" <<'PY'
import re
import sys
import html
import xml.etree.ElementTree as ET
from pathlib import Path

xml_path, output_path = map(Path, sys.argv[1:])
target = "MinimalAXI4MasterPipelined"
root = ET.parse(xml_path).getroot()
cells = [cell for cell in root.findall(".//cell")
         if cell.findtext("name") == target]
if len(cells) != 1:
    raise SystemExit(f"expected exactly one XML cell named {target}, found {len(cells)}")
module = cells[0].find("circuit/module_o")
if module is None or module.get("id") != target:
    raise SystemExit(f"unsupported XML structure for {target}: missing matching circuit/module_o")

parameters = []
for parameter in module.findall("parameter"):
    name = parameter.get("name")
    value = (parameter.text or "").strip()
    if not name or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        raise SystemExit(f"unsupported XML parameter descriptor: {ET.tostring(parameter, encoding='unicode')}")
    if not re.fullmatch(r"[0-9]+", value):
        raise SystemExit(f"unsupported non-integer default for parameter {name}: {value!r}")
    parameters.append((name, value))

ports = []
for port in module.findall("port_o"):
    name, direction = port.get("id"), port.get("dir")
    descriptor = port.find("structural_type_descriptor")
    if not name or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) or direction not in ("IN", "OUT"):
        raise SystemExit(f"unsupported XML port descriptor: {ET.tostring(port, encoding='unicode')}")
    if descriptor is None:
        raise SystemExit(f"unsupported XML port {name}: missing structural_type_descriptor")
    kind = descriptor.get("type")
    if kind == "BOOL" and descriptor.get("size") == "1":
        width = 1
    elif kind == "VECTOR_BOOL" and descriptor.get("size") == "1":
        vector_size = descriptor.get("vector_size", "")
        if not vector_size.isdigit() or int(vector_size) < 1:
            raise SystemExit(f"unsupported vector width for XML port {name}: {vector_size!r}")
        width = int(vector_size)
    else:
        raise SystemExit(f"unsupported XML type for port {name}: {ET.tostring(descriptor, encoding='unicode')}")
    ports.append((name, direction.lower(), width))
if not parameters or not ports:
    raise SystemExit(f"unsupported XML structure for {target}: no parameters or ports")

implementations = module.findall("NP_functionality")
if len(implementations) != 1:
    raise SystemExit(f"expected exactly one NP_functionality for {target}, found {len(implementations)}")

# XML normalizes literal newlines inside attribute values to spaces. Preserve
# the source HDL formatting by recovering this exact attribute from the already
# uniquely identified XML cell, then decoding its XML entities.
raw_xml = xml_path.read_text()
name_token = f"<name>{target}</name>"
name_at = raw_xml.find(name_token)
cell_start = raw_xml.rfind("<cell", 0, name_at)
cell_end = raw_xml.find("</cell>", name_at)
if name_at < 0 or cell_start < 0 or cell_end < 0:
    raise SystemExit(f"could not locate raw XML source for the unique {target} cell")
cell_text = raw_xml[cell_start:cell_end]
raw_match = re.search(r'\bVERILOG_PROVIDED="([^\"]*)"', cell_text, re.DOTALL)
if raw_match is None:
    raise SystemExit(f"could not recover VERILOG_PROVIDED for {target}")
body = html.unescape(raw_match.group(1)).replace("\r\n", "\n")
if not body.strip():
    raise SystemExit(f"{target} has no VERILOG_PROVIDED implementation")
parsed_body = implementations[0].get("VERILOG_PROVIDED") or ""
if " ".join(body.split()) != " ".join(parsed_body.split()):
    raise SystemExit(f"raw and parsed VERILOG_PROVIDED content disagree for {target}")

# Match Bambu's default reset-level and synchronous-reset substitutions.
body = body.replace("1RESET_EDGE", "")
body = body.replace("1RESET_VALUE", "reset == 1'b0")
if re.search(r"1[A-Z][A-Z0-9_]*", body):
    raise SystemExit(f"unsupported unresolved HDL placeholder in {target}")

# Mirror verilog_writer::write_module_parametrization_decl: every port named in
# the NP_functionality LIBRARY token list gets a BITSIZE_<name> parameter whose
# default is the XML vector_size, and is declared as [BITSIZE_<name>-1:0]. The
# real writer overrides those per instance; here the defaults are the point.
library_tokens = set((implementations[0].get("LIBRARY") or "").split())
param_lines = [f"    parameter integer {name} = {value}" for name, value in parameters]
port_lines = []
for name, direction, width in ports:
    sv_direction = "input" if direction == "in" else "output"
    if name in library_tokens and width > 1:
        param_lines.append(f"    parameter integer BITSIZE_{name} = {width}")
        port_lines.append(f"    {sv_direction} wire [BITSIZE_{name}-1:0] {name}")
    else:
        packed = "" if width == 1 else f" [{width - 1}:0]"
        port_lines.append(f"    {sv_direction} wire{packed} {name}")
parameter_text = ",\n".join(param_lines)
header = (f"// Generated from {xml_path.relative_to(xml_path.parents[1]) if xml_path.is_relative_to(xml_path.parents[1]) else xml_path}\n"
          f"module {target} # (\n{parameter_text}\n) (\n" + ",\n".join(port_lines) + "\n);\n")
output_path.write_text(header + body + "\nendmodule\n")
print(f"XML component: {target}; parameters={parameters}; ports={len(ports)}")
print("Reset substitution: 1RESET_VALUE -> (reset == 1'b0); 1RESET_EDGE -> empty (synchronous)")
print(f"Generated RTL: {output_path}")
PY

echo "Verilator: $(verilator --version)"
echo "Stimulus seed: deterministic (no random stimulus)"
echo "All generated RTL, logs, and build products: $TASK_OUT"

# Verilator can build and run simulations concurrently. SIM_JOBS bounds both:
# -j for the model build, and the number of scenario processes in flight.
SIM_JOBS=${SIM_JOBS:-4}
if [[ ! $SIM_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "SIM_JOBS must be a positive integer, got: $SIM_JOBS" >&2; exit 2
fi

wait_scenario_batch() {
    local -n pids_ref=$1
    local -n names_ref=$2
    local build_dir=$3
    local i status name log
    for i in "${!pids_ref[@]}"; do
        status=0
        wait "${pids_ref[$i]}" || status=$?
        name="${names_ref[$i]}"
        log="$build_dir/scenario-${name}.log"
        if (( status != 0 )); then
            echo "FAIL scenario $name (status $status); log: $log" >&2
            cat "$log" >&2 || true
            SCENARIO_FAILURES=$((SCENARIO_FAILURES + 1))
        fi
    done
    pids_ref=()
    names_ref=()
}

run_scenarios() {
    local binary=$1 build_dir=$2
    shift 2
    local batch_pids=() batch_names=() scenario
    SCENARIO_FAILURES=0
    for scenario in "$@"; do
        timeout 120s "$binary" "+$scenario" >"$build_dir/scenario-${scenario}.log" 2>&1 &
        batch_pids+=("$!")
        batch_names+=("$scenario")
        if (( ${#batch_pids[@]} >= SIM_JOBS )); then
            wait_scenario_batch batch_pids batch_names "$build_dir"
        fi
    done
    wait_scenario_batch batch_pids batch_names "$build_dir"
    if (( SCENARIO_FAILURES != 0 )); then
        echo "ERROR: $SCENARIO_FAILURES scenario(s) failed for $binary" >&2
        return 1
    fi
}

run_build() {
    local top=$1 burst_max=$2 tb=$3 snapshot=${4:-} outstanding=${5:-1} depth=${6:-256} launch=${7:-yes}
    local build="$TASK_OUT/build-${top}-B${burst_max}-O${outstanding}-D${depth}"
    mkdir -p "$build"
    local sources=("$TASK_OUT/MinimalAXI4MasterPipelined.sv")
    if [[ -n "$snapshot" ]]; then
        sources+=("$snapshot")
    fi
    sources+=("$tb")
    local parameter_args=(-GB_MAX="$burst_max")
    if [[ "$top" == tb_burst_engine ]]; then
        parameter_args+=(-GMAX_OUTSTANDING="$outstanding" -GFIFO_DEPTH="$depth")
    fi
    timeout 120s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module "$top" \
        "${parameter_args[@]}" --Mdir "$build" "${sources[@]}" \
        >"$build/compile.log" 2>&1 || {
            cat "$build/compile.log"
            return 1
        }
    if [[ "$launch" == yes ]]; then
        timeout 120s "$build/V$top"
    fi
}

for burst_max in 1 2 16 256; do
    run_build tb_burst_engine "$burst_max" "$TESTS/tb_burst_engine.sv"
    if [[ "$burst_max" == 16 ]]; then
        run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D256/Vtb_burst_engine" \
            "$TASK_OUT/build-tb_burst_engine-B16-O1-D256" \
            ERROR BADALIGN OVERFLOW BADSIZE_CONFIG STORE \
            RANGE_ZERO RANGE_LIMIT_BASE0 RANGE_LIMIT_BASE4 \
            RANGE_OVER_BASE4 RANGE_OVER_PLUS1 RANGE_OVER_HALF \
            RANGE_OVER_MAX RANGE_TOP_VALID RANGE_TOP_OVER RANGE_ORACLE \
            BADADDR_EMPTY BADSIZE_EMPTY BADADDR_FULL BADSIZE_FULL \
            DRAIN_CONFIG_RACE CONTINUOUS_CFG ADJACENT_CFG BADRID EARLY_RLAST \
            ADJACENT_CFG_TRUNC ADJACENT_CFG_SIZE \
            MISSING_RLAST RESET_TRAFFIC FIFO_WRAP
    fi
    if [[ "$burst_max" == 256 ]]; then
        run_scenarios "$TASK_OUT/build-tb_burst_engine-B256-O1-D256/Vtb_burst_engine" \
            "$TASK_OUT/build-tb_burst_engine-B256-O1-D256" FIFO_BACKPRESSURE
    fi
done

# Keep responses held until the requested descriptor queue is full. This proves
# actual AR overlap and exercises explicit descriptor-pointer wrap for O=3/15.
for outstanding in 3 15 16; do
    depth=256
    [[ "$outstanding" == 3 ]] && depth=64
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" "$outstanding" "$depth" no
    timeout 120s "$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}/Vtb_burst_engine" +MULTI
done

# Required-hit A+Q+C plus independent bus-derived descriptor accounting,
# malformed R during a held later AR, coordinated multi-outstanding reset,
# boundary/tail with a busy configure, and rejection of a second pending config.
run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 2 64 no
run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O2-D64/Vtb_burst_engine" \
    "$TASK_OUT/build-tb_burst_engine-B16-O2-D64" AQC FAULT_STALLED_AR RESET_MULTI PENDING_SECOND
run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 3 64 no
timeout 120s "$TASK_OUT/build-tb_burst_engine-B16-O3-D64/Vtb_burst_engine" +BOUNDARY_PENDING

# Distinguish descriptor-count limits from beat-credit limits. D=1 also checks
# minimum-width data FIFO pointers/counters and stop/restart behavior.
run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 16 32 no
run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O16-D32/Vtb_burst_engine" \
    "$TASK_OUT/build-tb_burst_engine-B16-O16-D32" CREDIT_LIMIT CREDIT_POP
run_build tb_burst_engine 256 "$TESTS/tb_burst_engine.sv" "" 2 8 no
timeout 120s "$TASK_OUT/build-tb_burst_engine-B256-O2-D8/Vtb_burst_engine" +CREDIT_LIMIT
run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 1 1 no
timeout 120s "$TASK_OUT/build-tb_burst_engine-B16-O1-D1/Vtb_burst_engine" +CREDIT_LIMIT

# Exercise the largest supported data FIFO and fail closed at every parameter
# boundary with a checked diagnostic (not merely a compile/runtime failure).
run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 1 4096 no
timeout 120s "$TASK_OUT/build-tb_burst_engine-B16-O1-D4096/Vtb_burst_engine" +DEPTH4096

expect_parameter_failure() {
    local name=$1 outstanding=$2 depth=$3 diagnostic=$4
    local binary="$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}/Vtb_burst_engine"
    local log="$TASK_OUT/invalid-${name}.log"
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" "$outstanding" "$depth" no
    python3 - "$binary" "$log" "$name" <<'PY'
import resource
import subprocess
import sys

binary, log_path, case_name = sys.argv[1:]

def disable_core_dumps():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

try:
    with open(log_path, "wb") as log:
        result = subprocess.run([binary], stdout=log, stderr=subprocess.STDOUT,
                                timeout=10, preexec_fn=disable_core_dumps,
                                check=False)
except subprocess.TimeoutExpired:
    with open(log_path, "ab") as log:
        log.write(b"\nparameter-validation simulator timed out\n")
    sys.exit(f"ERROR: invalid parameters {case_name} timed out")

with open(log_path, "ab") as log:
    log.write(f"\nparameter-validation simulator exit status: {result.returncode}\n".encode())
if result.returncode == 0:
    sys.exit(f"ERROR: invalid parameters {case_name} unexpectedly passed")
PY
    if ! rg -q "$diagnostic" "$log"; then
        cat "$log" >&2
        echo "ERROR: invalid parameters $name did not report '$diagnostic'" >&2
        return 1
    fi
    echo "PASS invalid parameters $name rejected with diagnostic '$diagnostic'"
}
expect_parameter_failure O0 0 256 'MAX_OUTSTANDING must be in \[1,16\]'
expect_parameter_failure O17 17 256 'MAX_OUTSTANDING must be in \[1,16\]'
expect_parameter_failure D0 1 0 'FIFO_DEPTH must be a power of two in \[1,4096\]'
expect_parameter_failure D3 1 3 'FIFO_DEPTH must be a power of two in \[1,4096\]'
expect_parameter_failure D8192 1 8192 'FIFO_DEPTH must be a power of two in \[1,4096\]'

# Element-count limit. The region counters are COUNT_W = BITSIZE_in5 - 1 bits
# wide, so a 64-bit address must not turn the address check into permission to
# configure a count the counters cannot hold.
ct_build="$TASK_OUT/build-tb_count_trunc-addr64"
mkdir -p "$ct_build"
timeout 120s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
    -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_count_trunc \
    -GB_MAX=16 -GMAX_OUTSTANDING=1 -GFIFO_DEPTH=256 -GADDR_BITS=64 \
    --Mdir "$ct_build" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" "$TESTS/tb_count_trunc.sv" \
    >"$ct_build/compile.log" 2>&1 || {
        cat "$ct_build/compile.log" >&2
        exit 1
    }
timeout 120s "$ct_build/Vtb_count_trunc" +COUNT_TRUNC_64 || exit 1
timeout 120s "$ct_build/Vtb_count_trunc" +COUNT_MAX_64 || exit 1
echo "PASS element-count limit enforced at the counter width, not the address limit"

echo "PASS burst engine RTL regression. Artifacts retained at $TASK_OUT"
