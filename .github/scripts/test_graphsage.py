#!/usr/bin/env python3
"""Focused fixtures for deterministic serial/SPARTA GraphSAGE evidence."""
import sys
import struct
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
REPOSITORY = SCRIPTS.parents[1]
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

from ci_results.graphsage import (GraphSAGEEvidenceError, compare_inventories,
                                    inventory_from_mdpi_dumps, parse_inventory)
from ci_results.regressions import REGRESSION_SPECS, _normalized_arguments, _serial_runtime_violations

def inventory(observed: str | None = None, *, immutable: int = 1,
              case_id: str = "negative-signed-division") -> str:
    golden = "-3,0,3,-8,5,-2,5,-8,9,-8,3,-1,10,1,-7,2,-2,0"
    observed_values = observed or golden
    mismatches = sum(left != right for left, right in zip(golden.split(","), observed_values.split(",")))
    return (f"GRAPHSAGE_CASE|id={case_id}|rows=0,3,4,6,8,9,12"
            "|neighbors=1,2,4,0,1,5,2,4,3,0,3,5"
            "|features=-8,5,-2,7,-4,11,-5,-8,3,10,1,-7,-11,14,-5,4,-13,8"
            f"|golden={golden}|observed={observed_values}"
            f"|count=18|mismatches={mismatches}|inputs_immutable={immutable}\n")

def complete_inventory(observed: str | None = None, *, immutable: int = 1) -> str:
    return "".join(inventory(observed, immutable=immutable, case_id=case_id) for case_id in (
        "regular", "irregular-zero-degree", "negative-signed-division", "duplicate-self-mixed"
    ))

def write_mdpi_dumps(directory: Path) -> None:
    lengths = (7, 12, 18, 18)
    for call in range(1, 5):
        for parameter, length in enumerate(lengths):
            values = list(range(call * 100 + parameter * 20, call * 100 + parameter * 20 + length))
            for kind in ("gold", "sim"):
                (directory / f"P{parameter}.{kind}.{call}.dat").write_bytes(
                    struct.pack(f"={length}i", *values)
                )

