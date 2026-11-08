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
SIM_JOBS=${SIM_JOBS:-1}
[[ $SIM_JOBS =~ ^[0-9]+$ ]] || { echo "SIM_JOBS must be a positive integer (got '$SIM_JOBS')" >&2; exit 2; }
while [[ $SIM_JOBS == 0* && ${#SIM_JOBS} -gt 1 ]]; do SIM_JOBS=${SIM_JOBS#0}; done
[[ $SIM_JOBS != 0 ]] || { echo "SIM_JOBS must be a positive integer (got '$SIM_JOBS')" >&2; exit 2; }
if ((${#SIM_JOBS} > 6)); then SIM_JOBS=6; else SIM_JOBS=$((10#$SIM_JOBS)); fi
(( SIM_JOBS <= 6 )) || SIM_JOBS=6

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
    if [[ $name == read* ]]; then
        local expected_max=16
        [[ $name != read_b1_o3 ]] || expected_max=1
        grep -Eq "Configured read burst regions for bundle src \\(B=$expected_max\\)" "$gen/bambu.stderr.log" || {
            echo "read kernel did not enable its requested burst path" >&2; return 1;
        }
        if [[ $name == read_b1_o3 ]]; then
            grep -Eq 'AXI_BURST_PROFILE root=kernel bundle=src enabled=1 burst_type=INCREMENTAL\(device\) read_max=1\(single-beat-default:outstanding\+fifo-hints\) read_outstanding=3\(explicit\) read_fifo=8\(explicit\)' \
                "$gen/bambu.stderr.log" || { echo "B1/O3 read profile missing from effective log" >&2; return 1; }
        fi
        grep -q 'MinimalAXI4MasterPipelined' "$rtl" || { echo "read engine missing from generated RTL" >&2; return 1; }
    else
        local expected_max=16
        [[ $name != write_b1_o3 ]] || expected_max=1
        grep -Eq "Configured write burst region for bundle output \\(B=$expected_max\\)" "$gen/bambu.stderr.log" || {
            echo "write kernel did not enable its requested burst path" >&2; return 1;
        }
        if [[ $name == write_b1_o3 ]]; then
            grep -Eq 'AXI_BURST_PROFILE root=write_count_seed_single_beat bundle=output enabled=1 burst_type=INCREMENTAL\(device\).*write_max=1\(single-beat-default:outstanding\+fifo-hints\) write_outstanding=3\(explicit\) write_fifo=8\(explicit\)' \
                "$gen/bambu.stderr.log" || { echo "B1/O3 write profile missing from effective log" >&2; return 1; }
        fi
        grep -q 'MinimalAXI4MasterPipelinedWrite' "$rtl" || { echo "write engine missing from generated RTL" >&2; return 1; }
    fi
    sha256sum "$rtl" "$lib" >>"$OUT/$name/provenance.txt"
}

check_log() {
    local log=$1 mode=$2 engine=$3 reset_type=${4:-} reset_level=${5:-}
    local -a args=(--check-log "$log" "$mode" "$engine")
    if [[ $mode == reset_pending ]]; then
        args+=("$reset_type" "$reset_level")
    elif [[ $mode == reset_* ]]; then
        args+=("$reset_type" "$reset_level" "${mode#reset_}")
    fi
    python3 -B "$TESTS/test_generated_memaxi_response_runner.py" "${args[@]}"
}

run_case() {
    local name=$1 define=$2 reset_type=${3:-} reset_level=${4:-}
    local engine
    local gen="$OUT/$name/generated" sim="$OUT/$name/modelsim" top=tb_generated_memaxi_response
    local rtl_top
    case $name in
        read|read_multi|read_b1_o3|read_reset_*) engine=read; rtl_top=kernel ;;
        write_multi|write_reset_*) engine=write; rtl_top=write_count_seed_three ;;
        write_b1_o3) engine=write; rtl_top=write_count_seed_single_beat ;;
        write) engine=write; rtl_top=write_count_seed ;;
        *) echo "unknown generated MEMAXI case: $name" >&2; return 2 ;;
    esac
    local -a vlog_defines=("+define+$define")
    [[ $name != write_multi && $name != write_reset_* ]] || vlog_defines+=("+define+RESPONSE_WRITE_MULTI")
    [[ $name != write_b1_o3 ]] || vlog_defines+=("+define+RESPONSE_WRITE_B1O3")
    modelsim_compile "$sim" "$top" "$gen/$rtl_top.v" \
        "$gen/panda_libtech.v" "$OUT/common/TestbenchMEMAXI.sv" "$BACKEND" "$TB" -- "${vlog_defines[@]}" || {
        echo "ModelSim compile failed for generated $name wrapper; see $sim/compile.log" >&2
        return 1
    }
    if grep -Eq '^\*\* (Error|Warning):' "$sim/compile.log"; then
        cat "$sim/compile.log" >&2
        echo "ModelSim compilation emitted errors or warnings for generated $name wrapper" >&2
        return 1
    fi
    sha256sum "$sim/compile.log" >>"$OUT/$name/provenance.txt"
    local case_mode log
    local -a case_modes=(control fault)
    if [[ $name == *_reset_* ]]; then
        if [[ $engine == read ]]; then
            case_modes=(reset_pending reset_ar)
        else
            case_modes=(reset_pending reset_aw reset_w)
        fi
    elif [[ $name == *_multi ]]; then
        case_modes=(multi)
    elif [[ $name == *_b1_o3 ]]; then
        case_modes=(b1_o3)
    fi
    for case_mode in "${case_modes[@]}"; do
        log="$OUT/$name/$case_mode.log"
        local -a runtime_args=()
        [[ $case_mode != fault && $case_mode != multi ]] || runtime_args+=(+FAULT +AXI_BURST_EXPECT_FAULT)
        [[ $case_mode != multi ]] || runtime_args+=(+MULTI)
        [[ $case_mode != b1_o3 ]] || runtime_args+=(+B1O3)
        if [[ $case_mode == reset_pending || $case_mode == reset_ar ||
              $case_mode == reset_aw || $case_mode == reset_w ]]; then
            runtime_args+=(+RESET_PENDING)
            [[ $reset_level != high ]] || runtime_args+=(+RESET_ACTIVE_HIGH)
            [[ $reset_type != async ]] || runtime_args+=(+RESET_ASYNC)
        fi
        case $case_mode in
            reset_ar) runtime_args+=(+RESET_STALL_CHANNEL=AR) ;;
            reset_aw) runtime_args+=(+RESET_STALL_CHANNEL=AW) ;;
            reset_w) runtime_args+=(+RESET_STALL_CHANNEL=W) ;;
        esac
        local status=0
        modelsim_run "$sim" "$top" "$log" 120 -- "${runtime_args[@]}" || status=$?
        if ((status != 0)); then
            cat "$log" >&2
            echo "ModelSim generated $name/$case_mode failed (status=$status)" >&2
            return 1
        fi
        if [[ $case_mode == control ]]; then
            check_log "$log" control "$engine" || return $?
        elif [[ $case_mode == multi ]]; then
            check_log "$log" drain "$engine" || return $?
        elif [[ $case_mode == b1_o3 ]]; then
            check_log "$log" b1o3 "$engine" || return $?
        elif [[ $case_mode == reset_pending ]]; then
            check_log "$log" reset_pending "$engine" "$reset_type" "$reset_level" || return $?
        elif [[ $case_mode == reset_* ]]; then
            check_log "$log" "$case_mode" "$engine" "$reset_type" "$reset_level" || return $?
        else
            check_log "$log" fault "$engine" || return $?
        fi
        modelsim_normalize_log "$log" "$log.normalized"
    done
}

