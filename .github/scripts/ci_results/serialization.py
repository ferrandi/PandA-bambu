"""Canonical JSON parsing and serialization."""

from __future__ import annotations

import json
import math
import os
import tempfile
from pathlib import Path
from typing import Any


class SerializationError(ValueError):
    """Raised for non-canonical or ambiguous JSON."""


def _reject_constant(value: str) -> None:
    raise SerializationError(f"non-finite JSON number is not allowed: {value}")


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise SerializationError(f"duplicate JSON object key: {key}")
        result[key] = value
    return result


def _check_finite(value: Any, path: str = "$") -> None:
    if isinstance(value, float) and not math.isfinite(value):
        raise SerializationError(f"{path}: non-finite number is not allowed")
    if isinstance(value, dict):
        for key, item in value.items():
            _check_finite(item, f"{path}.{key}")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            _check_finite(item, f"{path}[{index}]")


def canonical_text(value: Any) -> str:
    """Return deterministic UTF-8-compatible JSON text with a trailing newline."""

    _check_finite(value)
    return (
        json.dumps(
            value,
            allow_nan=False,
            ensure_ascii=False,
            indent=2,
            sort_keys=True,
        )
        + "\n"
    )


def canonical_bytes(value: Any) -> bytes:
    return canonical_text(value).encode("utf-8")


def load_json(path: Path) -> Any:
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as error:
        raise SerializationError(f"{path}: JSON must be UTF-8: {error}") from error
    try:
        return json.loads(
            text,
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_constant,
        )
    except json.JSONDecodeError as error:
        raise SerializationError(
            f"{path}:{error.lineno}:{error.colno}: invalid JSON: {error.msg}"
        ) from error


def require_canonical(path: Path, value: Any) -> None:
    actual = path.read_bytes()
    expected = canonical_bytes(value)
    if actual != expected:
        raise SerializationError(
            f"{path}: JSON is not canonically serialized "
            "(UTF-8, sorted keys, two-space indentation, trailing newline)"
        )


def write_json(path: Path, value: Any) -> None:
    """Atomically write canonical JSON."""

    path.parent.mkdir(parents=True, exist_ok=True)
    data = canonical_bytes(value)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
            if hasattr(os, "fchmod"):
                # GitHub-hosted workflow steps may read these files under another UID.
                os.fchmod(stream.fileno(), 0o644)
        os.replace(temporary_path, path)
    finally:
        temporary_path.unlink(missing_ok=True)
