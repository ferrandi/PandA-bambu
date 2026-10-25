#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Offline checks for the shared logging functions and the standalone copier."""

import ast
import os
from pathlib import Path
import select
import shutil
import subprocess
from tempfile import TemporaryDirectory
import time
import unittest


ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "etc/libtech/backend/utils/simulation_wrapper.sh"
LOGGING = ROOT / "etc/libtech/backend/utils/record_output.sh"
TOOL_SOURCE = ROOT / "src" / "utility" / "record_output.cpp"
PAYLOAD = bytes(range(256)) * 4096 + b"\nlast fragment without newline"


class SimulationLoggingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        scratch = Path(os.environ.get("BURST_TEST_TMPDIR", ROOT / "documentation/tmp"))
        scratch.mkdir(parents=True, exist_ok=True)
        cls.scratch = scratch
        cls.build_dir = TemporaryDirectory(dir=scratch)
        compiler = None
        for candidate in (os.environ.get("CXX"), "c++", "g++", "clang++"):
            if not candidate or not shutil.which(candidate):
                continue
            probe = subprocess.run([candidate, "--version"], capture_output=True, timeout=30)
            if probe.returncode == 0:
                compiler = candidate
                break
        if compiler is None:
            raise unittest.SkipTest("no C++ compiler to build the record_output tool")
        cls.tool = Path(cls.build_dir.name) / "record_output"
        compile_result = subprocess.run(
            [compiler, "-std=c++17", str(TOOL_SOURCE), "-o", str(cls.tool)],
            capture_output=True,
            timeout=120,
        )
        if compile_result.returncode != 0:
            # The compiler is available, so this is a failure compiling the
            # product source, e.g., a regression: it must fail the check, it
            # must never turn into a green run without any coverage.
            raise RuntimeError(
                f"{compiler} failed to build the record_output tool: "
                + compile_result.stderr.decode(errors="replace")
            )

    @classmethod
    def tearDownClass(cls):
        cls.build_dir.cleanup()

    def setUp(self):
        self.temporary = TemporaryDirectory(dir=self.scratch)
        self.addCleanup(self.temporary.cleanup)
        self.work = Path(self.temporary.name)
        self.simulation = self.work / "HLS_output/simulation"
        self.simulation.mkdir(parents=True)
        utils = self.work / "HLS_output/utils"
        utils.mkdir()
        shutil.copy(LOGGING, utils / LOGGING.name)
        self.wrapper = self.simulation / WRAPPER.name
        shutil.copy(WRAPPER, self.wrapper)
        (self.work / "bambu_time_simulation.txt").write_text("0|2\n", encoding="utf-8")
        # A fake installation tree: the wrapper resolves the copier through
        # ${BAMBU_HLS}/bin/record_output, exactly as the installed flow does.
        self.bambu_hls = self.work / "fake install"
        (self.bambu_hls / "bin").mkdir(parents=True)
        shutil.copy(self.tool, self.bambu_hls / "bin" / "record_output")
        self.payload_file = self.work / "payload"
        self.payload_file.write_bytes(PAYLOAD)
        self.producer = self.work / "producer"
        self.producer.write_text(
            "#!/usr/bin/env python3\n"
            "import os, sys\n"
            "sys.stdout.buffer.write(bytes(range(256)) * 4096)\n"
            "sys.stdout.buffer.flush()\n"
            "sys.stderr.buffer.write(b'\\nlast fragment without newline')\n"
            "sys.stderr.buffer.flush()\n"
            "sys.exit(int(os.environ.get('PRODUCER_STATUS', '0')))\n",
            encoding="utf-8",
        )
        self.producer.chmod(0o755)
        self.setup = self.work / "setup.sh"
        self.setup.write_text(
            "tee() { echo 'unexpected external tee invocation' >&2; return 134; }\n"
            "export -f tee\n"
            "export BAMBU_IPC_SIM_CMD=\"run_logged \\\"${SWD}/simulation.log\\\" \\\"${RECORD_PRODUCER}\\\"\"\n",
            encoding="utf-8",
        )
        self.bootstrap = self.work / "bootstrap.sh"
        self.bootstrap.write_text(
            "bambu_results() {\n"
            "  case \"$1\" in\n"
            "    /application/sources@compiler) printf /bin/true ;;\n"
            "    /application/backend@parallel) printf 1 ;;\n"
            "  esac\n"
            "}\n"
            "make() { return 0; }\n"
            "envsubst() { cat; }\n"
            'source "$1" "$2"\n',
            encoding="utf-8",
        )

    def run_wrapper(self, producer_status=0, nested=False, invalid_log=False, bash_producer=False,
                    extra_path=None, executable=None):
        if executable is None:
            executable = self.bash_producer() if bash_producer else self.producer
        if nested:
            executable = self.work / "driver"
            executable.write_text(
                '#!/usr/bin/env bash\nexec bash -c "$BAMBU_IPC_SIM_CMD"\n',
                encoding="utf-8",
            )
            executable.chmod(0o755)
        log = self.simulation / f"{executable.name}.log"
        if invalid_log:
            log.mkdir()
            self.producer.write_text("#!/usr/bin/env bash\nexit 0\n", encoding="utf-8")
        environment = dict(os.environ)
        if extra_path:
            environment["PATH"] = str(extra_path) + os.pathsep + environment.get("PATH", "")
        environment.update(
            BAMBU_HLS=str(self.bambu_hls),
            BAMBU_HLS_OUTDIR=str(self.work),
            OUT_LVL="1",
            TARGET="testbench",
            SYS_ELF=str(executable),
            SWD=str(self.work / "backend with spaces"),
            RECORD_PRODUCER=str(self.producer),
            PAYLOAD_FILE=str(self.payload_file),
            PRODUCER_STATUS=str(producer_status),
            TMPDIR=str(self.work),
            TMP=str(self.work),
            TEMP=str(self.work),
        )
        Path(environment["SWD"]).mkdir(exist_ok=True)
        result = subprocess.run(
            ["bash", str(self.bootstrap), str(self.wrapper), str(self.setup)],
            cwd=self.work,
            env=environment,
            capture_output=True,
            timeout=30,
        )
        return result, log

    def bash_producer(self):
        """A producer that does not need python3, to check the python-free flow."""
        producer = self.work / "bash_producer"
        producer.write_text(
            "#!/usr/bin/env bash\n"
            'cat "${PAYLOAD_FILE}"\n'
            'exit "${PRODUCER_STATUS:-0}"\n',
            encoding="utf-8",
        )
        producer.chmod(0o755)
        return producer

    def test_binary_output_survives_large_pipe_and_broken_system_tee(self):
        result, log = self.run_wrapper()
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(log.read_bytes(), PAYLOAD)
        self.assertIn(PAYLOAD, result.stdout)
        self.assertNotIn(b"unexpected external tee invocation", result.stderr)

    def test_copier_is_found_in_the_cmake_build_tree_layout(self):
        """Uninstalled build: BAMBU_HLS is the build root, the tool is in <build>/src."""
        (self.bambu_hls / "src").mkdir()
        (self.bambu_hls / "bin" / "record_output").rename(self.bambu_hls / "src" / "record_output")
        result, log = self.run_wrapper()
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_missing_copier_preserves_the_producer_and_log(self):
        (self.bambu_hls / "bin/record_output").unlink()
        for status in (0, 7):
            with self.subTest(status=status):
                result, log = self.run_wrapper(producer_status=status, bash_producer=True)
                self.assertEqual(result.returncode, status, result.stderr.decode(errors="replace"))
                self.assertEqual(log.read_bytes(), PAYLOAD)
                self.assertIn(b"record_output not found", result.stderr)
                self.assertIn(str(self.bambu_hls).encode(), result.stderr)
                self.assertNotIn(b"producer failed (141)", result.stderr)
        result, _ = self.run_wrapper(invalid_log=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"record_output not found", result.stderr)
        self.assertNotIn(b"producer failed", result.stderr)

    def test_unloadable_copier_preserves_the_producer_and_log(self):
        tool = self.bambu_hls / "bin/record_output"
        tool.write_text("#!/nonexistent/panda-record-output-loader\n", encoding="utf-8")
        for status in (0, 7):
            with self.subTest(status=status):
                result, log = self.run_wrapper(producer_status=status, bash_producer=True)
                self.assertEqual(result.returncode, status, result.stderr.decode(errors="replace"))
                self.assertEqual(log.read_bytes(), PAYLOAD)
                self.assertIn(b"cannot start", result.stderr)
                self.assertNotIn(b"producer failed (141)", result.stderr)

    def test_nonblocking_console_resumes_after_backpressure(self):
        read_fd, write_fd = os.pipe()
        copier = None
        try:
            os.set_blocking(write_fd, False)
            prefix = b""
            while True:
                try:
                    prefix += b"x" * os.write(write_fd, b"x" * 4096)
                except BlockingIOError:
                    break
            log = self.work / "nonblocking.log"
            with open(self.payload_file, "rb") as payload:
                copier = subprocess.Popen([str(self.tool), str(log)], stdin=payload,
                                          stdout=write_fd, stderr=subprocess.PIPE)
            os.close(write_fd)
            write_fd = -1
            deadline = time.monotonic() + 10
            while not log.exists() or log.stat().st_size == 0:
                self.assertLess(time.monotonic(), deadline, "copier did not start")
                time.sleep(0.005)
            console = bytearray()
            while True:
                remaining = deadline - time.monotonic()
                self.assertGreater(remaining, 0, "copier stalled")
                self.assertTrue(select.select([read_fd], [], [], remaining)[0], "copier stalled")
                chunk = os.read(read_fd, 65536)
                if not chunk:
                    break
                console.extend(chunk)
            _, errors = copier.communicate(timeout=5)
            self.assertEqual(copier.returncode, 0, errors.decode(errors="replace"))
            self.assertEqual(log.read_bytes(), PAYLOAD)
            self.assertEqual(console, prefix + PAYLOAD)
            self.assertNotIn(b"console copy failed", errors)
        finally:
            if copier is not None:
                if copier.poll() is None:
                    copier.kill()
                copier.communicate(timeout=5)
            os.close(read_fd)
            if write_fd >= 0:
                os.close(write_fd)

    def test_nonblocking_console_timeout_keeps_the_log_complete(self):
        read_fd, write_fd = os.pipe()
        try:
            os.set_blocking(write_fd, False)
            while True:
                try:
                    os.write(write_fd, b"x" * 4096)
                except BlockingIOError:
                    break
            log = self.work / "blocked-console.log"
            with open(self.payload_file, "rb") as payload:
                result = subprocess.run([str(self.tool), str(log)], stdin=payload,
                                        stdout=write_fd, stderr=subprocess.PIPE, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(log.read_bytes(), PAYLOAD)
            self.assertIn(b"console copy failed", result.stderr)
        finally:
            os.close(read_fd)
            os.close(write_fd)

    def test_unopenable_log_still_drains_the_input(self):
        """The producer completes and is not killed by SIGPIPE; the failure is reported."""
        log = self.work / "missing-dir" / "log"
        with open(self.payload_file, "rb") as payload:
            result = subprocess.run([str(self.tool), str(log)], stdin=payload,
                                    capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, PAYLOAD)
        self.assertIn(b"cannot open log file", result.stderr)

    def test_producer_failure_is_preserved(self):
        result, log = self.run_wrapper(producer_status=7)
        self.assertEqual(result.returncode, 7)
        self.assertIn(b"producer failed (7)", result.stderr)
        self.assertIn(str(log.relative_to(self.work)).encode(), result.stderr)
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_producer_crash_by_signal_does_not_hang_and_is_reported(self):
        """A logged tool killing itself must be reported, not deadlock the flow."""
        crasher = self.work / "crasher"
        crasher.write_text(
            "#!/usr/bin/env bash\n"
            'cat "${PAYLOAD_FILE}"\n'
            "kill -9 $$\n",
            encoding="utf-8",
        )
        crasher.chmod(0o755)
        result, log = self.run_wrapper(executable=crasher)
        self.assertEqual(result.returncode, 137)
        self.assertIn(b"producer failed (137)", result.stderr)
        self.assertEqual(log.read_bytes(), PAYLOAD)

    @staticmethod
    def extract_shell_constant(name):
        """The product text of a shell constant of file_IO_constants.hpp."""
        text = (ROOT / "src" / "constants" / "file_IO_constants.hpp").read_text(encoding="utf-8")
        anchor = text.index(name)
        start = text.index('R"sh(', anchor) + len('R"sh(')
        end = text.index(')sh"', start)
        return text[start:end]

    def run_generated_pipeline(self, producer_command, log_path):
        """Run the real PandaSystem generated-script tail after a real
        producer | record_output pipeline, as the generated command scripts do."""
        script = self.work / "command_script.sh"
        script.write_text(
            "#!/usr/bin/env bash\n"
            f'( {producer_command} ) 2>&1 | "{self.tool}" {log_path}\n'
            + self.extract_shell_constant("SHELL_CST_record_output_status"),
            encoding="utf-8",
        )
        return subprocess.run(["bash", "-f", str(script)], capture_output=True, timeout=30)

    def test_command_scripts_success_keeps_success(self):
        log = self.work / "tail_ok.log"
        result = self.run_generated_pipeline(f'cat "{self.payload_file}"', str(log))
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_command_scripts_producer_failure_wins(self):
        log = self.work / "tail_producer.log"
        result = self.run_generated_pipeline(f'cat "{self.payload_file}"; exit 7', str(log))
        self.assertEqual(result.returncode, 7)
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_command_scripts_never_hide_the_copier_failure(self):
        """A log opened but unwritable (/dev/full): the copier drains the
        producer, so only an explicit check of its status avoids reporting
        the run as a success with an incomplete log."""
        if not os.path.exists("/dev/full"):
            self.skipTest("/dev/full not available")
        result = self.run_generated_pipeline("head -c 2097152 /dev/zero", "/dev/full")
        self.assertEqual(result.returncode, 1, result.stderr.decode(errors="replace"))
        self.assertIn(b"log writer failed (1)", result.stderr)

    def test_log_writer_failure_is_not_hidden(self):
        result, _ = self.run_wrapper(invalid_log=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"log writer failed (1)", result.stderr)

    def test_nested_simulator_inherits_logging_and_failure_status(self):
        for producer_status in (0, 7):
            with self.subTest(producer_status=producer_status):
                result, _ = self.run_wrapper(producer_status=producer_status, nested=True)
                self.assertEqual(result.returncode, producer_status,
                                 result.stderr.decode(errors="replace"))
                simulator_log = self.work / "backend with spaces/simulation.log"
                self.assertEqual(simulator_log.read_bytes(), PAYLOAD)
                self.assertIn(PAYLOAD, result.stdout)
                self.assertNotIn(b"unexpected external tee invocation", result.stderr)

    def test_generated_backend_loads_the_shared_logging(self):
        backend = self.work / "backend with spaces"
        utils = backend / "HLS_output/utils"
        utils.mkdir(parents=True)
        shutil.copy(LOGGING, utils / LOGGING.name)
        source = (ROOT / "src/wrapper/BackendWrapper.cpp").read_text(encoding="utf-8")
        loader = ast.literal_eval(next(line.strip() for line in source.splitlines()
                                       if "/HLS_output/utils/record_output.sh" in line))
        environment = dict(os.environ, BAMBU_HLS=str(self.bambu_hls), BAMBU_HLS_OUTDIR=str(backend))
        for producer_status, bad_log in ((0, False), (7, False), (0, True), (7, True)):
            with self.subTest(producer_status=producer_status, bad_log=bad_log):
                log = backend / ("missing/log" if bad_log else "backend.log")
                environment["PRODUCER_STATUS"] = str(producer_status)
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c",
                     loader + '\nbash -c \'run_logged "$1" "$2"\' bash "$1" "$2"\n',
                     "bash", str(log), str(self.producer)],
                    env=environment, capture_output=True, timeout=30,
                )
                self.assertEqual(result.returncode, producer_status or int(bad_log),
                                 result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, PAYLOAD)
                if not bad_log:
                    self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_simulation_flow_needs_no_python3(self):
        """With a poisoned python3 on PATH the logging flow still runs fine."""
        poison = self.work / "poison-bin"
        poison.mkdir()
        python3 = poison / "python3"
        python3.write_text("#!/usr/bin/env bash\necho 'unexpected python3 invocation' >&2\nexit 127\n",
                           encoding="utf-8")
        python3.chmod(0o755)
        result, log = self.run_wrapper(bash_producer=True, extra_path=poison)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertNotIn(b"unexpected python3 invocation", result.stderr)
        self.assertNotIn(b"unexpected python3 invocation", result.stdout)
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_quiet_mode_logs_without_console_copy(self):
        log = self.work / "quiet.log"
        with open(self.payload_file, "rb") as payload:
            result = subprocess.run([str(self.tool), "-q", str(log)], stdin=payload,
                                    capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, b"")
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_console_copy_death_keeps_the_log_complete(self):
        """A console vanishing mid-stream must not kill the copier nor the log."""
        log = self.work / "closed-console.log"
        with open(self.payload_file, "rb") as payload:
            copier = subprocess.Popen([str(self.tool), str(log)], stdin=payload, stdout=subprocess.PIPE)
            copier.stdout.read(100)
            copier.stdout.close()
            return_code = copier.wait(timeout=30)
        self.assertEqual(return_code, 0)
        self.assertEqual(log.read_bytes(), PAYLOAD)

    def test_closed_standard_outputs_do_not_corrupt_the_log(self):
        for redirection in ("1>&-", "2>&-", "1>&- 2>&-"):
            with self.subTest(redirection=redirection):
                log = self.work / "closed-fd.log"
                with open(self.payload_file, "rb") as payload:
                    result = subprocess.run(
                        ["bash", "-c", 'exec "$1" "$2" ' + redirection,
                         "bash", str(self.tool), str(log)],
                        stdin=payload, capture_output=True, timeout=30,
                    )
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(log.read_bytes(), PAYLOAD)


if __name__ == "__main__":
    unittest.main()
