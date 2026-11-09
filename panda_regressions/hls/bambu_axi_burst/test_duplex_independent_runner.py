#!/usr/bin/env python3
from __future__ import annotations

import fcntl
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


FAMILY = Path(__file__).resolve().parent
REPO = FAMILY.parents[2]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)

STUB_BAMBU = r'''#!/usr/bin/env python3
import fcntl
import json
import os
import pathlib
import sys
import time

if sys.argv[1:] == ["--version"]:
    print("Bambu stub 1.0")
    raise SystemExit(0)
args = sys.argv[1:]
macro_arg = next((arg for arg in args if arg.startswith("--extra-cc-options=-DDUPLEX_INDEPENDENT_PROFILE=")), "")
macro = macro_arg.rsplit("=", 1)[-1]
profiles = {"1": "symmetric", "2": "asymmetric"}
if macro not in profiles:
    print("wrong profile macro", file=sys.stderr)
    raise SystemExit(3)
profile = profiles[macro]
if "--bambu-parameter=m-axi-burst=1" not in args:
    print("feature was not enabled", file=sys.stderr)
    raise SystemExit(4)
state = pathlib.Path(os.environ["STUB_STATE"])
with (state / "state.json").open("r+") as stream:
    fcntl.flock(stream, fcntl.LOCK_EX)
    current = json.load(stream)
    current["active"] += 1
    current["max_active"] = max(current["max_active"], current["active"])
    stream.seek(0)
    json.dump(current, stream)
    stream.truncate()
    fcntl.flock(stream, fcntl.LOCK_UN)
with (state / "runs.jsonl").open("a") as stream:
    fcntl.flock(stream, fcntl.LOCK_EX)
    stream.write(json.dumps({"profile": profile, "cwd": str(pathlib.Path.cwd()),
                             "tmp": os.environ.get("TMPDIR"), "args": args}) + "\n")
    stream.flush()
    fcntl.flock(stream, fcntl.LOCK_UN)
if os.environ.get("STUB_MODE") == "compiler-failure" and profile == os.environ.get("STUB_PROFILE"):
    print("requested compiler failure", file=sys.stderr)
    raise SystemExit(7)
time.sleep(0.15)
cwd = pathlib.Path.cwd()
(cwd / "bambu_results.xml").write_text("<results/>\n")
(cwd / "bambu_axi_burst_duplex_independent.v").write_text("module bambu_axi_burst_duplex_independent; endmodule\n")
if any(arg.startswith("--debug-classes=") for arg in args):
    (cwd / "panda-temp").mkdir()
    (cwd / "panda-temp" / "before_Frontend::InterfaceInfer.raw").write_text("raw IR witness\n")
log_dir = cwd / "HLS_output" / "simulation"
log_dir.mkdir(parents=True)
marker_profile = profile
if os.environ.get("STUB_MODE") == "wrong-marker" and profile == os.environ.get("STUB_PROFILE"):
    marker_profile = "unexpected_profile"
log = "no golden marker\n"
if os.environ.get("STUB_MODE") != "missing-marker" or profile != os.environ.get("STUB_PROFILE"):
    log = f"DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE={marker_profile} N=38\n"
(log_dir / "testbench.log").write_text(log)
if os.environ.get("STUB_MODE") == "shared-runner-modified" and profile == os.environ.get("STUB_PROFILE"):
    shared = pathlib.Path(os.environ["STUB_SHARED_RUNNER"])
    shared.write_text(shared.read_text() + "\\n# modified during run\\n")
with (state / "state.json").open("r+") as stream:
    fcntl.flock(stream, fcntl.LOCK_EX)
    current = json.load(stream)
    current["active"] -= 1
    stream.seek(0)
    json.dump(current, stream)
    stream.truncate()
    fcntl.flock(stream, fcntl.LOCK_UN)
'''

