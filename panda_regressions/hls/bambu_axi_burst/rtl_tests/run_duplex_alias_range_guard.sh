#!/usr/bin/env bash
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
tb="$script_dir/tb_duplex_alias_range_guard.sv"

usage() { echo "usage: $0 GENERATED_KERNEL_V FRESH_OUTPUT_DIR" >&2; }
if (($# != 2)); then usage; exit 2; fi
if [[ ${BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing Verilator RTL probe unless BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 is set" >&2
  exit 2
fi

kernel_arg=$1
if [[ $kernel_arg == */* ]]; then
  kernel=$(realpath "$kernel_arg" 2>/dev/null || true)
else
  kernel=$(realpath -m -- "$kernel_arg")
fi
[[ -n ${kernel:-} && -f $kernel && -s $kernel && ! -L $kernel ]] || {
  echo "generated kernel RTL is missing, empty, or a symlink: $kernel_arg" >&2; exit 2;
}
libtech="$(dirname "$kernel")/panda_libtech.v"
[[ -f $libtech && -s $libtech && ! -L $libtech ]] || {
  echo "generated Panda library RTL is missing, empty, or a symlink: $libtech" >&2; exit 2;
}
[[ -r $tb ]] || { echo "range-guard testbench is missing: $tb" >&2; exit 2; }
verilator=$(command -v verilator || true)
[[ -n $verilator && -x $verilator ]] || { echo "verilator is required" >&2; exit 2; }
verilator=$(realpath "$verilator")

out_arg=$2
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")
mkdir -p "$(dirname "$out")"
mkdir "$out"
out=$(realpath "$out")
mkdir "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"

binary="$out/obj/Vtb_duplex_alias_range_guard"
argv=(--binary --timing --trace --top-module tb_duplex_alias_range_guard --Mdir "$out/obj"
  -Wno-fatal -Wno-PINMISSING "$kernel" "$libtech" "$tb")
{
  printf 'kernel=%s\nkernel_sha256=%s\nlibtech=%s\nlibtech_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nrunner=%s\nrunner_sha256=%s\nverilator=%s\nverilator_sha256=%s\n' \
    "$kernel" "$(sha256sum -- "$kernel" | awk '{print $1}')" \
    "$libtech" "$(sha256sum -- "$libtech" | awk '{print $1}')" \
    "$tb" "$(sha256sum -- "$tb" | awk '{print $1}')" \
    "$(realpath "${BASH_SOURCE[0]}")" "$(sha256sum -- "${BASH_SOURCE[0]}" | awk '{print $1}')" \
    "$verilator" "$(sha256sum -- "$verilator" | awk '{print $1}')"
  printf 'argv='; printf '%q ' "$verilator" "${argv[@]}"; printf '\n'
} >"$out/provenance.txt"

if ! "$verilator" "${argv[@]}" >"$out/build.log" 2>&1; then
  tail -n 100 "$out/build.log" >&2 || true
  echo "Verilator range-guard probe build failed; see $out" >&2
  exit 1
fi
[[ -x $binary ]] || { echo "Verilator did not produce expected probe binary: $binary" >&2; exit 1; }
cases=(valid-disjoint read-end-overflow write-end-overflow byte-count-overflow read-end-equals-2p32)
: >"$out/probe.log"
for case_name in "${cases[@]}"; do
  if ! (cd "$out" && "$binary" "+CASE=$case_name") >>"$out/probe.log" 2>&1; then
    cat "$out/probe.log" >&2 || true
    echo "Verilator range-guard probe failed for $case_name; see $out" >&2
    exit 1
  fi
  [[ -s "$out/range_guard_$case_name.vcd" ]] || {
    echo "range-guard probe missing VCD trace for $case_name" >&2; exit 1;
  }
done
[[ $(rg -F -c 'DUPLEX_ALIAS_RANGE_GUARD_PASS CASES=1 NO_AXI_HANDSHAKES=1' "$out/probe.log") == 5 ]] || {
  echo "range-guard probe did not complete exactly five isolated cases" >&2
  cat "$out/probe.log" >&2 || true
  exit 1
}
expected=(
  'DUPLEX_ALIAS_RANGE_PASS CASE=valid-disjoint ARLEN=15'
  'DUPLEX_ALIAS_RANGE_PASS CASE=read-end-overflow ARLEN=0'
  'DUPLEX_ALIAS_RANGE_PASS CASE=write-end-overflow ARLEN=0'
  'DUPLEX_ALIAS_RANGE_PASS CASE=byte-count-overflow ARLEN=0'
  'DUPLEX_ALIAS_RANGE_PASS CASE=read-end-equals-2p32 ARLEN=15'
)
for marker in "${expected[@]}"; do
  rg -F -x -q -- "$marker" "$out/probe.log" || {
    echo "range-guard probe missing marker: $marker" >&2
    cat "$out/probe.log" >&2 || true
    exit 1
  }
done
printf 'DUPLEX_ALIAS_RANGE_PROBE PASS CASES=5 RTL_SHA256=%s OUTPUT=%s\n' \
  "$(sha256sum -- "$kernel" | awk '{print $1}')" "$out"
