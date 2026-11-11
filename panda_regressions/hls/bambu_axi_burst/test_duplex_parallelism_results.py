#!/usr/bin/env python3
"""Offline adversarial tests for the duplex parallelism evidence checker."""
from pathlib import Path
import os
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(HERE))
import check_duplex_parallelism_results as checker


def _summary(profile: str, **changes: int | str) -> str:
    values: dict[str, int | str] = {
        "CASE": checker.CASE, "PROFILE": profile, "CARRIER": checker.CARRIER,
        "INVOCATIONS": 1, "EXPECTED_WORDS": 256, "READ_BEATS": 256, "WRITE_BEATS": 256,
        "READ_WRITE_OUTSTANDING_CYCLES": 100, "SIMULTANEOUS_RW_HANDSHAKES": 30,
        "FIRST_AR_CYCLE": 10, "LAST_R_CYCLE": 800, "FIRST_AW_CYCLE": 20,
        "LAST_W_CYCLE": 900, "ACTIVE_WINDOW_CYCLES": 891, "TOTAL_CYCLES": 1000,
        **checker.PROFILES[profile], **checker.PERIODS,
    }
    values.update(changes)
    return checker.SUMMARY_MARKER + " " + " ".join(f"{key}={values[key]}" for key in sorted(values))


def _events(profile: str) -> list[str]:
    read = checker.PROFILES[profile]
    return [checker.EVENT_MARKER + " " + " ".join(f"{key}={value}" for key, value in (
        ("CASE", checker.CASE), ("INDEX", index), ("COUNT", checker.WORDS),
        ("CYCLE", 100 + index * 2), ("READ_B_MAX", read["READ_B_MAX"]),
        ("READ_OUTSTANDING", read["READ_OUTSTANDING"]),
        ("READ_FIFO_DEPTH", read["READ_FIFO_DEPTH"]))) for index in range(checker.WORDS)]


class DuplexParallelismResultsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="test-duplex-parallelism-", dir=TMP_ROOT)
        self.output = Path(self.temp.name)
        self.logs: dict[str, Path] = {}
        for profile in checker.PROFILES:
            path = self.output / f"{profile}_copy" / "scenarios" / f"{checker.CASE}.log"
            path.parent.mkdir(parents=True)
            path.write_text("\n".join([_summary(profile), *_events(profile)]) + "\n", encoding="utf-8")
            self.logs[profile] = path

    def tearDown(self):
        self.temp.cleanup()

    def _replace_summary(self, profile: str, old: str, new: str):
        path = self.logs[profile]
        text = path.read_text(encoding="utf-8")
        line = next(row for row in text.splitlines() if row.startswith(checker.SUMMARY_MARKER))
        self.assertIn(old, line)
        path.write_text(text.replace(line, line.replace(old, new), 1), encoding="utf-8")

    def _replace_event(self, profile: str, index: int, replacement: str):
        path = self.logs[profile]
        lines = path.read_text(encoding="utf-8").splitlines()
        event_indexes = [i for i, row in enumerate(lines) if row.startswith(checker.EVENT_MARKER)]
        lines[event_indexes[index]] = replacement
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def test_valid_logs_report_throughput_and_interval_statistics(self):
        result = checker.verify(self.output)
        self.assertEqual(set(result), {"symmetric", "asymmetric"})
        for metrics in result.values():
            self.assertAlmostEqual(metrics["throughput_words_per_active_cycle"], 256 / 891)
            self.assertEqual(metrics["completion_interval_average_cycles"], 2)
            self.assertEqual(metrics["completion_interval_min_cycles"], 2)
            self.assertEqual(metrics["completion_interval_max_cycles"], 2)
            self.assertEqual(metrics["central_interval_count"], 223)
            self.assertEqual(metrics["central_interval_average_cycles"], 2)

    def test_missing_log_is_rejected(self):
        self.logs["asymmetric"].unlink()
        with self.assertRaisesRegex(checker.CheckError, "exactly the two expected"):
            checker.verify(self.output)

    def test_duplicate_log_is_rejected(self):
        extra = self.output / "unexpected" / f"{checker.CASE}.log"
        extra.parent.mkdir()
        extra.write_text("duplicate\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "exactly the two expected"):
            checker.verify(self.output)

    def test_missing_metric_record_is_rejected(self):
        path = self.logs["symmetric"]
        path.write_text("\n".join(_events("symmetric")) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "exactly one DUPLEX_PARALLELISM_METRIC"):
            checker.verify(self.output)

    def test_duplicate_metric_record_is_rejected(self):
        path = self.logs["symmetric"]
        text = path.read_text(encoding="utf-8")
        line = text.splitlines()[0]
        path.write_text(text.replace(line, line + "\n" + line, 1), encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "exactly one DUPLEX_PARALLELISM_METRIC"):
            checker.verify(self.output)

    def test_wrong_profile_and_count_are_rejected(self):
        self._replace_summary("symmetric", "PROFILE=symmetric", "PROFILE=asymmetric")
        with self.assertRaisesRegex(checker.CheckError, "unexpected PROFILE"):
            checker.verify(self.output)
        self._replace_summary("symmetric", "PROFILE=asymmetric", "PROFILE=symmetric")
        self._replace_summary("symmetric", "EXPECTED_WORDS=256", "EXPECTED_WORDS=255")
        with self.assertRaisesRegex(checker.CheckError, "EXPECTED_WORDS must be 256"):
            checker.verify(self.output)

    def test_zero_overlap_or_handshake_is_rejected(self):
        path = self.logs["symmetric"]
        original = path.read_text(encoding="utf-8")
        for field, message in (("READ_WRITE_OUTSTANDING_CYCLES", "overlap must be positive"),
                               ("SIMULTANEOUS_RW_HANDSHAKES", "handshakes must be positive")):
            with self.subTest(field=field):
                path.write_text(original, encoding="utf-8")
                self._replace_summary("symmetric", f"{field}=100" if field.startswith("READ_") else f"{field}=30",
                                      f"{field}=0")
                with self.assertRaisesRegex(checker.CheckError, message):
                    checker.verify(self.output)
        path.write_text(original, encoding="utf-8")

    def test_invalid_timestamps_and_zero_denominators_are_rejected(self):
        self._replace_summary("symmetric", "ACTIVE_WINDOW_CYCLES=891", "ACTIVE_WINDOW_CYCLES=0")
        with self.assertRaisesRegex(checker.CheckError, "ACTIVE_WINDOW_CYCLES must equal"):
            checker.verify(self.output)
        self._replace_summary("symmetric", "ACTIVE_WINDOW_CYCLES=0", "ACTIVE_WINDOW_CYCLES=891")
        self._replace_summary("symmetric", "LAST_R_CYCLE=800", "LAST_R_CYCLE=9")
        with self.assertRaisesRegex(checker.CheckError, "cycle order is invalid"):
            checker.verify(self.output)
        self._replace_summary("symmetric", "LAST_R_CYCLE=9", "LAST_R_CYCLE=800")
        self._replace_summary("symmetric", "TOTAL_CYCLES=1000", "TOTAL_CYCLES=900")
        with self.assertRaisesRegex(checker.CheckError, "outside TOTAL_CYCLES"):
            checker.verify(self.output)

    def test_missing_duplicate_and_out_of_order_load_events_are_rejected(self):
        path = self.logs["asymmetric"]
        original = path.read_text(encoding="utf-8")
        path.write_text("\n".join(original.splitlines()[:-1]) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "expected exactly 256"):
            checker.verify(self.output)
        path.write_text(original, encoding="utf-8")
        lines = original.splitlines()
        event_positions = [i for i, line in enumerate(lines) if line.startswith(checker.EVENT_MARKER)]
        lines[event_positions[1]] = lines[event_positions[0]]
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "indices must be ordered"):
            checker.verify(self.output)
        path.write_text(original, encoding="utf-8")
        lines = original.splitlines()
        first = next(i for i, line in enumerate(lines) if line.startswith(checker.EVENT_MARKER))
        fields = lines[first].replace("INDEX=0", "INDEX=1")
        lines[first] = fields
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "indices must be ordered"):
            checker.verify(self.output)

    def test_duplicate_and_nonincreasing_event_timestamps_are_rejected(self):
        path = self.logs["symmetric"]
        lines = path.read_text(encoding="utf-8").splitlines()
        events = [i for i, line in enumerate(lines) if line.startswith(checker.EVENT_MARKER)]
        lines[events[1]] = lines[events[1]].replace("CYCLE=102", "CYCLE=100")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "timestamps must strictly increase"):
            checker.verify(self.output)

    def test_wrong_event_count_and_read_profile_are_rejected(self):
        path = self.logs["symmetric"]
        lines = path.read_text(encoding="utf-8").splitlines()
        event = next(i for i, line in enumerate(lines) if line.startswith(checker.EVENT_MARKER))
        lines[event] = lines[event].replace("COUNT=256", "COUNT=255")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "COUNT must be 256"):
            checker.verify(self.output)
        lines[event] = lines[event].replace("COUNT=255", "COUNT=256").replace("READ_FIFO_DEPTH=64", "READ_FIFO_DEPTH=32")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "wrong read profile"):
            checker.verify(self.output)

    def test_model_manifest_and_field_mutations_are_rejected(self):
        self._replace_summary("symmetric", "AWREADY_PERIOD=5", "AWREADY_PERIOD=6")
        with self.assertRaisesRegex(checker.CheckError, "AWREADY_PERIOD must be 5"):
            checker.verify(self.output)
        self._replace_summary("symmetric", "AWREADY_PERIOD=6", "AWREADY_PERIOD=5")
        self._replace_summary("symmetric", "CARRIER=copy", "CARRIER=other")
        with self.assertRaisesRegex(checker.CheckError, "unexpected CARRIER"):
            checker.verify(self.output)
        self._replace_summary("symmetric", "CARRIER=other", "CARRIER=copy")
        path = self.logs["symmetric"]
        text = path.read_text(encoding="utf-8")
        line = next(row for row in text.splitlines() if row.startswith(checker.SUMMARY_MARKER))
        path.write_text(text.replace(line, line + " EXTRA=1", 1), encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "unknown field"):
            checker.verify(self.output)

    def test_missing_duplicate_and_malformed_fields_are_rejected(self):
        path = self.logs["symmetric"]
        text = path.read_text(encoding="utf-8")
        line = next(row for row in text.splitlines() if row.startswith(checker.SUMMARY_MARKER))
        path.write_text(text.replace(line, line.replace("INVOCATIONS=1 ", "", 1), 1), encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "missing fields"):
            checker.verify(self.output)
        path.write_text(text.replace(line, line.replace("INVOCATIONS=1", "INVOCATIONS=1 INVOCATIONS=1", 1), 1),
                        encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "duplicate field"):
            checker.verify(self.output)
        path.write_text(text.replace(line, line.replace("INVOCATIONS=1", "INVOCATIONS=-1", 1), 1),
                        encoding="utf-8")
        with self.assertRaisesRegex(checker.CheckError, "nonnegative integer"):
            checker.verify(self.output)

    def test_unexpected_duplex_marker_is_rejected(self):
        path = self.logs["symmetric"]
        with path.open("a", encoding="utf-8") as stream:
            stream.write("unexpected prefix DUPLEX_PARALLELISM_METRIC CASE=bad\n")
        with self.assertRaisesRegex(checker.CheckError, "malformed metric marker row"):
            checker.verify(self.output)

    def test_existing_g2_pass_marker_is_ignored(self):
        path = self.logs["symmetric"]
        with path.open("a", encoding="utf-8") as stream:
            stream.write("DUPLEX_KERNEL_COPY_G2 PASS PROFILE=symmetric\n")
        self.assertEqual(set(checker.verify(self.output)), {"symmetric", "asymmetric"})


if __name__ == "__main__":
    unittest.main()
