#!/usr/bin/env python3
"""Trusted detached worktree support for controller-side execution.

Git is invoked with a deliberately minimal environment and hooks/filters are
blocked before checkout.  This hardens checkout behavior; it is not a complete
filesystem or network sandbox.
"""
from __future__ import annotations

import hashlib
import os
import re
import shutil
import stat
import subprocess
from dataclasses import dataclass
from pathlib import Path

import local_state

_COMMIT = re.compile(r"^[0-9a-f]{40}$")
_STATE_NAMESPACE = b"agentic-state"


class WorktreeError(RuntimeError):
    """Raised when a managed worktree cannot be safely created or verified."""


@dataclass(frozen=True)
class GitIdentity:
    executable: Path
    digest: str
    version: str


@dataclass(frozen=True)
class CallerSnapshot:
    repository: Path
    head: str
    symbolic_head: bytes | None
    index_digest: str | None
    status: bytes
    submodules: bytes


@dataclass(frozen=True, order=True)
class InventoryEntry:
    kind: str
    path: bytes
    unexpected: bool


@dataclass(frozen=True)
class WorktreeInventory:
    entries: tuple[InventoryEntry, ...]


@dataclass(frozen=True)
class ManagedWorktree:
    repository: Path
    path: Path
    execution_id: str
    base_commit: str
    caller_snapshot: CallerSnapshot
    git: GitIdentity
    locked: bool = True


def _digest(path: Path) -> str:
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


def resolve_git() -> GitIdentity:
    value = shutil.which("git")
    if value is None:
        raise WorktreeError("git-not-found")
    path = Path(value).resolve()
    try:
        mode = path.stat().st_mode
    except OSError:
        raise WorktreeError("git-not-regular") from None
    if not stat.S_ISREG(mode) or not os.access(path, os.X_OK):
        raise WorktreeError("git-not-executable")
    try:
        version = subprocess.run(
            [str(path), "--version"], check=True, stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, shell=False,
        ).stdout.decode("ascii", "strict").strip()
    except (OSError, subprocess.CalledProcessError, UnicodeError):
        raise WorktreeError("git-version-failed") from None
    return GitIdentity(path, _digest(path), version)


def _verify_git(identity: GitIdentity, target: Path | None = None) -> None:
    path = identity.executable.resolve()
    if path != identity.executable or (target is not None and path.is_relative_to(target)):
        raise WorktreeError("git-path-changed")
    try:
        mode = path.stat().st_mode
        valid = stat.S_ISREG(mode) and os.access(path, os.X_OK) and _digest(path) == identity.digest
    except OSError:
        valid = False
    if not valid:
        raise WorktreeError("git-identity-changed")


def _environment(repository: Path) -> dict[str, str]:
    hooks = repository / ".agentic-local" / "empty-git-hooks"
    try:
        local_state.prepare_directory(hooks.parent, hooks)
    except local_state.LocalStateError:
        raise WorktreeError("hooks-directory-invalid") from None
    return {
        "PATH": os.defpath,
        "HOME": str(hooks.parent),
        "LANG": "C",
        "LC_ALL": "C",
        "GIT_TERMINAL_PROMPT": "0",
        "GIT_PAGER": "cat",
        "GIT_EXTERNAL_DIFF": "",
        "GIT_CONFIG_NOSYSTEM": "1",
        "GIT_CONFIG_GLOBAL": os.devnull,
        "GIT_OPTIONAL_LOCKS": "0",
        "GIT_CONFIG_COUNT": "5",
        "GIT_CONFIG_KEY_0": "core.hooksPath",
        "GIT_CONFIG_VALUE_0": str(hooks),
        "GIT_CONFIG_KEY_1": "core.fsmonitor",
        "GIT_CONFIG_VALUE_1": "false",
        "GIT_CONFIG_KEY_2": "core.sparseCheckout",
        "GIT_CONFIG_VALUE_2": "false",
        "GIT_CONFIG_KEY_3": "core.sparseCheckoutCone",
        "GIT_CONFIG_VALUE_3": "false",
        "GIT_CONFIG_KEY_4": "core.untrackedCache",
        "GIT_CONFIG_VALUE_4": "false",
    }


