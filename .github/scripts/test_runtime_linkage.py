"""Fixture tests for deterministic OpenMP/MDPI linkage evidence."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from ci_results.runtime_linkage import (
    _command_inventory,
    _parse_readelf_dynamic,
    _parse_readelf_symbols,
    inspect_runtime_linkage,
)


class RuntimeLinkageTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="runtime-linkage-")
        self.repository = Path(self.temporary.name)
        self.output = self.repository / "output"
        simulation = self.output / "HLS_output" / "simulation"
        behavior = self.output / "HLS_output" / "VERILATOR_beh"
        simulation.mkdir(parents=True)
        behavior.mkdir(parents=True)
        for name in ("mdpi_wrapper.cpp", "libmdpi_driver.so", "testbench"):
            (simulation / name).write_text("fixture\n", encoding="utf-8")
        (behavior / "libmdpi.so").write_text("fixture\n", encoding="utf-8")
        self.log = "\n".join(
            (
                "/opt/panda/share/panda/libopenmp/kmp_single_file.cpp",
                "clang++ -c /opt/panda/share/panda/libmdpi/mdpi_driver.cpp -o "
                f"{simulation}/build/mdpi_driver.cpp.o",
                "clang -shared /opt/panda/share/panda/libmdpi/mdpi.c -o "
                f"{behavior}/libmdpi.so",
                f"clang++ -c {simulation}/mdpi_wrapper.cpp -o wrapper.o",
                "clang++ -c "
                f"{self.repository}/examples/GraphSAGE/graphsage_mean_test.cpp -o test.o",
                f"clang++ -shared -o {simulation}/libmdpi_driver.so driver.o",
                f"clang++ -o {simulation}/testbench test.o",
            )
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def inspect(self, dynamic: str | None = None, symbols: str | None = None):
        dynamic = dynamic or "Dynamic section at offset 0:\n 0 (NEEDED) [libc.so.6]\n"
        symbols = symbols or "Symbol table '.dynsym' contains 1 entry:\n"

        def tool(command: list[str]) -> str:
            if command[1] == "-d":
                return dynamic
            if command[1] == "-Ws":
                return symbols
            return ""

        with patch("ci_results.runtime_linkage.shutil.which", return_value="/usr/bin/tool"), patch(
            "ci_results.runtime_linkage._elf_artifacts",
            return_value=[self.output / "HLS_output/simulation/libmdpi_driver.so"],
        ), patch("ci_results.runtime_linkage._run_tool", side_effect=tool):
            return inspect_runtime_linkage(
                self.repository,
                self.output,
                self.log,
                "I386_CLANG16",
                "examples/GraphSAGE/graphsage_mean_test.cpp",
                (("kmp_bambu_cs_manager", "cs_manager"),),
            )

    def test_detects_bundled_openmp_mdpi_and_normalized_commands(self) -> None:
        report, errors = self.inspect()
        self.assertEqual(errors, [])
        self.assertIn("openmp-source\tlibopenmp/kmp_single_file.cpp", report)
        self.assertIn("mdpi-source\tmdpi_wrapper.cpp", report)
        self.assertIn("command\tclang++ -c <output>/", report)
        self.assertNotIn(str(self.repository), report)

    def test_missing_bundled_openmp_evidence_fails(self) -> None:
        self.log = self.log.replace("/opt/panda/share/panda/libopenmp/kmp_single_file.cpp\n", "")
        _, errors = self.inspect()
        self.assertTrue(any("missing bundled OpenMP" in error for error in errors))

    def test_missing_mdpi_driver_or_simulator_library_fails(self) -> None:
        for name in ("libmdpi_driver.so", "libmdpi.so"):
            with self.subTest(name=name):
                target = next(self.output.rglob(name))
                saved = target.read_text(encoding="utf-8")
                target.unlink()
                _, errors = self.inspect()
                self.assertTrue(any(f"missing MDPI artifact: {name}" in error for error in errors))
                target.write_text(saved, encoding="utf-8")

    def test_rejects_prohibited_dynamic_runtimes(self) -> None:
        for library in ("libgomp.so.1", "libomp.so.5", "libiomp5.so"):
            with self.subTest(library=library):
                _, errors = self.inspect(
                    dynamic=f"Dynamic section at offset 0:\n 0 (NEEDED) [{library}]\n"
                )
                self.assertTrue(any("prohibited OpenMP" in error for error in errors))

    def test_rejects_unresolved_openmp_symbols(self) -> None:
        for symbol in ("GOMP_parallel", "__kmpc_fork_call", "omp_get_num_threads"):
            with self.subTest(symbol=symbol):
                symbols = (
                    "Symbol table '.dynsym' contains 2 entries:\n"
                    f"  1: 0 0 FUNC GLOBAL DEFAULT UND {symbol}\n"
                )
                _, errors = self.inspect(symbols=symbols)
                self.assertTrue(any("unresolved OpenMP" in error for error in errors))

    def test_rejects_malformed_tool_and_command_inventory(self) -> None:
        with self.assertRaisesRegex(ValueError, "malformed readelf"):
            _parse_readelf_dynamic("not tool output")
        with self.assertRaisesRegex(ValueError, "malformed readelf"):
            _parse_readelf_symbols("not tool output")
        with self.assertRaisesRegex(ValueError, "malformed MDPI command"):
            _command_inventory("clang -c mdpi.c 'unterminated", self.repository, self.output)


if __name__ == "__main__":
    unittest.main()
