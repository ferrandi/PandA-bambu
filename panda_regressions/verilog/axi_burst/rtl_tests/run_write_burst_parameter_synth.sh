#!/usr/bin/env bash
# Synthesis-side witness for the write-burst engine parameter guards (F2 review closure).
#
# The engine checks B_MAX, MAX_OUTSTANDING, FIFO_DEPTH, DATA_BITS and ADDR_BITS at elaboration
# time. The ModelSim gate proves the same checks stop a simulation build; this script proves
# they also stop a synthesis build, with a second tool that the RTL does not otherwise drive:
# every rejected parameter set must make Vivado's synth_design fail with the guard message, and
# one legal boundary combination must synthesize cleanly so the guard is not over-broad.
#
# Run outside the file sandbox, because Vivado writes project state under the run directory:
#   BURST_WRITER_SYNTH_OUTSIDE_SANDBOX=1 ./run_write_burst_parameter_synth.sh <fresh-output-dir>
#
# The upper FIFO_DEPTH bound (4096) is not re-elaborated here; it is accepted by the simulation
# group core-B16-D4096 of run_write_burst_engine.sh.
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
TOP=MinimalAXI4MasterPipelinedWrite

if (($# != 1)); then
    echo "usage: $0 <fresh-output-directory>" >&2
    exit 2
fi
if [[ ${BURST_WRITER_SYNTH_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing synthesis unless BURST_WRITER_SYNTH_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
for tool in vivado timeout python3 sha256sum grep tail tee realpath git; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "required tool is missing from PATH: $tool" >&2
        exit 2
    }
done
VIVADO=$(realpath "$(command -v vivado)")
if [[ ! -x $VIVADO ]]; then
    echo "vivado is not executable: $VIVADO" >&2
    exit 2
fi
PART=${VIVADO_PART:-xcu250-figd2104-2L-e}

OUT_ARG=$1
OUT_PARENT=$(realpath -m -- "$(dirname "$OUT_ARG")")
TASK_OUT=$(realpath -m -- "$OUT_ARG")
if [[ -e "$TASK_OUT" || -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
    echo "output directory must be fresh and must not be a symlink: $OUT_ARG" >&2
    exit 2
fi
mkdir -p "$OUT_PARENT"
mkdir "$TASK_OUT"
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
RESULTS_LOG="$TASK_OUT/results.log"
: > "$RESULTS_LOG"

python3 -B "$EXTRACTOR" --xml "$XML" --out "$TASK_OUT/$TOP.sv" | tee "$TASK_OUT/extract.log"
VIVADO_VERSION=$(timeout 30s "$VIVADO" -version 2>&1) || {
    echo "vivado -version failed (status=$?)" >&2
    exit 1
}
{
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'vivado=%s\n' "$VIVADO"
    printf 'vivado_version=%s\n' "${VIVADO_VERSION%%$'\n'*}"
    printf 'part=%s\n' "$PART"
    printf 'git_head=%s\n' "$(git -C "$ROOT" rev-parse HEAD)"
} > "$TASK_OUT/provenance.txt"

# label B_MAX MAX_OUTSTANDING FIFO_DEPTH expect [message]
CASES=(
    reject-B0 0 1 1 reject "B_MAX must be in [1,256]"
    reject-B257 257 1 512 reject "B_MAX must be in [1,256]"
    reject-O0 16 0 16 reject "MAX_OUTSTANDING must be in [1,16]"
    reject-O17 16 17 32 reject "MAX_OUTSTANDING must be in [1,16]"
    reject-FIFO-BELOW-B 16 1 8 reject "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
    reject-FIFO-NONPOWER 3 1 3 reject "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
    accept-legal-boundaries 256 16 512 accept ""
)

run_case() {
    local label=$1 burst=$2 outstanding=$3 depth=$4 expect=$5 message=$6
    local dir="$TASK_OUT/synth-$label" log="$TASK_OUT/synth-$label.log" tcl="$TASK_OUT/synth-$label.tcl"
    mkdir -p "$dir"
    cat > "$tcl" <<TCL
create_project -force -dir "$dir/project" -part $PART witness_$label
read_verilog "$TASK_OUT/$TOP.sv"
if {[catch {synth_design -top $TOP -mode out_of_context -no_iobuf \\
            -generic B_MAX=$burst -generic MAX_OUTSTANDING=$outstanding \\
            -generic FIFO_DEPTH=$depth} err]} {
    puts "SYNTH_TOOL_OUTCOME error"
    puts "SYNTH_TOOL_MESSAGE \$err"
} else {
    puts "SYNTH_TOOL_OUTCOME ok"
}
TCL
    local status=0 outcome outcome_line error_line
    timeout 900s "$VIVADO" -mode batch -nojournal -log "$dir/vivado.log" -source "$tcl" \
        > "$log" 2>&1 || status=$?
    printf '%s\n' "$status" > "$log.status"
    local -a outcomes=() messages=() errors=() guard_errors=() module_errors=()
    mapfile -t outcomes < <(grep -E '^SYNTH_TOOL_OUTCOME ' "$log" || true)
    mapfile -t messages < <(grep -E '^SYNTH_TOOL_MESSAGE ' "$log" || true)
    mapfile -t errors < <(grep -E '^ERROR:' "$log" || true)
    if ((${#outcomes[@]} != 1)); then
        tail -n 40 "$log" >&2
        echo "synthesis did not emit exactly one outcome marker: $label (status=$status)" >&2
        return 1
    fi
    outcome_line=${outcomes[0]}
    if [[ $outcome_line == 'SYNTH_TOOL_OUTCOME ok' ]]; then
        outcome=ok
    elif [[ $outcome_line == 'SYNTH_TOOL_OUTCOME error' ]]; then
        outcome=error
    else
        tail -n 40 "$log" >&2
        echo "synthesis emitted an invalid outcome marker: $label ($outcome_line)" >&2
        return 1
    fi
    if grep -Eqi 'segmentation fault|stack smashing|addresssanitizer|core dumped|aborted|killed|timed out|timeout' "$log"; then
        tail -n 40 "$log" >&2
        echo "synthesis crashed or timed out: $label (status=$status)" >&2
        return 1
    fi
    if [[ $expect == reject ]]; then
        mapfile -t guard_errors < <(grep -F "ERROR: [Synth 8-6058] Synth Error: $message " "$log" || true)
        mapfile -t module_errors < <(grep -F "ERROR: [Synth 8-6156] failed synthesizing module '$TOP' " "$log" || true)
        if ((status != 0)) || [[ $outcome != error ]] || ((${#messages[@]} != 1)) ||
           [[ ${messages[0]} != 'SYNTH_TOOL_MESSAGE ERROR:'* ]] ||
           ((${#guard_errors[@]} != 1 || ${#module_errors[@]} != 1 || ${#errors[@]} != 2)); then
            tail -n 40 "$log" >&2
            echo "synthesis rejection did not emit one exact guard and its single module-failure cascade: $label (status=$status outcome=$outcome messages=${#messages[@]} errors=${#errors[@]})" >&2
            return 1
        fi
        local generated_sv="$TASK_OUT/$TOP.sv" guard_line module_line
        if [[ ${guard_errors[0]} != "ERROR: [Synth 8-6058] Synth Error: $message  [$generated_sv:"* ||
              ${module_errors[0]} != "ERROR: [Synth 8-6156] failed synthesizing module '$TOP' [$generated_sv:"* ]]; then
            tail -n 40 "$log" >&2
            echo "synthesis rejection diagnostics do not identify the generated module: $label" >&2
            return 1
        fi
        guard_line=${guard_errors[0]#*"$generated_sv:"}
        module_line=${module_errors[0]#*"$generated_sv:"}
        if ! [[ $guard_line =~ ^[0-9]+\]$ && $module_line =~ ^[0-9]+\]$ ]]; then
            tail -n 40 "$log" >&2
            echo "synthesis rejection diagnostics have an invalid source location: $label" >&2
            return 1
        fi
        for error_line in "${errors[@]}"; do
            if [[ $error_line != "${guard_errors[0]}" && $error_line != "${module_errors[0]}" ]]; then
                tail -n 40 "$log" >&2
                echo "synthesis rejection emitted an unrelated error: $label" >&2
                return 1
            fi
        done
        echo "SYNTH_REJECT_PASS $label B_MAX=$burst MAX_OUTSTANDING=$outstanding FIFO_DEPTH=$depth status=$status" \
            >> "$RESULTS_LOG"
        return 0
    fi
    if ((status != 0)) || [[ $outcome != ok ]] || ((${#messages[@]} != 0 || ${#errors[@]} != 0)); then
        tail -n 40 "$log" >&2
        echo "synthesis refused a legal parameter set or emitted an error: $label (status=$status outcome=$outcome messages=${#messages[@]})" >&2
        return 1
    fi
    echo "SYNTH_ACCEPT_PASS $label B_MAX=$burst MAX_OUTSTANDING=$outstanding FIFO_DEPTH=$depth" \
        >> "$RESULTS_LOG"
    return 0
}

if [[ ${#CASES[@]} -lt 7 ]]; then
    echo "malformed case table" >&2
    exit 2
fi

failures=0
for ((index = 0; index < ${#CASES[@]}; index += 6)); do
    if ! run_case "${CASES[index]}" "${CASES[index+1]}" "${CASES[index+2]}" "${CASES[index+3]}" \
                  "${CASES[index+4]}" "${CASES[index+5]}"; then
        failures=$((failures + 1))
    fi
done
cat "$RESULTS_LOG"
if (( failures )); then
    echo "WRITER_PARAMETER_SYNTH_FAIL failures=$failures" >&2
    exit 1
fi
echo "WRITER_PARAMETER_SYNTH_PASS cases=$((${#CASES[@]} / 6)) rejects=6 accepts=1"