def _git(identity: GitIdentity, repository: Path, args: list[str], *, cwd: Path | None = None,
         mutate: bool = False) -> bytes:
    _verify_git(identity)
    try:
        completed = subprocess.run(
            [str(identity.executable), *args], cwd=str(cwd or repository),
            env=_environment(repository), stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False,
            check=False,
        )
    except OSError:
        raise WorktreeError("git-launch-failed") from None
    if completed.returncode:
        raise WorktreeError("git-command-failed")
    return completed.stdout


def _top_level(identity: GitIdentity, repository: Path) -> Path:
    try:
        top = Path(_git(identity, repository, ["rev-parse", "--show-toplevel"]).rstrip(b"\n").decode()).resolve()
    except (UnicodeError, ValueError):
        raise WorktreeError("repository-invalid") from None
    if top != repository.resolve():
        raise WorktreeError("repository-path-mismatch")
    return top


def _reject_unsafe_config(identity: GitIdentity, repository: Path) -> None:
    data = _git(identity, repository, ["config", "--local", "--null", "--list"])
    for item in filter(None, data.split(b"\0")):
        key, _, value = item.partition(b"\n")
        lowered = key.lower()
        filter_command = (
            lowered.startswith(b"filter.")
            and lowered.rsplit(b".", 1)[-1] in {b"clean", b"smudge", b"process"}
        )
        if filter_command and value.strip():
            raise WorktreeError("unsafe-repository-config")
        if lowered == b"core.fsmonitor" and value.strip().lower() not in {
            b"false",
            b"0",
            b"no",
            b"off",
        }:
            raise WorktreeError("unsafe-repository-config")


def _in_state_namespace(path: bytes) -> bool:
    return path == _STATE_NAMESPACE or path.startswith(_STATE_NAMESPACE + b"/")


def _filtered_caller_status(raw: bytes) -> bytes:
    """Filter only untracked/ignored entries in the approved state namespace.

    Porcelain-v2 records remain byte-oriented and NUL-delimited.  Tracked records
    are never excluded, including tracked paths below ``agentic-state``.
    """
    records = raw.split(b"\0")
    if records[-1] != b"":
        raise WorktreeError("caller-status-not-nul-terminated")
    kept: list[bytes] = []
    index = 0
    while index < len(records) - 1:
        record = records[index]
        if not record:
            raise WorktreeError("caller-status-empty-record")
        tag = record[:1]
        if tag in {b"?", b"!"}:
            path = record[2:] if record[1:2] == b" " else b""
            if not path:
                raise WorktreeError("caller-status-invalid")
            if not _in_state_namespace(path):
                kept.append(record)
        elif tag == b"2":
            kept.append(record)
            index += 1
            if index >= len(records) - 1 or not records[index]:
                raise WorktreeError("caller-status-rename-invalid")
            kept.append(records[index])
        elif tag in {b"1", b"u"}:
            kept.append(record)
        else:
            raise WorktreeError("caller-status-invalid")
        index += 1
    return b"\0".join(kept) + (b"\0" if kept else b"")


def _gitlink_state(identity: GitIdentity, repository: Path) -> bytes:
    raw = _git(identity, repository, ["ls-files", "--stage", "-z"])
    if raw and not raw.endswith(b"\0"):
        raise WorktreeError("gitlink-state-not-nul-terminated")
    records = [
        record
        for record in raw.split(b"\0")
        if record.startswith(b"160000 ")
    ]
    return b"\0".join(records) + (b"\0" if records else b"")


def _snapshot(identity: GitIdentity, repository: Path) -> CallerSnapshot:
    head = _git(identity, repository, ["rev-parse", "HEAD"]).strip().decode("ascii")
    symbolic = None
    try:
        symbolic = _git(identity, repository, ["symbolic-ref", "-q", "HEAD"]).strip()
    except WorktreeError:
        pass
    index_path = _git(identity, repository, ["rev-parse", "--git-path", "index"])
    try:
        index = Path(os.fsdecode(index_path.rstrip(b"\n")))
    except UnicodeError:
        raise WorktreeError("index-path-invalid") from None
    if not index.is_absolute():
        index = repository / index
    index_digest = _digest(index) if index.is_file() else None
    raw_status = _git(
        identity,
        repository,
        [
            "status",
            "--porcelain=v2",
            "-z",
            "--untracked-files=all",
            "--ignored=matching",
        ],
    )
    status = _filtered_caller_status(raw_status)
    return CallerSnapshot(
        repository,
        head,
        symbolic,
        index_digest,
        status,
        _gitlink_state(identity, repository),
    )


