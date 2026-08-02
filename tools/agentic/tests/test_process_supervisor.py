#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))
import execution_worktree
import process_supervisor


class SupervisorTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        self.repo.mkdir()
        (self.repo / "seed").write_text("x")
        for args in (["init"], ["config", "user.email", "test@example.invalid"], ["config", "user.name", "test"], ["add", "."], ["commit", "-m", "base"]):
            subprocess.run(["git", *args], cwd=self.repo, check=True, stdout=subprocess.DEVNULL)
        base = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=self.repo, text=True).strip()
        self.worktree = execution_worktree.create_worktree(self.repo, "supervisor", base)

    def tearDown(self):
        self.tmp.cleanup()

    def command(self, code: str):
        return process_supervisor.TrustedCommand.create(
            Path(sys.executable).resolve(),
            ("-c", code),
            worktree=self.worktree,
        )

    def test_cwd_literal_arguments_and_secret_exclusion(self):
        os.environ["PAF05A2_SECRET"] = "do-not-pass"
        result = process_supervisor.run(self.command("import os, pathlib; print(pathlib.Path.cwd()); print(os.getenv('PAF05A2_SECRET'))"), self.worktree)
        self.assertEqual(result.process_exit_code, 0)
        self.assertIn(str(self.worktree.path).encode(), result.stdout)
        self.assertIn(b"None", result.stdout)

    def test_stdout_stderr_and_limit(self):
        result = process_supervisor.run(self.command("import sys; sys.stdout.write('a'*10000); sys.stderr.write('b'*10000)"), self.worktree, stdout_limit_bytes=64, stderr_limit_bytes=64, total_output_limit_bytes=100)
        self.assertTrue(result.output_limit_exceeded)
        self.assertLessEqual(len(result.stdout), 64)
        self.assertLessEqual(len(result.stderr), 64)
        self.assertLessEqual(len(result.stdout) + len(result.stderr), 100)
        self.assertGreater(
            result.stdout_observed_bytes + result.stderr_observed_bytes,
            len(result.stdout) + len(result.stderr),
        )

    def test_timeout(self):
        result = process_supervisor.run(self.command("import time; time.sleep(5)"), self.worktree, timeout_seconds=0.05)
        self.assertTrue(result.timed_out)

    def test_cancellation_and_nonzero_exit(self):
        cancellation = threading.Event()
        cancellation.set()
        cancelled = process_supervisor.run(
            self.command("import time; time.sleep(5)"),
            self.worktree,
            cancellation=cancellation,
        )
        self.assertTrue(cancelled.cancelled)
        nonzero = process_supervisor.run(
            self.command("raise SystemExit(7)"), self.worktree
        )
        self.assertEqual(nonzero.process_exit_code, 7)

    def test_command_boundaries_and_literal_metacharacters(self):
        with self.assertRaises(process_supervisor.ProcessSupervisorError):
            process_supervisor.TrustedCommand.create(
                Path("python"), worktree=self.worktree
            )
        target_executable = self.worktree.path / "target-command"
        target_executable.write_text("#!/bin/sh\nexit 0\n")
        target_executable.chmod(0o700)
        with self.assertRaises(process_supervisor.ProcessSupervisorError):
            process_supervisor.TrustedCommand.create(
                target_executable, worktree=self.worktree
            )
        literal = ";$(touch should-not-exist)"
        command = process_supervisor.TrustedCommand.create(
            Path(sys.executable).resolve(),
            ("-c", "import sys; print(sys.argv[1])", literal),
            worktree=self.worktree,
        )
        result = process_supervisor.run(command, self.worktree)
        self.assertIn(literal.encode(), result.stdout)
        self.assertFalse((self.worktree.path / "should-not-exist").exists())

    @unittest.skipUnless(sys.platform.startswith("linux"), "requires Linux /proc")
    def test_timeout_terminates_descendant_process_group(self):
        pid_file = Path(self.tmp.name) / "descendant.pid"
        code = (
            "import pathlib, subprocess, sys, time; "
            "child=subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(30)']); "
            f"pathlib.Path({str(pid_file)!r}).write_text(str(child.pid)); "
            "time.sleep(30)"
        )
        result = process_supervisor.run(
            self.command(code), self.worktree, timeout_seconds=0.5
        )
        self.assertTrue(result.timed_out)
        descendant = int(pid_file.read_text())
        for _ in range(50):
            if not Path(f"/proc/{descendant}").exists():
                break
            time.sleep(0.02)
        self.assertFalse(Path(f"/proc/{descendant}").exists())

    def test_controller_identity_file_and_mutation(self):
        script = Path(self.tmp.name) / "controller.py"
        script.write_text("print('controller')\n")
        command = process_supervisor.TrustedCommand.create(
            Path(sys.executable).resolve(),
            (str(script),),
            (script,),
            worktree=self.worktree,
        )
        self.assertEqual(command.identity_files[0].path, script)
        self.assertIsNotNone(command.controller_revision)
        script.write_text("print('changed')\n")
        with self.assertRaises(process_supervisor.ProcessSupervisorError):
            process_supervisor.run(command, self.worktree)

    def test_target_change_is_inventoried_without_a3_artifacts(self):
        result = process_supervisor.run(
            self.command("from pathlib import Path; Path('child-output').write_text('x')"),
            self.worktree,
        )
        self.assertTrue(result.caller_preserved)
        self.assertTrue(result.target_preserved)
        self.assertIn(
            b"child-output",
            [entry.path for entry in result.inventory.entries],
        )
        for path in (
            "agentic/fixtures/executions/fixture-local-output.txt",
            "execution-receipt.json",
            "result.jsonl",
            "evidence.json",
        ):
            self.assertFalse((self.worktree.path / path).exists())


if __name__ == "__main__":
    unittest.main()