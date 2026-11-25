# Author: Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

import os
from pathlib import Path
import signal
import sys
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch

import compiler_test_support as support
import compiler_case_manifest as manifest
import run_xml_profile_tests as xml_profile


TASK_TMP = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd() / "out_compiler_test_support"))
TASK_TMP.mkdir(parents=True, exist_ok=True)
os.environ["TMPDIR"] = str(TASK_TMP)
os.environ["TMP"] = str(TASK_TMP)
os.environ["TEMP"] = str(TASK_TMP)


class CompilerTestSupportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=TASK_TMP)
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def install(self, prefix_name="install"):
        prefix = self.root / prefix_name
        bambu = prefix / "bin/bambu"
        plugin = prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        bambu.parent.mkdir(parents=True)
        plugin.parent.mkdir(parents=True)
        bambu.write_text("compiler")
        plugin.write_text("plugin")
        return bambu, plugin

    def test_rejects_mismatched_install_and_plugin(self):
        bambu, plugin = self.install()
        other, other_plugin = self.install("other")
        with self.assertRaisesRegex(support.CompilerTestError, "BAMBU_HLS"):
            support.validate_install(bambu, other.parent.parent, plugin)
        with self.assertRaisesRegex(support.CompilerTestError, "ASTAnalyzer"):
            support.validate_install(bambu, bambu.parent.parent, other_plugin)

    def test_output_dir_must_be_fresh_and_not_symlink(self):
        exists = self.root / "exists"
        exists.mkdir()
        with self.assertRaisesRegex(support.CompilerTestError, "already exist"):
            support.create_output_dir(exists)
        target = self.root / "target"
        target.mkdir()
        link = self.root / "link"
        link.symlink_to(target, target_is_directory=True)
        with self.assertRaisesRegex(support.CompilerTestError, "already exist"):
            support.create_output_dir(link)

    def test_argv_rendering_is_shell_safe(self):
        rendered = support.format_argv(["bambu", "file with space.cpp", "--define=a'b"])
        self.assertEqual(rendered, "bambu 'file with space.cpp' '--define=a'\"'\"'b'")

    def test_expected_failure_requires_exact_diagnostic(self):
        script = self.root / "compiler.py"
        script.write_text("import sys; print('specific validation error'); sys.exit(2)\n")
        result = support.run_compiler([sys.executable, str(script)], cwd=self.root,
                                      timeout=3, expected_failure="specific validation error")
        self.assertEqual(result.returncode, 2)
        with self.assertRaisesRegex(support.CompilerTestError, "diagnostic not found"):
            support.run_compiler([sys.executable, str(script)], cwd=self.root,
                                 timeout=3, expected_failure="different error")

    def test_allow_success_is_explicit_and_nonzero_still_needs_diagnostic(self):
        success = self.root / "success.py"
        success.write_text("print('no diagnostic needed on success')\n")
        with self.assertRaisesRegex(support.CompilerTestError, "expected a diagnostic failure"):
            support.run_compiler([sys.executable, str(success)], cwd=self.root, timeout=3,
                                 expected_failure="required diagnostic")
        result = support.run_compiler([sys.executable, str(success)], cwd=self.root, timeout=3,
                                      expected_failure="required diagnostic", allow_success=True)
        self.assertEqual(result.returncode, 0)
        with self.assertRaisesRegex(support.CompilerTestError, "requires expected_failure"):
            support.run_compiler([sys.executable, str(success)], cwd=self.root, timeout=3,
                                 allow_success=True)

        failure = self.root / "failure.py"
        failure.write_text("import sys; print('different diagnostic'); sys.exit(2)\n")
        with self.assertRaisesRegex(support.CompilerTestError, "diagnostic not found"):
            support.run_compiler([sys.executable, str(failure)], cwd=self.root, timeout=3,
                                 expected_failure="required diagnostic", allow_success=True)

    def test_expected_failure_can_match_compiler_diagnostic_file(self):
        script = self.root / "compiler.py"
        script.write_text("import sys; print('generic failure'); sys.exit(2)\n")
        out = self.root / "case"
        cc_dir = out / "panda-temp"
        cc_dir.mkdir(parents=True)
        (cc_dir / "__cc_output1").write_text("plugin-specific validation message")
        result = support.run_compiler(
            [sys.executable, str(script)], cwd=self.root, timeout=3,
            expected_failure="plugin-specific validation message",
            diagnostic_output_dir=out)
        self.assertEqual(result.returncode, 2)

    def test_sandbox_marker_in_compiler_diagnostic_file_fails_closed(self):
        script = self.root / "compiler.py"
        script.write_text("import sys; print('generic failure'); sys.exit(2)\n")
        out = self.root / "case"
        cc_dir = out / "panda-temp"
        cc_dir.mkdir(parents=True)
        (cc_dir / "__cc_output1").write_text("Bad system call")
        with self.assertRaisesRegex(support.CompilerTestError, "sandbox failure"):
            support.run_compiler(
                [sys.executable, str(script)], cwd=self.root, timeout=3,
                expected_failure="generic failure", sandbox_markers=["Bad system call"],
                diagnostic_output_dir=out)

    def test_timeout_and_crash_fail_even_when_expected_text_is_present(self):
        sleepy = self.root / "sleepy.py"
        sleepy.write_text("import time; print('expected text', flush=True); time.sleep(2)\n")
        with self.assertRaisesRegex(support.CompilerTestError, "timed out"):
            support.run_compiler([sys.executable, str(sleepy)], cwd=self.root, timeout=0.05,
                                 expected_failure="expected text")
        crash = self.root / "crash.py"
        crash.write_text("import os, signal; print('expected text', flush=True); os.kill(os.getpid(), signal.SIGTERM)\n")
        with self.assertRaisesRegex(support.CompilerTestError, "crashed"):
            support.run_compiler([sys.executable, str(crash)], cwd=self.root, timeout=3,
                                 expected_failure="expected text")

    def test_timeout_terminates_command_descendants(self):
        child_pid_file = self.root / "child.pid"
        child_progress = self.root / "child.progress"
        parent = self.root / "parent.py"
        child_code = (
            "import pathlib, time\n"
            f"p = pathlib.Path({str(child_progress)!r})\n"
            "i = 0\n"
            "while True:\n"
            "    p.write_text(str(i))\n"
            "    i += 1\n"
            "    time.sleep(0.02)\n"
        )
        parent.write_text(
            "import subprocess, sys, time\n"
            f"child = subprocess.Popen([sys.executable, '-c', {child_code!r}])\n"
            f"open({str(child_pid_file)!r}, 'w').write(str(child.pid))\n"
            "print('child started', flush=True)\n"
            "time.sleep(30)\n",
            encoding="utf-8",
        )
        child_pid = None
        try:
            with self.assertRaisesRegex(support.CompilerTestError, "timed out"):
                support.run_compiler([sys.executable, str(parent)], cwd=self.root, timeout=0.2)
            child_pid = int(child_pid_file.read_text())
            progress = child_progress.read_text()
            time.sleep(0.1)
            self.assertEqual(child_progress.read_text(), progress)
        finally:
            if child_pid is not None:
                try:
                    os.kill(child_pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

    def test_external_sigterm_terminates_compiler_group_and_preserves_signal_exit(self):
        self.check_external_signal_terminates_compiler_group(signal.SIGTERM, -signal.SIGTERM)

    def test_external_sigint_terminates_compiler_group_and_preserves_signal_exit(self):
        self.check_external_signal_terminates_compiler_group(signal.SIGINT, -signal.SIGINT)

    def check_external_signal_terminates_compiler_group(self, signum, expected_returncode):
        compiler = self.root / "compiler.py"
        child_pid_file = self.root / "signal-child.pid"
        child_progress = self.root / "signal-child.progress"
        compiler_ready = self.root / "compiler.ready"
        child_code = (
            "import pathlib, time\n"
            f"p = pathlib.Path({str(child_progress)!r})\n"
            "i = 0\n"
            "while True:\n"
            "    p.write_text(str(i))\n"
            "    i += 1\n"
            "    time.sleep(0.02)\n"
        )
        compiler.write_text(
            "import subprocess, sys, time\n"
            "from pathlib import Path\n"
            f"child = subprocess.Popen([sys.executable, '-c', {child_code!r}])\n"
            f"open({str(child_pid_file)!r}, 'w').write(str(child.pid))\n"
            f"progress = Path({str(child_progress)!r})\n"
            "while not progress.exists(): time.sleep(0.01)\n"
            "print('compiler started', flush=True)\n"
            f"Path({str(compiler_ready)!r}).write_text('ready')\n"
            "time.sleep(30)\n",
            encoding="utf-8",
        )
        output = self.root / "signal-case"
        output.mkdir()
        wrapper = self.root / "wrapper.py"
        wrapper.write_text(
            "import sys\n"
            f"sys.path.insert(0, {str(Path(__file__).resolve().parent)!r})\n"
            "from pathlib import Path\n"
            "import compiler_test_support as support\n"
            f"support.run_compiler([sys.executable, {str(compiler)!r}], "
            f"cwd=Path({str(self.root)!r}), timeout=30, "
            f"diagnostic_output_dir=Path({str(output)!r}))\n",
            encoding="utf-8",
        )
        wrapper_process = subprocess.Popen(
            [sys.executable, str(wrapper)], cwd=self.root, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
        child_pid = None
        try:
            deadline = time.monotonic() + 5
            while not compiler_ready.exists() and time.monotonic() < deadline:
                time.sleep(0.02)
            self.assertTrue(compiler_ready.exists(), "compiler child did not start")
            child_pid = int(child_pid_file.read_text())
            wrapper_process.send_signal(signum)
            wrapper_process.communicate(timeout=5)
            self.assertEqual(wrapper_process.returncode, expected_returncode)
            self.assertIn("compiler started", (output / "stdout.log").read_text())
            self.assertEqual((output / "exit_code.txt").read_text(), "-9\n")
            progress = child_progress.read_text()
            time.sleep(0.1)
            self.assertEqual(child_progress.read_text(), progress)
        finally:
            if wrapper_process.poll() is None:
                wrapper_process.kill()
                wrapper_process.wait()
            if child_pid is not None:
                try:
                    os.killpg(child_pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

    def test_checks_compiler_diagnostic_files(self):
        out = self.root / "case"
        (out / "panda-temp").mkdir(parents=True)
        (out / "stdout.log").write_text("ordinary log")
        (out / "stderr.log").write_text("also ordinary")
        cc = out / "panda-temp/__cc_output0"
        cc.write_text("plugin: exact compiler diagnostic")
        self.assertIn(cc, support.check_compiler_diagnostics(out, "exact compiler diagnostic"))
        with self.assertRaisesRegex(support.CompilerTestError, "not found"):
            support.check_compiler_diagnostics(out, "missing diagnostic")

    def test_nonzero_exit_and_sandbox_marker_propagate(self):
        fail = self.root / "fail.py"
        fail.write_text("import sys; print('permission denied'); sys.exit(1)\n")
        out = self.root / "failed-case"
        out.mkdir()
        with self.assertRaisesRegex(support.CompilerTestError, "status 1"):
            support.run_compiler([sys.executable, str(fail)], cwd=self.root, timeout=3,
                                 diagnostic_output_dir=out)
        self.assertEqual((out / "stdout.log").read_text(), "permission denied\n")
        self.assertEqual((out / "stderr.log").read_text(), "")
        self.assertEqual((out / "exit_code.txt").read_text(), "1\n")
        with self.assertRaisesRegex(support.CompilerTestError, "sandbox failure"):
            support.run_compiler([sys.executable, str(fail)], cwd=self.root, timeout=3,
                                 expected_failure="permission denied",
                                 sandbox_markers=["permission denied"])

    def test_manifest_returns_harness_success_and_records_expected_compiler_status(self):
        bambu, _ = self.install()
        output = self.root / "manifest-case"
        result = support.CompilerResult((str(bambu),), 19, "", "")
        def run_and_write_diagnostic(*args, **kwargs):
            diagnostic_dir = output / "panda-temp"
            diagnostic_dir.mkdir()
            (diagnostic_dir / "__cc_output0").write_text("invalid profile diagnostic")
            return result
        with patch.dict(os.environ, {"BAMBU_HLS": str(bambu.parent.parent)}), \
                patch.object(support, "run_compiler", side_effect=run_and_write_diagnostic):
            status = manifest.execute_compiler(bambu, output, ["input.cpp"],
                                               "invalid profile diagnostic")
        self.assertEqual(status, 0)
        self.assertEqual((output / "exit_code.txt").read_text(), "19\n")

    def test_manifest_rejects_wrong_expected_diagnostic(self):
        bambu, _ = self.install()
        output = self.root / "manifest-case"
        compiler = self.root / "fake_compiler.py"
        compiler.write_text("import sys; print('wrong diagnostic'); sys.exit(19)\n")
        run_compiler = support.run_compiler
        def run_fake_compiler(argv, *, cwd, timeout, expected_failure, **kwargs):
            return run_compiler(
                [sys.executable, str(compiler)], cwd=cwd, timeout=timeout,
                expected_failure=expected_failure, **kwargs)
        with patch.dict(os.environ, {"BAMBU_HLS": str(bambu.parent.parent)}), \
                patch.object(support, "run_compiler", side_effect=run_fake_compiler):
            with self.assertRaisesRegex(support.CompilerTestError, "diagnostic not found"):
                manifest.execute_compiler(bambu, output, ["input.cpp"], "expected diagnostic")

    def test_xml_unbound_negative_passes_attribute_and_bundle_diagnostic(self):
        case_dir = self.root / "xml-invalid"
        case_dir.mkdir()
        result = support.CompilerResult(("bambu",), 19, "diagnostic", "")
        expected = "Invalid AXI interface attribute 'read_fifo_depth' for bundle 'unbound_read'"
        with patch.object(xml_profile.compiler_support, "run_compiler", return_value=result) as run:
            xml_profile.invoke(Path("bambu"), case_dir, Path("input.raw"), Path("arch.xml"),
                               "top", "m-axi-burst", expected_failure=expected)
        self.assertEqual(run.call_args.kwargs["expected_failure"], expected)
        self.assertEqual((case_dir / "exit_code.txt").read_text(), "19\n")


if __name__ == "__main__":
    unittest.main()
