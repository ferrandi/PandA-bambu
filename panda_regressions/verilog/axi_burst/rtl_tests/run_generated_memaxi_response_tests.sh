#!/usr/bin/env bash
# Connect the production TestbenchMEMAXI to generated read/write kernels and inject responses.
set -Eeuo pipefail

TESTS=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
if (($# != 2)); then
    echo "usage: BURST_MODELSIM_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR" >&2
    exit 2
fi
[[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-0} == 1 ]] || {
    echo "refusing generated ModelSim qualification without BURST_MODELSIM_OUTSIDE_SANDBOX=1" >&2
    exit 2
}

BAMBU_ARG=$1
if [[ $BAMBU_ARG == */* ]]; then
    BAMBU=$(realpath "$BAMBU_ARG")
else
    BAMBU=$(command -v "$BAMBU_ARG" || true)
    [[ -z $BAMBU ]] || BAMBU=$(realpath "$BAMBU")
fi
[[ -n ${BAMBU:-} && -x $BAMBU ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r $SETTINGS ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
    echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $PLUGIN ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
source "$TESTS/modelsim_helpers.sh"
for tool in python3 sha256sum timeout grep sed tail tr; do
    command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }
done
modelsim_resolve
MODELSIM_VERSION=$(modelsim_version) || exit 2

OUT_ARG=$2
OUT=$(realpath -m -- "$OUT_ARG")
if [[ -e $OUT || -e $OUT_ARG || -L $OUT_ARG ]]; then
    echo "output directory must be fresh and must not be a symlink: $OUT_ARG" >&2
    exit 2
fi
mkdir -p "$(dirname "$OUT")"
mkdir "$OUT" "$OUT/tmp" "$OUT/common"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

READ_SOURCE="$ROOT/panda_regressions/hls/bambu_axi_burst/bambu_axi_burst_sum_profile.cpp"
WRITE_SOURCE="$ROOT/panda_regressions/hls/bambu_axi_burst/write_compiler_cases/write_cases.cpp"
TB="$TESTS/tb_generated_memaxi_response.sv"
MODEL_EXTRACTOR="$TESTS/extract_testbench_memaxi.py"
T4B_EXTRACTOR="$ROOT/panda_regressions/verilog/axi_burst/t4b_tests/extract_t4b_cells.py"
BACKEND="$TESTS/tb_testbench_memaxi_backend.sv"
XML="$ROOT/etc/libtech/NC_TESTBENCH_IPs.xml"
AXI_XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
for file in "$READ_SOURCE" "$WRITE_SOURCE" "$TB" "$MODEL_EXTRACTOR" "$T4B_EXTRACTOR" "$BACKEND" "$XML" "$AXI_XML"; do
    [[ -s $file ]] || { echo "required input missing: $file" >&2; exit 2; }
done

{
    printf 'root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
        "$ROOT" "$OUT" "$BAMBU" "$PREFIX" "$(realpath "$SETTINGS")" "$PLUGIN"
    printf 'modelsim_lookup_source=%s\nmodelsim_bin=%s\nvsim=%s\nvlog=%s\nvlib=%s\nvmap=%s\n' \
        "$MODELSIM_SOURCE" "$MODELSIM_BIN" "$MODELSIM_VSIM" "$MODELSIM_VLOG" "$MODELSIM_VLIB" "$MODELSIM_VMAP"
    printf 'modelsim_version=%s\n' "$(printf '%s' "$MODELSIM_VERSION" | tr '\n' ' ')"
    printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
    "$BAMBU" --version 2>&1 | sed -n '1,2p'
    sha256sum "$BAMBU" "$PLUGIN" "$SETTINGS" "$READ_SOURCE" "$WRITE_SOURCE" "$TB" \
        "$MODEL_EXTRACTOR" "$T4B_EXTRACTOR" "$BACKEND" "$XML" "$AXI_XML" \
        "$MODELSIM_VSIM" "$MODELSIM_VLOG" "$MODELSIM_VLIB" "$MODELSIM_VMAP"
} >"$OUT/provenance.txt"

python3 -B "$MODEL_EXTRACTOR" --xml "$XML" --out "$OUT/common/TestbenchMEMAXI.sv" \
    --t4b-extractor "$T4B_EXTRACTOR" >"$OUT/common/extract.log"

generate_kernel() {
    local name=$1 source=$2 top=$3
    shift 3
    local gen="$OUT/$name/generated"
    mkdir -p "$gen"
    local -a args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
        --device-name=xcu250,-2L,figd2104 --no-clean
        --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU --extra-cc-options=-DNOCACHE)
    args+=("$@")
    {
        printf 'case=%s\nsource=%s\ntop=%s\nargv=' "$name" "$source" "$top"
        printf '%q ' "$BAMBU" "$source" "${args[@]}"
        printf '\n'
    } >"$OUT/$name/generation.command"
    if ! (cd "$gen" && "$BAMBU" "$source" "${args[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
        tail -n 80 "$gen/bambu.stderr.log" >&2 || true
        tail -n 80 "$gen/bambu.stdout.log" >&2 || true
        echo "Bambu generation failed for $name; see $gen/bambu.*.log" >&2
        return 1
    fi
    local rtl="$gen/$top.v" lib="$gen/panda_libtech.v"
    [[ -s $rtl && -s $lib ]] || { echo "generated RTL/library missing for $name" >&2; return 1; }
    if [[ $name == read ]]; then
        grep -Eq 'Configured read burst regions for bundle src \(B=16\)' "$gen/bambu.stderr.log" || {
            echo "read kernel did not enable its requested burst path" >&2; return 1;
        }
        grep -q 'MinimalAXI4MasterPipelined' "$rtl" || { echo "read engine missing from generated RTL" >&2; return 1; }
    else
        grep -Eq 'Configured write burst region for bundle output \(B=16\)' "$gen/bambu.stderr.log" || {
            echo "write kernel did not enable its requested burst path" >&2; return 1;
        }
        grep -q 'MinimalAXI4MasterPipelinedWrite' "$rtl" || { echo "write engine missing from generated RTL" >&2; return 1; }
    fi
    sha256sum "$rtl" "$lib" >>"$OUT/provenance.txt"
}

generate_kernel read "$READ_SOURCE" kernel
generate_kernel write "$WRITE_SOURCE" write_count_seed

check_log() {
    local log=$1 mode=$2 reason=$3 engine=$4
    local normalized="$log.normalized"
    modelsim_normalize_log "$log" "$normalized"
    [[ -s $log.raw-status && -s $log.status ]] || { echo "missing raw/effective statuses: $log" >&2; return 1; }
    if [[ $mode == control ]]; then
        if [[ $(<"$log.raw-status") != 0 || $(<"$log.status") != 0 ]] ||
           grep -Eqi '(Error|Warning|Fatal):|segmentation fault|stack smashing|addresssanitizer|aborted|watchdog|timed out|^AXI_BURST_' "$normalized"; then
            cat "$log" >&2; echo "generated $engine OKAY control failed" >&2; return 1
        fi
        grep -Fq "GENERATED_MEMAXI_RESPONSE_CONTROL mode=$engine done=1" "$normalized" || {
            cat "$log" >&2; echo "generated $engine control marker missing" >&2; return 1;
        }
        return 0
    fi

    if [[ $(<"$log.raw-status") != 0 || $(<"$log.status") != 0 ]] ||
       grep -Eqi '(Error|Warning|Fatal):|segmentation fault|stack smashing|addresssanitizer|aborted|watchdog|timed out' "$normalized"; then
        cat "$log" >&2; echo "generated $engine response-fault containment failed" >&2; return 1
    fi
    grep -Fqx "GENERATED_MEMAXI_RESPONSE_CONTAINED mode=$engine response=SLVERR handshakes=1 done=0" "$normalized" || {
        cat "$log" >&2; echo "generated $engine containment marker missing" >&2; return 1;
    }
    if [[ $engine == read ]]; then
        grep -Eq '^GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000$' "$normalized" || {
            cat "$log" >&2; echo "generated read wrapper reset/reinvocation recovery marker missing" >&2; return 1;
        }
    else
        grep -Eq '^GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1$' "$normalized" || {
            cat "$log" >&2; echo "generated write wrapper reset/reinvocation recovery marker missing" >&2; return 1;
        }
    fi
    python3 - "$normalized" "$engine" "$reason" <<'PY'
from pathlib import Path
import re
import sys

log, engine, reason = Path(sys.argv[1]).read_text(errors="replace"), sys.argv[2], sys.argv[3]
address = "00001000" if engine == "read" else "00002000"
records = [line for line in log.splitlines() if line.startswith("AXI_BURST_")]
if len(records) != 1:
    raise SystemExit(f"expected one generated-wrapper diagnostic, saw {len(records)}")
pattern = (rf"^AXI_BURST_FAULT engine={engine} reason={reason} "
           rf"instance=tb_generated_memaxi_response\.dut\..*\.burst_engine\.report_axi_{engine}_diagnostic "
           rf"opcode=-1 size=0 address={address} count=1 "
           + (r"payload=00000000 " if engine == "write" else "")
           + r"resp=10 id=0 "
           + (r"descriptors=1 remaining=0$" if engine == "write" else r"last=1 descriptors=1 remaining=1$"))
if not re.fullmatch(pattern, records[0]):
    raise SystemExit(f"unexpected generated-wrapper response diagnostic: {records[0]}")
PY
}

run_case() {
    local name=$1 define=$2
    local gen="$OUT/$name/generated" sim="$OUT/$name/modelsim" top=tb_generated_memaxi_response
    local -a vlog_defines=("+define+$define")
    modelsim_compile "$sim" "$top" "$gen/$([[ $name == read ]] && echo kernel || echo write_count_seed).v" \
        "$gen/panda_libtech.v" "$OUT/common/TestbenchMEMAXI.sv" "$BACKEND" "$TB" -- "${vlog_defines[@]}" || {
        echo "ModelSim compile failed for generated $name wrapper; see $sim/compile.log" >&2
        return 1
    }
    if grep -Eq '^\*\* (Error|Warning):' "$sim/compile.log"; then
        cat "$sim/compile.log" >&2
        echo "ModelSim compilation emitted errors or warnings for generated $name wrapper" >&2
        return 1
    fi
    sha256sum "$sim/compile.log" >>"$OUT/provenance.txt"
    local case_mode log
    for case_mode in control fault; do
        log="$OUT/$name/$case_mode.log"
        local -a runtime_args=()
        [[ $case_mode != fault ]] || runtime_args+=(+FAULT +AXI_BURST_EXPECT_FAULT)
        local status=0
        modelsim_run "$sim" "$top" "$log" 120 -- "${runtime_args[@]}" || status=$?
        if ((status != 0)); then
            cat "$log" >&2
            echo "ModelSim generated $name/$case_mode failed (status=$status)" >&2
            return 1
        fi
        if [[ $case_mode == control ]]; then
            check_log "$log" control "" "$name" || return $?
        else
            if [[ $name == read ]]; then reason=RRESP; else reason=BRESP; fi
            check_log "$log" fault "$reason" "$name" || return $?
        fi
        cat "$log.normalized" >>"$OUT/aggregate.log"
    done
}

: >"$OUT/aggregate.log"
run_case read RESPONSE_READ
run_case write RESPONSE_WRITE
echo "GENERATED_MEMAXI_RESPONSE_TESTS PASS: $OUT" | tee -a "$OUT/aggregate.log"
