#!/usr/bin/env bash
set -euo pipefail

output_directory=$1
if test -d "${output_directory}"; then
   chmod -R a+rX -- "${output_directory}"
fi
