from __future__ import annotations

import sys
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import provider_guided


class ScriptedIO:
    def __init__(self, answers: list[str], *, tty: bool = True):
        self.answers = iter(answers)
        self.messages: list[str] = []
        self.tty = tty

    def read(self, prompt: str) -> str:
        self.messages.append(prompt)
        return next(self.answers)

    def write(self, message: str) -> None:
        self.messages.append(message)

    def isatty(self) -> bool:
        return self.tty


class ProviderGuidedTests(unittest.TestCase):
    def setUp(self):
        self.clock = lambda: datetime(2026, 2, 3, 4, 5, 6, tzinfo=timezone.utc)

    def test_manual_accepts_recommendation_and_uses_paf04a_apply(self):
        io = ScriptedIO(
            [
                "Lab Gateway",
                "",
                "https://gateway.example.invalid/v1",
                "environment",
                "LAB_TOKEN",
                "openai",
                "openai-chat-completions",
                "model-b, model-a",
                "accept",
                "yes",
            ]
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = provider_guided.run(root, io=io, clock=self.clock)
            self.assertEqual(result.status, "applied")
            self.assertEqual(result.spec["provider_id"], "lab-gateway")
            self.assertEqual(result.spec["models"], ["model-a", "model-b"])
            self.assertEqual(
                {item["model"] for item in result.spec["role_assignments"]},
                {"model-a"},
            )
            self.assertTrue((root / ".agentic-local/providers/lab-gateway.json").exists())
            self.assertFalse(any("LAB_TOKEN" in message for message in io.messages))

    def test_manual_customizes_roles_without_network(self):
        io = ScriptedIO(
            [
                "Local",
                "",
                "http://127.0.0.1:8080/v1",
                "none",
                "openai",
                "openai-responses",
                "model-a, model-b",
                "customize",
                "model-a",
                "model-b",
                "",
                "yes",
            ]
        )
        with tempfile.TemporaryDirectory() as directory:
            result = provider_guided.run(Path(directory), io=io, clock=self.clock)
            assignments = {item["role_id"]: item["model"] for item in result.spec["role_assignments"]}
            self.assertEqual(assignments, {"planning": "model-a", "implementation": "model-b", "review": "model-a"})
            self.assertEqual(result.spec["execution_protocol"], "openai-responses")
            self.assertEqual(result.spec["discovery_evidence"]["method"], "manual")

    def test_cancel_and_final_refusal_leave_no_state(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            cancelled = provider_guided.run(root, io=ScriptedIO(["cancel"]), clock=self.clock)
            self.assertEqual(cancelled.status, "cancelled")
            self.assertFalse((root / ".agentic-local").exists())

            refusal = provider_guided.run(
                root,
                io=ScriptedIO(
                    [
                        "Local",
                        "",
                        "http://127.0.0.1:8080/v1",
                        "none",
                        "openai",
                        "openai-chat-completions",
                        "model-a",
                        "accept",
                        "no",
                    ]
                ),
                clock=self.clock,
            )
            self.assertEqual(refusal.status, "cancelled")
            self.assertFalse((root / ".agentic-local").exists())

    def test_eof_and_non_tty_are_rejected_without_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(provider_guided.GuidedOnboardingError):
                provider_guided.run(root, io=ScriptedIO([], tty=False), clock=self.clock)
            with self.assertRaises(provider_guided.GuidedOnboardingError):
                provider_guided.run(root, io=ScriptedIO([]), clock=self.clock)
            self.assertFalse((root / ".agentic-local").exists())


if __name__ == "__main__":
    unittest.main()