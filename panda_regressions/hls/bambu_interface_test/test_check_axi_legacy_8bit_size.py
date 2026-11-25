from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from check_axi_legacy_8bit_size import CheckError, check_vcd


class LegacySizeVcdTests(unittest.TestCase):
    @staticmethod
    def write_vcd(directory: Path, *, arsize: str = "000", awsize: str = "000",
                  arready: str = "1", awready: str = "1",
                  include_clock: bool = True, clock_edges: bool = True,
                  extra_clock: bool = False, controls_at_clock_edge: bool = False,
                  packed_size_vectors: bool = False) -> Path:
        path = directory / "test.vcd"

        def size_declarations(name: str, identifiers: tuple[str, str, str]) -> list[str]:
            if packed_size_vectors:
                return [f"$var wire 3 {identifiers[0]} sig_m_axi_shared_{name} [2:0] $end"]
            return [f"$var wire 1 {identifier} sig_m_axi_shared_{name} [{bit}] $end"
                    for identifier, bit in zip(identifiers, (2, 1, 0))]

        def size_values(value: str, identifiers: tuple[str, str, str]) -> list[str]:
            if packed_size_vectors:
                return [f"b{value.lstrip('0') or '0'} {identifiers[0]}"]
            return [f"{value[index]}{identifier}" for index, identifier in enumerate(identifiers)]

        declarations = [
            "$scope module clocked_bambu_testbench $end",
            "$var reg 1 k clock $end",
            "$scope module system $end",
            *( ["$var wire 1 k clock $end"] if include_clock else [] ),
            *( ["$var wire 1 l clock $end"] if extra_clock else [] ),
            "$var wire 1 a sig_m_axi_shared_arvalid $end",
            "$var wire 1 b sig_m_axi_shared_arready $end",
            *size_declarations("arsize", ("c", "d", "e")),
            "$var wire 1 f sig_m_axi_shared_awvalid $end",
            "$var wire 1 g sig_m_axi_shared_awready $end",
            *size_declarations("awsize", ("h", "i", "j")),
            "$upscope $end",
            "$upscope $end",
            "$enddefinitions $end",
            "$dumpvars",
            "0k",
            *( ["0l"] if extra_clock else [] ),
            "0a", "0b", *size_values("000", ("c", "d", "e")),
            "0f", "0g", *size_values("000", ("h", "i", "j")),
            "$end",
            "#0",
        ]
        if controls_at_clock_edge:
            declarations.extend((
                "#5", "1k", "1a", *size_values(arsize, ("c", "d", "e")), f"{arready}b",
                "#7", "0k", "0a", "0b",
                "#20", "1f", *size_values(awsize, ("h", "i", "j")), f"{awready}g",
                "#22", "0f", "0g",
            ))
        else:
            declarations.extend((
                "#5", "1a", *size_values(arsize, ("c", "d", "e")),
                "#7", f"{arready}b",
                "#10", *( ["1k"] if clock_edges else [] ),
                "#12", *( ["0k"] if clock_edges else [] ),
                "#15", "0a", "0b",
                "#20", "1f", *size_values(awsize, ("h", "i", "j")),
                "#22", f"{awready}g",
                "#25", *( ["1k"] if clock_edges else [] ),
                "#27", *( ["0k"] if clock_edges else [] ),
                "#30", "0f", "0g",
            ))
        path.write_text("\n".join(declarations) + "\n", encoding="ascii")
        return path

    def test_accepts_real_ar_and_aw_handshakes_at_size_zero(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(check_vcd(self.write_vcd(Path(temp))), (1, 1))

    def test_accepts_packed_size_vectors_with_short_zero_values(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = self.write_vcd(Path(temp), packed_size_vectors=True)
            self.assertEqual(check_vcd(path), (1, 1))

    def test_rejects_wrong_size_on_valid(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "ARSIZE=.*expected 000"):
                check_vcd(self.write_vcd(Path(temp), arsize="001"))

    def test_rejects_wrong_packed_size_on_valid(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = self.write_vcd(Path(temp), arsize="001", packed_size_vectors=True)
            with self.assertRaisesRegex(CheckError, "ARSIZE=.*expected 000"):
                check_vcd(path)

    def test_rejects_missing_handshake(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "missing active read/write"):
                check_vcd(self.write_vcd(Path(temp), arready="0"))

    def test_rejects_missing_scope_local_clock(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "one scalar clock in the AXI master scope"):
                check_vcd(self.write_vcd(Path(temp), include_clock=False))

    def test_rejects_ambiguous_scope_local_clocks(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "one scalar clock in the AXI master scope"):
                check_vcd(self.write_vcd(Path(temp), extra_clock=True))

    def test_rejects_valid_ready_overlap_without_active_clock_edge(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "missing active read/write"):
                check_vcd(self.write_vcd(Path(temp), clock_edges=False))

    def test_ignores_controls_changed_after_same_timestamp_clock_edge(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(CheckError, "missing active read/write"):
                check_vcd(self.write_vcd(Path(temp), controls_at_clock_edge=True))

    def test_rejects_missing_size_bit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = self.write_vcd(Path(temp))
            text = path.read_text(encoding="ascii").replace(
                "$var wire 1 c sig_m_axi_shared_arsize [2] $end\n", "")
            path.write_text(text, encoding="ascii")
            with self.assertRaisesRegex(CheckError, "complete AXI master"):
                check_vcd(path)


if __name__ == "__main__":
    unittest.main()
