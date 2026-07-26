#!/usr/bin/env python3
"""Focused self-tests for validate_ci.py."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


VALIDATOR_PATH = Path(__file__).with_name("validate_ci.py")
SPEC = importlib.util.spec_from_file_location("validate_ci", VALIDATOR_PATH)
assert SPEC is not None and SPEC.loader is not None
VALIDATE_CI = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = VALIDATE_CI
SPEC.loader.exec_module(VALIDATE_CI)


class ValidatorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        (self.root / ".github" / "workflows").mkdir(parents=True)
        (self.root / ".github" / "actions" / "example").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write(self, relative_path: str, content: str) -> None:
        path = self.root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def messages(self) -> list[str]:
        return [diagnostic.message for diagnostic in VALIDATE_CI.validate_repository(self.root)]

    def test_valid_repository(self) -> None:
        self.write(
            ".github/workflows/reusable.yml",
            """on:
  workflow_call:
    inputs:
      package:
        required: true
        type: string
jobs: {}
""",
        )
        self.write(
            ".github/actions/example/action.yml",
            """name: Example
outputs:
  result:
    description: result
runs:
  using: composite
  steps: []
""",
        )
        self.write(
            ".github/workflows/caller.yml",
            """on: pull_request
jobs:
  build:
    uses: ./.github/workflows/reusable.yml
    with:
      package: test
  test:
    needs: build
    runs-on: ubuntu-24.04
    steps:
      - id: example
        uses: ./.github/actions/example
      - run: echo '${{ steps.example.outputs.result }}'
""",
        )
        self.assertEqual(self.messages(), [])

    def test_reports_unknown_and_missing_workflow_inputs(self) -> None:
        self.write(
            ".github/workflows/reusable.yml",
            """on:
  workflow_call:
    inputs:
      required-input:
        required: true
        type: string
jobs: {}
""",
        )
        self.write(
            ".github/workflows/caller.yml",
            """on: pull_request
jobs:
  call:
    uses: ./.github/workflows/reusable.yml
    with:
      unexpected: value
""",
        )
        messages = self.messages()
        self.assertTrue(any("unknown input 'unexpected'" in message for message in messages))
        self.assertTrue(any("omits required input 'required-input'" in message for message in messages))

    def test_reports_unknown_local_action_output(self) -> None:
        self.write(
            ".github/actions/example/action.yml",
            """name: Example
outputs:
  actual:
    description: actual
runs:
  using: composite
  steps: []
""",
        )
        self.write(
            ".github/workflows/caller.yml",
            """on: pull_request
jobs:
  test:
    runs-on: ubuntu-24.04
    steps:
      - id: example
        uses: ./.github/actions/example
      - run: echo '${{ steps.example.outputs.missing }}'
""",
        )
        self.assertTrue(any("unknown output 'missing'" in message for message in self.messages()))

    def test_reports_unknown_needs_job(self) -> None:
        self.write(
            ".github/workflows/caller.yml",
            """on: pull_request
jobs:
  test:
    needs: missing
    runs-on: ubuntu-24.04
    steps: []
""",
        )
        self.assertTrue(any("needs unknown job 'missing'" in message for message in self.messages()))

    def test_reports_yaml_parse_error_with_line(self) -> None:
        self.write(
            ".github/workflows/broken.yml",
            """on: pull_request
jobs:
  test: [
""",
        )
        diagnostics = VALIDATE_CI.validate_repository(self.root)
        self.assertEqual(len(diagnostics), 1)
        self.assertEqual(diagnostics[0].path, Path(".github/workflows/broken.yml"))
        self.assertGreaterEqual(diagnostics[0].line, 2)
        self.assertIn("invalid YAML", diagnostics[0].message)


    def test_rejects_native_tuning_with_persistent_hosted_ccache(self) -> None:
        self.write(
            ".github/workflows/open-build-smoke.yml",
            """on: pull_request
jobs:
  build:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/cache/restore
        with:
          key: panda-ccache-v2-native
      - uses: ./.github/actions/example
        with:
          optimized-flags: -Ofast -march=native -mtune=native
""",
        )
        messages = self.messages()
        self.assertTrue(
            any(
                "native-tuned cached objects are unsafe across hosted runners" in message
                and "-march=native" in message
                for message in messages
            )
        )
        self.assertTrue(
            any(
                "native-tuned cached objects are unsafe across hosted runners" in message
                and "-mtune=native" in message
                for message in messages
            )
        )

    def test_accepts_portable_profile_in_hosted_cache_identity(self) -> None:
        self.write(
            ".github/workflows/fast-regressions-hosted.yml",
            """on: pull_request
jobs:
  build:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/cache/restore
        with:
          key: panda-ccache-v2-generic-x86-64-Ofast-Linux-X64
      - uses: ./.github/actions/example
        with:
          optimized-flags: -Ofast -march=x86-64 -mtune=generic
""",
        )
        self.assertEqual(self.messages(), [])


if __name__ == "__main__":
    unittest.main()