run_case_job() {
    local name=$1 source top define
    local -a extra_args=()
    mkdir -p "$OUT/$name/tmp"
    export TMPDIR="$OUT/$name/tmp" TMP="$OUT/$name/tmp" TEMP="$OUT/$name/tmp"
    : >"$OUT/$name/provenance.txt"
    local reset_type= reset_level=
    case $name in
        read) source=$READ_SOURCE; top=kernel; define=RESPONSE_READ ;;
        read_multi) source=$READ_SOURCE; top=kernel; define=RESPONSE_READ; extra_args=(--extra-cc-options=-DBAMBU_AXI_SUM_O5) ;;
        read_b1_o3) source=$READ_SOURCE; top=kernel; define=RESPONSE_READ; extra_args=(--extra-cc-options=-DBAMBU_AXI_SUM_B1_O3) ;;
        read_reset_*) source=$READ_SOURCE; top=kernel; define=RESPONSE_READ; extra_args=(--extra-cc-options=-DBAMBU_AXI_SUM_O5)
            reset_type=${name#read_reset_}; reset_level=${reset_type#*_}; reset_type=${reset_type%%_*} ;;
        write) source=$WRITE_SOURCE; top=write_count_seed; define=RESPONSE_WRITE ;;
        write_multi) source=$WRITE_SOURCE; top=write_count_seed_three; define=RESPONSE_WRITE ;;
        write_b1_o3) source=$WRITE_SOURCE; top=write_count_seed_single_beat; define=RESPONSE_WRITE ;;
        write_reset_*) source=$WRITE_SOURCE; top=write_count_seed_three; define=RESPONSE_WRITE; reset_type=${name#write_reset_}; reset_level=${reset_type#*_}; reset_type=${reset_type%%_*} ;;
        *) echo "unknown generated MEMAXI case: $name" >&2; return 2 ;;
    esac
    if [[ -n $reset_type ]]; then extra_args+=("--reset-type=$reset_type" "--reset-level=$reset_level"); fi
    generate_kernel "$name" "$source" "$top" "${extra_args[@]}" || return $?
    if [[ $name == write_multi ]] && ! grep -Fq 'write_max=16(explicit) write_outstanding=3(explicit)' \
        "$OUT/write_multi/generated/bambu.stderr.log"; then
        echo "multi-write kernel did not resolve the required B16/O3 profile" >&2
        return 1
    fi
    if [[ -n $reset_type ]]; then
        run_case "$name" "$define" "$reset_type" "$reset_level"
    else
        run_case "$name" "$define"
    fi
}

