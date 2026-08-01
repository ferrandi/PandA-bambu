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
DIGEST_ID = __import__("re").compile(r"^[0-9a-f]{64}$")


class LocalStateError(ValueError):
    """Raised when a local-state operation would be unsafe."""


def validate_identifier(value: object, field: str, digest: bool = False) -> str:
    if not isinstance(value, str) or not (DIGEST_ID if digest else SAFE_ID).fullmatch(value):
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


def preview(repository: Path, planned: list[PlannedWrite]) -> list[dict[str, object]]:
    result = []
    for item in sorted(planned, key=lambda entry: (entry.group, entry.parts)):
        target = safe_path(repository, item.group, *item.parts)
        result.append(
            {
                "group": item.group,
                "path": str(target.relative_to(repository.resolve())),
                "bytes": len(item.content),
                "replace_required": target.exists(),
            }
        )
    return result


def commit(
    repository: Path,
    planned: list[PlannedWrite],
    *,
    replace: bool = False,
    dry_run: bool = False,
    validator: Callable[[Path], None] | None = None,
) -> list[Path]:
    """Validate and atomically apply a deterministic set of ignored-local writes."""
    ordered = sorted(planned, key=lambda entry: (entry.group, entry.parts))
    targets: list[tuple[PlannedWrite, Path]] = []
    for item in ordered:
        if item.mode & 0o077:
            raise LocalStateError("local-state mode must be restrictive")
        target = safe_path(repository, item.group, *item.parts)
        root = approved_root(repository, item.group)
        _verify_parents(root, target)
        _verify_existing(target)
        if target.exists() and not replace:
            if target.read_bytes() == item.content:
                continue
            raise LocalStateError("refusing overwrite without explicit authorization")
        targets.append((item, target))
    if dry_run:
        return [target for _, target in targets]

    staged: list[tuple[Path, Path, PlannedWrite]] = []
    backups: list[tuple[Path, Path]] = []
    try:
        for item, target in targets:
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
            staged.append((temporary, target, item))
        for temporary, target, _ in staged:
            if target.exists():
                backup_dir = safe_path(repository, LOCAL_DIR, "backups")
                backup = backup_dir / f"{target.name}.bak"
                _verify_parents(approved_root(repository, LOCAL_DIR), backup)
                _verify_existing(backup)
                if backup.exists():
                    backup.unlink()
                os.replace(target, backup)
                backups.append((backup, target))
            os.replace(temporary, target)
        return [target for _, target, _ in staged]
    except Exception:
        for temporary, _, _ in staged:
            if temporary.exists():
                temporary.unlink()
        for backup, target in reversed(backups):
            if backup.exists() and not target.exists():
                os.replace(backup, target)
        raise