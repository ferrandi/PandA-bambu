#!/usr/bin/env bash
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

run_logged() {
  local log_file="$1"
  shift
  local bambu_record_output
  # Installed and uninstalled CMake layouts.
  for bambu_record_output in "${BAMBU_HLS}/bin/record_output" "${BAMBU_HLS}/src/record_output"; do
    if [ -x "${bambu_record_output}" ]; then
      break
    fi
  done
  # Probe in the producer's environment before connecting its output to a pipe.
  if [ ! -x "${bambu_record_output}" ] || ! "${bambu_record_output}" --help </dev/null >/dev/null 2>&1; then
    echo "WARNING: record_output not found or cannot start in ${BAMBU_HLS}; logging only to ${log_file}" >&2
    "$@" > "${log_file}" 2>&1
    return $?
  fi
  local -a pipeline_status
  # Capture both statuses even when the caller enables errexit or pipefail.
  if "$@" 2>&1 | "${bambu_record_output}" "${log_file}"; then
    pipeline_status=("${PIPESTATUS[@]}")
  else
    pipeline_status=("${PIPESTATUS[@]}")
  fi
  if [ "${pipeline_status[0]}" -ne 0 ]; then
    echo "ERROR: producer failed (${pipeline_status[0]}); log: ${log_file}" >&2
    return "${pipeline_status[0]}"
  fi
  if [ "${pipeline_status[1]}" -ne 0 ]; then
    echo "ERROR: log writer failed (${pipeline_status[1]}); log: ${log_file}" >&2
    return "${pipeline_status[1]}"
  fi
}
export -f run_logged
