#!/usr/bin/env bash
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

record_output() {
  local bambu_record_output
  # Installed and uninstalled CMake layouts.
  for bambu_record_output in "${BAMBU_HLS}/bin/record_output" "${BAMBU_HLS}/src/record_output"; do
    if [ -x "${bambu_record_output}" ]; then
      "${bambu_record_output}" "$@"
      return $?
    fi
  done
  echo "record_output: the Bambu log copier is missing in ${BAMBU_HLS}/bin and ${BAMBU_HLS}/src: check BAMBU_HLS" >&2
  return 127
}

run_logged() {
  local log_file="$1"
  shift
  local -a pipeline_status
  # Capture both statuses even when the caller enables errexit or pipefail.
  if "$@" 2>&1 | record_output "${log_file}"; then
    pipeline_status=("${PIPESTATUS[@]}")
  else
    pipeline_status=("${PIPESTATUS[@]}")
  fi
  if [ "${pipeline_status[0]}" -ne 0 ]; then
    echo "ERROR: producer failed (${pipeline_status[0]})" >&2
    return "${pipeline_status[0]}"
  fi
  if [ "${pipeline_status[1]}" -ne 0 ]; then
    echo "ERROR: log writer failed (${pipeline_status[1]})" >&2
    return "${pipeline_status[1]}"
  fi
}
export -f record_output run_logged
