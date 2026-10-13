# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Negative controls for the HLS artifact contracts used by the vector CI."""

import importlib.util
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "hls_checker", str(ROOT / "panda_regressions/hls/bambu_vector_test/check_vector_mask_hls.py"))
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


IR = '''@1 function_val_node name: @2 body: @3 parent: @9
@2 identifier_node strg: "vector_mask_compare"
@3 assign_stmt type: @4
@4 vector_ty_node bitsizealloc: 8 elts: @5 nelements: 2
@5 integer_ty_node bitsizealloc: 8 bitsize: 1 unsigned
@9 module_unit_node decls: @4
'''


def instance(required, name):
    parameters = ", ".join(".{}({})".format(key, value) for key, value in required["parameters"].items())
    return "{} #({}) {} (.out1(out), .in1(a), .in2(b));".format(required["name"], parameters, name)


class VectorMaskHLS(unittest.TestCase):
    def setUp(self):
        self.contract = CHECKER.contract("vector_mask_eq_unsigned", "I386_CLANG16")
        self.instances = [instance(required, "fu" + str(i)) for i, required in enumerate(self.contract["required"])]
        library = "\n".join("module {}(output out1, input in1, in2); assign out1 = in1; endmodule".format(r["name"])
                            for r in self.contract["required"])
        self.rtl = ("module vector_mask_compare(input a,b,output out); datapath dp (.a(a),.b(b),.out(out)); endmodule\n"
                    "module datapath(input a,b,output out);\n" + "\n".join(self.instances) + "\nendmodule\n" + library)

    def test_instantiated_baseline(self):
        self.assertEqual(len(CHECKER.check_rtl(CHECKER.parse_modules(self.rtl), self.contract)), 3)
        self.assertEqual(CHECKER.check_bambuir(IR, self.contract), [(2, 1)])

    def test_each_required_fu_must_survive(self):
        for item in self.instances:
            with self.subTest(item=item), self.assertRaisesRegex(ValueError, "missing instantiated FU"):
                CHECKER.check_rtl(CHECKER.parse_modules(self.rtl.replace(item, "")), self.contract)

    def test_comments_and_wire_names_do_not_count(self):
        replacement = "// " + self.instances[0] + "\nwire out_ui_vec_eq_node_FU;"
        with self.assertRaisesRegex(ValueError, "missing instantiated FU ui_vec_eq_node_FU"):
            CHECKER.check_rtl(CHECKER.parse_modules(self.rtl.replace(self.instances[0], replacement)), self.contract)

    def test_unused_module_and_library_definition_do_not_count(self):
        rtl = self.rtl.replace(self.instances[0], "") + "\nmodule unused(input a,b,output out);" + self.instances[0] + "endmodule"
        with self.assertRaisesRegex(ValueError, "missing instantiated FU ui_vec_eq_node_FU"):
            CHECKER.check_rtl(CHECKER.parse_modules(rtl), self.contract)

    def test_allocated_carrier_is_not_logical_mask(self):
        wrong = self.instances[0].replace(".BITSIZE_out1(1)", ".BITSIZE_out1(8)").replace(".NUM_ELEM_out1(2)", ".NUM_ELEM_out1(1)")
        with self.assertRaisesRegex(ValueError, "dimensions"):
            CHECKER.check_rtl(CHECKER.parse_modules(self.rtl.replace(self.instances[0], wrong)), self.contract)

    def test_missing_top(self):
        with self.assertRaisesRegex(ValueError, "Missing RTL top"):
            CHECKER.check_rtl(CHECKER.parse_modules(self.rtl.replace("module vector_mask_compare", "module other")), self.contract)

    def test_nonzero_nelements_are_required(self):
        for replacement in ("", "nelements: 0"):
            with self.subTest(replacement=replacement), self.assertRaisesRegex(ValueError, "nonzero nelements"):
                CHECKER.check_bambuir(IR.replace("nelements: 2", replacement), self.contract)

    def test_mask_lane_count_and_element_width(self):
        for before, after in (("nelements: 2", "nelements: 1"), ("bitsize: 1", "bitsize: 8")):
            with self.subTest(before=before), self.assertRaisesRegex(ValueError, "Missing reachable vector metadata"):
                CHECKER.check_bambuir(IR.replace(before, after), self.contract)

    def test_ir_parent_cannot_supply_an_unreachable_mask(self):
        with self.assertRaisesRegex(ValueError, "Missing reachable vector metadata"):
            CHECKER.check_bambuir(IR.replace("assign_stmt type: @4", "return_stmt"), self.contract)

    def test_constant_must_be_packed_and_connected(self):
        spec = CHECKER.contract("vector_mask_const4", "I386_CLANG16")
        rtl = "module mask_const4(input a,output out); assign b = 4'b1011;" + instance(spec["required"][0], "xor_fu") + instance(spec["required"][1], "cast_fu") + "endmodule"
        CHECKER.check_rtl(CHECKER.parse_modules(rtl), spec)
        for value in ("4'b0001", "32'h01000101"):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, "packed 4-bit constant"):
                CHECKER.check_rtl(CHECKER.parse_modules(rtl.replace("4'b1011", value)), spec)
        with self.assertRaisesRegex(ValueError, "packed 4-bit constant"):
            CHECKER.check_rtl(CHECKER.parse_modules(rtl.replace("assign b", "assign unused")), spec)

    def test_memory_round_trip_requires_memory_instance(self):
        spec = {"top": "top", "required": [], "memory": True}
        with self.assertRaisesRegex(ValueError, "missing instantiated memory"):
            CHECKER.check_rtl(CHECKER.parse_modules("module top(); endmodule"), spec)

    def test_unknown_contracts_fail_explicitly(self):
        for case, compiler in (("unknown", "I386_CLANG16"), ("vector_mask_eq_unsigned", "I386_CLANG7")):
            with self.subTest(case=case), self.assertRaises(ValueError):
                CHECKER.contract(case, compiler)

    def test_cpp_operator_contracts_are_compiler_specific(self):
        modern = CHECKER.contract("vector_mask_ne_unsigned", "I386_CLANG16")
        legacy = CHECKER.contract("vector_mask_ne_unsigned", "I386_CLANG6")
        self.assertEqual(modern["required"][0]["name"], "ui_vec_eq_node_FU")
        self.assertEqual(legacy["required"][0]["name"], "ui_vec_ne_node_FU")


if __name__ == "__main__":
    unittest.main()