def caller_preserved(managed: ManagedWorktree) -> bool:
    """Return whether the controller checkout still matches its captured state."""
    try:
        _verify_git(managed.git, managed.path)
        return _snapshot(managed.git, managed.repository) == managed.caller_snapshot
    except WorktreeError:
        return False


def create_worktree(repository: Path, execution_id: str, base_commit: str,
                    *, git: GitIdentity | None = None) -> ManagedWorktree:
    """Create one locked, detached, retained worktree at an exact commit."""
    if _COMMIT.fullmatch(base_commit) is None:
        raise WorktreeError("base-commit-invalid")
    identity = git or resolve_git()
    repository = _top_level(identity, repository)
    _reject_unsafe_config(identity, repository)
    if _git(identity, repository, ["cat-file", "-t", base_commit]).strip() != b"commit":
        raise WorktreeError("base-commit-not-commit")
    try:
        execution_id = local_state.validate_identifier(execution_id, "execution id")
        literal_root = repository / local_state.STATE_DIR
        try:
            root_metadata = literal_root.lstat()
        except FileNotFoundError:
            root_metadata = None
        if root_metadata is not None and (
            stat.S_ISLNK(root_metadata.st_mode)
            or not stat.S_ISDIR(root_metadata.st_mode)
        ):
            raise local_state.LocalStateError("invalid local-state root")
        root = local_state.approved_root(repository, local_state.STATE_DIR)
        if root != literal_root:
            raise local_state.LocalStateError("local-state root changed")
        execution = local_state.safe_path(repository, local_state.STATE_DIR, "executions", execution_id)
        target = execution / "worktree"
        local_state.prepare_directory(root, execution.parent)
    except local_state.LocalStateError:
        raise WorktreeError("execution-path-invalid") from None
    if execution.exists() or target.exists():
        raise WorktreeError("execution-id-already-used")
    if os.fsencode(target) in _worktree_records(identity, repository):
        raise WorktreeError("worktree-target-already-registered")
    try:
        local_state.prepare_directory(root, execution)
    except local_state.LocalStateError:
        raise WorktreeError("execution-path-invalid") from None
    created = False
    try:
        snapshot = _snapshot(identity, repository)
        _verify_git(identity, target)
        _git(identity, repository, ["worktree", "add", "--detach", str(target), base_commit], mutate=True)
        created = True
        _verify_git(identity, target)
        _git(identity, repository, ["worktree", "lock", "--reason", "retained managed execution worktree", str(target)], mutate=True)
        managed = ManagedWorktree(repository, target.resolve(), execution_id, base_commit, snapshot, identity)
        verify_worktree(managed)
        tracked = _git(
            identity,
            repository,
            ["-C", str(target), "status", "--porcelain=v2", "-z", "--untracked-files=no"],
        )
        if tracked:
            raise WorktreeError("initial-checkout-not-clean")
        return managed
    except WorktreeError:
        if created:
            try:
                _git(identity, repository, ["worktree", "remove", "--force", str(target)], mutate=True)
            except WorktreeError:
                pass
        if execution.exists() and execution.is_dir() and not execution.is_symlink():
            shutil.rmtree(execution)
        raise


def _worktree_records(
    identity: GitIdentity, repository: Path
) -> dict[bytes, dict[bytes, bytes]]:
    raw = _git(identity, repository, ["worktree", "list", "--porcelain", "-z"])
    records: dict[bytes, dict[bytes, bytes]] = {}
    current: dict[bytes, bytes] | None = None
    path: bytes | None = None
    for field in filter(None, raw.split(b"\0")):
        key, _, value = field.partition(b" ")
        if key == b"worktree":
            if path is not None and current is not None:
                records[path] = current
            path, current = value, {}
        elif current is not None:
            current[key] = value
    if path is not None and current is not None:
        records[path] = current
    return records


