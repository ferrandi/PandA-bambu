#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))
import execution_worktree


class WorktreeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        self.repo.mkdir()
        (self.repo / "tracked").write_text("base")
        for args in (["init"], ["config", "user.email", "test@example.invalid"], ["config", "user.name", "test"], ["add", "."]):
            subprocess.run(["git", *args], cwd=self.repo, check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["git", "commit", "-m", "content"], cwd=self.repo, check=True, stdout=subprocess.DEVNULL)
        self.base = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=self.repo, text=True).strip()

    def tearDown(self):
        shutil.rmtree(self.repo / "agentic-state", ignore_errors=True)
        self.tmp.cleanup()

    def test_exact_commit_creates_retained_detached_worktree(self):
        managed = execution_worktree.create_worktree(self.repo, "run-one", self.base)
        self.assertEqual(managed.path, self.repo / "agentic-state" / "executions" / "run-one" / "worktree")
        self.assertEqual(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=managed.path, text=True).strip(), self.base)
        self.assertRaises(subprocess.CalledProcessError, subprocess.check_call, ["git", "symbolic-ref", "-q", "HEAD"], cwd=managed.path)
        self.assertIn(str(managed.path), subprocess.check_output(["git", "worktree", "list"], cwd=self.repo, text=True))
        execution_worktree.verify_worktree(managed)

    def test_non_exact_revisions_rejected(self):
        for value in ("master", "HEAD", self.base[:8], self.base + "^", "-x"):
            with self.assertRaises(execution_worktree.WorktreeError):
                execution_worktree.create_worktree(self.repo, "run-" + str(len(value)), value)

    def test_inventory_categories_and_unusual_paths(self):
        managed = execution_worktree.create_worktree(self.repo, "inventory", self.base)
        unusual = managed.path / "untracked-\u00e9\nname"
        unusual.write_text("x")
        (managed.path / "tracked").write_text("changed")
        ignored = managed.path / "ignored"
        ignored.write_text("ignored")
        (managed.path / ".gitignore").write_text("ignored\n")
        entries = execution_worktree.inventory(
            managed, frozenset({unusual.name.encode()})
        ).entries
        by_path = {entry.path: entry for entry in entries}
        self.assertEqual(by_path[b"tracked"].kind, "tracked-modification")
        self.assertEqual(by_path[b"ignored"].kind, "ignored")
        self.assertEqual(by_path[unusual.name.encode()].kind, "untracked")
        self.assertFalse(by_path[unusual.name.encode()].unexpected)
        self.assertEqual(entries, tuple(sorted(entries)))

    def test_unrelated_caller_change_is_detected(self):
        managed = execution_worktree.create_worktree(self.repo, "caller", self.base)
        (self.repo / "outside").write_text("changed")
        self.assertFalse(execution_worktree.caller_preserved(managed))
        with self.assertRaises(execution_worktree.WorktreeError):
            execution_worktree.verify_worktree(managed)

    def test_unsafe_local_git_configuration_rejected(self):
        for index, (key, value) in enumerate(
            (
                ("filter.x.clean", "program"),
                ("filter.x.smudge", "program"),
                ("filter.x.process", "program"),
                ("core.fsmonitor", "program"),
            )
        ):
            subprocess.run(
                ["git", "config", "--local", key, value],
                cwd=self.repo,
                check=True,
            )
            with self.assertRaises(execution_worktree.WorktreeError):
                execution_worktree.create_worktree(
                    self.repo, f"unsafe-{index}", self.base
                )
            subprocess.run(
                ["git", "config", "--local", "--unset", key],
                cwd=self.repo,
                check=True,
            )

    def test_symlinked_state_root_rejected(self):
        outside = Path(self.tmp.name) / "outside"
        outside.mkdir()
        (self.repo / "agentic-state").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(execution_worktree.WorktreeError):
            execution_worktree.create_worktree(self.repo, "linked", self.base)


class CallerStatusParserTests(unittest.TestCase):
    def test_only_exact_untracked_or_ignored_namespace_is_excluded(self):
        raw = (
            b"? agentic-state/file\0"
            b"! agentic-state/ignored\0"
            b"? other-agentic-state/file\0"
            b"? agentic-state-other/file\0"
            b"! unrelated-ignored\0"
        )
        filtered = execution_worktree._filtered_caller_status(raw)
        self.assertNotIn(b"? agentic-state/file\0", filtered)
        self.assertNotIn(b"! agentic-state/ignored\0", filtered)
        self.assertIn(b"? other-agentic-state/file\0", filtered)
        self.assertIn(b"? agentic-state-other/file\0", filtered)
        self.assertIn(b"! unrelated-ignored\0", filtered)

    def test_tracked_state_namespace_and_unusual_bytes_are_retained(self):
        tracked = (
            b"1 .M N... 100644 100644 100644 "
            + b"0" * 40
            + b" "
            + b"0" * 40
            + b" agentic-state/tracked"
        )
        unusual = b"? line\nnon-ascii-\xff"
        filtered = execution_worktree._filtered_caller_status(
            tracked + b"\0" + unusual + b"\0"
        )
        self.assertIn(tracked + b"\0", filtered)
        self.assertIn(unusual + b"\0", filtered)


if __name__ == "__main__":
    unittest.main()