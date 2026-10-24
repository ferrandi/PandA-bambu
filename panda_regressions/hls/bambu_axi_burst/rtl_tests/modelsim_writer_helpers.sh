#!/usr/bin/env bash

# Shared Questa/ModelSim setup for the isolated write-engine testbenches.
# Each caller compiles and simulates from its own fresh directory, so the
# implicit `work` library and modelsim.ini are private to that scenario group.

modelsim_writer_resolve() {
    local selected_vsim
    if selected_vsim=$(command -v vsim 2>/dev/null) && [[ -n $selected_vsim ]]; then
        MODELSIM_WRITER_SOURCE=PATH
        MODELSIM_WRITER_VSIM=$selected_vsim
        MODELSIM_WRITER_BIN=${selected_vsim%/*}
    else
        # Keep backend lookup identical to Bambu's own launcher.
        # shellcheck disable=SC1091
        source "$WRITER_ROOT/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
        if ! bambu_modelsim_resolve; then
            echo "ModelSim vsim could not be resolved using Bambu's search path" >&2
            return 2
        fi
        MODELSIM_WRITER_SOURCE=$BAMBU_MODELSIM_SOURCE
        MODELSIM_WRITER_VSIM=$BAMBU_MODELSIM_PATH
        MODELSIM_WRITER_BIN=$BAMBU_MODELSIM_BIN
    fi

    MODELSIM_WRITER_VLIB="$MODELSIM_WRITER_BIN/vlib"
    MODELSIM_WRITER_VLOG="$MODELSIM_WRITER_BIN/vlog"
    MODELSIM_WRITER_VMAP="$MODELSIM_WRITER_BIN/vmap"
    local tool
    for tool in "$MODELSIM_WRITER_VSIM" "$MODELSIM_WRITER_VLIB" "$MODELSIM_WRITER_VLOG" "$MODELSIM_WRITER_VMAP"; do
        if [[ ! -x $tool ]]; then
            echo "required ModelSim sibling tool is not executable: $tool" >&2
            return 2
        fi
    done
    export PATH="$MODELSIM_WRITER_BIN${PATH:+:$PATH}"
}

modelsim_writer_version() {
    local output
    output=$(timeout 30s "$MODELSIM_WRITER_VSIM" -version 2>&1) || return $?
    printf '%s\n' "$output" | sed -n '1,3p'
}

modelsim_writer_compile() {
    local build=$1 top=$2 rtl=$3 tb=$4 plan_fifo=$5
    local log="$build/compile.log"
    mkdir -p "$build" || return $?
    # A private mapping prevents an inherited MODELSIM setting from directing
    # this group's work library into another concurrent group's directory.
    if ! (cd "$build" && timeout 180s "$MODELSIM_WRITER_VMAP" -c >vmap.log 2>&1 &&
        timeout 180s "$MODELSIM_WRITER_VLIB" work >vlib.log 2>&1 &&
        timeout 180s "$MODELSIM_WRITER_VMAP" -modelsimini "$build/modelsim.ini" \
            work "$build/work" >>vmap.log 2>&1); then
        cat "$build/vmap.log" "$build/vlib.log" >&2 2>/dev/null || true
        echo "ModelSim library setup failed for $build" >&2
        return 1
    fi

    local -a vlog_args=(-sv -modelsimini "$build/modelsim.ini" -work work -timescale 1ns/1ps)
    if [[ $plan_fifo == 1 ]]; then
        vlog_args+=(+define+PLAN_FIFO_PRESENT)
    fi
    local status=0
    printf '%q ' "$MODELSIM_WRITER_VLOG" "${vlog_args[@]}" "$rtl" "$tb" >"$build/compile.command" || return $?
    printf '\n' >>"$build/compile.command" || return $?
    if (cd "$build" && timeout 180s "$MODELSIM_WRITER_VLOG" "${vlog_args[@]}" \
        "$rtl" "$tb" >"$log" 2>&1); then
        status=0
    else
        status=$?
    fi
    printf '%s\n' "$status" >"$build/compile.status" || return $?
    if (( status != 0 )); then
        tail -n 100 "$log" >&2 || true
        if (( status == 124 || status == 137 )); then
            echo "ModelSim vlog timed out for $top in $build" >&2
        else
            echo "ModelSim vlog failed for $top (exit=$status) in $build" >&2
        fi
        return 1
    fi
}

modelsim_writer_run() {
    local build=$1 top=$2 log=$3 timeout_s=$4
    shift 4
    local -a elaboration_args=() runtime_args=()
    local in_runtime=0 arg
    for arg in "$@"; do
        if [[ $arg == -- && $in_runtime == 0 ]]; then
            in_runtime=1
            continue
        fi
        if (( in_runtime )); then
            runtime_args+=("$arg")
        else
            elaboration_args+=("$arg")
        fi
    done
    local do_file="$build/run.do"

    # Tcl errors fail immediately. Some Questa versions exit zero after a
    # runtime $fatal, so the transcript diagnostic below also determines the
    # effective status returned to the caller; retain the raw status as well.
    cat >"$do_file" <<'TCL'
onerror {quit -code 1}
run -all
quit -code 0
TCL

    local status=0
    printf '%q ' "$MODELSIM_WRITER_VSIM" -c -modelsimini "$build/modelsim.ini" "${elaboration_args[@]}" "work.$top" \
        "${runtime_args[@]}" -do "$do_file" >"$log.command" || return $?
    printf '\n' >>"$log.command" || return $?
    if (cd "$build" && timeout "${timeout_s}s" "$MODELSIM_WRITER_VSIM" -c -modelsimini "$build/modelsim.ini" \
        "${elaboration_args[@]}" "work.$top" "${runtime_args[@]}" \
        -do "$do_file" >"$log" 2>&1); then
        status=0
    else
        status=$?
    fi
    printf '%s\n' "$status" >"$log.raw-status" || return $?
    if (( status == 0 )) && grep -Eq '^(# )?\*\* (Fatal|Error):' "$log"; then
        status=1
    fi
    printf '%s\n' "$status" >"$log.status" || return $?
    if (( status == 124 || status == 137 )); then
        echo "ModelSim simulation timed out for $top (limit=${timeout_s}s): $log" >&2
        return 124
    fi
    return "$status"
}

modelsim_writer_normalize_log() {
    local source=$1 destination=$2
    # Questa prefixes HDL transcript output with '# '. Remove only that
    # prefix; keep diagnostics and marker contents otherwise byte-for-byte.
    sed -E 's/^# //' "$source" >"$destination"
}

modelsim_writer_expect_fatal() {
    local log=$1 expected=$2 label=$3 status=$4 allow_elaboration=${5:-0} normalized="$1.normalized"
    modelsim_writer_normalize_log "$log" "$normalized" || return $?
    # Runtime fatals use effective status 1; Questa reports constant parameter
    # checks during vopt elaboration with status 12. Only parameter tests opt in.
    if { (( status != 1 )) && { (( ! allow_elaboration || status != 12 )); }; } || \
       ! grep -Fq "** Fatal: $expected" "$normalized" || \
       grep -Eq '^WRITE_BURST_ENGINE PASS CASE=|^WRITE_BURST_ENGINE PARAM PASS ' "$normalized" || \
       [[ $(grep -Ec '^\*\* Fatal:' "$normalized" || true) != 1 ]] || \
       grep -Eq '^\*\* Error:|Segmentation fault|stack smashing|AddressSanitizer|Aborted' "$normalized"; then
        cat "$log" >&2
        echo "expected one named ModelSim fatal and no PASS marker for $label (status=$status): $expected" >&2
        return 1
    fi
}