class GraphSAGEEvidenceTests(unittest.TestCase):
    def test_mdpi_dump_inventory_is_the_observed_evidence_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            write_mdpi_dumps(directory)
            observed = list(range(160, 178))
            observed[0] = -91
            (directory / "P3.sim.1.dat").write_bytes(struct.pack("=18i", *observed))
            parsed = parse_inventory(inventory_from_mdpi_dumps(directory))
        self.assertEqual(parsed["regular"]["observed"], observed)
        self.assertEqual(parsed["regular"]["golden"][0], 160)
        self.assertEqual(parsed["regular"]["mismatches"], 1)

    def test_mdpi_dump_inventory_fails_closed_on_missing_truncated_and_extra(self):
        for failure in ("missing", "truncated", "extra"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                write_mdpi_dumps(directory)
                if failure == "missing":
                    (directory / "P3.sim.4.dat").unlink()
                elif failure == "truncated":
                    (directory / "P3.sim.4.dat").write_bytes(b"short")
                else:
                    (directory / "P3.sim.5.dat").write_bytes(struct.pack("=18i", *range(18)))
                with self.assertRaises(GraphSAGEEvidenceError):
                    inventory_from_mdpi_dumps(directory)

    def test_mdpi_dump_inventory_detects_input_mutation(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            write_mdpi_dumps(directory)
            mutated = list(range(100, 107))
            mutated[2] = -1
            (directory / "P0.sim.1.dat").write_bytes(struct.pack("=7i", *mutated))
            mutated_inventory = inventory_from_mdpi_dumps(directory)
            parsed = parse_inventory(mutated_inventory)
            pristine = Path(temporary) / "pristine"
            pristine.mkdir()
            write_mdpi_dumps(pristine)
            with self.assertRaises(GraphSAGEEvidenceError) as caught:
                compare_inventories(mutated_inventory, inventory_from_mdpi_dumps(pristine))
        self.assertEqual(parsed["regular"]["inputs_immutable"], 0)
        self.assertIsNotNone(caught.exception.report)
        self.assertEqual(len(caught.exception.report["cases"]), 4)

    def test_golden_serial_sparta_equality(self):
        report = compare_inventories(complete_inventory(), complete_inventory())
        self.assertEqual(report["outcome"], "pass")
        self.assertEqual(report["element_comparison_count"], 216)
        self.assertEqual(report["mismatch_count"], 0)

    def test_golden_serial_mismatch(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "element mismatches"):
            compare_inventories(complete_inventory("99,0,3,-8,5,-2,5,-8,9,-8,3,-1,10,1,-7,2,-2,0"), complete_inventory())

    def test_mismatch_retains_structured_evidence(self):
        with self.assertRaises(GraphSAGEEvidenceError) as caught:
            compare_inventories(complete_inventory("99,0,3,-8,5,-2,5,-8,9,-8,3,-1,10,1,-7,2,-2,0"),
                                complete_inventory())
        self.assertIsNotNone(caught.exception.report)
        self.assertEqual(caught.exception.report["outcome"], "fail")
        self.assertGreater(caught.exception.report["mismatch_count"], 0)
        self.assertEqual(len(caught.exception.report["cases"]), 4)

    def test_golden_sparta_and_serial_sparta_mismatch(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "element mismatches"):
            compare_inventories(complete_inventory(), complete_inventory("99,0,3,-8,5,-2,5,-8,9,-8,3,-1,10,1,-7,2,-2,0"))

    def test_missing_output_vector(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "missing fields: observed"):
            parse_inventory(inventory().replace("|observed=", "|not_observed="))

    def test_wrong_output_length(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "expected 18 integers"):
            parse_inventory(inventory().replace("|observed=-3,", "|observed="))

    def test_malformed_integer(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "malformed integer"):
            parse_inventory(inventory().replace("|observed=-3,", "|observed=nope,"))

    def test_modified_input_detection_preserves_inventory(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "input-immutability") as caught:
            compare_inventories(complete_inventory(immutable=0), complete_inventory())
        self.assertEqual(len(caught.exception.report["cases"]), 4)
        self.assertFalse(caught.exception.report["cases"][0]["serial_input_immutable"])

    def test_missing_inventory(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "missing GraphSAGE"):
            parse_inventory("simulation finished\n")

    def test_matching_incomplete_inventories_are_rejected(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "missing cases"):
            compare_inventories(inventory(), inventory())

    def test_unexpected_field_is_rejected(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "unexpected fields"):
            parse_inventory(complete_inventory().replace("|count=18", "|extra=1|count=18", 1))

    def test_serial_input_inventory_mismatch(self):
        with self.assertRaisesRegex(GraphSAGEEvidenceError, "incompatible features"):
            compare_inventories(complete_inventory(), complete_inventory().replace("features=-8", "features=-9", 1))

    def test_serial_and_sparta_invocations_are_separate(self):
        serial = next(spec for spec in REGRESSION_SPECS if spec.task_id == "regression-graphsage-serial")
        sparta = next(spec for spec in REGRESSION_SPECS if spec.task_id == "regression-graphsage")
        serial_args = _normalized_arguments(serial, "I386_CLANG16", 2)
        sparta_args = _normalized_arguments(sparta, "I386_CLANG16", 2)
        self.assertNotIn("-fopenmp", serial_args)
        self.assertFalse(any(item.startswith("--context_switch") for item in serial_args))
        self.assertIn("-fopenmp", sparta_args)
        self.assertIn("--context_switch=2", sparta_args)
        dump_option = "-DBAMBU_SIM_DUMP_OUTPUT"
        self.assertIn(dump_option, serial_args)
        self.assertIn(dump_option, sparta_args)
        makefile = (REPOSITORY / "etc" / "libmdpi" / "Makefile.mk").read_text(encoding="utf-8")
        self.assertIn("WRAPPER_CFLAGS :=", makefile)
        self.assertIn(chr(36) + "(CFLAGS)", makefile)
        self.assertEqual(serial.rtl_authenticity_instances, ())
        self.assertEqual(len(sparta.rtl_authenticity_instances), 3)

    def test_serial_runtime_scan_rejects_every_prohibited_family(self):
        tokens = ("kmp_bambu_cs_manager", "kmp_bambu_omp_start_cs", "kmp_bambu_omp_done_cs",
                  "__kmpc_fork_call", "GOMP_parallel", "libgomp.so", "libomp.so", "libiomp5.so")
        for token in tokens:
            with self.subTest(token=token):
                self.assertTrue(_serial_runtime_violations([], token))

    def test_every_existing_regression_remains_present(self):
        ids = {spec.task_id for spec in REGRESSION_SPECS}
        old = {"regression-scalar", "regression-control", "regression-loop-cxx", "regression-memory-interface",
               "regression-callgraph", "regression-sparta", "regression-graphsage"}
        self.assertTrue(old <= ids)
        self.assertIn("regression-graphsage-serial", ids)
if __name__ == "__main__":
    unittest.main()
