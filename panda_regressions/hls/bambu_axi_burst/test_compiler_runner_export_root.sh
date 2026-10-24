#!/usr/bin/env bash
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(realpath "$script_dir/../../..")
test_tmp=${BURST_TEST_TMPDIR:-}
if [[ -z $test_tmp ]]; then
  echo "BURST_TEST_TMPDIR must point inside the caller's output root" >&2
  exit 2
fi
export TMPDIR="$test_tmp" TMP="$test_tmp" TEMP="$test_tmp"
mkdir -p "$TMPDIR"

export_tree=$(mktemp -d "$TMPDIR/compiler-runner-export-test.XXXXXX")
cleanup() {
  rm -rf -- "$export_tree"
}
trap cleanup EXIT

runner_rel=panda_regressions/hls/bambu_axi_burst/run_compiler_tests.sh
mkdir -p "$export_tree/$(dirname "$runner_rel")"
cp "$script_dir/run_compiler_tests.sh" "$export_tree/$runner_rel"
if [[ -e $export_tree/.git ]]; then
  echo "test export unexpectedly contains .git" >&2
  exit 1
fi

help_output=$(bash "$export_tree/$runner_rel" --help)
grep -Fq 'Output must be a fresh directory.' <<<"$help_output"
printf 'compiler runner no-.git root calculation PASS (%s)\n' "$export_tree"
