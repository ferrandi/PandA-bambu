#!/usr/bin/env python3
"""Bounded controller-side process execution in a managed detached worktree."""
from __future__ import annotations

import hashlib
import os
import selectors
import signal
import stat
import subprocess
import tempfile
import threading
import time
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

import execution_worktree


class ProcessSupervisorError(RuntimeError):
    """Raised for an unsafe trusted-command or launch configuration."""


@dataclass(frozen=True)
class IdentityFile:
    path: Path
    digest: str


@dataclass(frozen=True)
class TrustedCommand:
    executable: Path
    arguments: tuple[str, ...]
    executable_digest: str
    identity_files: tuple[IdentityFile, ...] = ()
    controller_revision: str | None = None

    @classmethod
    def create(cls, executable: Path, arguments: tuple[str, ...] = (),
               identity_files: tuple[Path, ...] = (), *,
               worktree: execution_worktree.ManagedWorktree) -> "TrustedCommand":
        executable = _identity_path(executable, worktree, executable=True)
        if len(arguments) > 64 or any(not isinstance(value, str) or "\0" in value or len(value) > 8192 for value in arguments):
            raise ProcessSupervisorError("invalid-arguments")
        files = tuple(
            IdentityFile(
                _identity_path(value, worktree, executable=False),
                _digest(_identity_path(value, worktree, executable=False)),
            )
            for value in identity_files
        )
        revision = None
        try:
            revision = subprocess.run(
                [str(worktree.git.executable), "-C", str(worktree.repository), "rev-parse", "HEAD"],
                check=True,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                shell=False,
                env={"LANG": "C", "LC_ALL": "C"},
            ).stdout.strip().decode("ascii")
        except (OSError, subprocess.CalledProcessError, UnicodeError):
            pass
        return cls(executable, arguments, _digest(executable), files, revision)


@dataclass(frozen=True)
class ProcessResult:
    launched: bool
    started_at: str
    completed_at: str
    process_exit_code: int | None
    timed_out: bool
    cancelled: bool
    output_limit_exceeded: bool
    stdout: bytes
    stdout_observed_bytes: int
    stdout_truncated: bool
    stderr: bytes
    stderr_observed_bytes: int
    stderr_truncated: bool
    command: TrustedCommand
    worktree: execution_worktree.ManagedWorktree
    diagnostic: str
    inventory: execution_worktree.WorktreeInventory | None
    caller_preserved: bool
    target_preserved: bool


def _now() -> str:
    return datetime.now(UTC).isoformat().replace("+00:00", "Z")


def _digest(path: Path) -> str:
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


def _identity_path(
    path: Path,
    worktree: execution_worktree.ManagedWorktree,
    *,
    executable: bool,
) -> Path:
    if not path.is_absolute():
        raise ProcessSupervisorError("executable-not-absolute")
    supplied = path
    path = path.resolve()
    if supplied != path:
        raise ProcessSupervisorError("identity-path-not-canonical")
    try:
        mode = path.stat().st_mode
    except OSError:
        raise ProcessSupervisorError("identity-file-unavailable") from None
    if not stat.S_ISREG(mode) or (executable and not os.access(path, os.X_OK)):
        raise ProcessSupervisorError("identity-file-invalid")
    if path.is_relative_to(worktree.path):
        raise ProcessSupervisorError("identity-file-in-worktree")
    return path


def _verify(command: TrustedCommand, worktree: execution_worktree.ManagedWorktree) -> None:
    if _identity_path(command.executable, worktree, executable=True) != command.executable or _digest(command.executable) != command.executable_digest:
        raise ProcessSupervisorError("executable-identity-changed")
    for item in command.identity_files:
        if _identity_path(item.path, worktree, executable=False) != item.path or _digest(item.path) != item.digest:
            raise ProcessSupervisorError("identity-file-changed")


