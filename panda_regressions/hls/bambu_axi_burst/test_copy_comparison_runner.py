#!/usr/bin/env python3
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).resolve().parent
REPO_ROOT = SOURCE_ROOT.parents[2]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)

PROFILES = {
    "1": "burst-two-bundles",
    "2": "legacy-two-bundles",
    "3": "legacy-shared",
    "4": "duplex-shared",
}
FEATURES = {"burst-two-bundles": "1", "legacy-two-bundles": "0",
            "legacy-shared": "0", "duplex-shared": "1"}
COMMON_SETTINGS = (
    "--top-fname=kernel", "--compiler=I386_CLANG16", "--generate-interface=INFER",
    "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL",
    "--mem-delay-read=64", "--mem-delay-write=2", "--tb-queue-size=32",
    "--memory-allocation-policy=NO_BRAM", "--distram-threshold=0", "--clock-period=3.33",
    "--no-clean", "--simulate", "--simulator=VERILATOR",
)


STUB_BAMBU = r'''#!/usr/bin/env python3
import fcntl
import json
import os
import pathlib
import re
import sys
import time

if sys.argv[1:] == ["--version"]:
    print("stub bambu 1.0")
    raise SystemExit(0)

args = sys.argv[1:]
profile_arg = next((item for item in args if item.startswith("--extra-cc-options=-DCOPY_COMPARISON_PROFILE=")), None)
if profile_arg is None:
    print("missing COPY_COMPARISON_PROFILE", file=sys.stderr)
    raise SystemExit(3)
profile_id = profile_arg.rsplit("=", 1)[1]
names = {"1": "burst-two-bundles", "2": "legacy-two-bundles", "3": "legacy-shared", "4": "duplex-shared"}
if profile_id not in names:
    print("unsupported profile", file=sys.stderr)
    raise SystemExit(3)
profile = names[profile_id]
cwd = pathlib.Path.cwd()
mode = os.environ.get("STUB_MODE", "")
target = os.environ.get("STUB_PROFILE", "duplex-shared")
if mode == "compiler-failure" and profile == target:
    print("requested simulated compiler failure", file=sys.stderr)
    raise SystemExit(7)

state = pathlib.Path(os.environ["STUB_STATE"])
lock_path = state / "lock"
with lock_path.open("a+") as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    active_path = state / "active"
    active = int(active_path.read_text()) if active_path.exists() else 0
    active += 1
    active_path.write_text(str(active))
    maximum_path = state / "maximum"
    maximum = int(maximum_path.read_text()) if maximum_path.exists() else 0
    maximum_path.write_text(str(max(maximum, active)))
    with (state / "runs.jsonl").open("a") as stream:
        stream.write(json.dumps({"profile": profile, "cwd": str(cwd), "tmp": os.environ.get("TMPDIR"),
                                 "args": args}) + "\n")
    fcntl.flock(lock, fcntl.LOCK_UN)
time.sleep(0.12)
with lock_path.open("a+") as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    active_path = state / "active"
    active_path.write_text(str(int(active_path.read_text()) - 1))
    fcntl.flock(lock, fcntl.LOCK_UN)

(cwd / "bambu_results.xml").write_text("<results/>\n")
if not (mode == "no-rtl" and profile == target):
    (cwd / "kernel.v").write_text("module kernel; endmodule\n")
if not (mode == "no-log" and profile == target):
    log_dir = cwd / "HLS_output" / "simulation"
    log_dir.mkdir(parents=True, exist_ok=True)
    if mode == "no-scenario" and profile == target:
        log = "COPY_COMPARISON_GOLDEN_PASS PROFILE=wrong-profile N=1024\n"
    elif mode == "incorrect-marker" and profile == target:
        log = f"COPY_COMPARISON_GOLDEN_PASS PROFILE={profile} N=1023\n"
    else:
        log = f"COPY_COMPARISON_GOLDEN_PASS PROFILE={profile} N=1024\n"
    (log_dir / "testbench.log").write_text(log)

if mode == "no-xml" and profile == target:
    (cwd / "bambu_results.xml").unlink()
if mode == "tamper-provenance" and profile == target:
    path = pathlib.Path(os.environ["STUB_OUT"]) / "provenance.txt"
    text = path.read_text()
    text = re.sub(r"(?m)^source_sha256=.*$", "source_sha256=" + "0" * 64, text)
    path.write_text(text)
'''


