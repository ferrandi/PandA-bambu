#!/usr/bin/env bash
# Manually qualify generated RTL for independent read/write duplex traffic.
set -Eeuo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
exec bash "$script_dir/run_duplex_generated_family.sh" independent "$0" "$@"
