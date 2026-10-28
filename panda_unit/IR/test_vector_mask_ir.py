# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Protect the vector-mask fixtures and check the structural checker itself."""

import importlib.util
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "panda_regressions/hls/bambu_vector_test"
SPEC = importlib.util.spec_from_file_location("checker", str(FIXTURES / "check_vector_mask_ir.py"))
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


class IRText:
    def __init__(self, text):
        self.text = text

    def read_text(self):
        return self.text

    def __str__(self):
        return "unit fixture"


class VectorMaskIR(unittest.TestCase):
    def setUp(self):
        self.source = (FIXTURES / "vector_mask_ir.ll").read_text()
        self.parsed = CHECKER.parse_ir(IRText(self.source))

    def test_all_definitions_are_protected(self):
        for path, count in [(FIXTURES / "vector_mask_ir.ll", 11),
                            (FIXTURES / "vector_mask_qor.ll", 1),
                            (FIXTURES / "vector_mask_mem_layout.ll", 2),
                            (ROOT / "panda_unit/IR/vector_mask_scalable.ll", 1)]:
            with self.subTest(path=path):
                self.assertEqual(len(CHECKER.parse_ir(path)), count)

    def test_comments_cannot_replace_attributes(self):
        for remaining in ("noinline", "optnone"):
            text = "; Function Attrs: noinline optnone\n" + self.source.replace("noinline optnone", remaining)
            with self.subTest(remaining=remaining), self.assertRaises(ValueError):
                CHECKER.parse_ir(IRText(text))
        with self.assertRaises(ValueError):
            CHECKER.parse_ir(IRText(self.source.replace("noinline optnone", "noinline ; optnone\n")))

    def test_array_pointer_syntax_preserves_memory_addresses(self):
        opaque = self.source.replace("[2 x i8]*", "ptr").replace("i8*", "ptr")
        opaque = opaque.replace("%slot, i32 0, i32", "%slot, i64 0, i64")
        self.assertEqual(CHECKER.parse_ir(IRText(opaque)), self.parsed)
        swapped = self.source.replace("%slot, i32 0, i32 1", "%slot, i32 0, i32 0")
        self.assertNotEqual(CHECKER.parse_ir(IRText(swapped)), self.parsed)

    def test_trunc_zext_and_mask_are_equivalent_only_with_the_same_bits(self):
        before = "%packed = trunc i8 %round to i4\n  %wide = zext i4 %packed to i32"
        after = "%packed = and i8 %round, 15\n  %wide = zext i8 %packed to i32"
        self.assertIn(before, self.source)
        transformed = self.source.replace(before, after)
        self.assertEqual(CHECKER.parse_ir(IRText(transformed)), self.parsed)
        for changed in (after.replace(", 15", ", 14"), after.replace("zext", "sext")):
            with self.subTest(changed=changed):
                self.assertNotEqual(CHECKER.parse_ir(IRText(self.source.replace(before, changed))), self.parsed)
        # A second use of the narrow value prohibits the rewrite.
        reference = self.source.replace("  %wide = zext i4 %packed", "  %other = add i4 %packed, 1\n  %wide = zext i4 %packed")
        changed = transformed.replace("  %wide = zext i8 %packed", "  %other = add i4 %packed, 1\n  %wide = zext i8 %packed")
        self.assertNotEqual(CHECKER.parse_ir(IRText(reference)), CHECKER.parse_ir(IRText(changed)))

    def test_scalar_and_vector_byte_slices_preserve_byte_order(self):
        before = ("%bits = bitcast <16 x i1> %cmp to i16\n"
                  "  %low = trunc i16 %bits to i8\n"
                  "  %shifted = lshr i16 %bits, 8\n"
                  "  %high = trunc i16 %shifted to i8")
        after = ("%bytes = bitcast <16 x i1> %cmp to <2 x i8>\n"
                 "  %low = extractelement <2 x i8> %bytes, i64 0\n"
                 "  %high = extractelement <2 x i8> %bytes, i64 1")
        self.assertIn(before, self.source)
        self.assertEqual(CHECKER.parse_ir(IRText(self.source.replace(before, after))), self.parsed)
        for changed in (after.replace("i64 1", "i64 0"), before.replace("lshr i16 %bits, 8", "lshr i16 %bits, 7")):
            with self.subTest(changed=changed):
                self.assertNotEqual(CHECKER.parse_ir(IRText(self.source.replace(before, changed))), self.parsed)
        extra_use = self.source.replace("  %high = trunc i16 %shifted", "  %other = add i16 %shifted, 1\n  %high = trunc i16 %shifted")
        self.assertTrue(any("lshr i16" in line for line in CHECKER.parse_ir(IRText(extra_use))["mask_store_load16"]))

    def test_later_insert_can_hide_poison_only_without_an_intermediate_read(self):
        before = "insertelement <16 x i32> zeroinitializer, i32 %a, i32 7"
        lanes = ["i32 0"] * 16
        lanes[7] = lanes[8] = "i32 poison"
        after = "insertelement <16 x i32> <" + ", ".join(lanes) + ">, i32 %a, i64 7"
        self.assertIn(before, self.source)
        poisoned = self.source.replace(before, after)
        self.assertEqual(CHECKER.parse_ir(IRText(poisoned)), self.parsed)
        lanes[9] = "i32 poison"
        bad = "insertelement <16 x i32> <" + ", ".join(lanes) + ">, i32 %a, i64 7"
        self.assertNotEqual(CHECKER.parse_ir(IRText(self.source.replace(before, bad))), self.parsed)
        # Compare otherwise identical functions: an earlier read exposes lane 8.
        next_insert = "  %av = insertelement <16 x i32> %av7"
        read = "  %observed = extractelement <16 x i32> %av7, i32 8\n" + next_insert
        reference = self.source.replace(next_insert, read)
        changed = poisoned.replace(next_insert, read)
        self.assertNotEqual(CHECKER.parse_ir(IRText(reference)), CHECKER.parse_ir(IRText(changed)))

    def test_changes_to_the_specification_are_detected(self):
        replacements = [
            ("<i1 true, i1 true, i1 false, i1 true>", "<i1 true, i1 false, i1 false, i1 true>"),
            ("bitcast <4 x i1> %mask to i4", "bitcast <8 x i1> %mask to i8"),
            ("  %bits = bitcast <4 x i1> %mask to i4\n", ""),
            ("  %wide = zext i4 %bits to i8\n", ""),
            ("shl i32 %selected, 16", "shl i32 %selected, 15"),
            ("or i32 %wide, %high", "or i32 %selected, %high"),
        ]
        for before, after in replacements:
            with self.subTest(before=before):
                self.assertIn(before, self.source)
                self.assertNotEqual(CHECKER.parse_ir(IRText(self.source.replace(before, after))), self.parsed)

    def test_equivalent_scalar_bit_packing(self):
        text = self.source.replace("  %selected = zext i1 %lane to i32\n"
                                   "  %high = shl i32 %selected, 16",
                                   "  %high = select i1 %lane, i32 65536, i32 0")
        text = text.replace("or i32 %wide, %high", "or i32 %high, %wide")
        self.assertNotEqual(text, self.source)
        self.assertEqual(CHECKER.parse_ir(IRText(text)), self.parsed)

    def test_return_range_attribute_does_not_hide_parameters(self):
        text = self.source.replace("define i32 @mask_eq16", "define range(i32 0, 65536) i32 @mask_eq16")
        self.assertEqual(CHECKER.parse_ir(IRText(text)), self.parsed)

    def test_only_the_overwritten_insert_lane_can_change(self):
        initial = "<i32 poison, " + ", ".join(["i32 0"] * 15) + ">"
        before = "insertelement <16 x i32> zeroinitializer, i32 %a, i32 0"
        after = "insertelement <16 x i32> " + initial + ", i32 %a, i64 0"
        text = self.source.replace(before, after)
        self.assertNotEqual(text, self.source)
        self.assertEqual(CHECKER.parse_ir(IRText(text)), self.parsed)
        bad = initial.replace("i32 poison, i32 0", "i32 poison, i32 poison", 1)
        self.assertNotEqual(CHECKER.parse_ir(IRText(text.replace(initial, bad))), self.parsed)
        self.assertNotEqual(CHECKER.parse_ir(IRText(text.replace(", i32 %a, i64 0", ", i32 %a, i64 1"))), self.parsed)


if __name__ == "__main__":
    unittest.main()