STUB_CHECKER = r'''#!/usr/bin/env python3
import hashlib
import json
import os
import pathlib
import sys

run_root = pathlib.Path(sys.argv[1])
developer_proof = "--developer-proof" in sys.argv[2:]
checker_args = [arg for arg in sys.argv[2:] if arg != "--developer-proof"]
options = dict(zip(checker_args[::2], checker_args[1::2]))
required = {"--source", "--testbench", "--bambu", "--settings", "--plugin",
            "--simulator", "--simulator-invocation", "--runner", "--report"}
if set(options) != required or len(checker_args) != 2 * len(required):
    raise SystemExit("unexpected checker arguments")
expected = {"symmetric": ("1", "16/1/64", "16/1/32"),
            "asymmetric": ("2", "16/2/64", "8/1/16")}
for profile, (macro, read_profile, write_profile) in expected.items():
    path = run_root / profile
    fields = dict(line.split("=", 1) for line in (path / "provenance.txt").read_text().splitlines())
    if (fields.get("macro"), fields.get("feature"), fields.get("read_profile"),
        fields.get("write_profile"), fields.get("proof_mode")) != \
            (macro, "1", read_profile, write_profile, "developer" if developer_proof else "functional"):
        raise SystemExit("profile provenance mismatch")
    helper = pathlib.Path(fields["parallel_helper"])
    if not helper.is_file() or fields.get("parallel_helper_sha256") is None:
        raise SystemExit("parallel helper provenance missing")
    shared = pathlib.Path(fields["shared_runner"])
    if not shared.is_file() or hashlib.sha256(shared.read_bytes()).hexdigest() != fields.get("shared_runner_sha256"):
        raise SystemExit("shared runner provenance mismatch")
    if options.get("--simulator-invocation") != fields.get("simulator_invocation"):
        raise SystemExit("simulator invocation argument does not match provenance")
    manifest = path / "artifact-sha256.txt"
    if not manifest.is_file():
        raise SystemExit("missing artifact manifest")
    raw = "panda-temp/before_Frontend::InterfaceInfer.raw"
    has_raw = any(line.endswith("  " + raw) for line in manifest.read_text().splitlines())
    if has_raw != developer_proof:
        raise SystemExit("raw IR manifest membership does not match proof mode")
if os.environ.get("STUB_MODE") == "checker-failure":
    raise SystemExit("requested checker failure")
pathlib.Path(options["--report"]).write_text(json.dumps({"profiles": list(expected)}) + "\n")
(run_root / "checker-called").write_text("yes\n")
print("stub checker pass")
'''


class DuplexIndependentRunnerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="duplex-independent-runner-", dir=TMP_ROOT)
        self.scratch = Path(self.temp.name)
        self.repo = self.scratch / "repo"
        self.family = self.repo / "panda_regressions/hls/bambu_axi_burst"
        self.family.mkdir(parents=True)
        self.runner = self.family / "run_duplex_independent_tests.sh"
        shutil.copy2(FAMILY / self.runner.name, self.runner)
        shutil.copy2(FAMILY / "run_duplex_generated_family.sh", self.family / "run_duplex_generated_family.sh")
        shutil.copy2(FAMILY / "parallel_helpers.sh", self.family / "parallel_helpers.sh")
        (self.family / "bambu_axi_burst_duplex_independent.cpp").write_text("// fixture\n")
        (self.family / "duplex_independent_tb.cpp").write_text("// testbench\n")
        self.checker = self.family / "check_duplex_independent_results.py"
        self.checker.write_text(STUB_CHECKER)
        self.checker.chmod(0o755)
        (self.repo / "etc/libtech/xilinx").mkdir(parents=True)
        (self.repo / "etc/libtech/xilinx/xcu250-2Lfigd2104.xml").write_text("<device/>\n")
        self.prefix = self.repo / "install"
        self.bambu = self.prefix / "bin/bambu"
        self.bambu.parent.mkdir(parents=True)
        self.bambu.write_text(STUB_BAMBU)
        self.bambu.chmod(0o755)
        self.settings = self.prefix / "settings.sh"
        self.settings.write_text(f"export BAMBU_HLS='{self.prefix}'\n")
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("plugin\n")
        self.modelsim = self.scratch / "bin"
        self.modelsim.mkdir()
        for tool in ("vlib", "vmap", "vlog"):
            path = self.modelsim / tool
            path.write_text("#!/usr/bin/env bash\necho 'ModelSim 2026.10 stub'\n")
            path.chmod(0o755)
        vco = self.modelsim / "vco"
        vco.write_text("#!/usr/bin/env bash\n[[ ${0##*/} == vsim ]] || exit 99\necho 'ModelSim 2026.10 stub'\n")
        vco.chmod(0o755)
        (self.modelsim / "vsim").symlink_to(vco.name)
        self.state = self.scratch / "state"
        self.state.mkdir()
        (self.state / "state.json").write_text(json.dumps({"active": 0, "max_active": 0}))
        self.output = self.repo / "out_duplex_independent"
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.modelsim}:{self.env['PATH']}",
            "TMPDIR": str(self.scratch),
            "TMP": str(self.scratch),
            "TEMP": str(self.scratch),
            "STUB_STATE": str(self.state),
            "STUB_SHARED_RUNNER": str(self.family / "run_duplex_generated_family.sh"),
            "BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX": "1",
            "DUPLEX_INDEPENDENT_JOBS": "2",
        })
        subprocess.run(["git", "init", "-q", str(self.repo)], check=True)
        subprocess.run(["git", "-C", str(self.repo), "config", "user.email", "test@example.invalid"], check=True)
        subprocess.run(["git", "-C", str(self.repo), "config", "user.name", "Test"], check=True)
        subprocess.run(["git", "-C", str(self.repo), "add", "."], check=True)
        subprocess.run(["git", "-C", str(self.repo), "commit", "-qm", "fixture"], check=True)

    def tearDown(self):
        self.temp.cleanup()

    def run_runner(self, *, outside="1", jobs="2", output=None, mode="", profile="symmetric",
                   developer_proof=False):
        env = self.env.copy()
        env["BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX"] = outside
        env["DUPLEX_INDEPENDENT_JOBS"] = jobs
        env.pop("STUB_MODE", None)
        env.pop("STUB_PROFILE", None)
        if mode:
            env["STUB_MODE"] = mode
            env["STUB_PROFILE"] = profile
        args = [str(self.runner)]
        if developer_proof:
            args.append("--development-proof")
        args.append(str(self.bambu))
        if output is not None:
            args.append(str(output))
        return subprocess.run(args, cwd=self.repo, env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def reset_output_and_state(self):
        if self.output.exists():
            shutil.rmtree(self.output)
        (self.state / "runs.jsonl").unlink(missing_ok=True)
        (self.state / "state.json").write_text(json.dumps({"active": 0, "max_active": 0}))
        self.repo.mkdir(parents=True, exist_ok=True)

    def test_requires_outside_sandbox_opt_in(self):
        result = self.run_runner(outside="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run outside the sandbox", result.stdout)
        self.assertFalse(self.output.exists())

    def test_invalid_jobs_and_output_fail_before_creation(self):
        for invalid_jobs in ("0", "-1", "bad"):
            with self.subTest(jobs=invalid_jobs):
                self.reset_output_and_state()
                result = self.run_runner(jobs=invalid_jobs)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("DUPLEX_INDEPENDENT_JOBS", result.stdout)
                self.assertFalse(self.output.exists())
        outside = self.scratch / "outside-output"
        outside.mkdir()
        result = self.run_runner(output=outside)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be fresh", result.stdout)
        self.output.parent.mkdir(parents=True, exist_ok=True)
        self.output.mkdir()
        result = self.run_runner()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be fresh", result.stdout)

    def test_two_profiles_use_isolated_directories_and_bounded_parallelism(self):
        result = self.run_runner()
        self.assertEqual(result.returncode, 0, result.stdout)
        runs = [json.loads(line) for line in (self.state / "runs.jsonl").read_text().splitlines()]
        self.assertEqual({run["profile"] for run in runs}, {"symmetric", "asymmetric"})
        self.assertEqual(len({run["cwd"] for run in runs}), 2)
        self.assertEqual(len({run["tmp"] for run in runs}), 2)
        self.assertEqual(json.loads((self.state / "state.json").read_text())["max_active"], 2)
        profiles = {"symmetric": ("1", "16/1/64", "16/1/32"),
                    "asymmetric": ("2", "16/2/64", "8/1/16")}
        for run in runs:
            args = run["args"]
            self.assertIn("--top-fname=bambu_axi_burst_duplex_independent", args)
            self.assertIn("--simulate", args)
            self.assertIn("--simulator=MODELSIM", args)
            self.assertEqual("--debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based" in args,
                             False)
            self.assertIn("--bambu-parameter=m-axi-burst=1", args)
            self.assertIn("--tb-param-size=src:16384", args)
            self.assertIn("--tb-param-size=dst:16384", args)
            self.assertIn("--addr-bus-bitsize=64", args)
            self.assertTrue(any(arg == "--generate-tb=" + str((self.family / "duplex_independent_tb.cpp").resolve())
                                for arg in args))
            macro, read_profile, write_profile = profiles[run["profile"]]
            self.assertIn(f"--extra-cc-options=-DDUPLEX_INDEPENDENT_PROFILE={macro}", args)
            provenance = dict(line.split("=", 1) for line in
                              (self.output / run["profile"] / "provenance.txt").read_text().splitlines())
            self.assertEqual(provenance["proof_mode"], "functional")
            self.assertEqual(provenance["simulator_invocation"], str(self.modelsim / "vsim"))
            self.assertEqual(provenance["simulator"], str((self.modelsim / "vsim").resolve()))
            self.assertEqual(provenance["simulator_version"], "ModelSim 2026.10 stub")
            self.assertEqual(provenance["read_profile"], read_profile)
            self.assertEqual(provenance["write_profile"], write_profile)
            self.assertEqual(provenance["parallel_helper"], str((self.family / "parallel_helpers.sh").resolve()))
            self.assertIn("parallel_helper_sha256", provenance)
        self.assertEqual((self.output / "aggregate.log").read_text(),
                         "DUPLEX_INDEPENDENT_PASS profiles=symmetric,asymmetric output="
                         + str(self.output.resolve()) + "\n")
        self.assertTrue((self.output / "checker-called").is_file())

    def test_development_proof_is_explicit_and_preserves_raw_witnesses(self):
        result = self.run_runner(developer_proof=True)
        self.assertEqual(result.returncode, 0, result.stdout)
        runs = [json.loads(line) for line in (self.state / "runs.jsonl").read_text().splitlines()]
        self.assertTrue(all("--debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based" in run["args"]
                            for run in runs))
        for profile in ("symmetric", "asymmetric"):
            raw = self.output / profile / "panda-temp/before_Frontend::InterfaceInfer.raw"
            self.assertTrue(raw.is_file())

    def test_compiler_failure_prevents_checker_and_pass_marker(self):
        result = self.run_runner(mode="compiler-failure", profile="asymmetric")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((self.output / "checker-called").exists())
        self.assertNotIn("DUPLEX_INDEPENDENT_PASS", (self.output / "aggregate.log").read_text())

    def test_missing_or_wrong_golden_marker_fails(self):
        for mode in ("missing-marker", "wrong-marker"):
            with self.subTest(mode=mode):
                self.reset_output_and_state()
                result = self.run_runner(mode=mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("missing exact testbench marker", result.stdout)
                self.assertFalse((self.output / "checker-called").exists())

    def test_checker_failure_prevents_pass_marker(self):
        result = self.run_runner(mode="checker-failure")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("requested checker failure", result.stdout)
        self.assertNotIn("DUPLEX_INDEPENDENT_PASS", (self.output / "aggregate.log").read_text())

    def test_shared_runner_mutation_fails_provenance(self):
        result = self.run_runner(mode="shared-runner-modified")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("shared runner provenance mismatch", result.stdout)
        self.assertNotIn("DUPLEX_INDEPENDENT_PASS", (self.output / "aggregate.log").read_text())


if __name__ == "__main__":
    unittest.main()
