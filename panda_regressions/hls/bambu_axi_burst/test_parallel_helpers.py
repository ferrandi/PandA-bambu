#!/usr/bin/env python3
"""Behavioral regression tests for the Bash 5.0-compatible parallel helper."""

from __future__ import annotations

import os
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
HELPER = ROOT / "panda_regressions/hls/bambu_axi_burst/parallel_helpers.sh"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", ROOT / "documentation/tmp")) / (
    f"out_{Path(__file__).stem}"
)


class ParallelHelpersTests(unittest.TestCase):
    def setUp(self) -> None:
        SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = SCRATCH_ROOT / uuid.uuid4().hex
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)

    def run_bash(self, body: str) -> subprocess.CompletedProcess[str]:
        script = f"""\
set -u
source {str(HELPER)!r}
wait() {{
  local arg
  for arg in "$@"; do
    if [[ $arg == -p ]]; then
      echo 'Bash 5.0 compatibility shim: wait -p is unavailable' >&2
      return 2
    fi
  done
  builtin wait "$@"
}}
BURST_JOB_LOG_DIR={str(self.scratch / 'logs')!r}
mkdir -p "$BURST_JOB_LOG_DIR"
{body}
"""
        env = os.environ.copy()
        env.update(
            TMPDIR=str(self.scratch), TMP=str(self.scratch), TEMP=str(self.scratch)
        )
        return subprocess.run(
            ["bash", "-c", script],
            cwd=self.scratch,
            env=env,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_overlaps_jobs_and_maps_out_of_order_completions(self) -> None:
        starts = self.scratch / "starts"
        starts.mkdir()
        result = self.run_bash(
            f"""
STARTS={str(starts)!r}
worker() {{
  local name=$1
  : >"$STARTS/$name.started"
  for ((attempt=0; attempt<100; ++attempt)); do
    [[ -e "$STARTS/first.started" && -e "$STARTS/second.started" ]] && break
    sleep 0.01
  done
  [[ -e "$STARTS/first.started" && -e "$STARTS/second.started" ]] || return 91
  if [[ $name == first ]]; then sleep 0.30; else sleep 0.05; fi
  printf 'JOB_BODY=%s\\n' "$name"
}}
run_bounded_jobs 2 worker first second
"""
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS: burst job first", result.stdout)
        self.assertIn("PASS: burst job second", result.stdout)
        self.assertLess(
            result.stdout.index("PASS: burst job second"),
            result.stdout.index("PASS: burst job first"),
            result.stdout,
        )
        self.assertEqual(
            (self.scratch / "logs/first.log").read_text(), "JOB_BODY=first\n"
        )
        self.assertEqual(
            (self.scratch / "logs/second.log").read_text(), "JOB_BODY=second\n"
        )
        self.assertNotIn("wait -p is unavailable", result.stderr)
        self.assertNotIn("bad array subscript", result.stderr)

    def test_simultaneous_completions_are_both_drained(self) -> None:
        result = self.run_bash(
            """
worker() { sleep 0.10; printf 'JOB_BODY=%s\\n' "$1"; }
run_bounded_jobs 2 worker left right
"""
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS: burst job left", result.stdout)
        self.assertIn("PASS: burst job right", result.stdout)
        for name in ("left", "right"):
            self.assertEqual(
                (self.scratch / f"logs/{name}.log").read_text(),
                f"JOB_BODY={name}\n",
            )

    def test_failure_stops_new_jobs_and_waits_for_active_jobs(self) -> None:
        state = self.scratch / "failure-state"
        state.mkdir()
        result = self.run_bash(
            f"""
STATE={str(state)!r}
worker() {{
  local name=$1
  : >"$STATE/$name.started"
  case $name in
    fail) sleep 0.05; printf 'JOB_BODY=fail\\n'; return 7 ;;
    slow) sleep 0.30; : >"$STATE/slow.finished"; printf 'JOB_BODY=slow\\n'; return 0 ;;
    forbidden) : >"$STATE/forbidden.started"; return 0 ;;
  esac
}}
if run_bounded_jobs 2 worker fail slow forbidden; then
  echo 'expected nonzero result' >&2; exit 90
else
  status=$?
fi
[[ $status == 1 ]] || {{ echo "unexpected pool status: $status" >&2; exit 91; }}
[[ -e "$STATE/slow.finished" ]] || {{ echo 'active job was not drained' >&2; exit 92; }}
[[ ! -e "$STATE/forbidden.started" ]] || {{ echo 'queued job started after failure' >&2; exit 93; }}
"""
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("FAIL: burst job fail (status 7)", result.stderr)
        self.assertIn("JOB_BODY=fail", result.stderr)
        self.assertNotIn("bad array subscript", result.stderr)

    def test_refills_free_slot_without_head_of_line_and_respects_limit(self) -> None:
        state = self.scratch / "refill-state"
        state.mkdir()
        result = self.run_bash(
            f"""
STATE={str(state)!r}
mkdir "$STATE/active"
active_count() {{
  find "$STATE/active" -mindepth 1 -maxdepth 1 -type d | wc -l
}}
worker() {{
  local name=$1
  mkdir "$STATE/active/$name"
  : >"$STATE/$name.started"
  case $name in
    first)
      for ((attempt=0; attempt<200; ++attempt)); do
        [[ -e "$STATE/second.started" ]] && break
        sleep 0.01
      done
      [[ -e "$STATE/second.started" ]] || {{ echo 'second job did not start' >&2; return 101; }}
      sleep 0.80
      : >"$STATE/first.finished"
      ;;
    second)
      for ((attempt=0; attempt<200; ++attempt)); do
        [[ -e "$STATE/first.started" ]] && break
        sleep 0.01
      done
      [[ -e "$STATE/first.started" ]] || {{ echo 'first job did not start' >&2; return 102; }}
      active_count >"$STATE/second_active_count"
      sleep 0.15
      : >"$STATE/second.finished"
      ;;
    third)
      active_count >"$STATE/third_active_count"
      if [[ -e "$STATE/second.finished" && ! -e "$STATE/first.finished" && \\
            -d "$STATE/active/first" && ! -d "$STATE/active/second" && \\
            -d "$STATE/active/third" ]]; then
        : >"$STATE/refill_before_first_finished"
      else
        : >"$STATE/refill_order_failed"
      fi
      ;;
  esac
  rmdir "$STATE/active/$name"
}}
run_bounded_jobs 2 worker first second third || {{ echo 'bounded pool failed' >&2; exit 94; }}
[[ -e "$STATE/third.started" ]] || {{ echo 'third job never started' >&2; exit 95; }}
[[ -e "$STATE/refill_before_first_finished" ]] || {{ echo 'third job did not refill before first completed' >&2; exit 96; }}
[[ ! -e "$STATE/refill_order_failed" ]] || {{ echo 'third job started outside the refill window' >&2; exit 97; }}
[[ -e "$STATE/first.finished" && -e "$STATE/second.finished" ]] || {{ echo 'jobs were not drained' >&2; exit 98; }}
second_active_count=0
third_active_count=0
read -r second_active_count <"$STATE/second_active_count"
read -r third_active_count <"$STATE/third_active_count"
printf 'REFILL_WITNESS=PASS\\nSECOND_ACTIVE=%s\\nTHIRD_ACTIVE=%s\\n' \\
  "$second_active_count" "$third_active_count"
[[ $second_active_count == 2 ]] || {{ echo "overlap witness was $second_active_count, expected 2" >&2; exit 99; }}
[[ $third_active_count == 2 ]] || {{ echo "third job started with $third_active_count active jobs, expected 2" >&2; exit 100; }}
"""
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("REFILL_WITNESS=PASS", result.stdout)
        self.assertIn("SECOND_ACTIVE=2", result.stdout)
        self.assertIn("THIRD_ACTIVE=2", result.stdout)
        self.assertTrue((state / "refill_before_first_finished").is_file())
        self.assertTrue((state / "first.finished").is_file())
        self.assertTrue((state / "second.finished").is_file())
        self.assertTrue((state / "third.started").is_file())

    def test_signalled_child_is_reported_with_its_label_and_log(self) -> None:
        result = self.run_bash(
            """
worker() { printf 'JOB_BODY=signal-job\\n'; kill -TERM "$BASHPID"; }
if run_bounded_jobs 1 worker signal-job; then
  echo 'expected signalled child to fail' >&2; exit 90
fi
"""
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("FAIL: burst job signal-job (status 143)", result.stderr)
        self.assertRegex(
            result.stderr.replace("\\", "/"),
            r"/logs/signal-job\.log\nJOB_BODY=signal-job",
        )

    def test_empty_job_list_succeeds(self) -> None:
        result = self.run_bash("run_bounded_jobs 2 worker\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Starting 0 burst helper jobs", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
