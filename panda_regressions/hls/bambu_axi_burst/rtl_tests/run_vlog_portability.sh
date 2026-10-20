#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
DEFAULT_OUT="$PWD/out_axi_core_vlog_portability"
if (($# > 1)); then
    echo "usage: $0 [fresh-output-directory]" >&2
    exit 2
fi
OUT=${1:-$DEFAULT_OUT}

if [[ -e "$OUT" || -L "$OUT" ]]; then
    echo "output path must be fresh and must not be a symlink: $OUT" >&2
    exit 2
fi

for tool in python3 vlib vlog timeout sha256sum tail; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "missing required tool: $tool" >&2
        exit 2
    }
done

OUT=$(realpath -m "$OUT")
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

PYTHON=$(realpath "$(command -v python3)")
VLIB_CMD=$(command -v vlib)
VLOG_CMD=$(command -v vlog)
VLIB_RESOLVED=$(realpath "$VLIB_CMD")
VLOG_RESOLVED=$(realpath "$VLOG_CMD")
TIMEOUT=$(realpath "$(command -v timeout)")
SHA256SUM=$(realpath "$(command -v sha256sum)")
GIT_HEAD=$(git -C "$ROOT" rev-parse HEAD)

sha256() { "$SHA256SUM" "$1" | awk '{print $1}'; }
print_log_tail() {
    local log_path=$1
    echo "--- last 20 lines: $log_path ---" >&2
    tail -n 20 "$log_path" >&2 || true
}
tool_version() {
    local name=$1 path=$2 flag=$3 result
    result=$("$path" "$flag" 2>&1 || true)
    printf '%s\n' "$result" | head -n 3
}

{
    printf 'git_head=%s\n' "$GIT_HEAD"
    printf 'repo_root=%q\n' "$ROOT"
    printf 'python3=%q\n' "$PYTHON"
    printf 'python3_version=%q\n' "$($PYTHON --version 2>&1)"
    printf 'vlib_invocation=%q\n' "$VLIB_CMD"
    printf 'vlib_resolved=%q\n' "$VLIB_RESOLVED"
    printf 'vlib_version=%q\n' "$(tool_version vlib "$VLIB_CMD" -version | tr '\n' ' ')"
    printf 'vlog_invocation=%q\n' "$VLOG_CMD"
    printf 'vlog_resolved=%q\n' "$VLOG_RESOLVED"
    printf 'vlog_version=%q\n' "$(tool_version vlog "$VLOG_CMD" -version | tr '\n' ' ')"
    printf 'timeout=%q\n' "$TIMEOUT"
    printf 'sha256sum=%q\n' "$SHA256SUM"
    printf 'sha256_xml=%s\n' "$(sha256 "$XML")"
    printf 'sha256_extractor=%s\n' "$(sha256 "$EXTRACTOR")"
    printf 'sha256_runner=%s\n' "$(sha256 "$SCRIPT_DIR/run_vlog_portability.sh")"
    printf 'sha256_python3=%s\n' "$(sha256 "$PYTHON")"
    printf 'sha256_vlib_invocation=%s\n' "$(sha256 "$VLIB_CMD")"
    printf 'sha256_vlib_resolved=%s\n' "$(sha256 "$VLIB_RESOLVED")"
    printf 'sha256_vlog_invocation=%s\n' "$(sha256 "$VLOG_CMD")"
    printf 'sha256_vlog_resolved=%s\n' "$(sha256 "$VLOG_RESOLVED")"
    printf 'sha256_timeout=%s\n' "$(sha256 "$TIMEOUT")"
    printf 'sha256_sha256sum=%s\n' "$(sha256 "$SHA256SUM")"
} > "$OUT/provenance.txt"

variants=(
    MinimalAXI4MasterPipelined:sync:active-low
    MinimalAXI4MasterPipelined:sync:active-high
    MinimalAXI4MasterPipelined:async:active-low
    MinimalAXI4MasterPipelined:async:active-high
    MinimalAXI4MasterPipelinedWrite:sync:active-low
    MinimalAXI4MasterPipelinedWrite:sync:active-high
    MinimalAXI4MasterPipelinedWrite:async:active-low
    MinimalAXI4MasterPipelinedWrite:async:active-high
)

for variant in "${variants[@]}"; do
    IFS=: read -r target reset_mode reset_polarity <<< "$variant"
    name="${target}_${reset_mode}_${reset_polarity}"
    variant_dir="$OUT/$name"
    mkdir -p "$variant_dir/workdir"
    hdl="$OUT/$name.v"
    {
        printf '%q ' "$PYTHON" -B "$EXTRACTOR" --xml "$XML" --target "$target" \
            --reset-mode "$reset_mode" --reset-polarity "$reset_polarity" --out "$hdl"
        printf '\n'
        printf 'cd %q && %q %q\n' "$variant_dir/workdir" "$VLIB_CMD" work
        printf 'cd %q && %q 60s %q -work work %q\n' "$variant_dir/workdir" \
            "$TIMEOUT" "$VLOG_CMD" "$hdl"
    } >> "$OUT/commands.txt"
    if ! "$PYTHON" -B "$EXTRACTOR" --xml "$XML" --target "$target" \
        --reset-mode "$reset_mode" --reset-polarity "$reset_polarity" --out "$hdl" \
        > "$variant_dir/extract.log" 2>&1; then
        echo "AXI_CORE_VLOG FAIL VARIANT=$name stage=extract log=$variant_dir/extract.log" >&2
        exit 1
    fi
    printf '%s  %s\n' "$(sha256 "$hdl")" "$hdl" >> "$OUT/extracted_hdl_sha256.txt"
    (
        cd "$variant_dir/workdir"
        "$VLIB_CMD" work
    ) > "$variant_dir/vlib.log" 2>&1 || {
        echo "AXI_CORE_VLOG FAIL VARIANT=$name stage=vlib log=$variant_dir/vlib.log" >&2
        print_log_tail "$variant_dir/vlib.log"
        exit 1
    }
    (
        cd "$variant_dir/workdir"
        "$TIMEOUT" 60s "$VLOG_CMD" -work work "$hdl"
    ) > "$variant_dir/vlog.log" 2>&1 || {
        echo "AXI_CORE_VLOG FAIL VARIANT=$name stage=vlog log=$variant_dir/vlog.log" >&2
        print_log_tail "$variant_dir/vlog.log"
        exit 1
    }
    printf 'AXI_CORE_VLOG PASS VARIANT=%s\n' "$name" | tee -a "$OUT/results.log"
done

printf 'AXI_CORE_VLOG_PORTABILITY_PASS: 8 variants\n' | tee "$OUT/aggregate.log"
