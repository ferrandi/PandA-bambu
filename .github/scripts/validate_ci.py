#!/usr/bin/env python3
"""Validate repository-local GitHub Actions workflow contracts."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

import yaml


LOCAL_WORKFLOW_PREFIX = "./.github/workflows/"
LOCAL_ACTION_PREFIX = "./.github/actions/"
STEP_OUTPUT_RE = re.compile(
    r"steps(?:\.([A-Za-z_][A-Za-z0-9_-]*)|\[['\"]([^'\"]+)['\"]\])"
    r"\.outputs(?:\.([A-Za-z_][A-Za-z0-9_-]*)|\[['\"]([^'\"]+)['\"]\])"
)


@dataclass(frozen=True, order=True)
class Diagnostic:
    path: Path
    line: int
    message: str

    def __str__(self) -> str:
        return f"{self.path}:{self.line}: {self.message}"


def _yaml_files(directory: Path) -> list[Path]:
    return sorted((*directory.glob("*.yml"), *directory.glob("*.yaml")))


def _line_for(path: Path, needles: Iterable[str]) -> int:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return 1
    for needle in needles:
        for number, line in enumerate(lines, 1):
            if needle in line:
                return number
    return 1


def _strings(value: Any) -> Iterable[str]:
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for key, item in value.items():
            yield from _strings(key)
            yield from _strings(item)
    elif isinstance(value, list):
        for item in value:
            yield from _strings(item)


def _mapping(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


class RepositoryValidator:
    def __init__(self, root: Path):
        self.root = root.resolve()
        self.diagnostics: list[Diagnostic] = []
        self.documents: dict[Path, dict[str, Any]] = {}

    def error(self, path: Path, line: int, message: str) -> None:
        display_path = path.relative_to(self.root) if path.is_relative_to(self.root) else path
        self.diagnostics.append(Diagnostic(display_path, line, message))

    def load_yaml(self, path: Path) -> dict[str, Any] | None:
        if path in self.documents:
            return self.documents[path]
        try:
            value = yaml.load(path.read_text(encoding="utf-8"), Loader=yaml.BaseLoader)
        except OSError as error:
            self.error(path, 1, f"cannot read YAML: {error}")
            return None
        except yaml.YAMLError as error:
            mark = getattr(error, "problem_mark", None)
            line = mark.line + 1 if mark is not None else 1
            problem = getattr(error, "problem", None) or str(error).splitlines()[0]
            self.error(path, line, f"invalid YAML: {problem}")
            return None
        if not isinstance(value, dict):
            self.error(path, 1, "top-level YAML value must be a mapping")
            return None
        self.documents[path] = value
        return value

    def validate(self) -> list[Diagnostic]:
        workflow_dir = self.root / ".github" / "workflows"
        action_dir = self.root / ".github" / "actions"
        workflow_paths = _yaml_files(workflow_dir)

        for path in workflow_paths:
            self.load_yaml(path)
        for path in sorted((*action_dir.glob("*/action.yml"), *action_dir.glob("*/action.yaml"))):
            self.load_yaml(path)

        for path in workflow_paths:
            document = self.documents.get(path)
            if document is None:
                continue
            self.validate_needs(path, document)
            self.validate_workflow_calls(path, document)
            self.validate_local_action_outputs(path, document)

        for path, document in tuple(self.documents.items()):
            if LOCAL_ACTION_PREFIX in f"./{path.relative_to(self.root)}":
                self.validate_local_action_outputs(path, document)

        return sorted(set(self.diagnostics))

    def validate_needs(self, path: Path, document: dict[str, Any]) -> None:
        jobs = _mapping(document.get("jobs"))
        known_jobs = {str(job_id).lower() for job_id in jobs}
        for job_id, job_value in jobs.items():
            job = _mapping(job_value)
            needs_value = job.get("needs", [])
            needs = needs_value if isinstance(needs_value, list) else [needs_value]
            for dependency in (str(item) for item in needs if item is not None):
                if dependency.lower() not in known_jobs:
                    line = _line_for(path, [f"- {dependency}", f"needs: {dependency}"])
                    self.error(
                        path,
                        line,
                        f"job '{job_id}' needs unknown job '{dependency}'",
                    )

    def validate_workflow_calls(self, path: Path, document: dict[str, Any]) -> None:
        for job_id, job_value in _mapping(document.get("jobs")).items():
            job = _mapping(job_value)
            uses = job.get("uses")
            if not isinstance(uses, str) or not uses.startswith(LOCAL_WORKFLOW_PREFIX):
                continue
            target = (self.root / uses.removeprefix("./")).resolve()
            line = _line_for(path, [f"uses: {uses}"])
            target_document = self.load_yaml(target)
            if target_document is None:
                self.error(path, line, f"job '{job_id}' references unreadable workflow '{uses}'")
                continue
            workflow_call = _mapping(_mapping(target_document.get("on")).get("workflow_call"))
            declared = _mapping(workflow_call.get("inputs"))
            supplied = _mapping(job.get("with"))
            for input_name in supplied:
                if input_name not in declared:
                    input_line = _line_for(path, [f"{input_name}:"])
                    self.error(
                        path,
                        input_line,
                        f"job '{job_id}' passes unknown input '{input_name}' to '{uses}'",
                    )
            for input_name, definition_value in declared.items():
                definition = _mapping(definition_value)
                if definition.get("required") == "true" and input_name not in supplied:
                    self.error(
                        path,
                        line,
                        f"job '{job_id}' omits required input '{input_name}' for '{uses}'",
                    )

    def _action_outputs(self, action_path: Path) -> set[str] | None:
        action_document = self.load_yaml(action_path)
        if action_document is None:
            return None
        return set(_mapping(action_document.get("outputs")))

    def validate_local_action_outputs(self, path: Path, document: dict[str, Any]) -> None:
        containers: list[dict[str, Any]] = []
        jobs = _mapping(document.get("jobs"))
        containers.extend(_mapping(value) for value in jobs.values())
        runs = _mapping(document.get("runs"))
        if isinstance(runs.get("steps"), list):
            containers.append(runs)

        for container in containers:
            steps = container.get("steps")
            if not isinstance(steps, list):
                continue
            local_steps: dict[str, tuple[str, set[str]]] = {}
            for step_value in steps:
                step = _mapping(step_value)
                step_id = step.get("id")
                uses = step.get("uses")
                if not isinstance(step_id, str) or not isinstance(uses, str):
                    continue
                if not uses.startswith(LOCAL_ACTION_PREFIX):
                    continue
                action_path = (self.root / uses.removeprefix("./") / "action.yml").resolve()
                if not action_path.exists():
                    action_path = action_path.with_suffix(".yaml")
                outputs = self._action_outputs(action_path)
                if outputs is not None:
                    local_steps[step_id] = (uses, outputs)

            for text in _strings(container):
                for match in STEP_OUTPUT_RE.finditer(text):
                    step_id = match.group(1) or match.group(2)
                    output_name = match.group(3) or match.group(4)
                    action = local_steps.get(step_id)
                    if action is None:
                        continue
                    uses, outputs = action
                    if output_name not in outputs:
                        expression = match.group(0)
                        line = _line_for(path, [expression, output_name])
                        self.error(
                            path,
                            line,
                            f"step '{step_id}' references unknown output '{output_name}' "
                            f"from '{uses}'",
                        )


def validate_repository(root: Path) -> list[Diagnostic]:
    return RepositoryValidator(root).validate()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root")
    args = parser.parse_args(argv)
    diagnostics = validate_repository(args.root)
    for diagnostic in diagnostics:
        print(diagnostic, file=sys.stderr)
    if diagnostics:
        print(f"CI validation failed with {len(diagnostics)} error(s).", file=sys.stderr)
        return 1
    print("CI validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
