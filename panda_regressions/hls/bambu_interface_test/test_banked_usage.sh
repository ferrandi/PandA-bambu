#!/bin/bash
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
export TMPDIR="$repo_root/documentation/tmp" TMP="$repo_root/documentation/tmp" TEMP="$repo_root/documentation/tmp"
export OMP_NUM_THREADS=1
mkdir -p "$TMPDIR"
scratch_dir="$(mktemp -d "$TMPDIR/banked-usage.XXXXXX")"
trap 'rm -rf -- "$scratch_dir"' EXIT
source "$repo_root/install/settings.sh"
for mode in INVALID_CHUNK INVALID_DUPLICATE_BANK INVALID_BANK_ID INVALID_BANK_COUNT; do
   mkdir "$scratch_dir/$mode"
   pushd "$scratch_dir/$mode" >/dev/null
   if timeout 120 bambu "$script_dir/banked_sparse_bundles.cpp" "-D$mode" \
      --top-fname=banked_copy --compiler=I386_CLANG16 -O2 --generate-interface=INFER \
      --memory-allocation-policy=NO_BRAM > run.log 2>&1; then
      echo "FAIL: $mode was accepted" >&2
      exit 1
   fi
   case "$mode" in
      INVALID_CHUNK) expected='Invalid bank chunk' ;;
      INVALID_BANK_COUNT) expected='too many or no banks' ;;
      *) expected='duplicate or out-of-range bank' ;;
   esac
   if ! grep -q "$expected" run.log; then
      tail -n 15 run.log >&2
      echo "FAIL: $mode failed without the expected usage diagnostic" >&2
      exit 1
   fi
   echo "PASS: $mode"
   popd >/dev/null
done
