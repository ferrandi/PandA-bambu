"""Artifact and implementation identity helpers."""

from __future__ import annotations

import hashlib
import re
import subprocess
from pathlib import Path
from typing import Any


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def path_size(path: Path) -> int | None:
    if not path.exists():
        return None
    if path.is_file():
        return path.stat().st_size
    total = 0
    try:
        for item in path.rglob("*"):
            if item.is_file():
                total += item.stat().st_size
    except OSError:
        return None
    return total


def regular_file_metadata(path: Path) -> tuple[int | None, str | None]:
    if not path.is_file():
        return None, None
    try:
        return path.stat().st_size, sha256_file(path)
    except OSError:
        return None, None


def docker_base_image(dockerfile: Path) -> str | None:
    if not dockerfile.is_file():
        return None
    for line in dockerfile.read_text(encoding="utf-8").splitlines():
        match = re.match(r"\s*FROM\s+([^\s]+)", line, re.IGNORECASE)
        if match:
            return match.group(1)
    return None


def submodule_commits(repository: Path) -> list[dict[str, str]]:
    result = subprocess.run(
        ["git", "submodule", "status", "--recursive"],
        cwd=repository,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return []
    submodules: list[dict[str, str]] = []
    for line in result.stdout.splitlines():
        match = re.match(r"^[ +\-U]?([0-9a-f]{40})\s+(\S+)", line)
        if match:
            submodules.append({"commit_sha": match.group(1), "path": match.group(2)})
    return sorted(submodules, key=lambda item: item["path"])
