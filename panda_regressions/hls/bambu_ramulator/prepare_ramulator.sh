#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd -P)
if (( $# < 1 || $# > 2 )); then
    echo "usage: $0 FRESH_OUTPUT_DIRECTORY [--checkout]" >&2
    exit 2
fi
out=$(realpath -m -- "$1")
if [[ -e "$out" ]]; then
    echo "output directory must be fresh: $out" >&2
    exit 2
fi
if (( $# == 2 )); then
    [[ $2 == --checkout ]] || { echo "unknown option: $2" >&2; exit 2; }
    git -C "$root" submodule update --init -- ext/ramulator2-no-spdlog
fi
src="$root/ext/ramulator2-no-spdlog"
jobs=${RAMULATOR_BUILD_JOBS:-${J:-2}}
[[ $jobs =~ ^[1-9][0-9]*$ ]] || { echo "invalid build job count: $jobs" >&2; exit 2; }
mkdir -p "$out"
# An external build directory keeps both dependency downloads and build products
# out of the submodule. No legacy-layout fallback: CI tests the public revision's
# standard installation contract.
cmake -S "$src" -B "$out/build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$out/install" 2>&1 | tee "$out/configure.log"
cmake --build "$out/build" --parallel "$jobs" 2>&1 | tee "$out/build.log"
cmake --install "$out/build" 2>&1 | tee "$out/install.log"
test -f "$out/install/include/ramulator/base/config.h"
"$out/install/bin/ramulator2" --help > "$out/cli-help.log"
python3 - "$src" "$out" <<'PY'
import json, pathlib, subprocess, sys
src, out = map(pathlib.Path, sys.argv[1:])
libraries = sorted((out / 'install').glob('lib*/**/libramulator.so'))
if not libraries:
    raise SystemExit('Ramulator installation did not provide libramulator.so')
(out / 'provenance.json').write_text(json.dumps({
    'revision': subprocess.check_output(['git', '-C', str(src), 'rev-parse', 'HEAD'], text=True).strip(),
    'prefix': str(out / 'install'), 'library': str(libraries[0]),
}, indent=2) + '\n')
PY