STUB_CHECKER = r'''#!/usr/bin/env python3
import hashlib
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
args = sys.argv[2:]
required = {"--source", "--testbench", "--bambu", "--settings", "--plugin", "--runner", "--report"}
if set(args[::2]) != required or len(args) != 14:
    print("checker received unexpected CLI", file=sys.stderr)
    raise SystemExit(9)
options = dict(zip(args[::2], args[1::2]))
prov = (out / "provenance.txt").read_text()
source = pathlib.Path(options["--source"])
expected = hashlib.sha256(source.read_bytes()).hexdigest()
recorded = next((line.split("=", 1)[1].split()[0] for line in prov.splitlines() if line.startswith("source_sha256=")), None)
if expected != recorded:
    print("source provenance digest mismatch", file=sys.stderr)
    raise SystemExit(10)
names = ["burst-two-bundles", "legacy-two-bundles", "legacy-shared", "duplex-shared"]
for name in names:
    for repeat in ("run1", "run2"):
        run = out / name / repeat
        if not (run / "bambu_results.xml").is_file() or not (run / "artifact-sha256.txt").is_file():
            print("missing run artifact", file=sys.stderr)
            raise SystemExit(11)
        text = (run / "provenance.txt").read_text()
        macro = {"burst-two-bundles": "1", "legacy-two-bundles": "2",
                 "legacy-shared": "3", "duplex-shared": "4"}[name]
        expected_fields = {"profile": name, "run": repeat, "macro": macro,
                           "source": str(pathlib.Path(options["--source"]).resolve()),
                           "testbench": str(pathlib.Path(options["--testbench"]).resolve())}
        for key, value in expected_fields.items():
            if f"{key}={value}\n" not in text:
                print("run provenance identity mismatch", file=sys.stderr)
                raise SystemExit(12)
        for key in ("--source", "--testbench", "--bambu", "--settings", "--plugin", "--runner"):
            path = pathlib.Path(options[key]).resolve()
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if f"{digest}  {path}\n" not in text:
                print("run provenance digest mismatch", file=sys.stderr)
                raise SystemExit(13)
pathlib.Path(options["--report"]).write_text(json.dumps({"profiles": names}) + "\n")
(out / "checker-called").write_text("yes\n")
'''


class CopyComparisonRunnerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="copy-comparison-runner-", dir=TMP_ROOT)
        self.root = Path(self.temp.name) / "repo"
        self.runner_dir = self.root / "panda_regressions" / "hls" / "bambu_axi_burst"
        self.runner_dir.mkdir(parents=True)
        self.script = self.runner_dir / "run_copy_comparison_tests.sh"
        for name in ("run_copy_comparison_tests.sh", "parallel_helpers.sh"):
            shutil.copy2(SOURCE_ROOT / name, self.runner_dir / name)
        for name in ("bambu_axi_burst_copy_comparison.cpp", "copy_comparison_tb.cpp"):
            (self.runner_dir / name).write_text("// stub fixture\n")
        checker = self.runner_dir / "check_copy_comparison_results.py"
        checker.write_text(STUB_CHECKER)
        checker.chmod(0o755)
        libtech = self.root / "etc" / "libtech" / "xilinx"
        libtech.mkdir(parents=True)
        (libtech / "xcu250-2Lfigd2104.xml").write_text("<device/>\n")
        self.prefix = self.root / "install"
        (self.prefix / "bin").mkdir(parents=True)
        (self.prefix / "lib" / "panda" / "clang-16").mkdir(parents=True)
        self.bambu = self.prefix / "bin" / "bambu"
        self.bambu.write_text(STUB_BAMBU)
        self.bambu.chmod(0o755)
        self.settings = self.prefix / "settings.sh"
        self.settings.write_text(f"export BAMBU_HLS='{self.prefix}'\n")
        (self.prefix / "lib" / "panda" / "clang-16" / "ASTAnalyzer.so").write_text("plugin\n")
        bin_dir = self.root / "stub-bin"
        bin_dir.mkdir()
        verilator = bin_dir / "verilator"
        verilator.write_text("#!/usr/bin/env bash\necho 'Verilator 5.000 stub'\n")
        verilator.chmod(0o755)
        (bin_dir / "grep").symlink_to(shutil.which("grep"))
        (bin_dir / "getconf").symlink_to(shutil.which("getconf"))
        (bin_dir / "sha256sum").symlink_to(shutil.which("sha256sum"))
        (bin_dir / "sed").symlink_to(shutil.which("sed"))
        (bin_dir / "find").symlink_to(shutil.which("find"))
        (bin_dir / "sort").symlink_to(shutil.which("sort"))
        (bin_dir / "cut").symlink_to(shutil.which("cut"))
        (bin_dir / "tail").symlink_to(shutil.which("tail"))
        self.bin_path = str(bin_dir) + os.pathsep + os.environ["PATH"]
        git = subprocess.run(["git", "init", "-q", str(self.root)], text=True, capture_output=True)
        self.assertEqual(git.returncode, 0, git.stderr)
        subprocess.run(["git", "-C", str(self.root), "config", "user.email", "test@example.invalid"], check=True)
        subprocess.run(["git", "-C", str(self.root), "config", "user.name", "Test"], check=True)
        subprocess.run(["git", "-C", str(self.root), "add", "."], check=True)
        subprocess.run(["git", "-C", str(self.root), "commit", "-qm", "fixture"], check=True)
        self.state = Path(self.temp.name) / "state"
        self.state.mkdir()
        self.out = Path(self.temp.name) / "out"

    def tearDown(self):
        self.temp.cleanup()

    def run_runner(self, *, outside="1", mode="", profile="duplex-shared", jobs="3"):
        if self.out.exists():
            shutil.rmtree(self.out)
        for path in self.state.iterdir():
            if path.is_file():
                path.unlink()
        env = os.environ.copy()
        scratch = self.root / "tmp"
        scratch.mkdir(parents=True, exist_ok=True)
        env.update({
            "PATH": self.bin_path,
            "TMPDIR": str(scratch), "TMP": str(scratch), "TEMP": str(scratch),
            "STUB_STATE": str(self.state), "STUB_OUT": str(self.out),
            "BURST_COPY_COMPARISON_OUTSIDE_SANDBOX": outside,
            "COPY_COMPARISON_JOBS": jobs,
        })
        env.pop("STUB_MODE", None)
        env.pop("STUB_PROFILE", None)
        if mode:
            env["STUB_MODE"] = mode
            env["STUB_PROFILE"] = profile
        return subprocess.run([str(self.script), str(self.bambu), str(self.out)], cwd=self.root,
                              env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_gate_rejects_before_creating_output(self):
        result = self.run_runner(outside="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run outside the sandbox", result.stdout)
        self.assertFalse(self.out.exists())

    def test_eight_invocations_are_isolated_parallel_and_marked(self):
        result = self.run_runner()
        self.assertEqual(result.returncode, 0, result.stdout)
        runs = [json.loads(line) for line in (self.state / "runs.jsonl").read_text().splitlines()]
        self.assertEqual(len(runs), 8)
        self.assertEqual(len({run["cwd"] for run in runs}), 8)
        self.assertEqual(len({run["tmp"] for run in runs}), 8)
        self.assertTrue(1 < int((self.state / "maximum").read_text()) <= 3)
        observed = {}
        common_argv = None
        for run in runs:
            profile_arg = next(arg for arg in run["args"] if arg.startswith("--extra-cc-options=-DCOPY_COMPARISON_PROFILE="))
            profile = PROFILES[profile_arg.rsplit("=", 1)[1]]
            self.assertIn("--simulator=VERILATOR", run["args"])
            feature_args = [arg for arg in run["args"]
                            if arg.startswith("--bambu-parameter=experimental-m-axi-burst=")]
            self.assertEqual(feature_args,
                             [f"--bambu-parameter=experimental-m-axi-burst={FEATURES[profile]}"])
            normalized = [arg for arg in run["args"] if arg not in feature_args and arg != profile_arg]
            if common_argv is None:
                common_argv = normalized
            self.assertEqual(normalized, common_argv)
            self.assertTrue(all(setting in normalized for setting in COMMON_SETTINGS))
            marker = f"COPY_COMPARISON_GOLDEN_PASS PROFILE={profile} N=1024"
            log = Path(run["cwd"]) / "HLS_output" / "simulation" / "testbench.log"
            self.assertEqual(log.read_text().strip(), marker)
            observed[profile] = observed.get(profile, 0) + 1
        self.assertEqual(observed, {profile: 2 for profile in PROFILES.values()})
        self.assertTrue((self.out / "comparison.json").is_file())
        self.assertTrue((self.out / "checker-called").is_file())

    def test_bad_job_count_fails_before_output_creation(self):
        result = self.run_runner(jobs="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("COPY_COMPARISON_JOBS", result.stdout)
        self.assertFalse(self.out.exists())

    def test_compiler_failure_stops_gate(self):
        result = self.run_runner(mode="compiler-failure")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((self.out / "checker-called").exists())

    def test_zero_exit_without_xml_fails_closed(self):
        result = self.run_runner(mode="no-xml")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing nonempty bambu_results.xml", result.stdout)

    def test_missing_rtl_or_log_fails_closed(self):
        for mode, expected in (("no-rtl", "missing generated RTL"),
                               ("no-log", "missing nonempty testbench log")):
            with self.subTest(mode=mode):
                result = self.run_runner(mode=mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(expected, result.stdout)

    def test_missing_or_incorrect_scenario_marker_fails_closed(self):
        for mode in ("no-scenario", "incorrect-marker"):
            with self.subTest(mode=mode):
                result = self.run_runner(mode=mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("missing exact testbench marker", result.stdout)
                self.assertFalse((self.out / "checker-called").exists())

    def test_provenance_tampering_is_rejected_by_final_checker(self):
        result = self.run_runner(mode="tamper-provenance")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("source provenance digest mismatch", result.stdout)


if __name__ == "__main__":
    unittest.main()
