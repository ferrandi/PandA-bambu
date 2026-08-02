#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))
import execution_worktree
import local_state


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

    def _assert_include_rejected_before_filter(self, key: str) -> None:
        marker = Path(self.tmp.name) / "filter-executed"
        included = Path(self.tmp.name) / "included.config"
        included.write_text(
            "[filter \"attack\"]\n"
            f"\tsmudge = sh -c 'touch {marker}'\n"
        )
        subprocess.run(["git", "config", "--local", key, str(included)],
                       cwd=self.repo, check=True)
        with self.assertRaisesRegex(execution_worktree.WorktreeError, "unsafe-repository-config"):
            execution_worktree.create_worktree(self.repo, "include-rejected", self.base)
        self.assertFalse(marker.exists(), "untrusted smudge filter must never execute")

    def test_include_path_with_filter_is_rejected_before_execution(self):
        self._assert_include_rejected_before_filter("include.path")

    def test_include_if_path_with_filter_is_rejected_before_execution(self):
        self._assert_include_rejected_before_filter("includeIf.gitdir:*/repo/.path")

    def test_worktree_specific_unsafe_configuration_rejected(self):
        subprocess.run(["git", "config", "extensions.worktreeConfig", "true"],
                       cwd=self.repo, check=True)
        supported = subprocess.run(
            ["git", "config", "--worktree", "filter.attack.smudge", "program"],
            cwd=self.repo, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        ).returncode == 0
        if not supported:
            self.skipTest("installed Git does not support worktree config")
        with self.assertRaisesRegex(execution_worktree.WorktreeError, "unsafe-repository-config"):
            execution_worktree.create_worktree(self.repo, "worktree-config", self.base)

    def test_symbolic_ref_failure_is_not_detached_in_snapshot_or_verification(self):
        identity = execution_worktree.resolve_git()
        original = execution_worktree._git_result

        def fail_symbolic(*args, **kwargs):
            if args[2] == ["symbolic-ref", "-q", "HEAD"]:
                return subprocess.CompletedProcess(args[2], 128, b"", b"fatal: injected")
            return original(*args, **kwargs)

        with mock.patch.object(execution_worktree, "_git_result", side_effect=fail_symbolic):
            with self.assertRaisesRegex(execution_worktree.WorktreeError, "symbolic-head-failed"):
                execution_worktree._snapshot(identity, self.repo)
        managed = execution_worktree.create_worktree(self.repo, "symbolic-verify", self.base)
        with mock.patch.object(execution_worktree, "_git_result", side_effect=fail_symbolic):
            with self.assertRaisesRegex(execution_worktree.WorktreeError, "symbolic-head-failed"):
                execution_worktree.verify_worktree(managed)

    def _dirty_status_after_lock(self, commands: list[list[str]], *, fail_remove: bool = False):
        original = execution_worktree._git
        locked = False

        def controlled_git(identity, repository, args, **kwargs):
            nonlocal locked
            commands.append(args)
            if args[:2] == ["worktree", "lock"]:
                locked = True
            if (
                locked
                and args[0] == "-C"
                and args[2:] == ["status", "--porcelain=v2", "-z", "--untracked-files=no"]
            ):
                return b"1 .M N... 100644 100644 100644 " + b"0" * 40 + b" " + b"0" * 40 + b" tracked\0"
            if fail_remove and args[:3] == ["worktree", "remove", "--force"]:
                raise execution_worktree.WorktreeError("injected-remove-failure")
            return original(identity, repository, args, **kwargs)

        return controlled_git

    def test_post_lock_failure_unlocks_removes_and_reuses_execution_id(self):
        unrelated = Path(self.tmp.name) / "unrelated"
        subprocess.run(["git", "worktree", "add", "--detach", str(unrelated), self.base],
                       cwd=self.repo, check=True, stdout=subprocess.DEVNULL)
        commands: list[list[str]] = []
        with mock.patch.object(
            execution_worktree, "_git",
            side_effect=self._dirty_status_after_lock(commands),
        ):
            with self.assertRaisesRegex(execution_worktree.WorktreeError, "initial-checkout-not-clean"):
                execution_worktree.create_worktree(self.repo, "reusable", self.base)
        target = self.repo / "agentic-state" / "executions" / "reusable" / "worktree"
        records = execution_worktree._worktree_records(execution_worktree.resolve_git(), self.repo)
        self.assertFalse(target.exists())
        self.assertNotIn(os.fsencode(target), records)
        self.assertFalse(target.parent.exists())
        self.assertIn(os.fsencode(unrelated), records)
        self.assertIn(["worktree", "unlock", str(target)], commands)
        self.assertIn(["worktree", "remove", "--force", str(target)], commands)
        self.assertFalse(any(args[:2] == ["worktree", "prune"] for args in commands))
        managed = execution_worktree.create_worktree(self.repo, "reusable", self.base)
        self.assertTrue(managed.path.exists())

    def test_cleanup_failure_retains_post_lock_forensics_and_rejects_execution_id_reuse(self):
        commands: list[list[str]] = []
        with mock.patch.object(
            execution_worktree, "_git",
            side_effect=self._dirty_status_after_lock(commands, fail_remove=True),
        ):
            with self.assertRaises(execution_worktree.WorktreeCleanupError) as caught:
                execution_worktree.create_worktree(self.repo, "retained", self.base)
        target = self.repo / "agentic-state" / "executions" / "retained" / "worktree"
        error = caught.exception
        self.assertIn("initial-checkout-not-clean", str(error.original))
        self.assertIn("injected-remove-failure", str(error.cleanup))
        self.assertIn("retained-worktree:" + str(target), str(error))
        self.assertIn("operator-action:", str(error))
        self.assertTrue(target.exists())
        self.assertIn(
            os.fsencode(target),
            execution_worktree._worktree_records(execution_worktree.resolve_git(), self.repo),
        )
        self.assertIn(["worktree", "unlock", str(target)], commands)
        with self.assertRaisesRegex(execution_worktree.WorktreeError, "execution-id-already-used"):
            execution_worktree.create_worktree(self.repo, "retained", self.base)
        subprocess.run(["git", "worktree", "remove", "--force", str(target)],
                       cwd=self.repo, check=True)

    def test_valueless_benign_configuration_is_accepted(self):
        config = self.repo / ".git" / "config"
        with config.open("a") as handle:
            handle.write("\n[foo]\n\tbar\n")
        managed = execution_worktree.create_worktree(self.repo, "valueless-benign", self.base)
        self.assertTrue(managed.path.exists())

    def test_valueless_unsafe_configuration_is_rejected(self):
        identity = execution_worktree.resolve_git()
        config = self.repo / ".git" / "config"
        for key in (b"core.fsmonitor", b"include.path", b"includeif.gitdir:*/repo/.path"):
            with self.subTest(key=key):
                with mock.patch.object(
                    execution_worktree, "_repository_config_paths", return_value=(config,)
                ), mock.patch.object(
                    execution_worktree, "_config_entries", return_value=[(key, b"")]
                ):
                    with self.assertRaisesRegex(
                        execution_worktree.WorktreeError, "unsafe-repository-config"
                    ):
                        execution_worktree._reject_unsafe_config(identity, self.repo)

    def test_symlinked_state_root_rejected(self):
        outside = Path(self.tmp.name) / "outside"
        outside.mkdir()
        (self.repo / "agentic-state").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(execution_worktree.WorktreeError):
            execution_worktree.create_worktree(self.repo, "linked", self.base)


