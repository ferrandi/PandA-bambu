#!/usr/bin/env python3
"""Offline smoke tests for the revision-3 PAF bootstrap bundle."""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


class BundleSmoke(unittest.TestCase):
    root: Path

    @classmethod
    def setUpClass(cls) -> None:
        cls.root = ROOT

    def test_required_files(self) -> None:
        required = [
            "documentation/evolvehls/paf-product-roadmap.md",
            "documentation/evolvehls/paf-runtime-specification.md",
            "documentation/evolvehls/paf-security-data-governance-plan.md",
            "documentation/evolvehls/paf-observability-operations-plan.md",
            "documentation/evolvehls/paf-bootstrap-operational-invariants.md",
            "documentation/evolvehls/paf-bootstrap-self-hosting-plan.md",
            "documentation/evolvehls/paf-responsibility-transfer-matrix.md",
            "documentation/evolvehls/paf-interface-state-machine-map.md",
            "documentation/evolvehls/paf-revision-3-critical-review.md",
            "config/paf-bootstrap-campaign.json",
            "config/schemas/paf-bootstrap-campaign.schema.json",
            "config/schemas/paf-bootstrap-task.schema.json",
            "tools/paf-bootstrap/templates/next-task-prompt.md",
            "tools/paf-bootstrap/paf-cline-cycle",
            "tools/paf-bootstrap/paf-cline-next-task",
            "tools/paf-bootstrap/paf-cline-campaign",
            "tools/paf-bootstrap/paf-cline-monitor",
            "tools/paf-bootstrap/paf-cline-preflight",
            "tools/paf-bootstrap/paf-cline-review-resume",
        ]
        missing = [path for path in required if not (self.root / path).is_file()]
        self.assertEqual([], missing)

    def test_config_and_backlog(self) -> None:
        config = json.loads((self.root / "config/paf-bootstrap-campaign.json").read_text())
        self.assertEqual(2, config["schema_version"])
        self.assertEqual(3, config["version"])
        ids = [item["id"] for item in config["backlog"]]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual("BS-000", ids[0])
        self.assertIn("BS-025", ids)
        self.assertIn("BS-035", ids)
        known = set(ids)
        for item in config["backlog"]:
            self.assertTrue(set(item["depends_on"]).issubset(known))
        bundled_docs = {
            path for path in config["documents"].values()
            if Path(path).name in {
                "paf-product-roadmap.md", "paf-runtime-specification.md",
                "paf-security-data-governance-plan.md", "paf-observability-operations-plan.md",
                "paf-bootstrap-operational-invariants.md", "paf-bootstrap-self-hosting-plan.md",
                "paf-responsibility-transfer-matrix.md", "paf-interface-state-machine-map.md",
                "paf-revision-3-critical-review.md",
            }
        }
        for path in bundled_docs:
            self.assertTrue((self.root / path).is_file(), path)

    def test_script_syntax(self) -> None:
        cycle = self.root / "tools/paf-bootstrap/paf-cline-cycle"
        result = subprocess.run(["bash", "-n", str(cycle)], check=False)
        self.assertEqual(0, result.returncode)
        for name in ("paf-cline-next-task", "paf-cline-campaign", "paf-cline-monitor", "paf-cline-preflight", "paf-cline-review-resume"):
            result = subprocess.run(["python3", "-m", "py_compile", str(self.root / "tools/paf-bootstrap" / name)], check=False)
            self.assertEqual(0, result.returncode, name)

    def test_next_task_list_eligible(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            state = Path(tmp) / "state.json"
            state.write_text(json.dumps({"schema_version": 2, "campaign_id": "paf-bootstrap-self-hosting", "completed_backlog_items": ["BS-000"], "tasks": {}, "current_task_id": None}))
            cmd = [
                str(self.root / "tools/paf-bootstrap/paf-cline-next-task"),
                "--repo", str(self.root),
                "--campaign-config", str(self.root / "config/paf-bootstrap-campaign.json"),
                "--campaign-state", str(state),
                "--output-dir", str(Path(tmp) / "tasks"),
                "--list-eligible",
            ]
            result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertEqual("BS-010", json.loads(result.stdout)["id"])

    def test_monitor_synthetic_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); run = root / "run"; run.mkdir(); (root / "latest").symlink_to(run)
            (run / "controller.log").write_text("[x] Starting sonnet-1 in act mode (attempt 1/1)\n")
            event = {"ts": "x", "type": "agent_event", "event": {"type": "iteration_start", "iteration": 3}}
            (run / "cycle-1-sonnet.attempt-1.jsonl").write_text(json.dumps(event) + "\n")
            result = subprocess.run([str(self.root / "tools/paf-bootstrap/paf-cline-monitor"), "--state-root", str(root), "status"], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertIn("sonnet-1", result.stdout)
            self.assertIn("Iteration: 3", result.stdout)

    def test_campaign_init(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"; repo.mkdir()
            subprocess.run(["git", "init", "-q", str(repo)], check=True)
            config_dir = repo / "config"; config_dir.mkdir()
            (config_dir / "paf-bootstrap-campaign.json").write_text((self.root / "config/paf-bootstrap-campaign.json").read_text())
            state = Path(tmp) / "campaign"
            command = [str(self.root / "tools/paf-bootstrap/paf-cline-campaign"), "--repo", str(repo), "--state", str(state), "init", "--complete", "BS-000"]
            result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
            self.assertEqual(0, result.returncode, result.stderr)
            saved = json.loads((state / "campaign-state.json").read_text())
            self.assertEqual(["BS-000"], saved["completed_backlog_items"])


    def test_controller_mock_end_to_end(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            origin = root / "origin.git"
            repo = root / "repo"
            fake_bin = root / "bin"
            home = root / "home"
            fake_bin.mkdir()
            home.mkdir()
            for profile in (".cline-sol", ".cline-terra", ".cline-opus", ".cline-sonnet"):
                (home / profile).mkdir()

            subprocess.run(["git", "init", "--bare", "-q", str(origin)], check=True)
            subprocess.run(["git", "clone", "-q", str(origin), str(repo)], check=True)
            subprocess.run(["git", "-C", str(repo), "config", "user.name", "PAF Smoke"], check=True)
            subprocess.run(["git", "-C", str(repo), "config", "user.email", "paf-smoke@example.invalid"], check=True)
            (repo / "README.md").write_text("base\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(repo), "add", "README.md"], check=True)
            subprocess.run(["git", "-C", str(repo), "commit", "-q", "-m", "base"], check=True)
            subprocess.run(["git", "-C", str(repo), "branch", "-M", "dev/panda"], check=True)
            subprocess.run(["git", "-C", str(repo), "push", "-q", "-u", "origin", "dev/panda"], check=True)

            fake_cline = fake_bin / "cline"
            fake_cline.write_text(
                """#!/usr/bin/env python3
import json
import pathlib
import sys

args = sys.argv[1:]
config = ""
cwd = "."
for index, value in enumerate(args):
    if value == "--config":
        config = args[index + 1]
    elif value == "--cwd":
        cwd = args[index + 1]

role = pathlib.Path(config).name
if role == ".cline-sol":
    final = "Mock plan\\nPLAN_STATUS=READY"
elif role == ".cline-terra":
    (pathlib.Path(cwd) / "implemented.txt").write_text("implemented\\n", encoding="utf-8")
    final = "Mock implementation\\nIMPLEMENTATION_STATUS=COMPLETE"
elif role == ".cline-opus":
    final = "Mock review plan\\nREVIEW_PLAN_STATUS=READY"
elif role == ".cline-sonnet":
    final = "Mock approval\\nPAF_REVIEW_VERDICT=APPROVED"
else:
    raise SystemExit("unknown profile")

print(json.dumps({"type": "agent_event", "event": {"type": "iteration_start", "iteration": 1}}), flush=True)
print(json.dumps({"type": "run_result", "finishReason": "completed", "text": final}), flush=True)
""",
                encoding="utf-8",
            )
            fake_cline.chmod(0o755)

            task = root / "task.md"
            task.write_text("# Mock task\n\nCreate implemented.txt.\n", encoding="utf-8")
            state_root = root / "runs"

            env = os.environ.copy()
            env.update(
                {
                    "PATH": f"{fake_bin}:{env.get('PATH', '')}",
                    "HOME": str(home),
                    "LITELLM_API_KEY": "smoke-key",
                    "OPENAI_API_KEY": "smoke-key",
                    "PAF_REPO": str(repo),
                    "PAF_BRANCH": "agent/mock-controller",
                    "PAF_BASE_BRANCH": "dev/panda",
                    "PAF_PUBLISH": "0",
                    "PAF_STAGE_MAX_ATTEMPTS": "1",
                    "PAF_REVIEW_STAGE_MAX_ATTEMPTS": "1",
                    "PAF_PRECOMMIT_VALIDATION_CMD": "test -f implemented.txt",
                    "PAF_STATE_ROOT": str(state_root),
                    "PAF_GLOBAL_LOG": str(root / "global.log"),
                }
            )
            result = subprocess.run(
                [str(self.root / "tools/paf-bootstrap/paf-cline-cycle"), str(task)],
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=45,
            )
            self.assertEqual(0, result.returncode, result.stdout + "\n" + result.stderr)
            latest = (state_root / "latest").resolve()
            summary = json.loads((latest / "run-summary.json").read_text(encoding="utf-8"))
            self.assertEqual("approved", summary["status"])
            self.assertEqual("sonnet-1", summary["role"])
            self.assertEqual("implemented\n", (repo / "implemented.txt").read_text(encoding="utf-8"))
            status = subprocess.run(
                ["git", "-C", str(repo), "status", "--porcelain"],
                text=True,
                stdout=subprocess.PIPE,
                check=True,
            ).stdout
            self.assertEqual("", status)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--bundle-root", required=True)
    args, remaining = parser.parse_known_args()
    ROOT = Path(args.bundle_root).resolve()
    unittest.main(argv=[__file__, *remaining])
