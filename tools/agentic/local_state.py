#!/usr/bin/env python3
"""Safe ignored-local-state paths and atomic transactional file writes."""
from __future__ import annotations

import json
import os
import stat
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Mapping

LOCAL_DIR = ".agentic-local"
STATE_DIR = "agentic-state"
SAFE_ID = __import__("re").compile(r"^[a-z][a-z0-9.-]*$")
SAFE_JSON_NAME = __import__("re").compile(r"^[a-z][a-z0-9.-]*\.json$")
DIGEST_ID = __import__("re").compile(r"^[0-9a-f]{64}$")
MAX_FILENAME_LENGTH = 200
_JSON_SUFFIX = ".json"
_BACKUP_SUFFIX = ".bak"
MAX_IDENTIFIER_LENGTH = MAX_FILENAME_LENGTH - len(_JSON_SUFFIX) - len(_BACKUP_SUFFIX)
MAX_JSON_NAME_LENGTH = MAX_FILENAME_LENGTH - len(_BACKUP_SUFFIX)


class LocalStateError(ValueError):
    """Raised when a local-state operation would be unsafe."""


def validate_identifier(value: object, field: str, digest: bool = False) -> str:
    pattern = DIGEST_ID if digest else SAFE_ID
    is_json_name = isinstance(value, str) and not digest and SAFE_JSON_NAME.fullmatch(value) is not None
    maximum_length = MAX_JSON_NAME_LENGTH if is_json_name else MAX_IDENTIFIER_LENGTH
    if not isinstance(value, str) or len(value) > maximum_length or not (pattern.fullmatch(value) or is_json_name):
        raise LocalStateError(f"invalid {field}")
    return value


def approved_root(repository: Path, group: str) -> Path:
    if group not in {LOCAL_DIR, STATE_DIR}:
        raise LocalStateError("unapproved local-state root")
    return (repository / group).resolve()


def safe_path(repository: Path, group: str, *parts: str) -> Path:
    root = approved_root(repository, group)
    checked = [validate_identifier(part, "path identifier") for part in parts]
    path = root.joinpath(*checked)
    if path.parent != root and root not in path.parents:
        raise LocalStateError("local-state path escapes approved root")
    return path


def _verify_existing(path: Path) -> None:
    try:
        metadata = path.lstat()
    except FileNotFoundError:
        return
    if stat.S_ISLNK(metadata.st_mode):
        raise LocalStateError("refusing symbolic-link target")
    if not stat.S_ISREG(metadata.st_mode):
        raise LocalStateError("refusing non-regular target")


def _verify_parents(root: Path, path: Path) -> None:
    root.mkdir(mode=0o700, parents=True, exist_ok=True)
    parents = [root]
    current = path.parent
    while current != root:
        parents.append(current)
        current = current.parent
    for parent in reversed(parents):
        try:
            metadata = parent.lstat()
        except FileNotFoundError:
            parent.mkdir(mode=0o700)
            metadata = parent.lstat()
        if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISDIR(metadata.st_mode):
            raise LocalStateError("refusing symbolic-link or non-directory parent")


def canonical_json(value: Mapping[str, Any]) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2, ensure_ascii=True) + "\n").encode("utf-8")


@dataclass(frozen=True)
class PlannedWrite:
    group: str
    parts: tuple[str, ...]
    content: bytes
    mode: int = 0o600


@dataclass(frozen=True)
class PlannedDelete:
    group: str
    parts: tuple[str, ...]


def preview(repository: Path, planned: list[PlannedWrite]) -> list[dict[str, object]]:
    result = []
    for item in sorted(planned, key=lambda entry: (entry.group, entry.parts)):
        target = safe_path(repository, item.group, *item.parts)
        result.append({"group": item.group, "path": str(target.relative_to(repository.resolve())), "bytes": len(item.content), "replace_required": target.exists()})
    return result