class LocalStateDirectoryTests(unittest.TestCase):
    def test_prepare_directory_does_not_chmod_unrelated_siblings_or_ancestors(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            root = base / "owned-root"
            sibling = base / "unrelated-directory"
            file = base / "unrelated-file"
            root.mkdir(mode=0o755)
            sibling.mkdir(mode=0o755)
            file.write_text("unchanged")
            os.chmod(root, 0o755)
            os.chmod(sibling, 0o755)
            os.chmod(file, 0o644)
            local_state.prepare_directory(root, root / "created" / "child")
            self.assertEqual(os.stat(root).st_mode & 0o777, 0o755)
            self.assertEqual(os.stat(sibling).st_mode & 0o777, 0o755)
            self.assertEqual(os.stat(file).st_mode & 0o777, 0o644)
            self.assertEqual(file.read_text(), "unchanged")


class ConfigParserTests(unittest.TestCase):
    def test_valueless_record_has_an_implicit_empty_value(self):
        identity = execution_worktree.resolve_git()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "config"
            path.write_text("")
            with mock.patch.object(execution_worktree, "_git", return_value=b"foo.bar\0"):
                self.assertEqual(
                    execution_worktree._config_entries(identity, Path(temporary), path),
                    [(b"foo.bar", b"")],
                )

    def test_malformed_config_records_fail_closed(self):
        identity = execution_worktree.resolve_git()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "config"
            path.write_text("")
            for output in (b"\0", b"\nvalue\0", b"foo.bar"):
                with self.subTest(output=output):
                    with mock.patch.object(execution_worktree, "_git", return_value=output):
                        with self.assertRaisesRegex(execution_worktree.WorktreeError, "repository-config-output-invalid"):
                            execution_worktree._config_entries(identity, Path(temporary), path)


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