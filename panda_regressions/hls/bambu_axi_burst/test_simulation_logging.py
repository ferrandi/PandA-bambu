#!/usr/bin/env python3
"""Offline integration checks for the shared simulation logging wrapper."""

import os
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import unittest


ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "etc/libtech/backend/utils/simulation_wrapper.sh"
PAYLOAD = bytes(range(256)) * 4096 + b"\nlast fragment without newline"


class SimulationLoggingTest(unittest.TestCase):
    def setUp(self):
        scratch = Path(os.environ.get("BURST_TEST_TMPDIR", ROOT / "documentation/tmp"))
        scratch.mkdir(parents=True, exist_ok=True)
        self.temporary = TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temporary.cleanup)
        self.work = Path(self.temporary.name)
        self.simulation = self.work / "HLS_output/simulation"
        self.simulation.mkdir(parents=True)
        (self.work / "bambu_time_simulation.txt").write_text("0|2\n", encoding="utf-8")
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

    def run_wrapper(self, producer_status=0, nested=False, invalid_log=False):
        executable = self.producer
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
        environment.update(
            BAMBU_HLS=str(ROOT),
            OUT_LVL="1",
            TARGET="testbench",
            SYS_ELF=str(executable),
            SWD=str(self.work / "backend with spaces"),
            RECORD_PRODUCER=str(self.producer),
            PRODUCER_STATUS=str(producer_status),
            TMPDIR=str(self.work),
            TMP=str(self.work),
            TEMP=str(self.work),
        )
        Path(environment["SWD"]).mkdir(exist_ok=True)
        result = subprocess.run(
            ["bash", str(self.bootstrap), str(WRAPPER), str(self.setup)],
            cwd=self.work,
            env=environment,
            capture_output=True,
            timeout=30,
        )
        return result, log

    def test_binary_output_survives_large_pipe_and_broken_system_tee(self):
        result, log = self.run_wrapper()
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(log.read_bytes(), PAYLOAD)
        self.assertIn(PAYLOAD, result.stdout)
        self.assertNotIn(b"unexpected external tee invocation", result.stderr)

    def test_producer_failure_is_preserved(self):
        result, log = self.run_wrapper(producer_status=7)
        self.assertEqual(result.returncode, 7)
        self.assertIn(b"producer failed (7)", result.stderr)
        self.assertEqual(log.read_bytes(), PAYLOAD)

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


if __name__ == "__main__":
    unittest.main()