def transaction(
    repository: Path,
    writes: list[PlannedWrite],
    deletes: list[PlannedDelete] = [],
    *,
    replace: bool = False,
    dry_run: bool = False,
    validator: Callable[[Path], None] | None = None,
    fail_after: int | None = None,
) -> list[Path]:
    """Apply deterministic writes/deletes with complete content rollback.

    Existing bytes are retained in restrictive temporary snapshots.  This avoids
    treating a provider setup as successful unless every owned document changes
    together. ``fail_after`` exists solely for injected failure tests.
    """
    write_targets: list[tuple[PlannedWrite, Path]] = []
    delete_targets: list[Path] = []
    targets: set[Path] = set()
    for item in sorted(writes, key=lambda entry: (entry.group, entry.parts)):
        if item.mode & 0o077:
            raise LocalStateError("local-state mode must be restrictive")
        target = safe_path(repository, item.group, *item.parts)
        if not dry_run:
            _verify_parents(approved_root(repository, item.group), target)
        _verify_existing(target)
        if target in targets:
            raise LocalStateError("duplicate transaction target")
        targets.add(target)
        if target.exists() and not replace and target.read_bytes() != item.content:
            raise LocalStateError("refusing overwrite without explicit authorization")
        if not (target.exists() and target.read_bytes() == item.content):
            write_targets.append((item, target))
    for item in sorted(deletes, key=lambda entry: (entry.group, entry.parts)):
        target = safe_path(repository, item.group, *item.parts)
        if not dry_run:
            _verify_parents(approved_root(repository, item.group), target)
        _verify_existing(target)
        if target in targets:
            raise LocalStateError("duplicate transaction target")
        targets.add(target)
        if target.exists():
            delete_targets.append(target)
    if dry_run:
        return [target for _, target in write_targets] + delete_targets

    staged: list[tuple[Path, Path]] = []
    snapshots: dict[Path, bytes | None] = {}
    changed: list[Path] = []
    try:
        for item, target in write_targets:
            handle = tempfile.NamedTemporaryFile(dir=target.parent, delete=False)
            temporary = Path(handle.name)
            try:
                os.fchmod(handle.fileno(), item.mode)
                handle.write(item.content)
                handle.flush()
                os.fsync(handle.fileno())
            finally:
                handle.close()
            if validator is not None:
                validator(temporary)
            staged.append((temporary, target))
        operations: list[tuple[str, Path, Path | None]] = [("write", target, temporary) for temporary, target in staged] + [("delete", target, None) for target in delete_targets]
        for index, (kind, target, temporary) in enumerate(operations, 1):
            snapshots.setdefault(target, target.read_bytes() if target.exists() else None)
            if kind == "write":
                os.replace(temporary, target)
            else:
                target.unlink()
            changed.append(target)
            if fail_after == index:
                raise OSError("injected transaction failure")
        for target, original in snapshots.items():
            if original is not None:
                backup = safe_path(repository, LOCAL_DIR, "backups", f"{target.name}{_BACKUP_SUFFIX}")
                _verify_parents(approved_root(repository, LOCAL_DIR), backup)
                with tempfile.NamedTemporaryFile(dir=backup.parent, delete=False) as handle:
                    os.fchmod(handle.fileno(), 0o600)
                    handle.write(original)
                    temporary = Path(handle.name)
                os.replace(temporary, backup)
        return changed
    except OSError as error:
        for target, original in snapshots.items():
            if original is None:
                if target.exists():
                    target.unlink()
            else:
                with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as handle:
                    os.fchmod(handle.fileno(), 0o600)
                    handle.write(original)
                    temporary = Path(handle.name)
                os.replace(temporary, target)
        raise LocalStateError("local-state filesystem operation failed") from None
    finally:
        for temporary, _ in staged:
            if temporary.exists():
                temporary.unlink()


def commit(
    repository: Path,
    planned: list[PlannedWrite],
    *,
    replace: bool = False,
    dry_run: bool = False,
    validator: Callable[[Path], None] | None = None,
) -> list[Path]:
    """Backward-compatible write-only transaction."""
    return transaction(repository, planned, replace=replace, dry_run=dry_run, validator=validator)