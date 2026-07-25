"""Small standard-library JSON Schema subset used by the protocol schemas."""

from __future__ import annotations

import math
import re
from pathlib import Path
from typing import Any

from .serialization import load_json


class SchemaValidationError(ValueError):
    """Raised when an instance violates a versioned schema."""


class SchemaValidator:
    """Validate the JSON Schema keywords used by `.github/schemas/ci/v1`."""

    def __init__(self, schema_directory: Path):
        self.schema_directory = schema_directory.resolve()
        self._cache: dict[Path, dict[str, Any]] = {}

    def load_schema(self, path: Path) -> dict[str, Any]:
        resolved = path.resolve()
        if resolved not in self._cache:
            value = load_json(resolved)
            if not isinstance(value, dict):
                raise SchemaValidationError(f"{resolved}: schema must be an object")
            self._cache[resolved] = value
        return self._cache[resolved]

    def validate(self, instance: Any, schema_file: str) -> None:
        path = (self.schema_directory / schema_file).resolve()
        schema = self.load_schema(path)
        errors = self._errors(instance, schema, path, "$")
        if errors:
            raise SchemaValidationError("\n".join(errors))

    def _resolve_ref(self, reference: str, current_path: Path) -> tuple[Any, Path]:
        file_part, separator, fragment = reference.partition("#")
        target_path = (current_path.parent / file_part).resolve() if file_part else current_path
        if not target_path.is_relative_to(self.schema_directory):
            raise SchemaValidationError(f"schema reference escapes schema directory: {reference}")
        target: Any = self.load_schema(target_path)
        if separator and fragment:
            if not fragment.startswith("/"):
                raise SchemaValidationError(f"unsupported schema fragment: {reference}")
            for token in fragment[1:].split("/"):
                token = token.replace("~1", "/").replace("~0", "~")
                if not isinstance(target, dict) or token not in target:
                    raise SchemaValidationError(f"unresolved schema reference: {reference}")
                target = target[token]
        return target, target_path

    @staticmethod
    def _matches_type(instance: Any, expected: str) -> bool:
        if expected == "null":
            return instance is None
        if expected == "boolean":
            return isinstance(instance, bool)
        if expected == "integer":
            return isinstance(instance, int) and not isinstance(instance, bool)
        if expected == "number":
            return (
                isinstance(instance, (int, float))
                and not isinstance(instance, bool)
                and (not isinstance(instance, float) or math.isfinite(instance))
            )
        if expected == "string":
            return isinstance(instance, str)
        if expected == "array":
            return isinstance(instance, list)
        if expected == "object":
            return isinstance(instance, dict)
        raise SchemaValidationError(f"unsupported JSON Schema type: {expected}")

    def _errors(
        self,
        instance: Any,
        schema: Any,
        schema_path: Path,
        location: str,
    ) -> list[str]:
        if not isinstance(schema, dict):
            return [f"{location}: schema node must be an object"]
        if "$ref" in schema:
            target, target_path = self._resolve_ref(str(schema["$ref"]), schema_path)
            return self._errors(instance, target, target_path, location)

        if "anyOf" in schema:
            alternatives = schema["anyOf"]
            if any(not self._errors(instance, item, schema_path, location) for item in alternatives):
                return []
            return [f"{location}: value does not match any allowed schema"]

        errors: list[str] = []
        expected_type = schema.get("type")
        if expected_type is not None:
            expected_types = expected_type if isinstance(expected_type, list) else [expected_type]
            if not any(self._matches_type(instance, item) for item in expected_types):
                names = ", ".join(expected_types)
                return [f"{location}: expected type {names}, got {type(instance).__name__}"]

        if "const" in schema and instance != schema["const"]:
            errors.append(f"{location}: expected constant {schema['const']!r}")
        if "enum" in schema and instance not in schema["enum"]:
            errors.append(f"{location}: value {instance!r} is not in the allowed enum")

        if isinstance(instance, dict):
            required = schema.get("required", [])
            for key in required:
                if key not in instance:
                    errors.append(f"{location}: missing required property {key!r}")
            properties = schema.get("properties", {})
            for key, value in instance.items():
                child_location = f"{location}.{key}"
                if key in properties:
                    errors.extend(self._errors(value, properties[key], schema_path, child_location))
                elif schema.get("additionalProperties") is False:
                    errors.append(f"{child_location}: additional property is not allowed")

        if isinstance(instance, list):
            if len(instance) < schema.get("minItems", 0):
                errors.append(f"{location}: array has fewer than {schema['minItems']} items")
            if schema.get("uniqueItems"):
                normalized = [repr(item) for item in instance]
                if len(normalized) != len(set(normalized)):
                    errors.append(f"{location}: array items must be unique")
            if "items" in schema:
                for index, value in enumerate(instance):
                    errors.extend(
                        self._errors(value, schema["items"], schema_path, f"{location}[{index}]")
                    )

        if isinstance(instance, str):
            if len(instance) < schema.get("minLength", 0):
                errors.append(f"{location}: string is shorter than {schema['minLength']}")
            pattern = schema.get("pattern")
            if pattern is not None and re.search(pattern, instance) is None:
                errors.append(f"{location}: string does not match {pattern!r}")

        if isinstance(instance, (int, float)) and not isinstance(instance, bool):
            minimum = schema.get("minimum")
            if minimum is not None and instance < minimum:
                errors.append(f"{location}: value must be at least {minimum}")

        return errors
