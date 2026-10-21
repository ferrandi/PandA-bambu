#!/usr/bin/env bash
# Compiler-generation gate for the always-on fast STORE acknowledgement.
#
# There is no compiler switch anymore: every generated m_axi burst write engine carries
# FAST_STORE_ACK(1) and the libtech module declares the parameter with default 1. This gate proves
# the wiring in the duplex wrapper and in the standalone write-only interface, and proves that the
# historical option name is inert (accepted, ignored, still fast-ack).
set -Eeuo pipefail

tests=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
root=$(realpath "$tests/../../..")
checker="$tests/check_duplex_fast_store_ack_artifacts.py"
duplex_source="$tests/bambu_axi_burst_duplex_copy.cpp"
write_source="$tests/write_compiler_cases/write_cases.cpp"
read_source="$tests/bambu_axi_burst_sum_o1_lat128.cpp"
legacy_option="experimental-m-axi-duplex-fast-store-ack"

usage() { echo "Usage: $0 BAMBU [OUTPUT_DIR] (default: ./out_duplex_fast_store_ack)"; }
if [[ ${1:-} == -h || ${1:-} == --help ]]; then usage; exit 0; fi
if (($# < 1 || $# > 2)); then usage >&2; exit 2; fi

bambu_arg=$1
out_arg=${2:-"$PWD/out_duplex_fast_store_ack"}
if [[ $bambu_arg == */* ]]; then
  bambu=$(realpath "$bambu_arg" 2>/dev/null || true)
else
  bambu=$(command -v "$bambu_arg" || true)
  [[ -z $bambu ]] || bambu=$(realpath "$bambu")
fi
[[ -n ${bambu:-} && -x $bambu ]] || { echo "Bambu executable not found: $bambu_arg" >&2; exit 2; }
[[ ! -e $out_arg && ! -L $out_arg ]] || { echo "output path must be fresh and not a symlink: $out_arg" >&2; exit 2; }
for tool in python3 sha256sum; do command -v "$tool" >/dev/null || { echo "required tool not found: $tool" >&2; exit 2; }; done
for file in "$checker" "$tests/run_duplex_fast_store_ack_tests.sh" \
    "$duplex_source" "$write_source" "$read_source"; do
  [[ -f $file && ! -L $file ]] || { echo "required regression input missing: $file" >&2; exit 2; }
done

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
if [[ -r $settings ]]; then
  # shellcheck disable=SC1090
  source "$settings"
fi
[[ -n ${BAMBU_HLS:-} ]] || { echo "BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin" >&2; exit 2; }
plugin="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $plugin ]] || { echo "selected ASTAnalyzer plugin is missing: $plugin" >&2; exit 2; }

out=$(realpath -m -- "$out_arg")
mkdir -p -- "$(dirname -- "$out")"
mkdir -- "$out"
mkdir -p "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum "$1" | awk '{print $1}'; }
git_head=$(git -C "$root" rev-parse HEAD)
{
  printf 'git_head=%s\nbambu=%s\nbambu_sha256=%s\nplugin=%s\nplugin_sha256=%s\n' \
    "$git_head" "$bambu" "$(digest "$bambu")" "$plugin" "$(digest "$plugin")"
  for file in "$checker" "$tests/run_duplex_fast_store_ack_tests.sh" \
      "$duplex_source" "$write_source" "$read_source"; do
    printf 'input=%s sha256=%s\n' "$file" "$(digest "$file")"
  done
  printf 'legacy_option=%s\n' "$legacy_option"
} >"$out/provenance.txt"

common=(--compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33
  --AXI-burst-type=INCREMENTAL --bambu-parameter=experimental-m-axi-burst=1
  --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

run_case() {
  local name=$1 source=$2 top=$3 kind=$4
  local write_burst=$5 write_outstanding=$6 write_fifo=$7
  local read_burst=$8 read_outstanding=$9 read_fifo=${10}
  local profile=${11:-} legacy=${12:-}
  local case_dir="$out/$name"
  [[ ! -e $case_dir && ! -L $case_dir ]] || { echo "case output must be fresh: $case_dir" >&2; return 2; }
  mkdir -p "$case_dir/tmp"
  local -a args=("--top-fname=$top" "${common[@]}")
  [[ -z $profile ]] || args+=("--extra-cc-options=-DDUPLEX_PROFILE=$profile")
  [[ -z $legacy ]] || args+=("--bambu-parameter=${legacy_option}=${legacy}")
  {
    printf 'case=%s\nsource=%s\nsource_sha256=%s\nkind=%s\n' "$name" "$source" \
      "$(digest "$source")" "$kind"
    printf 'legacy_option_value=%s\n' "${legacy:-none}"
    printf 'argv='; printf '%q ' "$bambu" "$source" "${args[@]}"; printf '\n'
  } >"$case_dir/provenance.txt"
  if ! (cd "$case_dir" && TMPDIR="$case_dir/tmp" TMP="$case_dir/tmp" TEMP="$case_dir/tmp" \
      "$bambu" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    echo "$name compilation failed; see $case_dir" >&2
    return 1
  fi
  if [[ -n $legacy ]]; then
    python3 -B "$checker" inert-option --output "$case_dir"
  else
    local -a check_args=(success --output "$case_dir" --kind "$kind")
    if [[ $kind != read-only ]]; then
      check_args+=(--write-burst "$write_burst" --write-outstanding "$write_outstanding"
        --write-fifo "$write_fifo")
    fi
    if [[ $kind != write-only ]]; then
      check_args+=(--read-burst "$read_burst" --read-outstanding "$read_outstanding"
        --read-fifo "$read_fifo")
    fi
    python3 -B "$checker" "${check_args[@]}"
  fi
  printf 'DUPLEX_FAST_STORE_ACK PASS CASE=%s\n' "$name" | tee -a "$out/aggregate.log"
}

# Both duplex profiles carry FAST_STORE_ACK(1); reader profiles keep the pragma geometry.
run_case duplex-symmetric "$duplex_source" kernel duplex 16 1 32 16 1 64 1
run_case duplex-asymmetric "$duplex_source" kernel duplex 8 1 16 16 2 64 2
run_case duplex-symmetric-repeat "$duplex_source" kernel duplex 16 1 32 16 1 64 1

# The standalone (write only) writer carries the same fast acknowledgement, and the generated
# libtech module must declare the parameter with default 1.
run_case write-only "$write_source" write_count write-only 16 1 32 16 1 64
run_case read-only "$read_source" kernel read-only 16 1 32 16 1 256

# Same write-engine profile, including FAST_STORE_ACK(1), in the duplex and in the standalone flow.
python3 -B "$checker" compare-writers --left "$out/duplex-symmetric" --right "$out/write-only"
python3 -B "$checker" compare-readers --left "$out/duplex-symmetric" --right "$out/duplex-symmetric-repeat"

# The removed option name is accepted but inert: the kernel stays fast-ack.
run_case legacy-option-numeric "$duplex_source" kernel duplex 16 1 32 16 1 64 1 1
run_case legacy-option-text "$write_source" write_count write-only 16 1 32 16 1 64 "" fast

printf 'DUPLEX_FAST_STORE_ACK PASS: 7 compiler cases and profile comparisons\n' | tee -a "$out/aggregate.log"