source "$ROOT/panda_regressions/hls/bambu_axi_burst/parallel_helpers.sh"
BURST_JOB_LOG_DIR="$OUT/jobs"
mkdir -p "$BURST_JOB_LOG_DIR"
if ! run_bounded_jobs "$SIM_JOBS" run_case_job read read_multi read_b1_o3 write write_multi write_b1_o3 \
    read_reset_sync_low read_reset_sync_high read_reset_async_low read_reset_async_high \
    write_reset_sync_low write_reset_sync_high write_reset_async_low write_reset_async_high; then
    echo "generated MEMAXI cases failed; case logs retained under $OUT/jobs" >&2
    exit 1
fi

: >"$OUT/aggregate.log"
for name in read read_multi read_b1_o3 write write_multi write_b1_o3 \
    read_reset_sync_low read_reset_sync_high read_reset_async_low read_reset_async_high \
    write_reset_sync_low write_reset_sync_high write_reset_async_low write_reset_async_high; do
    cat "$OUT/$name/provenance.txt" >>"$OUT/provenance.txt"
    case $name in
        read|write) case_modes=(control fault) ;;
        *_multi) case_modes=(multi) ;;
        *_b1_o3) case_modes=(b1_o3) ;;
        read_reset_*) case_modes=(reset_pending reset_ar) ;;
        write_reset_*) case_modes=(reset_pending reset_aw reset_w) ;;
    esac
    for case_mode in "${case_modes[@]}"; do
        cat "$OUT/$name/$case_mode.log.normalized" >>"$OUT/aggregate.log"
    done
done
echo "GENERATED_MEMAXI_RESPONSE_TESTS PASS: $OUT" | tee -a "$OUT/aggregate.log"