def _environment() -> tuple[dict[str, str], tempfile.TemporaryDirectory[str]]:
    directory = tempfile.TemporaryDirectory(prefix="paf05a2-")
    root = Path(directory.name)
    home, temporary = root / "home", root / "tmp"
    home.mkdir(mode=0o700); temporary.mkdir(mode=0o700)
    return {
        "HOME": str(home), "TMPDIR": str(temporary), "LANG": "C", "LC_ALL": "C",
        "TZ": "UTC", "PYTHONIOENCODING": "utf-8", "PYTHONDONTWRITEBYTECODE": "1",
    }, directory


def _terminate(process: subprocess.Popen[bytes]) -> None:
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=0.5)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=0.5)
    except subprocess.TimeoutExpired:
        raise ProcessSupervisorError("process-reap-failed") from None


def run(command: TrustedCommand, worktree: execution_worktree.ManagedWorktree, *,
        timeout_seconds: float = 30.0, cancellation: threading.Event | None = None,
        stdout_limit_bytes: int = 1_000_000, stderr_limit_bytes: int = 1_000_000,
        total_output_limit_bytes: int = 2_000_000) -> ProcessResult:
    if min(timeout_seconds, stdout_limit_bytes, stderr_limit_bytes, total_output_limit_bytes) < 0:
        raise ProcessSupervisorError("invalid-limits")
    execution_worktree.verify_worktree(worktree)
    _verify(command, worktree)
    started = _now()
    env, temporary = _environment()
    stdout = bytearray(); stderr = bytearray()
    observed = {"stdout": 0, "stderr": 0}; truncated = {"stdout": False, "stderr": False}
    timed_out = cancelled = overflow = False
    process: subprocess.Popen[bytes] | None = None
    diagnostic = "completed"
    try:
        try:
            process = subprocess.Popen(
                [str(command.executable), *command.arguments], cwd=str(worktree.path), env=env,
                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                shell=False, start_new_session=True,
            )
        except OSError:
            return ProcessResult(False, started, _now(), None, False, False, False, b"", 0, False, b"", 0, False, command, worktree, "launch-failed", None, False, False)
        selector = selectors.DefaultSelector()
        assert process.stdout is not None and process.stderr is not None
        selector.register(process.stdout, selectors.EVENT_READ, "stdout")
        selector.register(process.stderr, selectors.EVENT_READ, "stderr")
        deadline = time.monotonic() + timeout_seconds
        terminated = False
        while selector.get_map():
            if not terminated and cancellation is not None and cancellation.is_set():
                cancelled = terminated = True; diagnostic = "cancelled"; _terminate(process)
            if not terminated and time.monotonic() >= deadline:
                timed_out = terminated = True; diagnostic = "timeout"; _terminate(process)
            for key, _ in selector.select(0.05):
                data = os.read(key.fileobj.fileno(), 65536)
                if not data:
                    selector.unregister(key.fileobj); continue
                name = key.data; observed[name] += len(data)
                buffer, limit = (stdout, stdout_limit_bytes) if name == "stdout" else (stderr, stderr_limit_bytes)
                available = max(0, limit - len(buffer))
                aggregate = max(0, total_output_limit_bytes - len(stdout) - len(stderr))
                keep = min(len(data), available, aggregate)
                buffer.extend(data[:keep])
                if keep != len(data):
                    truncated[name] = True; overflow = True
                    if not terminated:
                        terminated = True; diagnostic = "output-limit"; _terminate(process)
        process.wait()
    finally:
        if process is not None:
            if process.stdout is not None:
                process.stdout.close()
            if process.stderr is not None:
                process.stderr.close()
        temporary.cleanup()
    caller_preserved = target_preserved = False
    try:
        execution_worktree.verify_worktree(worktree)
        target_preserved = True
        caller_preserved = execution_worktree.caller_preserved(worktree)
        if not caller_preserved:
            raise execution_worktree.WorktreeError("caller-checkout-changed")
        inv = execution_worktree.inventory(worktree)
    except execution_worktree.WorktreeError:
        inv = None
        diagnostic = "post-run-worktree-invalid"
    return ProcessResult(True, started, _now(), process.returncode if process else None, timed_out, cancelled, overflow,
                         bytes(stdout), observed["stdout"], truncated["stdout"], bytes(stderr), observed["stderr"],
                         truncated["stderr"], command, worktree, diagnostic, inv, caller_preserved, target_preserved)
