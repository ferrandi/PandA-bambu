#!/usr/bin/env bash
set -euo pipefail

suite=${1:?Expected short or long Ramulator suite}
case "$suite" in
   short|long) ;;
   *) echo "Unknown Ramulator suite: $suite" >&2; exit 2 ;;
esac
results_dir=${2:?Expected a fresh directory for reports}
mkdir -- "$results_dir"

# Do not bind mount the runner workspace. Files created by the container
# stay in its volume even if the job is cancelled.
container=$(docker create --init --workdir /work --volume /work \
   --env RAMULATOR_BUILD_JOBS --env RAMULATOR_TEST_JOBS ubuntu:24.04 sleep infinity)
trap 'docker rm -fv "$container" >/dev/null' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
docker start "$container" >/dev/null

tar -cf - . | docker exec -i "$container" tar --no-same-owner -xf -

status=0
docker exec -i "$container" bash -eo pipefail -s -- "$suite" <<'BENCHMARK' || status=$?
suite=$1
tools=(ca-certificates git curl wget xz-utils build-essential cmake python3 verilator gettext-base)
if [[ "$suite" == long ]]; then
   tools+=(clang binutils bc)
fi
apt-get update
apt-get install --no-install-recommends -y "${tools[@]}"
if [[ "$suite" == short ]]; then
   python3 -B -m unittest discover -s panda_regressions/hls/bambu_ramulator -p test_runner.py -v
fi
bash panda_regressions/hls/bambu_ramulator/prepare_ramulator.sh /work/ramulator-build
source panda_dist/settings.sh
export RAMULATOR2_DIR=/work/ramulator-build/install
if [[ "$suite" == long ]]; then
   ./examples/ramulator-examples/run_tests.sh --all --no-compare --tag full-ci \
      2>&1 | tee ramulator-long.log
   exit
fi

status=0
python3 -B panda_regressions/verilog/ramulator/run_tests.py \
   --bambu-prefix "$PWD/panda_dist" --out "$PWD/out_ramulator_rtl" \
   --junit "$PWD/out_ramulator_rtl/junit.xml" || status=$?
if (( status == 0 )); then
   python3 panda_regressions/hls/bambu_ramulator/run_tests.py \
      --bambu "$PWD/panda_dist/bin/bambu" --out "$PWD/out_ramulator_ci" -j "$RAMULATOR_TEST_JOBS" \
      --max-cycles 5000 --timeout 180 --hbm-max-cycles 12000 --hbm-timeout 600 || status=$?
fi
# The cache suite is independent of the RTL and HLS results.
cache_status=0
python3 -B panda_regressions/hls/bambu_bus_cache/run_tests.py --ramulator \
   --bambu "$PWD/panda_dist/bin/bambu" --out "$PWD/out_ramulator_cache" --jobs "$RAMULATOR_TEST_JOBS" \
   --junit "$PWD/out_ramulator_cache/junit.xml" || cache_status=$?
if (( status != 0 )); then
   exit "$status"
fi
exit "$cache_status"
BENCHMARK

# Export reports after a benchmark failure as well. The runner extracts
# them under its own account without restoring ownership from the container.
report_status=0
docker exec -i "$container" bash -eo pipefail -s -- "$suite" "$status" <<'REPORTS' | \
   tar --no-same-owner --no-same-permissions -xf - -C "$results_dir" || report_status=$?
{
   if [[ -d ramulator-build ]]; then
      find ramulator-build -maxdepth 1 -type f \( -name '*.log' -o -name provenance.json \) -print0
   fi
   if [[ "$1" == short ]]; then
      for output in out_ramulator_rtl out_ramulator_ci out_ramulator_cache; do
         if [[ -d "$output" ]]; then
            find "$output" -type f \( -name junit.xml -o -name 'summary.*' -o -name run.log \
               -o -name command.txt -o -name input.xml -o -name evidence.json \
               -o -name 'bambu_results*.xml' \) -print0
         fi
      done
      if (( $2 != 0 )) && [[ -d out_ramulator_ci ]]; then
         find out_ramulator_ci -type f -name test.vcd -print0
      fi
   else
      if [[ -f ramulator-long.log ]]; then
         printf '%s\0' ramulator-long.log
      fi
      runs=examples/ramulator-examples/.runs
      if [[ -d "$runs" ]]; then
         find "$runs" -type f \( -name batch.json -o -name metadata.json -o -name summary.csv \
            -o -path '*/cases/*.json' -o -path '*/logs/*.gz' -o -path '*/logs/*.xml' \
            -o -path '*/config_snapshot/*' \( -name inputs.json -o -name bambu_flags.json \
               -o -name '*.yaml' -o -name '*.yml' \) \) -print0
      fi
   fi
} | tar -cf - --null -T -
REPORTS

if (( report_status != 0 )); then
   echo "Failed to export Ramulator reports (status $report_status)." >&2
fi
if (( status != 0 )); then
   exit "$status"
fi
exit "$report_status"
