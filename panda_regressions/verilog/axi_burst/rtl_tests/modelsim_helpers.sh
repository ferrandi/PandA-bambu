#!/usr/bin/env bash

# Shared Questa/ModelSim setup for isolated RTL testbench builds.
# Each build directory owns its `work` library and modelsim.ini.

modelsim_resolve() {
    # Keep PATH and backend lookup identical to Bambu's launcher.
    # shellcheck disable=SC1091
    source "$ROOT/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
    if ! bambu_modelsim_resolve; then
        echo "ModelSim vsim could not be resolved using Bambu's search path" >&2
        return 2
    fi
    MODELSIM_SOURCE=$BAMBU_MODELSIM_SOURCE
    MODELSIM_VSIM=$BAMBU_MODELSIM_PATH
    MODELSIM_BIN=$BAMBU_MODELSIM_BIN

    MODELSIM_VLIB="$MODELSIM_BIN/vlib"
    MODELSIM_VLOG="$MODELSIM_BIN/vlog"
    MODELSIM_VMAP="$MODELSIM_BIN/vmap"
    local tool
    for tool in "$MODELSIM_VSIM" "$MODELSIM_VLIB" "$MODELSIM_VLOG" "$MODELSIM_VMAP"; do
        if [[ ! -x $tool ]]; then
            echo "required ModelSim sibling tool is not executable: $tool" >&2
            return 2
        fi
    done
    export PATH="$MODELSIM_BIN${PATH:+:$PATH}"
}

modelsim_version() {
    local output
    output=$(timeout 30s "$MODELSIM_VSIM" -version 2>&1) || return $?
    printf '%s\n' "$output" | sed -n '1,3p'
}

modelsim_compile() {
    local build=$1 top=$2
    shift 2
    local -a sources=() extra_vlog_args=()
    local extra=0 arg
    for arg in "$@"; do
        if [[ $arg == -- && $extra == 0 ]]; then
            extra=1
        elif (( extra )); then
            extra_vlog_args+=("$arg")
        else
            sources+=("$arg")
        fi
    done
    if ((${#sources[@]} == 0)); then
        echo "at least one Verilog source is required for $top" >&2
        return 2
    fi
    local log="$build/compile.log"
    mkdir -p "$build" || return $?
    # A private mapping prevents an inherited MODELSIM setting from directing
    # this group's work library into another concurrent group's directory.
    if ! (cd "$build" && timeout 180s "$MODELSIM_VMAP" -c >vmap.log 2>&1 &&
        timeout 180s "$MODELSIM_VLIB" work >vlib.log 2>&1 &&
        timeout 180s "$MODELSIM_VMAP" -modelsimini "$build/modelsim.ini" \
            work "$build/work" >>vmap.log 2>&1); then
        cat "$build/vmap.log" "$build/vlib.log" >&2 2>/dev/null || true
        echo "ModelSim library setup failed for $build" >&2
        return 1
    fi

    local -a vlog_args=(-sv -modelsimini "$build/modelsim.ini" -work work -timescale 1ns/1ps
                        "${extra_vlog_args[@]}")
    local status=0
    printf '%q ' "$MODELSIM_VLOG" "${vlog_args[@]}" "${sources[@]}" >"$build/compile.command" || return $?
    printf '\n' >>"$build/compile.command" || return $?
    if (cd "$build" && timeout 180s "$MODELSIM_VLOG" "${vlog_args[@]}" \
        "${sources[@]}" >"$log" 2>&1); then
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

modelsim_run() {
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
    printf '%q ' "$MODELSIM_VSIM" -c -modelsimini "$build/modelsim.ini" "${elaboration_args[@]}" "work.$top" \
        "${runtime_args[@]}" -do "$do_file" >"$log.command" || return $?
    printf '\n' >>"$log.command" || return $?
    if (cd "$build" && timeout "${timeout_s}s" "$MODELSIM_VSIM" -c -modelsimini "$build/modelsim.ini" \
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

modelsim_normalize_log() {
    local source=$1 destination=$2
    # Questa prefixes HDL transcript output with '# '. Remove only that
    # prefix; keep diagnostics and marker contents otherwise byte-for-byte.
    sed -E 's/^# //' "$source" >"$destination"
}

