#!/usr/bin/env bash
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
set -euo pipefail

: "${OPENMP_BATCH_SCRIPT:?test script is required}"
: "${OPENMP_BATCH_CONFIGURATIONS:?batch configurations are required}"
report_dir="${OPENMP_REPORT_DIR:-test-reports}"
dist_dir="${OPENMP_DIST_DIR:-panda_dist}"
# Preserve the runner's concurrency setting; mantis defaults to one job.
export J="${J:-1}"
[[ "$J" =~ ^[1-9][0-9]*$ ]] || { echo "J must be a positive integer" >&2; exit 1; }
# Installed settings do not necessarily define every optional shell variable.
set +u
source "${dist_dir}/settings.sh"
set -u
mkdir -p "$report_dir"
out_dir="$PWD/$report_dir"
printf 'test-reports-dir=%s\n' "$out_dir" >> "${GITHUB_OUTPUT:-/dev/null}"
read -r -a inherited_args <<< "${OPENMP_EXTRA_ARGS:-}"
IFS=',' read -r -a configurations <<< "$OPENMP_BATCH_CONFIGURATIONS"
for configuration in "${configurations[@]}"; do
   IFS='_ ' read -r device period additional_options <<< "$configuration"
   [[ -n "$device" && -n "$period" ]] || { echo "Invalid batch: $configuration" >&2; exit 1; }
   read -r -a batch_args <<< "$additional_options"
   suffix="$(printf '%s' "$additional_options" | tr -cs '[:alnum:]_.-' '_')"
   config_out="$out_dir/${device}_${period}${suffix:+_$suffix}"
   mkdir -p "$config_out"
   # Keep the workdir beneath this runner's workspace.
   test_dir="test_out"
   rm -rf -- "$test_dir"
   common_args=("-c=--device-name=$device" "-c=--clock-period=$period")
   if [[ -n "${OPENMP_BATCH_COMPILER:-}" ]]; then
      common_args+=("-c=--compiler=$OPENMP_BATCH_COMPILER")
   fi
   for option in "${batch_args[@]}"; do common_args+=("-c=$option"); done
   printf '::group::Run %s at %s %s\n' "$device" "$period" "$additional_options"
   bash "$OPENMP_BATCH_SCRIPT" -o "$test_dir" --returnfail \
      --junitdir="$config_out" --summary="$config_out/perf.xml" \
      "${common_args[@]}" "${inherited_args[@]}"
   echo '::endgroup::'
done