def verify_worktree(managed: ManagedWorktree) -> None:
    _verify_git(managed.git, managed.path)
    expected = local_state.safe_path(
        managed.repository,
        local_state.STATE_DIR,
        "executions",
        managed.execution_id,
    ) / "worktree"
    if managed.path != expected or managed.path.resolve() != expected:
        raise WorktreeError("worktree-path-changed")
    records = _worktree_records(managed.git, managed.repository)
    record = records.get(os.fsencode(managed.path))
    if record is None:
        raise WorktreeError("worktree-not-registered")
    if record.get(b"locked") != b"retained managed execution worktree":
        raise WorktreeError("worktree-not-locked")
    top = _git(
        managed.git,
        managed.repository,
        ["-C", str(managed.path), "rev-parse", "--show-toplevel"],
    ).rstrip(b"\n")
    if os.path.realpath(os.fsdecode(top)) != str(managed.path):
        raise WorktreeError("worktree-top-level-changed")
    if _git(managed.git, managed.repository, ["-C", str(managed.path), "rev-parse", "HEAD"]).strip().decode() != managed.base_commit:
        raise WorktreeError("worktree-commit-changed")
    try:
        _git(managed.git, managed.repository, ["-C", str(managed.path), "symbolic-ref", "-q", "HEAD"])
    except WorktreeError:
        pass
    else:
        raise WorktreeError("worktree-not-detached")
    linkage = managed.path / ".git"
    if not linkage.is_file() or b"gitdir:" not in linkage.read_bytes():
        raise WorktreeError("worktree-git-linkage-invalid")
    if not caller_preserved(managed):
        raise WorktreeError("caller-checkout-changed")


def _tracked_kind(status: bytes) -> str:
    states = set(status) - {ord(".")}
    if not states or states - set(b"AMDRCTU"):
        raise WorktreeError("inventory-tracked-status-invalid")
    for marker, kind in (
        (ord("T"), "tracked-type-change"),
        (ord("D"), "tracked-deletion"),
        (ord("A"), "tracked-addition"),
        (ord("C"), "tracked-addition"),
        (ord("R"), "tracked-modification"),
        (ord("U"), "tracked-modification"),
        (ord("M"), "tracked-modification"),
    ):
        if marker in states:
            return kind
    raise WorktreeError("inventory-tracked-status-invalid")


def inventory(
    managed: ManagedWorktree, approved: frozenset[bytes] = frozenset()
) -> WorktreeInventory:
    verify_worktree(managed)
    raw = _git(
        managed.git,
        managed.repository,
        [
            "-C",
            str(managed.path),
            "status",
            "--porcelain=v2",
            "-z",
            "--untracked-files=all",
            "--ignored=matching",
        ],
    )
    if raw and not raw.endswith(b"\0"):
        raise WorktreeError("inventory-not-nul-terminated")
    records = raw.split(b"\0")
    entries: list[InventoryEntry] = []
    seen_paths: set[bytes] = set()
    index = 0
    while index < len(records) - 1:
        record = records[index]
        tag = record[:1]
        if tag == b"1":
            fields = record.split(b" ", 8)
            if len(fields) != 9:
                raise WorktreeError("inventory-record-invalid")
            kind, path = _tracked_kind(fields[1]), fields[8]
        elif tag == b"2":
            fields = record.split(b" ", 9)
            if len(fields) != 10:
                raise WorktreeError("inventory-record-invalid")
            kind, path = _tracked_kind(fields[1]), fields[9]
            index += 1
            if index >= len(records) - 1 or not records[index]:
                raise WorktreeError("inventory-rename-invalid")
        elif tag == b"u":
            fields = record.split(b" ", 10)
            if len(fields) != 11:
                raise WorktreeError("inventory-record-invalid")
            kind, path = "tracked-modification", fields[10]
        elif tag == b"?":
            path, kind = record[2:], "untracked"
        elif tag == b"!":
            path, kind = record[2:], "ignored"
        else:
            raise WorktreeError("inventory-record-invalid")
        if not path or path in seen_paths:
            raise WorktreeError("inventory-duplicate")
        seen_paths.add(path)
        entries.append(InventoryEntry(kind, path, path not in approved))
        index += 1
    return WorktreeInventory(tuple(sorted(entries)))
