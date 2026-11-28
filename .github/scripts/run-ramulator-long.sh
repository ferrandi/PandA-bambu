#!/usr/bin/env bash
set -euo pipefail

results_dir=${1:?Expected a fresh directory for benchmark reports}
mkdir -- "$results_dir"

# Do not bind mount the runner workspace. Files created by the container
# stay in its volume even if the job is cancelled.
container=$(docker create --init --workdir /work --volume /work \
   --env RAMULATOR_BUILD_JOBS ubuntu:24.04 sleep infinity)
trap 'docker rm -fv "$container" >/dev/null' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
docker start "$container" >/dev/null

tar -cf - . | docker exec -i "$container" tar --no-same-owner -xf -

status=0
docker exec -i "$container" bash -eo pipefail -s <<'BENCHMARK' || status=$?
apt-get update
apt-get install --no-install-recommends -y \
   ca-certificates git curl wget xz-utils build-essential cmake python3 \
   clang verilator binutils gettext-base bc
bash panda_regressions/hls/bambu_ramulator/prepare_ramulator.sh /work/ramulator-build
source panda_dist/settings.sh
export RAMULATOR2_DIR=/work/ramulator-build/install
./examples/ramulator-examples/run_tests.sh --all --no-compare --tag full-ci \
   2>&1 | tee ramulator-long.log
BENCHMARK

# Export reports after a benchmark failure as well. The runner extracts
# them under its own account without restoring ownership from the container.
report_status=0
docker exec -i "$container" bash -eo pipefail -s <<'REPORTS' | \
   tar --no-same-owner --no-same-permissions -xf - -C "$results_dir" || report_status=$?
{
   if [[ -f ramulator-long.log ]]; then
      printf '%s\0' ramulator-long.log
   fi
   if [[ -d ramulator-build ]]; then
      find ramulator-build -maxdepth 1 -type f \( -name '*.log' -o -name provenance.json \) -print0
   fi
   runs=examples/ramulator-examples/.runs
   if [[ -d "$runs" ]]; then
      find "$runs" -type f \( -name batch.json -o -name metadata.json -o -name summary.csv \
         -o -path '*/cases/*.json' -o -path '*/logs/*.gz' -o -path '*/logs/*.xml' \
         -o -path '*/config_snapshot/*' \( -name inputs.json -o -name bambu_flags.json \
            -o -name '*.yaml' -o -name '*.yml' \) \) -print0
   fi
} | tar -cf - --null -T -
REPORTS

if (( report_status != 0 )); then
   echo "Failed to export Ramulator benchmark reports (status $report_status)." >&2
fi
if (( status != 0 )); then
   exit "$status"
fi
exit "$report_status"
