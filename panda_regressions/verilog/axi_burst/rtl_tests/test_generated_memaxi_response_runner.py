#!/usr/bin/env python3
"""Contract checks for generated TestbenchMEMAXI response simulations."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
RUNNER = Path(__file__).resolve().with_name("run_generated_memaxi_response_tests.sh")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", ROOT / "documentation/tmp")) / (
    "generated_memaxi_response_runner_contract"
)


def validate_transcript(transcript: str, raw_status: str, status: str, mode: str, engine: str,
                        reset_type: str = "", reset_level: str = "", channel: str = "") -> None:
    normalized = "\n".join(line[2:] if line.startswith("# ") else line for line in transcript.splitlines())
    if raw_status.strip() != "0" or status.strip() != "0":
        raise ValueError(f"nonzero ModelSim status: raw={raw_status.strip()} effective={status.strip()}")
    lines = normalized.splitlines()
    proc_warning = re.compile(
        r'^\*\* Warning: \(vsim-7\) Failed to open process file "/proc/[0-9]+/stat" in read mode\.$')
    proc_warning_detail = "Operation not permitted. (errno = EPERM)"
    for index, line in enumerate(lines):
        if re.search(r"\*\* Warning:", line, re.IGNORECASE) and not (
                proc_warning.fullmatch(line) and index + 1 < len(lines) and
                lines[index + 1] == proc_warning_detail):
            raise ValueError(f"unexpected ModelSim warning: {line}")
    if re.search(r"(\*\* (?:Error|Fatal):|segmentation fault|stack smashing|addresssanitizer|"
                 r"core dumped|aborted|watchdog|timed out)", normalized, re.IGNORECASE):
        raise ValueError("ModelSim error, crash, watchdog, or timeout")

    markers = [line for line in normalized.splitlines()
               if line.startswith("GENERATED_MEMAXI_RESPONSE_")]
    records = [line for line in normalized.splitlines() if line.startswith("AXI_BURST_")]
    if mode == "control":
        expected = (f"GENERATED_MEMAXI_RESPONSE_CONTROL mode=read done=1 result=00000000" if engine == "read"
                    else f"GENERATED_MEMAXI_RESPONSE_CONTROL mode=write done=1 commits=1")
        if markers != [expected] or records:
            raise ValueError(f"wrong control marker/diagnostics: markers={markers}, diagnostics={records}")
        return

    if mode == "b1o3":
        expected = ("GENERATED_MEMAXI_RESPONSE_B1O3 mode=read n=17 outstanding_before_response=3 "
                    "ar=17 r=17 rlast=17 sum=153 backend=17 done=1" if engine == "read" else
                    "GENERATED_MEMAXI_RESPONSE_B1O3 mode=write n=17 outstanding_before_response=3 "
                    "aw=17 w=17 b=17 wlast=17 backend=17 done=1")
        if markers != [expected] or records:
            raise ValueError(f"wrong B1/O3 marker/diagnostics: markers={markers}, diagnostics={records}")
        return

    if mode == "reset_pending":
        if reset_type not in ("sync", "async") or reset_level not in ("low", "high"):
            raise ValueError(f"invalid reset profile: {reset_type}/{reset_level}")
        pending = (f"GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=read reset_type={reset_type} "
                   f"reset_level={reset_level} ar=3 r=0 model_pending=1 done=0" if engine == "read" else
                   f"GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=write reset_type={reset_type} "
                   f"reset_level={reset_level} aw=3 w=48 wlast=3 backend=48 b=0 model_pending=1 done=0")
        recovery = (f"GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=read reset_type={reset_type} "
                    f"reset_level={reset_level} reinvocation=1 ar=2 r=17 rlast=2 sum=153 backend=17 done=1"
                    if engine == "read" else
                    f"GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=write reset_type={reset_type} "
                    f"reset_level={reset_level} reinvocation=1 aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1")
        if markers != [pending, recovery] or records:
            raise ValueError(f"wrong pending-reset markers/diagnostics: markers={markers}, diagnostics={records}")
        return

    if mode in ("reset_ar", "reset_aw", "reset_w"):
        expected_channel = mode.removeprefix("reset_").upper()
        channel = channel.upper()
        if (reset_type not in ("sync", "async") or reset_level not in ("low", "high") or
                channel != expected_channel):
            raise ValueError(f"invalid stalled-reset profile: {engine}/{reset_type}/{reset_level}/{channel}")
        if (engine, channel) not in (("read", "AR"), ("write", "AW"), ("write", "W")):
            raise ValueError(f"invalid stalled-reset engine/channel: {engine}/{channel}")
        profile = f"engine={engine} channel={channel} reset_type={reset_type} reset_level={reset_level}"
        pending_counts = {
            "AR": "ar=0 r=0 backend=0 done=0",
            "AW": "aw=0 w=0 b=0 backend=0 done=0",
            "W": "aw=1 w=0 b=0 backend=0 done=0",
        }[channel]
        recovery_counts = ("ar=2 r=17 rlast=2 sum=153 backend=17 done=1" if channel == "AR" else
                           "aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1")
        expected_markers = [
            f"GENERATED_MEMAXI_RESPONSE_RESET_STALLED {profile} {pending_counts}",
            f"GENERATED_MEMAXI_RESPONSE_RESET_STALLED_RECOVERY {profile} reinvocation=1 {recovery_counts}",
        ]
        if markers != expected_markers or records:
            raise ValueError(f"wrong stalled-reset markers/diagnostics: markers={markers}, diagnostics={records}")
        return

    if mode == "fault":
        expected_markers = [
            f"GENERATED_MEMAXI_RESPONSE_CONTAINED mode={engine} response=SLVERR handshakes=1 done=0",
            ("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000"
             if engine == "read" else
             "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1"),
        ]
        count = 1
        descriptors = 1
        remaining = 1 if engine == "read" else 0
        last = r" last=1" if engine == "read" else ""
    elif mode == "drain":
        expected_markers = [
            ("GENERATED_MEMAXI_RESPONSE_DRAIN mode=read descriptors=5 beats=80 fault_beats=16 okay_beats=64 rlast=5 done=0"
             if engine == "read" else
             "GENERATED_MEMAXI_RESPONSE_DRAIN mode=write descriptors=3 beats=48 fault_responses=1 okay_responses=2 commits=48 done=0"),
            ("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000"
             if engine == "read" else
             "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1"),
        ]
        count = 112 if engine == "read" else 80
        descriptors = 5 if engine == "read" else 3
        remaining = 16 if engine == "read" else 0
        last = r" last=0" if engine == "read" else ""
    else:
        raise ValueError(f"unknown simulation mode: {mode}")

    if markers != expected_markers:
        raise ValueError(f"missing, duplicate, or unexpected result marker: {markers}")
    if len(records) != 1:
        raise ValueError(f"expected one generated-wrapper diagnostic, saw {len(records)}")
    address = "00001000" if engine == "read" else "00002000"
    payload = r" payload=00000000" if engine == "write" else ""
    reason = "RRESP" if engine == "read" else "BRESP"
    pattern = (rf"AXI_BURST_FAULT engine={engine} reason={reason} "
               rf"instance=tb_generated_memaxi_response\.dut\..*\.burst_engine\.report_axi_{engine}_diagnostic "
               rf"opcode=-1 size=0 address={address} count={count}{payload} resp=10 id=0"
               + last + rf" descriptors={descriptors} remaining={remaining}")
    if not re.fullmatch(pattern, records[0]):
        raise ValueError(f"unexpected generated-wrapper response diagnostic: {records[0]}")


def check_log(path: Path, mode: str, engine: str, reset_type: str = "", reset_level: str = "",
              channel: str = "") -> None:
    raw = path.with_suffix(path.suffix + ".raw-status")
    effective = path.with_suffix(path.suffix + ".status")
    if not path.is_file() or not raw.is_file() or not effective.is_file():
        raise ValueError(f"missing transcript or raw/effective status: {path}")
    validate_transcript(path.read_text(errors="replace"), raw.read_text(), effective.read_text(),
                        mode, engine, reset_type, reset_level, channel)


class GeneratedMemaxiResponseRunnerContract(unittest.TestCase):
    def diagnostic(self, engine: str, multi: bool = False) -> str:
        if engine == "read":
            count, desc, rem, last = (112, 5, 16, 0) if multi else (1, 1, 1, 1)
            tail = f"last={last} descriptors={desc} remaining={rem}"
            address, reason = "00001000", "RRESP"
            payload = ""
        else:
            count, desc, rem = (80, 3, 0) if multi else (1, 1, 0)
            tail = f"descriptors={desc} remaining={rem}"
            address, reason = "00002000", "BRESP"
            payload = " payload=00000000"
        return (f"AXI_BURST_FAULT engine={engine} reason={reason} "
                f"instance=tb_generated_memaxi_response.dut.region.burst_engine.report_axi_{engine}_diagnostic "
                f"opcode=-1 size=0 address={address} count={count}{payload} resp=10 id=0 {tail}")

    def transcript(self, engine: str, mode: str) -> str:
        if mode == "control":
            lines = [("GENERATED_MEMAXI_RESPONSE_CONTROL mode=read done=1 result=00000000" if engine == "read"
                      else "GENERATED_MEMAXI_RESPONSE_CONTROL mode=write done=1 commits=1")]
        elif mode == "fault":
            lines = [f"GENERATED_MEMAXI_RESPONSE_CONTAINED mode={engine} response=SLVERR handshakes=1 done=0",
                     ("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000"
                      if engine == "read" else
                      "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1"),
                     self.diagnostic(engine)]
        else:
            lines = [
                ("GENERATED_MEMAXI_RESPONSE_DRAIN mode=read descriptors=5 beats=80 fault_beats=16 okay_beats=64 rlast=5 done=0"
                 if engine == "read" else
                 "GENERATED_MEMAXI_RESPONSE_DRAIN mode=write descriptors=3 beats=48 fault_responses=1 okay_responses=2 commits=48 done=0"),
                ("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000"
                 if engine == "read" else
                 "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1"),
                self.diagnostic(engine, True),
            ]
        return "\n".join(lines)

    def check(self, engine: str, mode: str, transcript: str | None = None,
              raw: str = "0", effective: str = "0", reset_type: str = "", reset_level: str = "",
              channel: str = "") -> None:
        validate_transcript(transcript if transcript is not None else self.transcript(engine, mode),
                             raw, effective, mode, engine, reset_type, reset_level, channel)

    def test_accepts_all_six_scenario_contracts(self) -> None:
        for engine in ("read", "write"):
            for mode in ("control", "fault", "drain"):
                with self.subTest(engine=engine, mode=mode):
                    self.check(engine, mode)

    def test_accepts_b1_o3_reader_and_writer_contracts(self) -> None:
        for engine in ("read", "write"):
            marker = ("GENERATED_MEMAXI_RESPONSE_B1O3 mode=read n=17 outstanding_before_response=3 "
                      "ar=17 r=17 rlast=17 sum=153 backend=17 done=1" if engine == "read" else
                      "GENERATED_MEMAXI_RESPONSE_B1O3 mode=write n=17 outstanding_before_response=3 "
                      "aw=17 w=17 b=17 wlast=17 backend=17 done=1")
            with self.subTest(engine=engine):
                self.check(engine, "b1o3", marker)

    def test_rejects_b1_o3_missing_duplicate_wrong_counts_and_diagnostics(self) -> None:
        good = ("GENERATED_MEMAXI_RESPONSE_B1O3 mode=read n=17 outstanding_before_response=3 "
                "ar=17 r=17 rlast=17 sum=153 backend=17 done=1")
        for changed in ("", good + "\n" + good,
                        good.replace("outstanding_before_response=3", "outstanding_before_response=2"),
                        good.replace("sum=153", "sum=152"),
                        good + "\nAXI_BURST_FAULT unexpected"):
            with self.subTest(transcript=changed):
                with self.assertRaises(ValueError):
                    self.check("read", "b1o3", changed)

    def test_rejects_missing_or_duplicate_markers(self) -> None:
        for changed in ("", self.transcript("read", "fault") + "\n" +
                        "GENERATED_MEMAXI_RESPONSE_CONTAINED mode=read response=SLVERR handshakes=1 done=0"):
            with self.subTest(transcript=changed):
                with self.assertRaises(ValueError):
                    self.check("read", "fault", changed)

    def test_rejects_wrong_counts_or_duplicate_diagnostics(self) -> None:
        good = self.transcript("read", "drain")
        with self.assertRaises(ValueError):
            self.check("read", "drain", good.replace("beats=80", "beats=79"))
        with self.assertRaises(ValueError):
            self.check("read", "drain", good + "\n" + self.diagnostic("read", True))

    def test_rejects_wrong_diagnostic_reason_or_instance(self) -> None:
        good = self.transcript("read", "drain")
        for changed in (good.replace("reason=RRESP", "reason=BRESP"),
                        good.replace("tb_generated_memaxi_response.dut.",
                                     "tb_generated_memaxi_response.other.")):
            with self.subTest(transcript=changed):
                with self.assertRaises(ValueError):
                    self.check("read", "drain", changed)

    def test_rejects_missing_recovery(self) -> None:
        good = self.transcript("write", "drain")
        recovery = "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1"
        with self.assertRaises(ValueError):
            self.check("write", "drain", good.replace(recovery + "\n", ""))

    def test_accepts_all_eight_pending_reset_profiles(self) -> None:
        for engine in ("read", "write"):
            for reset_type in ("sync", "async"):
                for reset_level in ("low", "high"):
                    with self.subTest(engine=engine, reset_type=reset_type, reset_level=reset_level):
                        self.check(engine, "reset_pending", self.reset_transcript(engine, reset_type, reset_level),
                                   reset_type=reset_type, reset_level=reset_level)

    @staticmethod
    def reset_transcript(engine: str, reset_type: str, reset_level: str) -> str:
        if engine == "read":
            return (f"GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=read reset_type={reset_type} "
                    f"reset_level={reset_level} ar=3 r=0 model_pending=1 done=0\n"
                    f"GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=read reset_type={reset_type} "
                    f"reset_level={reset_level} reinvocation=1 ar=2 r=17 rlast=2 sum=153 backend=17 done=1")
        return (f"GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=write reset_type={reset_type} "
                f"reset_level={reset_level} aw=3 w=48 wlast=3 backend=48 b=0 model_pending=1 done=0\n"
                f"GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=write reset_type={reset_type} "
                f"reset_level={reset_level} reinvocation=1 aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1")

    def test_rejects_pending_reset_contract_errors(self) -> None:
        good = self.reset_transcript("read", "sync", "low")
        variants = (good.replace(" ar=3", " ar=2"),
                    good.replace(" reset_level=low", " reset_level=high"),
                    good.replace("GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY", "REMOVED"),
                    good + "\nAXI_BURST_FAULT unexpected",
                    good + "\n" + good.splitlines()[0])
        for changed in variants:
            with self.subTest(transcript=changed):
                with self.assertRaises(ValueError):
                    self.check("read", "reset_pending", changed, reset_type="sync", reset_level="low")

    @staticmethod
    def stalled_reset_transcript(engine: str, channel: str, reset_type: str, reset_level: str) -> str:
        profile = f"engine={engine} channel={channel} reset_type={reset_type} reset_level={reset_level}"
        pending_counts = {"AR": "ar=0 r=0 backend=0 done=0",
                          "AW": "aw=0 w=0 b=0 backend=0 done=0",
                          "W": "aw=1 w=0 b=0 backend=0 done=0"}[channel]
        recovery_counts = ("ar=2 r=17 rlast=2 sum=153 backend=17 done=1" if channel == "AR" else
                           "aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1")
        return (f"GENERATED_MEMAXI_RESPONSE_RESET_STALLED {profile} {pending_counts}\n"
                f"GENERATED_MEMAXI_RESPONSE_RESET_STALLED_RECOVERY {profile} reinvocation=1 {recovery_counts}")

    def test_accepts_all_stalled_request_reset_profiles(self) -> None:
        for engine, channel in (("read", "AR"), ("write", "AW"), ("write", "W")):
            for reset_type in ("sync", "async"):
                for reset_level in ("low", "high"):
                    with self.subTest(engine=engine, channel=channel, reset_type=reset_type, reset_level=reset_level):
                        self.check(engine, f"reset_{channel.lower()}",
                                   self.stalled_reset_transcript(engine, channel, reset_type, reset_level),
                                   reset_type=reset_type, reset_level=reset_level, channel=channel)

    def test_rejects_stalled_reset_wrong_profile_counts_channel_and_diagnostics(self) -> None:
        good = self.stalled_reset_transcript("read", "AR", "async", "low")
        variants = (good.replace(" ar=0", " ar=1"),
                    good.replace(" reset_level=low", " reset_level=high"),
                    good.replace(" channel=AR", " channel=AW"),
                    good.replace("GENERATED_MEMAXI_RESPONSE_RESET_STALLED_RECOVERY", "REMOVED"),
                    good + "\n" + good.splitlines()[0],
                    good + "\nAXI_BURST_FAULT unexpected")
        for changed in variants:
            with self.subTest(transcript=changed):
                with self.assertRaises(ValueError):
                    self.check("read", "reset_ar", changed, reset_type="async", reset_level="low", channel="AR")
        with self.assertRaises(ValueError):
            self.check("read", "reset_aw", good, reset_type="async", reset_level="low", channel="AW")

    def test_rejects_nonzero_status_crash_and_timeout_even_with_pass_markers(self) -> None:
        transcript = self.transcript("write", "control")
        for raw, effective, extra in (("134", "134", ""), ("0", "1", ""),
                                      ("0", "0", "Aborted (core dumped)"),
                                      ("124", "124", "watchdog timed out")):
            with self.subTest(raw=raw, effective=effective, extra=extra):
                with self.assertRaises(ValueError):
                    self.check("write", "control", transcript + "\n" + extra, raw, effective)

    def test_rejects_error_warning_and_fatal_with_zero_status_and_pass_markers(self) -> None:
        transcript = self.transcript("write", "control")
        for diagnostic in ("** Error: synthetic error", "** Warning: synthetic warning",
                           "** Fatal: synthetic fatal"):
            with self.subTest(diagnostic=diagnostic):
                with self.assertRaises(ValueError):
                    self.check("write", "control", transcript + "\n" + diagnostic)

    def test_accepts_only_the_exact_proc_permission_warning_pair(self) -> None:
        transcript = self.transcript("read", "control")
        warning = '** Warning: (vsim-7) Failed to open process file "/proc/123/stat" in read mode.'
        detail = "Operation not permitted. (errno = EPERM)"
        self.check("read", "control", transcript + "\n" + warning + "\n" + detail)
        self.check("read", "control", transcript + "\n# " + warning + "\n# " + detail)
        for invalid in (warning.replace("/proc/123/stat", "/proc/self/stat") + "\n" + detail,
                        warning.replace("** Warning:", "** warning:") + "\n" + detail,
                        warning + "\nOperation not permitted. (errno = EACCES)",
                        warning,
                        warning + "\n" + detail + "\n** Warning: synthetic warning",
                        "  ** Warning: synthetic warning",
                        "prefix ** Warning: synthetic warning",
                        warning + "\n" + detail + "\n  ** Warning: synthetic warning",
                        warning + "\n" + detail + "\nprefix ** Warning: synthetic warning"):
            with self.subTest(transcript=invalid):
                with self.assertRaises(ValueError):
                    self.check("read", "control", transcript + "\n" + invalid)
        with self.assertRaises(ValueError):
            self.check("read", "control", transcript + "\n" + warning + "\n" + detail,
                       raw="134", effective="134")
        for diagnostic in ("** Error: synthetic error", "** Fatal: synthetic fatal"):
            with self.subTest(diagnostic=diagnostic):
                with self.assertRaises(ValueError):
                    self.check("read", "control", transcript + "\n" + warning + "\n" + detail +
                               "\n" + diagnostic)


class GeneratedMemaxiResponseParallelRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = TMP_ROOT / uuid.uuid4().hex
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)
        self.prefix = self.scratch / "install"
        (self.prefix / "bin").mkdir(parents=True)
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.touch()
        (self.prefix / "settings.sh").write_text(
            'export BAMBU_HLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"\n',
            encoding="utf-8",
        )
        self.toolbin = self.scratch / "toolbin"
        self.toolbin.mkdir()
        self.trace = self.scratch / "trace"
        self.trace.mkdir()
        self.fake_script = self.scratch / "fake_tool.py"
        self.fake_script.write_text(self.fake_tools(), encoding="utf-8")
        for tool in ("bambu", "vsim", "vlog", "vlib", "vmap"):
            target = self.prefix / "bin" / tool if tool == "bambu" else self.toolbin / tool
            target.write_text(f"#!/usr/bin/env python3\nexec(compile(open({str(self.fake_script)!r}).read(), 'fake_tool.py', 'exec'))\n",
                              encoding="utf-8")
            target.chmod(0o755)

    @staticmethod
    def fake_tools() -> str:
        return r'''import fcntl
import os
import sys
import time
from pathlib import Path

tool = Path(sys.argv[0]).name
if tool == "bambu" and sys.argv[1:] == ["--version"]:
    print("fake Bambu 1.0")
    raise SystemExit(0)
if tool == "vsim" and "-version" in sys.argv:
    print("QuestaSim Fake 2026.4")
    raise SystemExit(0)

def activity(begin):
    root = Path(os.environ["FAKE_TRACE"])
    with (root / "lock").open("a+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        active_file, maximum_file = root / "active", root / "maximum"
        active = int(active_file.read_text() or "0") if active_file.exists() else 0
        active += 1 if begin else -1
        active_file.write_text(str(active))
        maximum = int(maximum_file.read_text() or "0") if maximum_file.exists() else 0
        maximum_file.write_text(str(max(maximum, active)))

if tool == "bambu":
    if os.environ.get("FAKE_BAMBU_FAIL_CASE"):
        print("synthetic Bambu failure", file=sys.stderr)
        raise SystemExit(9)
    args = sys.argv[1:]
    top = next(arg.split("=", 1)[1] for arg in args if arg.startswith("--top-fname="))
    source = args[0]
    case = Path.cwd().parent.name
    b1 = any("B1_O3" in arg for arg in args) or top == "write_count_seed_single_beat"
    report_b1 = b1 and os.environ.get("FAKE_BAMBU_PROFILE_MISMATCH_CASE") != case
    activity(True)
    time.sleep(0.12)
    Path(f"{top}.v").write_text("MinimalAXI4MasterPipelined MinimalAXI4MasterPipelinedWrite\n")
    Path("panda_libtech.v").write_text("module fake; endmodule\n")
    if source.endswith("bambu_axi_burst_sum_profile.cpp"):
        print("Configured read burst regions for bundle src (B=%d)" % (1 if report_b1 else 16), file=sys.stderr)
        if report_b1:
            print("AXI_BURST_PROFILE root=kernel bundle=src enabled=1 burst_type=INCREMENTAL(device) read_max=1(explicit) read_outstanding=3(explicit) read_fifo=8(explicit)", file=sys.stderr)
    else:
        print("Configured write burst region for bundle output (B=%d)" % (1 if report_b1 else 16), file=sys.stderr)
        print("AXI_BURST_PROFILE root=%s bundle=output enabled=1 burst_type=INCREMENTAL(device) write_max=%d(explicit) write_outstanding=3(explicit) write_fifo=8(explicit)" % (top, 1 if report_b1 else 16), file=sys.stderr)
    activity(False)
    if os.environ.get("FAKE_BAMBU_FAIL_AFTER_ARTIFACTS_CASE") == case:
        print("synthetic Bambu failure after artifact generation", file=sys.stderr)
        raise SystemExit(9)
    raise SystemExit(0)

if tool == "vmap":
    if sys.argv[1:] == ["-c"]:
        Path("modelsim.ini").write_text("[Library]\n")
elif tool == "vlib":
    Path(sys.argv[-1]).mkdir(exist_ok=True)
elif tool == "vlog":
    pass
elif tool == "vsim":
    case = Path.cwd().parent.name
    args = sys.argv[1:]
    read = case.startswith("read")
    engine = "read" if read else "write"
    multi = "+MULTI" in args
    b1 = "+B1O3" in args
    reset_pending = "+RESET_PENDING" in args
    fault = "+FAULT" in args
    stall_channel = next((arg.split("=", 1)[1] for arg in args if arg.startswith("+RESET_STALL_CHANNEL=")), "")
    reset_type, reset_level = case.removeprefix("read_reset_").removeprefix("write_reset_").split("_") if reset_pending else ("", "")
    with (Path.cwd() / "runtime-plusargs.log").open("a", encoding="utf-8") as plusarg_log:
        plusarg_log.write(" ".join(arg for arg in args if arg.startswith("+")) + "\n")
    activity(True)
    time.sleep(0.12)
    if reset_pending and stall_channel:
        profile = "engine=%s channel=%s reset_type=%s reset_level=%s" % (engine, stall_channel, reset_type, reset_level)
        pending_counts = {"AR": "ar=0 r=0 backend=0 done=0",
                          "AW": "aw=0 w=0 b=0 backend=0 done=0",
                          "W": "aw=1 w=0 b=0 backend=0 done=0"}[stall_channel]
        recovery_counts = ("ar=2 r=17 rlast=2 sum=153 backend=17 done=1" if stall_channel == "AR" else
                           "aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1")
        print("GENERATED_MEMAXI_RESPONSE_RESET_STALLED %s %s" % (profile, pending_counts))
        print("GENERATED_MEMAXI_RESPONSE_RESET_STALLED_RECOVERY %s reinvocation=1 %s" %
              (profile, recovery_counts))
    elif reset_pending:
        print("GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=read reset_type=%s reset_level=%s ar=3 r=0 model_pending=1 done=0" %
              (reset_type, reset_level) if read else
              "GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=write reset_type=%s reset_level=%s aw=3 w=48 wlast=3 backend=48 b=0 model_pending=1 done=0" %
              (reset_type, reset_level))
        print("GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=read reset_type=%s reset_level=%s reinvocation=1 ar=2 r=17 rlast=2 sum=153 backend=17 done=1" %
              (reset_type, reset_level) if read else
              "GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=write reset_type=%s reset_level=%s reinvocation=1 aw=2 w=17 wlast=2 b=2 backend=17 strobes=ffff done=1" %
              (reset_type, reset_level))
    elif b1:
        marker = ("GENERATED_MEMAXI_RESPONSE_B1O3 mode=read n=17 outstanding_before_response=3 ar=17 r=17 rlast=17 sum=153 backend=17 done=1" if read else
                  "GENERATED_MEMAXI_RESPONSE_B1O3 mode=write n=17 outstanding_before_response=3 aw=17 w=17 b=17 wlast=17 backend=17 done=1")
        print(marker)
    elif multi:
        marker = ("GENERATED_MEMAXI_RESPONSE_DRAIN mode=read descriptors=5 beats=80 fault_beats=16 okay_beats=64 rlast=5 done=0" if read else
                  "GENERATED_MEMAXI_RESPONSE_DRAIN mode=write descriptors=3 beats=48 fault_responses=1 okay_responses=2 commits=48 done=0")
        print(marker)
        print("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000" if read else
              "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1")
        print("AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_memaxi_response.dut.region.burst_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00001000 count=112 resp=10 id=0 last=0 descriptors=5 remaining=16" if read else
              "AXI_BURST_FAULT engine=write reason=BRESP instance=tb_generated_memaxi_response.dut.region.burst_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00002000 count=80 payload=00000000 resp=10 id=0 descriptors=3 remaining=0")
    elif fault:
        print("GENERATED_MEMAXI_RESPONSE_CONTAINED mode=%s response=SLVERR handshakes=1 done=0" % engine)
        print("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=1 result=00000000" if read else
              "GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=1 commits=1")
        print("AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_memaxi_response.dut.region.burst_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00001000 count=1 resp=10 id=0 last=1 descriptors=1 remaining=1" if read else
              "AXI_BURST_FAULT engine=write reason=BRESP instance=tb_generated_memaxi_response.dut.region.burst_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00002000 count=1 payload=00000000 resp=10 id=0 descriptors=1 remaining=0")
    else:
        print("GENERATED_MEMAXI_RESPONSE_CONTROL mode=read done=1 result=00000000" if read else
              "GENERATED_MEMAXI_RESPONSE_CONTROL mode=write done=1 commits=1")
    activity(False)
'''

    def run_runner(self, output: Path, **extra_env: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.update(
            PATH=f"{self.toolbin}:{env['PATH']}",
            TMPDIR=str(self.scratch), TMP=str(self.scratch), TEMP=str(self.scratch),
            BURST_TEST_TMPDIR=str(self.scratch), BURST_MODELSIM_OUTSIDE_SANDBOX="1",
            SIM_JOBS="3", FAKE_TRACE=str(self.trace),
        )
        env.update(extra_env)
        return subprocess.run(
            ["bash", str(RUNNER), str(self.prefix / "bin/bambu"), str(output)],
            cwd=ROOT, env=env, capture_output=True, text=True, check=False, timeout=60,
        )

    def test_runs_isolated_cases_in_parallel_and_merges_in_fixed_order(self) -> None:
        output = self.scratch / "qualified"
        result = self.run_runner(output)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("parallel limit 3", result.stdout)
        self.assertIn("GENERATED_MEMAXI_RESPONSE_TESTS PASS", result.stdout)
        maximum = int((self.trace / "maximum").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 3)
        expected_cases = ["read", "read_multi", "read_b1_o3", "write", "write_multi", "write_b1_o3",
                          "read_reset_sync_low", "read_reset_sync_high", "read_reset_async_low", "read_reset_async_high",
                          "write_reset_sync_low", "write_reset_sync_high", "write_reset_async_low", "write_reset_async_high"]
        aggregate = (output / "aggregate.log").read_text()
        starts = [aggregate.index("GENERATED_MEMAXI_RESPONSE_" + token) for token in
                  ("CONTROL mode=read", "CONTAINED mode=read", "DRAIN mode=read", "B1O3 mode=read",
                   "CONTROL mode=write", "CONTAINED mode=write", "DRAIN mode=write", "B1O3 mode=write")]
        self.assertEqual(starts, sorted(starts))
        for name in expected_cases:
            self.assertTrue((output / name / "modelsim/modelsim.ini").is_file())
            self.assertTrue((output / name / "provenance.txt").is_file())
        for reset_type in ("sync", "async"):
            for reset_level in ("low", "high"):
                for engine in ("read", "write"):
                    name = f"{engine}_reset_{reset_type}_{reset_level}"
                    command = (output / name / "generation.command").read_text()
                    self.assertIn(f"--reset-type={reset_type}", command)
                    self.assertIn(f"--reset-level={reset_level}", command)
                    self.assertIn(f"RESET_PENDING engine={engine} reset_type={reset_type} reset_level={reset_level}", aggregate)
                    runtime = (output / name / "modelsim/runtime-plusargs.log").read_text().splitlines()
                    reset_flags = {"+RESET_PENDING"}
                    if reset_type == "async":
                        reset_flags.add("+RESET_ASYNC")
                    if reset_level == "high":
                        reset_flags.add("+RESET_ACTIVE_HIGH")
                    channels = ("AR",) if engine == "read" else ("AW", "W")
                    expected_runtime = [reset_flags, *(
                        reset_flags | {f"+RESET_STALL_CHANNEL={channel}"} for channel in channels
                    )]
                    self.assertEqual([set(line.split()) for line in runtime], expected_runtime)
                    for channel in channels:
                        self.assertIn(f"RESET_STALLED engine={engine} channel={channel} reset_type={reset_type} "
                                      f"reset_level={reset_level}", aggregate)

    def test_failure_does_not_emit_aggregate_pass(self) -> None:
        output = self.scratch / "failed"
        result = self.run_runner(output, FAKE_BAMBU_FAIL_CASE="read")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((output / "aggregate.log").exists())
        self.assertNotIn("GENERATED_MEMAXI_RESPONSE_TESTS PASS", result.stdout + result.stderr)

    def test_generation_failure_after_writing_artifacts_does_not_simulate(self) -> None:
        output = self.scratch / "failed-after-artifacts"
        result = self.run_runner(output, SIM_JOBS="1", FAKE_BAMBU_FAIL_AFTER_ARTIFACTS_CASE="read")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((output / "read/generated/kernel.v").is_file())
        self.assertTrue((output / "read/generated/panda_libtech.v").is_file())
        self.assertFalse((output / "read/modelsim").exists())
        self.assertFalse((output / "aggregate.log").exists())
        self.assertNotIn("GENERATED_MEMAXI_RESPONSE_TESTS PASS", result.stdout + result.stderr)

    def test_profile_mismatch_does_not_simulate_or_emit_aggregate_pass(self) -> None:
        output = self.scratch / "profile-mismatch"
        result = self.run_runner(output, SIM_JOBS="1", FAKE_BAMBU_PROFILE_MISMATCH_CASE="read_b1_o3")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((output / "read_b1_o3/generated/kernel.v").is_file())
        self.assertFalse((output / "read_b1_o3/modelsim").exists())
        self.assertFalse((output / "aggregate.log").exists())
        self.assertNotIn("GENERATED_MEMAXI_RESPONSE_TESTS PASS", result.stdout + result.stderr)

    def test_rejects_invalid_sim_jobs_before_tool_preflight(self) -> None:
        env = os.environ.copy()
        env.update(TMPDIR=str(self.scratch), TMP=str(self.scratch), TEMP=str(self.scratch),
                   BURST_MODELSIM_OUTSIDE_SANDBOX="1", SIM_JOBS="0")
        result = subprocess.run(
            ["bash", str(RUNNER), "missing-bambu", str(self.scratch / "invalid")],
            cwd=ROOT, env=env, capture_output=True, text=True, check=False, timeout=10,
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("SIM_JOBS must be a positive integer", result.stderr)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--check-log":
        if len(sys.argv) not in (5, 7, 8):
            raise SystemExit("usage: test_generated_memaxi_response_runner.py --check-log LOG MODE ENGINE "
                             "[RESET_TYPE RESET_LEVEL [CHANNEL]]")
        try:
            check_log(Path(sys.argv[2]), sys.argv[3], sys.argv[4],
                      sys.argv[5] if len(sys.argv) >= 7 else "",
                      sys.argv[6] if len(sys.argv) >= 7 else "",
                      sys.argv[7] if len(sys.argv) == 8 else "")
        except (OSError, ValueError) as exc:
            raise SystemExit(f"generated MEMAXI transcript check failed: {exc}") from exc
        raise SystemExit(0)
    unittest.main(argv=[sys.argv[0]])
