#!/usr/bin/env python3
"""Offline mutation tests for the manual duplex effect checker."""
from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
TMP_ROOT = REPO / "documentation" / "tmp"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_duplex_effect_analysis_artifacts import CheckError, SUCCESS, verify  # noqa: E402


TRACE = ('AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=3 read_accesses=1 '
         'write_accesses=1 read_requested=1 read_recognized=0 read_fallback="bundle contains stores" '
         'write_requested=1 write_recognized=0 '
         'write_fallback="bundle contains reads; same-bundle read/write requires duplex support" '
         'same_base_duplex_candidate=1 same_base_duplex_reason="same-base pointwise read-before-write candidate"\n')
RECORD = {
    "bundle": "shared", "semantic_proof": True,
    "semantic_reason": "same-base pointwise read-before-write candidate",
    "profile_eligible": True, "profile_reason": "read/write profile is eligible",
    "owner_argument_index": 3, "base_ssa_index": 9,
    "load_statement_index": 5, "store_statement_index": 6,
    "read_preheader_bb": 0, "read_header_bb": 1, "read_branch_index": 10,
    "read_guard_index": 11, "read_bound_index": 4, "read_induction_index": 13,
    "write_preheader_bb": 0, "write_header_bb": 1, "write_exit_bb": 2,
    "write_guard_index": 11, "write_bound_index": 4, "write_induction_index": 13,
}
RAW = '''COMPILER_VERSION: "Clang 16"
@1 function_val_node name: @2 arg: @3 arg: @4 body: @70
@2 identifier_node strg: "same_base_pointwise"
@3 argument_val_node type: @24 parent: @1
@4 argument_val_node type: @56 parent: @1
@5 assign_stmt parent: @1 bb_index: 1 vuse: @18 op: @19 op: @20
@6 assign_stmt parent: @1 bb_index: 1 vdef: @18 op: @21 op: @45
@8 ssa_node type: @24 def_stmt: @25
@9 ssa_node var: @3
@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 1 bloc: 2
@11 ssa_node def_stmt: @12
@12 assign_stmt parent: @1 bb_index: 0 op: @11 op: @26
@13 ssa_node def_stmt: @32
@14 assign_stmt parent: @1 bb_index: 1 op: @36 op: @37
@19 ssa_node type: @23
@20 mem_access_node type: @23 op: @8 ordinary_memory_access
@21 mem_access_node type: @23 op: @8 ordinary_memory_access
@23 integer_ty_node bitsizealloc: 32 bitsize: 32
@24 pointer_ty_node
@25 assign_stmt parent: @1 bb_index: 1 op: @8 op: @27
@26 gt_node op: @47 op: @55
@27 gep_node type: @24 op: @9 op: @50
@29 constant_int_val_node type: @23 value: 0
@32 phi_stmt parent: @1 bb_index: 1 res: @13 def: @29 edge: 0 def: @36 edge: 1
@34 constant_int_val_node value: 2
@35 constant_int_val_node value: 1
@36 ssa_node type: @23 def_stmt: @14
@37 add_node op: @13 op: @35
@38 ssa_node def_stmt: @39
@39 assign_stmt parent: @1 bb_index: 1 op: @38 op: @40
@40 eq_node op: @36 op: @4
@42 constant_int_val_node value: 3
@43 mul_node op: @19 op: @42
@44 add_node op: @43 op: @35
@45 ssa_node type: @23 def_stmt: @46
@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44
@50 ssa_node type: @23 def_stmt: @51
@51 assign_stmt parent: @1 bb_index: 1 op: @50 op: @52
@52 shl_node op: @13 op: @53
@53 constant_int_val_node value: 2
@55 constant_int_val_node type: @23 value: 0
@47 ssa_node type: @23 def_stmt: @48
@48 assign_stmt parent: @1 bb_index: 0 op: @47 op: @49
@49 nop_node type: @23 op: @4
@56 integer_ty_node bitsizealloc: 32 bitsize: 32 unsigned
@60 multi_way_if_stmt parent: @1 bb_index: 1 op: @38 bloc: 2 bloc: 1
@70 statement_list_node bloc: 0 loop_id: 0 pred: ENTRY succ: 1 succ: 2 stmt: @12 stmt: @10 bloc: 1 loop_id: 5 pred: 0 pred: 1 succ: 1 succ: 2 phi: @32 stmt: @5 stmt: @6 stmt: @14 stmt: @25 stmt: @39 stmt: @51 stmt: @60 bloc: 2 loop_id: 0 pred: 0 pred: 1 succ: EXIT
'''


class DuplexEffectArtifactTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.output = self.root / "case"
        (self.output / "panda-temp").mkdir(parents=True)
        self.source = self.root / "source.cpp"
        self.bambu = self.root / "bambu"
        self.plugin = self.root / "ASTAnalyzer.so"
        self.checker = Path(__file__).resolve().parent / "check_duplex_effect_analysis_artifacts.py"
        self.runner = Path(__file__).resolve().parent / "run_duplex_effect_analysis_tests.sh"
        for path, value in ((self.source, "int fixture;\n"), (self.bambu, "binary\n"),
                            (self.plugin, "plugin\n")):
            path.write_text(value, encoding="utf-8")
        self.bambu.chmod(0o755)
        self.write_artifact("same_base_pointwise.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("same_base_pointwise"))
        self.write_artifact("stderr.log", self.stderr())
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", RAW)
        self.write_artifact("provenance.txt", self.provenance())

    def tearDown(self) -> None:
        self.temp.cleanup()

    def write_artifact(self, relative: str, text: str) -> None:
        (self.output / relative).write_text(text, encoding="utf-8")

    @staticmethod
    def architecture(top: str) -> str:
        return f'''<?xml version="1.0"?>
<root><functions><function name="{top}"><bundles>
<bundle name="shared" mode="m_axi" max_read_burst_length="16"
 max_write_burst_length="16" write_burst_outstanding="1" />
</bundles></function></functions></root>
'''

    @staticmethod
    def stderr(record: dict[str, object] | None = None, trace: str = TRACE) -> str:
        value = dict(RECORD if record is None else record)
        return trace + "AXI_BURST_DUPLEX_PROOF " + json.dumps(value, separators=(",", ":")) + "\n"

    def provenance(self) -> str:
        paths = {"source": self.source, "bambu": self.bambu, "plugin": self.plugin,
                 "checker": self.checker, "runner": self.runner}
        rows = ["top=same_base_pointwise"]
        for name, path in paths.items():
            rows.extend((f"{name}={path}", f"{name}_sha256={hashlib.sha256(path.read_bytes()).hexdigest()}"))
        raw = self.output / "panda-temp/before_Frontend::InterfaceInfer.raw"
        rows.extend((f"raw_ir={raw}", f"raw_ir_sha256={hashlib.sha256(raw.read_bytes()).hexdigest()}"))
        return "\n".join(rows) + "\n"

    def refresh_provenance(self) -> None:
        self.write_artifact("provenance.txt", self.provenance())

    def check(self, top: str = "same_base_pointwise") -> None:
        verify(self.output, top, self.source, self.bambu, self.plugin, self.checker, self.runner)

    def replace_record(self, old: str, new: str, record: dict[str, object] | None = None) -> None:
        self.write_artifact("stderr.log", self.stderr(record).replace(old, new))

    def test_valid_structured_proof_passes(self) -> None:
        self.check()

    def test_missing_structured_record_is_rejected(self) -> None:
        self.write_artifact("stderr.log", TRACE)
        with self.assertRaisesRegex(CheckError, "structured duplex proof record"):
            self.check()

    def test_missing_raw_input_is_rejected(self) -> None:
        raw = self.output / "panda-temp/before_Frontend::InterfaceInfer.raw"
        raw.unlink()
        with self.assertRaisesRegex(CheckError, "raw pre-InterfaceInfer IR missing"):
            self.check()

    def test_fabricated_node_identity_is_rejected(self) -> None:
        record = dict(RECORD, load_statement_index=9999)
        self.write_artifact("stderr.log", self.stderr(record))
        with self.assertRaisesRegex(CheckError, "LOAD statement @9999 does not resolve"):
            self.check()

    def test_other_function_node_identity_is_rejected(self) -> None:
        raw = RAW + '''@80 identifier_node strg: "other_function"
@81 function_val_node name: @80 arg: @82 body: @83
@82 argument_val_node parent: @81
@83 statement_list_node bloc: 0 loop_id: 0 pred: ENTRY succ: EXIT stmt: @84
@84 assign_stmt parent: @81 bb_index: 0 op: @85 op: @86
@85 ssa_node
@86 mem_access_node type: @23 op: @8 ordinary_memory_access
'''
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        record = dict(RECORD, load_statement_index=84)
        self.write_artifact("stderr.log", self.stderr(record))
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "LOAD statement @84 is not a node in selected function"):
            self.check()

    def test_wrong_base_is_rejected(self) -> None:
        record = dict(RECORD, base_ssa_index=27)
        self.write_artifact("stderr.log", self.stderr(record))
        with self.assertRaisesRegex(CheckError, "base identity is not SSA"):
            self.check()

    def test_wrong_index_is_rejected(self) -> None:
        raw = RAW.replace("@27 gep_node type: @24 op: @9 op: @50",
                          "@27 gep_node type: @24 op: @9 op: @53")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "byte offset has no unique defining assignment"):
            self.check()

    def test_base_cannot_substitute_for_loop_bound(self) -> None:
        self.write_artifact("stderr.log", self.stderr(dict(RECORD, read_bound_index=9,
                                                             write_bound_index=9)))
        with self.assertRaisesRegex(CheckError, "read guard does not depend"):
            self.check()

    def test_loop_comparison_cannot_substitute_base_for_bound(self) -> None:
        raw = RAW.replace("@40 eq_node op: @36 op: @4", "@40 eq_node op: @36 op: @9")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "loop comparison is not equality"):
            self.check()

    def test_loop_relation_must_be_equality(self) -> None:
        raw = RAW.replace("@40 eq_node op: @36 op: @4", "@40 ne_node op: @36 op: @4")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "loop comparison is not equality"):
            self.check()

    def test_entry_guard_must_be_signed_greater_than_zero(self) -> None:
        raw = RAW.replace("@26 gt_node op: @47 op: @55", "@26 lt_node op: @47 op: @55")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "entry predicate is not signed declared-bound > 0"):
            self.check()

    def test_entry_bound_conversion_must_be_exact_nop(self) -> None:
        raw = RAW.replace("@49 nop_node type: @23 op: @4",
                          "@49 add_node type: @23 op: @4 op: @57\n@57 constant_int_val_node type: @23 value: 1")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "entry bound conversion is not a single nop"):
            self.check()

    def test_entry_branch_true_and_default_targets_are_not_swappable(self) -> None:
        raw = RAW.replace("@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 1 bloc: 2",
                          "@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 2 bloc: 1")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "preheader branch polarity"):
            self.check()

    def test_loop_branch_true_and_default_targets_are_not_swappable(self) -> None:
        raw = RAW.replace("@60 multi_way_if_stmt parent: @1 bb_index: 1 op: @38 bloc: 2 bloc: 1",
                          "@60 multi_way_if_stmt parent: @1 bb_index: 1 op: @38 bloc: 1 bloc: 2")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "loop branch polarity"):
            self.check()

    def test_byte_offset_must_be_induction_times_four(self) -> None:
        raw = RAW.replace("@52 shl_node op: @13 op: @53", "@52 add_node op: @13 op: @53")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "scaled by four bytes"):
            self.check()

    def test_store_cannot_be_relabelled_as_load(self) -> None:
        raw = RAW.replace("@6 assign_stmt parent: @1 bb_index: 1 vdef: @18 op: @21 op: @45",
                          "@6 assign_stmt parent: @1 bb_index: 1 vdef: @18 op: @45 op: @21")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "STORE assignment has no memory access in the expected LHS"):
            self.check()

    def test_load_cannot_be_relabelled_as_store(self) -> None:
        raw = RAW.replace("@5 assign_stmt parent: @1 bb_index: 1 vuse: @18 op: @19 op: @20",
                          "@5 assign_stmt parent: @1 bb_index: 1 vuse: @18 op: @20 op: @19")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "LOAD assignment has no memory access in the expected RHS"):
            self.check()

    def test_memory_access_width_must_be_full_word(self) -> None:
        raw = RAW.replace("@20 mem_access_node type: @23", "@20 mem_access_node type: @54")
        raw += "@54 integer_ty_node bitsizealloc: 16 bitsize: 16\n"
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "LOAD access width is not the expected 32 bits"):
            self.check()

    def test_forged_exit_block_is_rejected(self) -> None:
        self.write_artifact("stderr.log", self.stderr(dict(RECORD, write_exit_bb=0)))
        with self.assertRaisesRegex(CheckError, "declared write exit is not an actual"):
            self.check()

    def test_forged_cfg_edge_is_rejected(self) -> None:
        raw = RAW.replace("pred: 0 pred: 1 succ: 1 succ: 2 phi:",
                          "pred: 0 pred: 1 succ: 1 phi:")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "declared write exit is not an actual"):
            self.check()

    def test_forged_entry_branch_target_is_rejected(self) -> None:
        raw = RAW.replace("@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 1 bloc: 2",
                          "@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 1 bloc: 1")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "preheader branch polarity"):
            self.check()

    def test_induction_must_start_at_zero(self) -> None:
        raw = RAW.replace("@29 constant_int_val_node type: @23 value: 0",
                          "@29 constant_int_val_node type: @23 value: 1")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "does not start at zero"):
            self.check()

    def test_induction_must_increment_by_one(self) -> None:
        raw = RAW.replace("@35 constant_int_val_node value: 1", "@35 constant_int_val_node value: 2")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "does not increment by one"):
            self.check()

    def test_forged_header_is_rejected(self) -> None:
        self.write_artifact("stderr.log", self.stderr(dict(RECORD, read_header_bb=0,
                                                             write_header_bb=0)))
        with self.assertRaisesRegex(CheckError, "header is not marked as a loop block"):
            self.check()

    def test_wrong_load_store_order_is_rejected(self) -> None:
        raw = RAW.replace("stmt: @5 stmt: @6 stmt: @14", "stmt: @6 stmt: @5 stmt: @14")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "LOAD does not precede STORE"):
            self.check()

    def test_different_loop_block_is_rejected(self) -> None:
        raw = RAW.replace("stmt: @5 stmt: @6 stmt: @14", "stmt: @5 stmt: @14")
        raw = raw.replace("@6 assign_stmt parent: @1 bb_index: 1", "@6 assign_stmt parent: @1 bb_index: 3")
        raw = raw.replace("bloc: 2 loop_id: 0 pred: 0 pred: 1 succ: EXIT",
                          "bloc: 2 loop_id: 0 pred: 0 pred: 1 succ: EXIT "
                          "bloc: 3 loop_id: 6 pred: 1 succ: EXIT stmt: @6")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "same original loop block"):
            self.check()

    def test_semantic_identity_fields_are_required(self) -> None:
        record = dict(RECORD)
        del record["load_statement_index"]
        self.write_artifact("stderr.log", self.stderr(record))
        with self.assertRaisesRegex(CheckError, "missing identity fields"):
            self.check()

    def test_semantic_false_cannot_carry_identity_fields(self) -> None:
        record = {"bundle": "shared", "semantic_proof": False,
                  "semantic_reason": "canonical contiguous loop recognition failed",
                  "profile_eligible": True, "profile_reason": "read/write profile is eligible"}
        trace = TRACE.replace("same_base_duplex_candidate=1", "same_base_duplex_candidate=0")
        trace = trace.replace(SUCCESS, "canonical contiguous loop recognition failed")
        self.write_artifact("stderr.log", trace + "AXI_BURST_DUPLEX_PROOF " +
                            json.dumps(record, separators=(",", ":")) + "\n")
        self.write_artifact("shifted_index.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("shifted_index"))
        self.write_artifact("provenance.txt", self.provenance().replace(
            "top=same_base_pointwise", "top=shifted_index", 1))
        self.check("shifted_index")
        self.write_artifact("stderr.log", trace)
        self.check("shifted_index")
        record["load_statement_index"] = 5
        self.write_artifact("stderr.log", trace + "AXI_BURST_DUPLEX_PROOF " +
                            json.dumps(record, separators=(",", ":")) + "\n")
        with self.assertRaisesRegex(CheckError, "success-only identity fields"):
            self.check("shifted_index")

    def test_cached_profile_promotion_is_rejected(self) -> None:
        record = dict(RECORD, profile_eligible=False, profile_reason="cache-enabled bundle")
        cache_trace = TRACE.replace('read_fallback="bundle contains stores"',
                                    'read_fallback="cache-enabled bundle"')
        cache_trace = cache_trace.replace(
            'write_fallback="bundle contains reads; same-bundle read/write requires duplex support"',
            'write_fallback="cache-enabled bundle"')
        self.write_artifact("stderr.log", self.stderr(record, cache_trace))
        self.write_artifact("same_base_cached.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("same_base_cached"))
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw",
                            RAW.replace('strg: "same_base_pointwise"', 'strg: "same_base_cached"'))
        manifest = self.provenance().replace("top=same_base_pointwise", "top=same_base_cached", 1)
        self.write_artifact("provenance.txt", manifest)
        self.check("same_base_cached")
        record["profile_eligible"] = True
        self.write_artifact("stderr.log", self.stderr(record, cache_trace))
        with self.assertRaisesRegex(CheckError, "profile eligibility"):
            self.check("same_base_cached")

    def test_wrong_raw_digest_is_rejected(self) -> None:
        self.write_artifact("provenance.txt", self.provenance().replace("raw_ir_sha256=", "raw_ir_sha256=0", 1))
        with self.assertRaisesRegex(CheckError, "wrong raw IR provenance digest"):
            self.check()

    def test_wrong_trace_reason_is_rejected(self) -> None:
        self.write_artifact("stderr.log", self.stderr().replace(SUCCESS, "canonical contiguous loop recognition failed"))
        with self.assertRaisesRegex(CheckError, "candidate outcome/reason mismatch"):
            self.check()

    def test_output_symlink_is_rejected(self) -> None:
        link = self.root / "case-link"
        link.symlink_to(self.output, target_is_directory=True)
        with self.assertRaisesRegex(CheckError, "invalid output directory"):
            verify(link, "same_base_pointwise", self.source, self.bambu, self.plugin, self.checker, self.runner)


if __name__ == "__main__":
    unittest.main()
