#!/usr/bin/env python3
"""Offline mutation tests for the manual duplex effect checker."""
from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[3]
TMP_ROOT = REPO / "documentation" / "tmp"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_duplex_effect_analysis_artifacts import (  # noqa: E402
    CASES, CheckError, FALLBACK_READ, FALLBACK_WRITE, SUCCESS,
    verify, verify_restrict_architecture_metadata,
    verify_success_record, check_production_duplex, expected_owner_recognition,
    verify_configure_count_expressions,
    verify_unsigned_prefix_comparison,
    verify_prefix_loop_trip_control,
    verify_direction_guard_dominance,
    function_graph,
)
from check_duplex_activation_matrix import (
    ALL_CASES, INCOMPATIBLE, INCOMPATIBLE_PROFILE_DIAGNOSTIC, VETO_REASON, VETO_REASON_FIELD,
    activation_markers, verify as verify_activation_matrix,
)
from check_duplex_activation_matrix import (
    EXPECTED_ROOTS, POSITIVE_ROOTS, require_raw, root_calls, verify_architecture,
    verify_backend_guard, verify_backend_rtl, verify_call_contract, verify_drain_exit,
    verify_burst_call_arguments, DATAFLOW_ANALYSIS_ORDER, verify_dataflow_analysis_order,
    verify_dataflow_task_call, verify_dataflow_task_memory,
    verify_no_duplex_backend_rtl, verify_no_legacy_access_contract, verify_preanalysis_veto,
    verify_unused_peer_veto, pointer_argument_ids,
)


TRACE = ('AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=3 read_accesses=1 '
         'write_accesses=1 read_requested=1 read_recognized=1 read_fallback="-" '
         'write_requested=1 write_recognized=1 '
         'write_fallback="-" '
         'same_base_duplex_candidate=1 same_base_duplex_reason="same-base pointwise read-before-write candidate" '
         'production_activated=1 activation_reason="frozen application-wide proof and profile accepted"\n')
RECORD = {
    "bundle": "shared", "semantic_proof": True,
    "semantic_reason": "same-base pointwise read-before-write candidate",
    "profile_eligible": True, "profile_reason": "read/write profile is eligible",
    "proof_kind": "same_base_pointwise", "owner_argument_index": 3,
    "read_owner_argument_index": 3, "write_owner_argument_index": 3,
    "base_ssa_index": 9, "read_base_ssa_index": 9, "write_base_ssa_index": 9,
    "load_statement_index": 5, "store_statement_index": 6,
    "read_preheader_bb": 0, "read_header_bb": 1, "read_branch_index": 10,
    "read_guard_index": 11, "read_bound_index": 63, "read_induction_index": 13,
    "write_preheader_bb": 0, "write_header_bb": 1, "write_exit_bb": 2,
    "write_guard_index": 11, "write_bound_index": 63, "write_induction_index": 13,
    "loop_trip_bound_ssa_index": 63, "loop_trip_compare_index": 40,
    "read_effective_bound_ssa_index": 63, "write_effective_bound_ssa_index": 63,
    "read_effective_compare_index": 0, "write_effective_compare_index": 0,
    "read_access_bb": 1, "write_access_bb": 1,
    "read_count_recipe": "trip_count", "write_count_recipe": "trip_count",
    "pairing_domain_recipe": "min_read_write_counts_prefix",
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
@40 eq_node op: @36 op: @63
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
@49 nop_node type: @23 op: @63
@56 integer_ty_node bitsizealloc: 32 bitsize: 32 unsigned
@63 ssa_node type: @56 var: @4
@60 multi_way_if_stmt parent: @1 bb_index: 1 op: @38 bloc: 2 bloc: 1
@70 statement_list_node bloc: 0 loop_id: 0 pred: ENTRY succ: 1 succ: 2 stmt: @12 stmt: @10 bloc: 1 loop_id: 5 pred: 0 pred: 1 succ: 1 succ: 2 phi: @32 stmt: @5 stmt: @6 stmt: @14 stmt: @25 stmt: @39 stmt: @51 stmt: @60 bloc: 2 loop_id: 0 pred: 0 pred: 1 succ: EXIT
'''

RAW_DISTINCT = RAW.replace(
    '@1 function_val_node name: @2 arg: @3 arg: @4 body: @70',
    '@1 function_val_node name: @2 arg: @3 arg: @57 arg: @4 body: @70').replace(
    '@2 identifier_node strg: "same_base_pointwise"',
    '@2 identifier_node strg: "distinct_bases_source_first"').replace(
    '@21 mem_access_node type: @23 op: @8 ordinary_memory_access',
    '@21 mem_access_node type: @23 op: @59 ordinary_memory_access').replace(
    '@56 integer_ty_node bitsizealloc: 32 bitsize: 32 unsigned',
    '@56 integer_ty_node bitsizealloc: 32 bitsize: 32 unsigned\n'
    '@57 argument_val_node type: @24 parent: @1\n'
    '@58 ssa_node var: @57\n'
    '@59 ssa_node type: @24 def_stmt: @61\n'
    '@61 assign_stmt parent: @1 bb_index: 1 op: @59 op: @62\n'
    '@62 gep_node type: @24 op: @58 op: @50').replace(
    'stmt: @5 stmt: @6 stmt: @14', 'stmt: @5 stmt: @61 stmt: @6 stmt: @14')

RECORD_DISTINCT = dict(RECORD, semantic_reason="distinct-base restricted pointwise read-before-write candidate",
                       proof_kind="distinct_base_restricted_pointwise", write_owner_argument_index=57,
                       write_base_ssa_index=58)


class DuplexEffectArtifactTests(unittest.TestCase):
    def test_activation_matrix_has_explicit_multi_root_contracts(self):
        self.assertEqual(ALL_CASES, set(range(1, 18)))
        self.assertEqual(INCOMPATIBLE, {13, 14})
        self.assertNotIn(12, VETO_REASON)
        self.assertNotIn(12, VETO_REASON_FIELD)
        self.assertEqual(POSITIVE_ROOTS[1], {"matrix_candidate", "matrix_peer"})
        for first, second in ((3, 4), (5, 6), (7, 8), (9, 10)):
            self.assertEqual(VETO_REASON[first], VETO_REASON[second])

    def test_activation_veto_owner_lookup_preserves_raw_ir_reference_prefix(self):
        raw = (
            '@1 function_val_node name: @2 arg: @3 arg: @4\n'
            '@2 identifier_node strg: "matrix_candidate"\n'
            '@3 argument_val_node type: @5\n'
            '@4 argument_val_node type: @6\n'
            '@5 pointer_ty_node\n'
            '@6 integer_ty_node\n'
        )
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as temporary:
            path = Path(temporary) / "before.raw"
            path.write_text(raw, encoding="utf-8")
            self.assertEqual(pointer_argument_ids(path, ("matrix_candidate",)), {3})

    def test_activation_veto_reason_must_be_in_preanalysis_record(self):
        reason = VETO_REASON[3]
        record = ('AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=3 read_accesses=1 '
                  'write_accesses=1 read_requested=1 read_recognized=0 read_fallback="-" '
                  'write_requested=1 write_recognized=0 write_fallback="-" '
                  f'same_base_duplex_candidate=0 same_base_duplex_reason="{reason}" '
                  'production_activated=0 activation_reason="candidate rejected"\n')
        verify_preanalysis_veto(record, reason, "same_base_duplex_reason", {3})
        with self.assertRaisesRegex(ValueError, "no shared-bundle preanalysis record"):
            verify_preanalysis_veto(record.replace(reason, "different veto"), reason,
                                    "same_base_duplex_reason", {3})
        with self.assertRaisesRegex(ValueError, "no shared-bundle preanalysis record"):
            verify_preanalysis_veto(record.replace("bundle=shared", "bundle=other") + reason,
                                    reason, "same_base_duplex_reason", {3})
        with self.assertRaisesRegex(ValueError, "selected pointer owner"):
            verify_preanalysis_veto(record.replace("owner_argument_index=3", "owner_argument_index=4"),
                                    reason, "same_base_duplex_reason", {3})

    def test_single_direction_veto_uses_activation_reason_for_both_orders(self):
        for case in (5, 6, 7, 8):
            reason = VETO_REASON[case]
            self.assertEqual(VETO_REASON_FIELD[case], "activation_reason")
            for owner_order in ((3, 6), (6, 3)):
                traces = []
                for owner in owner_order:
                    candidate, semantic_reason = (("1", "same-base pointwise read-before-write candidate")
                                                  if owner == 3 else ("0", "read/write profile not requested"))
                    traces.append(
                        f'AXI_BURST_PREANALYSIS bundle=shared owner_argument_index={owner} '
                        'read_accesses=1 write_accesses=1 read_requested=1 read_recognized=1 '
                        'read_fallback="-" write_requested=1 write_recognized=1 write_fallback="-" '
                        f'same_base_duplex_candidate={candidate} '
                        f'same_base_duplex_reason="{semantic_reason}" '
                        f'production_activated=0 activation_reason="{reason}"')
                verify_preanalysis_veto("\n".join(traces), reason, VETO_REASON_FIELD[case], {3, 6})
                with self.assertRaisesRegex(ValueError, "no shared-bundle preanalysis record"):
                    verify_preanalysis_veto("\n".join(traces).replace(
                        f'activation_reason="{reason}"', 'activation_reason="unrelated"'),
                        reason, VETO_REASON_FIELD[case], {3, 6})

    def test_vetoed_multi_root_allows_legacy_rtl_but_rejects_duplex_aggregator(self):
        verify_no_duplex_backend_rtl(self.output)
        top = self.output / "top.v"
        top.write_text(
            "module datapath_matrix_candidate();\n"
            "  MinimalAXI4AdapterSingleBeat legacy_adapter();\n"
            "endmodule\n", encoding="utf-8")
        verify_no_duplex_backend_rtl(self.output)
        top.write_text(
            "module datapath_matrix_candidate();\n"
            "  MinimalAXI4AdapterSingleBeat legacy_adapter();\n"
            "  shared_duplex_aggregator duplex_adapter();\n"
            "endmodule\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "unexpectedly instantiates a duplex aggregator"):
            verify_no_duplex_backend_rtl(self.output)
        top.write_text("", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "nonempty regular file"):
            verify_no_duplex_backend_rtl(self.output)
        top.unlink()
        top.symlink_to(self.output / "missing.v")
        with self.assertRaisesRegex(ValueError, "nonempty regular file"):
            verify_no_duplex_backend_rtl(self.output)

    def test_dataflow_cases_require_exact_task_candidate_peer_analysis_order(self):
        for case, expected in DATAFLOW_ANALYSIS_ORDER.items():
            log = "".join(f"Analyzing function {root}\nAnalyzed function {root}\n"
                           for root in expected)
            verify_dataflow_analysis_order(log, case)
            with self.assertRaisesRegex(ValueError, "root set/order mismatch"):
                verify_dataflow_analysis_order(log.replace(expected[0], "unexpected_task", 1), case)
            with self.assertRaisesRegex(ValueError, "root set/order mismatch"):
                verify_dataflow_analysis_order("\n".join(log.splitlines()[:-2]), case)

    def test_dataflow_task_forwarded_pointer_and_memory_effects_are_witnessed(self):
        raw = '''COMPILER_VERSION: "fixture"
@1 function_val_node name: @2 arg: @3 body: @4
@2 identifier_node strg: "matrix_peer"
@3 argument_val_node type: @10
@4 statement_list_node bloc: 0 loop_id: 0
@5 function_val_node name: @6 arg: @7 body: @8
@6 identifier_node strg: "matrix_dataflow_task"
@7 argument_val_node type: @10
@8 statement_list_node bloc: 0 loop_id: 0
@9 call_stmt parent: @1 bb_index: 0 op: @11 arg: @12
@11 call_node fn: @13
@13 addr_node op: @5
@12 ssa_node var: @3 type: @10
@16 assign_stmt parent: @5 bb_index: 0 vuse: @20 op: @21
@21 mem_access_node ordinary_memory_access
@17 assign_stmt parent: @5 bb_index: 0 vdef: @22 op: @23
@23 mem_access_node ordinary_memory_access
@10 pointer_ty_node
'''
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            path = Path(directory) / "before.raw"
            path.write_text(raw, encoding="utf-8")
            verify_dataflow_task_call(path, "matrix_peer", "matrix_dataflow_task")
            verify_dataflow_task_memory(path, "matrix_dataflow_task")
            path.write_text(raw.replace("var: @3", "var: @7"), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "does not forward the peer pointer unchanged"):
                verify_dataflow_task_call(path, "matrix_peer", "matrix_dataflow_task")
            path.write_text(raw.replace("vdef: @22 ", ""), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "one ordinary LOAD and one STORE"):
                verify_dataflow_task_memory(path, "matrix_dataflow_task")

    def test_unused_peer_semantic_proof_does_not_promote_to_activation(self):
        reason = "original effects are not fully classified ordinary accesses"
        candidate = ('AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=11 '
                     'read_accesses=1 write_accesses=1 read_requested=1 read_recognized=0 '
                     'read_fallback="bundle contains stores" write_requested=1 write_recognized=0 '
                     'write_fallback="bundle contains reads; same-bundle read/write requires duplex support" '
                     'same_base_duplex_candidate=1 same_base_duplex_reason="same-base pointwise read-before-write candidate" '
                     f'production_activated=0 activation_reason="{reason}"\n')
        candidate_proof = {"semantic_proof": True,
                           "semantic_reason": "same-base pointwise read-before-write candidate"}
        unused_proof = {"semantic_proof": False, "semantic_reason": reason}
        log = (candidate + "  AXI_BURST_DUPLEX_PROOF " + json.dumps(candidate_proof) + "\n" +
               "  AXI_BURST_DUPLEX_PROOF " + json.dumps(unused_proof) + "\n")
        verify_unused_peer_veto(log, reason)
        with self.assertRaisesRegex(ValueError, "vetoed application-wide"):
            verify_unused_peer_veto(log.replace("production_activated=0", "production_activated=1"), reason)
        with self.assertRaisesRegex(ValueError, "lacks its unused contribution"):
            verify_unused_peer_veto(candidate + "AXI_BURST_DUPLEX_PROOF " +
                                    json.dumps(candidate_proof) + "\n", reason)
        self.assertEqual(activation_markers(
            "---Activated same-base pointwise duplex m_axi bundle shared for root matrix_candidate\n"
            "Activated same-base pointwise duplex m_axi bundle shared for root matrix_peer\n"),
            [("shared", "matrix_candidate"), ("shared", "matrix_peer")])
        with self.assertRaisesRegex(ValueError, "did not veto production activation"):
            verify_unused_peer_veto(log +
                                    "Activated same-base pointwise duplex m_axi bundle shared for root matrix_candidate\n",
                                    reason)

    def test_multi_root_contract_requires_five_calls_in_each_positive_root(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            raw = Path(directory) / "after.raw"
            raw.write_text('''@1 function_val_node name: @2 body: @3
@2 identifier_node strg: "matrix_candidate"
@3 statement_list_node bloc: 0 loop_id: 0
@4 function_val_node name: @5 body: @6
@5 identifier_node strg: "matrix_peer"
@6 statement_list_node bloc: 0 loop_id: 0
''')
            with self.assertRaisesRegex(ValueError, "exactly the five directional burst calls"):
                verify_call_contract(raw, EXPECTED_ROOTS[1], POSITIVE_ROOTS[1], "post-InterfaceInfer")

    def test_multi_root_calls_are_filtered_by_parent_function(self):
        lines = [
            '@1 function_val_node name: @2 body: @3',
            '@2 identifier_node strg: "matrix_candidate"',
            '@3 statement_list_node bloc: 0 loop_id: 0',
            '@4 function_val_node name: @5 body: @6',
            '@5 identifier_node strg: "matrix_peer"',
            '@6 statement_list_node bloc: 0 loop_id: 0',
        ]
        target_index = 10
        statement_index = 100
        for label, callee in enumerate((
                "shared_bambu_artificial_ParmMgr_configure_read",
                "shared_bambu_artificial_ParmMgr_burst_read",
                "shared_bambu_artificial_ParmMgr_configure_write",
                "shared_bambu_artificial_ParmMgr_store_write",
                "shared_bambu_artificial_ParmMgr_drain_write")):
            target = target_index + label * 5
            call = target + 1
            addr = target + 2
            ident = target + 3
            statement = statement_index + label
            lines.extend((
                f'@{target} function_val_node name: @{ident} body: @{target + 4}',
                f'@{ident} identifier_node strg: "{callee}"',
                f'@{target + 4} statement_list_node bloc: 0 loop_id: 0',
                f'@{call} call_node fn: @{addr}', f'@{addr} addr_node op: @{target}',
                f'@{statement} call_stmt parent: @4 op: @{call}',
            ))
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            raw = Path(directory) / "after.raw"
            raw.write_text("\n".join(lines) + "\n")
            self.assertEqual(root_calls(raw, "matrix_candidate"), [])
            self.assertEqual(len(root_calls(raw, "matrix_peer")), 5)
            with self.assertRaisesRegex(ValueError, "root matrix_candidate must have exactly"):
                verify_call_contract(raw, EXPECTED_ROOTS[1], POSITIVE_ROOTS[1], "post-InterfaceInfer")

    def test_burst_calls_accept_arbitrary_bundle_prefix_but_reject_wrong_operation(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            raw = Path(directory) / "after.raw"
            raw.write_text(self.burst_contract_raw(bundle_prefix="arbitrary_bundle"))
            verify_call_contract(raw, ("matrix_candidate",), {"matrix_candidate"}, "post-InterfaceInfer")
            raw.write_text(raw.read_text().replace(
                "arbitrary_bundle_bambu_artificial_ParmMgr_configure_read",
                "arbitrary_bundle_bambu_artificial_ParmMgr_configure_write"))
            with self.assertRaisesRegex(ValueError, "exactly the five directional burst calls"):
                verify_call_contract(raw, ("matrix_candidate",), {"matrix_candidate"}, "post-InterfaceInfer")

    def test_multi_root_raw_rejects_missing_snapshot_and_root_body(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            missing = Path(directory) / "missing.raw"
            with self.assertRaisesRegex(ValueError, "raw IR is missing or empty"):
                require_raw(missing, "pre-InterfaceInfer")
            raw = Path(directory) / "bad.raw"
            raw.write_text('''@1 function_val_node name: @2
@2 identifier_node strg: "matrix_candidate"
''')
            with self.assertRaisesRegex(ValueError, "no unique statement-list body"):
                root_calls(raw, "matrix_candidate")

    def test_multi_root_guard_rejects_crash_and_wrong_failure(self):
        with self.assertRaisesRegex(ValueError, "without crashing"):
            verify_backend_guard("Expected single top function name", -6)
        with self.assertRaisesRegex(ValueError, "exact BackendWrapper single-top assertion"):
            verify_backend_guard("some unrelated compiler error", 1)
        verify_backend_guard("Expected single top function name (top_symbols.size() == 1)", 1)

    def test_legacy_access_contract_requires_actual_directional_calls(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            path = Path(directory) / "legacy.raw"
            path.write_text('''@1 function_val_node name: @2 body: @3
@2 identifier_node strg: "matrix_candidate"
@3 statement_list_node bloc: 0 loop_id: 0
@4 function_val_node name: @5 body: @6
@5 identifier_node strg: "shared_bambu_artificial_ParmMgr_Read"
@6 statement_list_node bloc: 0 loop_id: 0
@7 call_node fn: @8
@8 addr_node op: @4
@9 call_stmt parent: @1 op: @7
''')
            from check_duplex_activation_matrix import verify_legacy_access_contract
            from check_duplex_activation_matrix import verify_no_legacy_access_contract
            verify_legacy_access_contract(path, {"matrix_candidate": {"read"}})
            with self.assertRaisesRegex(ValueError, "lacks legacy direction witness"):
                verify_legacy_access_contract(path, {"matrix_candidate": {"read", "write"}})
            with self.assertRaisesRegex(ValueError, "unused root matrix_candidate unexpectedly"):
                verify_no_legacy_access_contract(path, {"matrix_candidate"})

    def test_drain_must_match_proof_recorded_exit_block(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            raw = Path(directory) / "after.raw"
            raw.write_text(self.burst_contract_raw())
            proof = 'AXI_BURST_DUPLEX_PROOF {"owner_argument_index":3,"write_exit_bb":2}\n'
            verify_drain_exit(raw, proof, "matrix_candidate")
            verify_burst_call_arguments(raw, "matrix_candidate")
            with self.assertRaisesRegex(ValueError, "DRAIN is not in the proof-recorded"):
                verify_drain_exit(raw, proof.replace('"write_exit_bb":2', '"write_exit_bb":0'),
                                  "matrix_candidate")
            raw.write_text(self.burst_contract_raw(write_count_bound_ref=802))
            with self.assertRaisesRegex(ValueError, "read/write CONFIGURE count expressions differ"):
                verify_burst_call_arguments(raw, "matrix_candidate")

    @staticmethod
    def burst_contract_raw(write_count_ref: int = 604, bundle_prefix: str = "shared",
                           write_count_bound_ref: int = 801) -> str:
        names = (
            f"{bundle_prefix}_bambu_artificial_ParmMgr_configure_read",
            f"{bundle_prefix}_bambu_artificial_ParmMgr_burst_read",
            f"{bundle_prefix}_bambu_artificial_ParmMgr_configure_write",
            f"{bundle_prefix}_bambu_artificial_ParmMgr_store_write",
            f"{bundle_prefix}_bambu_artificial_ParmMgr_drain_write",
        )
        args = (
            (502, 503, 504, 600, 601), (500, 503, 504, 602, 504),
            (502, 503, 504, 600, write_count_ref), (501, 503, 603, 602, 504),
            (505, 503, 504, 600, 504),
        )
        blocks = (0, 1, 0, 1, 2)
        lines = [
            '@1 function_val_node name: @2 arg: @3 body: @4',
            '@2 identifier_node strg: "matrix_candidate"',
            '@3 argument_val_node parent: @1',
            '@4 statement_list_node bloc: 0 loop_id: 0 pred: ENTRY succ: 1 stmt: @200 stmt: @202 '
            'bloc: 1 loop_id: 4 pred: 0 succ: 2 stmt: @201 stmt: @203 '
            'bloc: 2 loop_id: 0 pred: 1 succ: EXIT stmt: @204',
        ]
        for ref, value in ((500, 0), (501, 1), (502, 2), (503, 32), (504, 0), (505, 3)):
            lines.append(f'@{ref} constant_int_val_node type: @503 value: {value}')
        for ref in (600, 602, 603, 800, 801, 802):
            lines.append(f'@{ref} ssa_node type: @503')
        lines.extend((
            '@700 select_node type: @503 op: @800 op: @801 op: @504',
            '@601 ssa_node type: @503 def_stmt: @702',
            '@702 assign_stmt parent: @1 bb_index: 0 op: @601 op: @700',
            f'@710 select_node type: @503 op: @800 op: @{write_count_bound_ref} op: @504',
            '@604 ssa_node type: @503 def_stmt: @712',
            '@712 assign_stmt parent: @1 bb_index: 0 op: @604 op: @710',
        ))
        for index, (name, call_args, block) in enumerate(zip(names, args, blocks)):
            target = 10 + index * 10
            stmt = 200 + index
            lines.extend((
                f'@{target} function_val_node name: @{target + 1} body: @{target + 2}',
                f'@{target + 1} identifier_node strg: "{name}"',
                f'@{target + 2} statement_list_node bloc: 0 loop_id: 0',
                f'@{target + 3} call_node fn: @{target + 4}',
                f'@{target + 4} addr_node op: @{target}',
                f'@{stmt} call_stmt parent: @1 bb_index: {block} op: @{target + 3} '
                + " ".join(f"arg: @{arg}" for arg in call_args),
            ))
        return "\n".join(lines) + "\n"

    def test_backend_aggregator_must_connect_all_physical_pins(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            rtl = Path(directory) / "top.v"
            rtl.write_text(self.activated_rtl("datapath_matrix_candidate", aggregator_pins=True,
                                              external_top=False))
            verify_backend_rtl(rtl, ("matrix_candidate",), {"matrix_candidate"})
            rtl.write_text(self.activated_rtl("datapath_matrix_candidate", aggregator_pins=True,
                                              external_top=False).replace(
                ".p_m_axi_shared_buser(p_m_axi_shared_buser)",
                ".p_m_axi_shared_buser(1'b0)"))
            with self.assertRaisesRegex(ValueError, "does not connect physical pin buser"):
                verify_backend_rtl(rtl, ("matrix_candidate",), {"matrix_candidate"})

    def test_backend_accepts_numeric_endpoint_fu_instance_types(self):
        rtl = self.activated_rtl("datapath_matrix_candidate", aggregator_pins=True,
                                 external_top=False).replace(
            "shared_duplex_read_endpoint_modgen read_endpoint",
            "shared_duplex_read_endpoint_15_i0 read_endpoint").replace(
            "shared_duplex_write_endpoint_modgen write_endpoint",
            "shared_duplex_write_endpoint_16_i0 write_endpoint")
        operation_names = {
            "fuselector_shared_duplex_read_endpoint_configure_read":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_configure_read",
            "fuselector_shared_duplex_read_endpoint_burst_read":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_burst_read",
            "fuselector_shared_duplex_write_endpoint_configure_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_configure_write",
            "fuselector_shared_duplex_write_endpoint_store_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_store_write",
            "fuselector_shared_duplex_write_endpoint_drain_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_drain_write",
        }
        for previous, actual in operation_names.items():
            rtl = rtl.replace(previous, actual)
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            path = Path(directory) / "top.v"
            path.write_text(rtl)
            verify_backend_rtl(path, ("matrix_candidate",), {"matrix_candidate"})
            wrong_direction = rtl.replace(
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_configure_read",
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_configure_read")
            path.write_text(wrong_direction)
            with self.assertRaisesRegex(ValueError,
                                        "lacks duplex read_endpoint operation mapping configure_read"):
                verify_backend_rtl(path, ("matrix_candidate",), {"matrix_candidate"})

    def test_production_checker_accepts_real_fu_operation_name_shape(self) -> None:
        rtl = self.activated_rtl()
        replacements = {
            "fuselector_shared_duplex_read_endpoint_configure_read":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_configure_read",
            "fuselector_shared_duplex_read_endpoint_burst_read":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0_"
                "shared_bambu_artificial_ParmMgr_burst_read",
            "fuselector_shared_duplex_write_endpoint_configure_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_configure_write",
            "fuselector_shared_duplex_write_endpoint_store_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_store_write",
            "fuselector_shared_duplex_write_endpoint_drain_write":
                "fuselector_shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0_"
                "shared_bambu_artificial_ParmMgr_drain_write",
        }
        for previous, actual in replacements.items():
            rtl = rtl.replace(previous, actual)
        self.write_artifact("same_base_pointwise.v", rtl)
        self.check()

    def test_external_top_pins_use_m_axi_names_and_map_through_pbi(self) -> None:
        self.configure_activated_case("same_base_pointwise")
        self.check()

    def test_top_rejects_missing_or_wrong_pbi_pin_mapping(self) -> None:
        rtl = self.activated_rtl().replace(
            ".p_m_axi_shared_awaddr(m_axi_shared_awaddr),\n", "")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "map every physical pin through exactly one PBI"):
            self.check()
        rtl = self.activated_rtl().replace(
            ".p_m_axi_shared_awaddr(m_axi_shared_awaddr)",
            ".p_m_axi_shared_awaddr(m_axi_shared_awlen)")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "PBI mapping for p_m_axi_shared_awaddr"):
            self.check()

    def test_incompatible_profile_fixture_requires_distinct_root_profiles(self):
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            architecture = Path(directory) / "architecture.xml"
            architecture.write_text('''<architecture>
<function symbol="matrix_candidate"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16" /></bundles></function>
<function symbol="matrix_peer"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16" /></bundles></function>
</architecture>
''')
            with self.assertRaisesRegex(ValueError, "wrong read profile"):
                verify_architecture(architecture, EXPECTED_ROOTS[13], 13)

    def configure_incompatible_matrix_failure(self, diagnostic: str = INCOMPATIBLE_PROFILE_DIAGNOSTIC) -> None:
        (self.output / "same_base_pointwise.v").unlink(missing_ok=True)
        (self.output / "stdout.log").write_text("Analyzing function matrix_peer\n", encoding="utf-8")
        (self.output / "stderr.log").write_text(diagnostic + "\n", encoding="utf-8")
        (self.output / "panda-temp/before_Frontend::InterfaceInfer.raw").write_text('''COMPILER_VERSION: "fixture"
@1 function_val_node name: @2 body: @3
@2 identifier_node strg: "matrix_candidate"
@3 statement_list_node bloc: 0 loop_id: 0
@4 function_val_node name: @5 body: @6
@5 identifier_node strg: "matrix_peer"
@6 statement_list_node bloc: 0 loop_id: 0
''', encoding="utf-8")
        (self.output / "panda-temp/architecture.xml").write_text('''<architecture>
<function symbol="matrix_candidate"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16" latency="0" num_read_outstanding="1" read_fifo_depth="32" write_burst_outstanding="1" /></bundles></function>
<function symbol="matrix_peer"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="8" max_write_burst_length="16" latency="0" num_read_outstanding="1" read_fifo_depth="32" write_burst_outstanding="1" /></bundles></function>
</architecture>
''', encoding="utf-8")

    def test_incompatible_profile_expected_failure_needs_no_complete_root_order(self) -> None:
        self.configure_incompatible_matrix_failure()
        verify_activation_matrix(13, self.output, 1)

    def test_incompatible_profile_rejects_wrong_diagnostic(self) -> None:
        self.configure_incompatible_matrix_failure("Incompatible profile")
        with self.assertRaisesRegex(ValueError, "exact incompatible-profile diagnostic"):
            verify_activation_matrix(13, self.output, 1)

    def test_incompatible_profile_rejects_crash_exit(self) -> None:
        self.configure_incompatible_matrix_failure()
        with self.assertRaisesRegex(ValueError, "not crash"):
            verify_activation_matrix(13, self.output, 139)

    def test_incompatible_profile_requires_nonempty_pre_interface_raw(self) -> None:
        self.configure_incompatible_matrix_failure()
        (self.output / "panda-temp/before_Frontend::InterfaceInfer.raw").unlink()
        with self.assertRaisesRegex(ValueError, "pre-InterfaceInfer raw IR is missing or empty"):
            verify_activation_matrix(13, self.output, 1)

    def test_incompatible_profile_requires_architecture_xml(self) -> None:
        self.configure_incompatible_matrix_failure()
        (self.output / "panda-temp/architecture.xml").unlink()
        with self.assertRaisesRegex(ValueError, "architecture XML is missing or empty"):
            verify_activation_matrix(13, self.output, 1)

    def test_incompatible_profile_rejects_collapsed_root_profiles(self) -> None:
        self.configure_incompatible_matrix_failure()
        architecture = self.output / "panda-temp/architecture.xml"
        architecture.write_text(architecture.read_text().replace(
            'max_read_burst_length="8"', 'max_read_burst_length="16"'), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "profiles are not distinct"):
            verify_activation_matrix(13, self.output, 1)

    def configure_called_helper_matrix_case(self) -> None:
        raw = '''COMPILER_VERSION: "fixture"
@1 function_val_node name: @2 arg: @3 arg: @4 body: @5
@2 identifier_node strg: "matrix_helper"
@3 argument_val_node type: @20 parent: @1
@4 argument_val_node type: @21 parent: @1
@5 statement_list_node bloc: 0 loop_id: 0
@6 function_val_node name: @7 arg: @8 arg: @9 body: @10
@7 identifier_node strg: "matrix_candidate"
@8 argument_val_node type: @20 parent: @6
@9 argument_val_node type: @21 parent: @6
@10 statement_list_node bloc: 0 loop_id: 0
@11 function_val_node name: @12 arg: @13 arg: @14 body: @15
@12 identifier_node strg: "matrix_peer"
@13 argument_val_node type: @20 parent: @11
@14 argument_val_node type: @21 parent: @11
@15 statement_list_node bloc: 0 loop_id: 0
@16 call_stmt parent: @6 bb_index: 0 op: @17 arg: @18 arg: @19
@17 call_node fn: @22
@22 addr_node op: @1
@18 ssa_node var: @8 type: @20
@19 ssa_node var: @9 type: @21
@23 call_stmt parent: @1 bb_index: 0 op: @24
@24 call_node fn: @25
@25 addr_node op: @26
@26 function_val_node name: @27
@27 identifier_node strg: "shared_bambu_artificial_ParmMgr_Read"
@28 call_stmt parent: @1 bb_index: 0 op: @29
@29 call_node fn: @30
@30 addr_node op: @31
@31 function_val_node name: @32
@32 identifier_node strg: "shared_bambu_artificial_ParmMgr_Write"
@33 call_stmt parent: @11 bb_index: 0 op: @34
@34 call_node fn: @35
@35 addr_node op: @36
@36 function_val_node name: @37
@37 identifier_node strg: "shared_bambu_artificial_ParmMgr_Read"
@38 call_stmt parent: @11 bb_index: 0 op: @39
@39 call_node fn: @40
@40 addr_node op: @41
@41 function_val_node name: @42
@42 identifier_node strg: "shared_bambu_artificial_ParmMgr_Write"
@20 pointer_ty_node
@21 integer_ty_node bitsizealloc: 32 bitsize: 32
'''
        (self.output / "panda-temp/before_Frontend::InterfaceInfer.raw").write_text(raw, encoding="utf-8")
        (self.output / "panda-temp/after_Frontend::InterfaceInfer.raw").write_text(raw, encoding="utf-8")
        (self.output / "panda-temp/architecture.xml").write_text('''<module>
<function symbol="matrix_candidate"><bundles><bundle name="n" mode="default"/><bundle name="shared" mode="ptrdefault"/></bundles></function>
<function symbol="matrix_helper"><bundles><bundle name="n" mode="default"/><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16" latency="0" num_read_outstanding="1" read_fifo_depth="32" write_burst_outstanding="1"/></bundles></function>
<function symbol="matrix_peer"><bundles><bundle name="n" mode="default"/><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16" latency="0" num_read_outstanding="1" read_fifo_depth="32" write_burst_outstanding="1"/></bundles></function>
</module>
''', encoding="utf-8")
        reason = "bundle contribution is not an independent entry root"
        def witness(owner_argument_index: int) -> tuple[str, str]:
            trace = (f'AXI_BURST_PREANALYSIS bundle=shared owner_argument_index={owner_argument_index} '
                     'read_accesses=1 write_accesses=1 read_requested=1 read_recognized=0 '
                     'read_fallback="bundle contains stores" write_requested=1 write_recognized=0 '
                     'write_fallback="bundle contains reads; same-bundle read/write requires duplex support" '
                     'same_base_duplex_candidate=1 '
                     'same_base_duplex_reason="same-base pointwise read-before-write candidate" '
                     f'production_activated=0 activation_reason="{reason}"\n')
            proof = {"bundle": "shared", "semantic_proof": True,
                     "semantic_reason": "same-base pointwise read-before-write candidate",
                     "profile_eligible": True, "profile_reason": "read/write profile is eligible",
                     "owner_argument_index": owner_argument_index}
            return trace, "AXI_BURST_DUPLEX_PROOF " + json.dumps(proof)
        helper_trace, helper_proof = witness(3)
        peer_trace, peer_proof = witness(13)
        (self.output / "stdout.log").write_text(
            "Analyzing function matrix_helper\nAnalyzed function matrix_helper\n"
            "Analyzing function matrix_candidate\nAnalyzed function matrix_candidate\n"
            "Analyzing function matrix_peer\nAnalyzed function matrix_peer\n", encoding="utf-8")
        (self.output / "stderr.log").write_text(
            helper_trace + helper_proof + "\n" + peer_trace + peer_proof +
            "\nerror -> Expected single top function name (top_symbols.size() == 1)\n", encoding="utf-8")

    def test_case16_real_called_contribution_and_independent_peer_witness(self) -> None:
        self.configure_called_helper_matrix_case()
        verify_activation_matrix(16, self.output, 1)
        before = self.output / "panda-temp/before_Frontend::InterfaceInfer.raw"
        before.write_text(before.read_text().replace("var: @8", "var: @13"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "does not forward the pointer unchanged"):
            verify_activation_matrix(16, self.output, 1)

    def test_case16_requires_exact_called_contribution_veto_reason(self) -> None:
        self.configure_called_helper_matrix_case()
        stderr = self.output / "stderr.log"
        stderr.write_text(stderr.read_text().replace(
            "bundle contribution is not an independent entry root", "not eligible"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "matrix_helper contribution lacks the exact"):
            verify_activation_matrix(16, self.output, 1)

    def test_case16_requires_exact_independent_peer_veto_reason(self) -> None:
        self.configure_called_helper_matrix_case()
        stderr = self.output / "stderr.log"
        lines = stderr.read_text().splitlines()
        changed = 0
        for index, line in enumerate(lines):
            if "AXI_BURST_PREANALYSIS " in line and "owner_argument_index=13 " in line:
                lines[index] = line.replace(
                    'activation_reason="bundle contribution is not an independent entry root"',
                    'activation_reason="not eligible"')
                changed += 1
        self.assertEqual(changed, 1)
        stderr.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "matrix_peer contribution lacks the exact"):
            verify_activation_matrix(16, self.output, 1)

    def test_case16_requires_peer_positive_proof(self) -> None:
        self.configure_called_helper_matrix_case()
        stderr = self.output / "stderr.log"
        lines = stderr.read_text().splitlines()
        lines = [line for line in lines if not (
            line.startswith("AXI_BURST_DUPLEX_PROOF ") and
            json.loads(line[len("AXI_BURST_DUPLEX_PROOF "):]).get("owner_argument_index") == 13)]
        stderr.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "matrix_peer lacks its positive semantic"):
            verify_activation_matrix(16, self.output, 1)

    def test_case17_unrelated_root_may_keep_non_m_axi_bundle_metadata(self) -> None:
        architecture = self.output / "architecture.xml"
        architecture.write_text('''<module>
<function symbol="matrix_candidate"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="16" max_write_burst_length="16"/></bundles></function>
<function symbol="unrelated_architectureless"><bundles><bundle name="data" mode="ptrdefault"/><bundle name="n" mode="default"/></bundles></function>
</module>
''', encoding="utf-8")
        baseline = architecture.read_text()
        verify_architecture(architecture, EXPECTED_ROOTS[17], 17)
        bad = baseline.replace('name="data" mode="ptrdefault"', 'name="data" mode="m_axi"')
        architecture.write_text(bad, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "data=ptrdefault and n=default"):
            verify_architecture(architecture, EXPECTED_ROOTS[17], 17)
        architecture.write_text(baseline.replace('name="n" mode="default"',
                                                   'name="n" mode="default" max_read_burst_length="8"'),
                                 encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "acquired an m_axi bundle or read/write burst profile"):
            verify_architecture(architecture, EXPECTED_ROOTS[17], 17)

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
        self.configure_activated_case("same_base_pointwise")

    def tearDown(self) -> None:
        self.temp.cleanup()

    def write_artifact(self, relative: str, text: str) -> None:
        (self.output / relative).write_text(text, encoding="utf-8")

    @staticmethod
    def architecture(top: str, read_burst: int = 16, read_outstanding: int = 1,
                     write_burst: int = 16, write_outstanding: int = 1) -> str:
        restrict_metadata = {
            "distinct_bases": {"src": "0", "dst": "0"},
            "distinct_bases_source_first": {"src": "1", "dst": "1"},
            "distinct_bases_destination_first": {"dst": "1", "src": "1"},
            "distinct_bases_one_restrict": {"src": "1", "dst": "0"},
        }.get(top, {})
        parameters = "".join(
            f'<parameter port="{port}" index="{index}" source_restrict="{restricted}" />'
            for index, (port, restricted) in enumerate(restrict_metadata.items()))
        return f'''<?xml version="1.0"?>
<root><functions><function name="{top}"><parameters>{parameters}</parameters><bundles>
<bundle name="shared" mode="m_axi" max_read_burst_length="{read_burst}"
 num_read_outstanding="{read_outstanding}" read_fifo_depth="32" write_fifo_depth="16"
 max_write_burst_length="{write_burst}"
 write_burst_outstanding="{write_outstanding}" />
</bundles></function></functions></root>
'''

    @staticmethod
    def activated_rtl(top: str = "same_base_pointwise", address_bits: int = 32, read_burst: int = 16,
                      read_outstanding: int = 1, write_burst: int = 16, write_outstanding: int = 1,
                      aggregator_pins: bool = False, external_top: bool = True) -> str:
        inputs = {"awready", "wready", "bid", "bresp", "buser", "bvalid", "arready", "rid", "rdata",
                  "rresp", "rlast", "ruser", "rvalid"}
        suffixes = sorted({"awready", "wready", "bid", "bresp", "buser", "bvalid", "arready", "rid", "rdata",
                           "rresp", "rlast", "ruser", "rvalid", "awid", "awaddr", "awlen", "awsize",
                           "awburst", "awlock", "awcache", "awprot", "awqos", "awregion", "awuser",
                           "awvalid", "wdata", "wstrb", "wlast", "wuser", "wvalid", "bready", "arid",
                           "araddr", "arlen", "arsize", "arburst", "arlock", "arcache", "arprot", "arqos",
                           "arregion", "aruser", "arvalid", "rready"})
        top_ports = ",\n".join(f"  m_axi_shared_{suffix}" for suffix in suffixes)
        ports = ",\n".join(f"  p_m_axi_shared_{suffix}" for suffix in suffixes)
        declarations = "\n".join(
            f"  {'input' if suffix in inputs else 'output'} "
            f"{'[' + str(address_bits - 1) + ':0]' if suffix in {'araddr', 'awaddr'} else '[0:0]'} "
            f"p_m_axi_shared_{suffix};" for suffix in suffixes)
        top_declarations = "\n".join(
            f"  {'input' if suffix in inputs else 'output'} "
            f"{'[' + str(address_bits - 1) + ':0]' if suffix in {'araddr', 'awaddr'} else '[0:0]'} "
            f"m_axi_shared_{suffix};" for suffix in suffixes)
        top_connections = ",\n".join(
            f"    .p_m_axi_shared_{suffix}(m_axi_shared_{suffix})" for suffix in suffixes)
        physical_connections = ",\n".join(
            f"    .p_m_axi_shared_{suffix}(p_m_axi_shared_{suffix})" for suffix in suffixes)
        private_fields = ("start", "opcode", "size", "data", "address", "count", "done", "result")
        private_widths = {"start": 1, "opcode": 2, "size": 32, "data": 32,
                          "address": address_bits, "count": 32, "done": 1, "result": 32}
        parent_wires = "\n".join(
            f"  wire{' [' + str(private_widths[field] - 1) + ':0]' if private_widths[field] > 1 else ''} "
            f"{direction}_{field}_wire;"
            for direction in ("read", "write") for field in private_fields)
        private_connections = ",\n".join(
            f"    .{direction}_{field}({direction}_{field}_wire)"
            for direction in ("read", "write") for field in private_fields)
        read_endpoint_connections = ", ".join(
            f".duplex_{field}(read_{field}_wire)" for field in private_fields)
        write_endpoint_connections = ", ".join(
            f".duplex_{field}(write_{field}_wire)" for field in private_fields)
        aggregator_cores = (
            f"  MinimalAXI4MasterPipelined #(.B_MAX({read_burst}), .MAX_OUTSTANDING({read_outstanding}), "
            f".FIFO_DEPTH(32), .PEER_FAULT_ENABLE(1), .BITSIZE_in4({address_bits}), "
            f".BITSIZE_m_axi_araddr({address_bits})) read_core ("
            ".peer_fault(read_peer_fault), .fault(read_core_fault));\n"
            f"  MinimalAXI4MasterPipelinedWrite #(.B_MAX({write_burst}), "
            f".MAX_OUTSTANDING({write_outstanding}), .FIFO_DEPTH(16), .PEER_FAULT_ENABLE(1), "
            f".BITSIZE_in4({address_bits}), .BITSIZE_m_axi_awaddr({address_bits})) write_core ("
            ".peer_fault(write_peer_fault), .fault(write_core_fault));\n"
            "  wire read_peer_fault = write_core_fault;\n"
            "  wire write_peer_fault = read_core_fault;\n")
        endpoint_ports = "\n".join(
            f"  {'output' if field in {'start', 'opcode', 'size', 'data', 'address', 'count'} else 'input'} "
            f"{'[' + str(private_widths[field] - 1) + ':0]' if private_widths[field] > 1 else ''} "
            f"duplex_{field};" for field in private_fields if field not in {"done", "result"})
        endpoint_assignments = "\n".join((
            "  assign duplex_start = start_port[0];",
            "  assign duplex_opcode = in1;",
            "  assign duplex_size = in2;",
            "  assign duplex_data = in3;",
            "  assign duplex_address = in4;",
            "  assign duplex_count = in5;",
            "  assign done_port = duplex_done;",
            "  assign out1 = duplex_result;",
        ))
        endpoint_adapter_ports = ("start_port", "in1", "in2", "in3", "in4", "in5",
                                  "duplex_done", "duplex_result", "done_port", "out1")
        endpoint_port_list = ",\n".join(f"  {port}" for port in (*endpoint_adapter_ports,
                                                        *(f"duplex_{field}" for field in private_fields
                                                          if field not in {"done", "result"})))
        endpoint_adapter_declarations = (
            "  input [0:0] start_port;\n  input [1:0] in1;\n"
            "  input [31:0] in2;\n  input [31:0] in3;\n"
            f"  input [{address_bits - 1}:0] in4;\n  input [31:0] in5;\n"
            "  input duplex_done;\n  input [31:0] duplex_result;\n"
            "  output done_port;\n  output [31:0] out1;")
        aggregator_private_ports = "\n".join(
            f"  {'input' if field in {'start', 'opcode', 'size', 'data', 'address', 'count'} else 'output'} "
            f"{'[' + str(private_widths[field] - 1) + ':0]' if private_widths[field] > 1 else ''} "
            f"{direction}_{field};"
            for direction in ("read", "write") for field in private_fields)
        aggregator_interface = f"({ports})" if aggregator_pins else "()"
        aggregator_pin_declarations = declarations if aggregator_pins else ""
        if external_top:
            return (f"module {top}(\n{top_ports});\n{top_declarations}\n"
                f"  PBI_{top} PBI_{top}_i0 (\n{top_connections});\nendmodule\n"
                f"module PBI_{top}(\n{ports});\n{declarations}\n{parent_wires}\n"
                "  wire fuselector_shared_duplex_read_endpoint_configure_read;\n"
                "  wire fuselector_shared_duplex_read_endpoint_burst_read;\n"
                "  wire fuselector_shared_duplex_write_endpoint_configure_write;\n"
                "  wire fuselector_shared_duplex_write_endpoint_store_write;\n"
                "  wire fuselector_shared_duplex_write_endpoint_drain_write;\n"
                f"  shared_duplex_read_endpoint_modgen read_endpoint ({read_endpoint_connections});\n"
                f"  shared_duplex_write_endpoint_modgen write_endpoint ({write_endpoint_connections});\n"
                f"  shared_duplex_aggregator_modgen aggregator (\n{physical_connections},\n"
                f"{private_connections});\nendmodule\n"
                f"module shared_duplex_read_endpoint_modgen(\n{endpoint_port_list});\n"
                f"{endpoint_adapter_declarations}\n{endpoint_ports}\n{endpoint_assignments}\nendmodule\n"
                f"module shared_duplex_write_endpoint_modgen(\n{endpoint_port_list});\n"
                f"{endpoint_adapter_declarations}\n{endpoint_ports}\n{endpoint_assignments}\nendmodule\n"
                f"module shared_duplex_aggregator_modgen{aggregator_interface};\n"
                f"{aggregator_pin_declarations}\n{aggregator_private_ports}\n"
                f"{aggregator_cores}endmodule\n")
        return (f"module {top}(\n{ports});\n{declarations}\n{parent_wires}\n"
                "  wire fuselector_shared_duplex_read_endpoint_configure_read;\n"
                "  wire fuselector_shared_duplex_read_endpoint_burst_read;\n"
                "  wire fuselector_shared_duplex_write_endpoint_configure_write;\n"
                "  wire fuselector_shared_duplex_write_endpoint_store_write;\n"
                "  wire fuselector_shared_duplex_write_endpoint_drain_write;\n"
                f"  shared_duplex_read_endpoint_modgen read_endpoint ({read_endpoint_connections});\n"
                f"  shared_duplex_write_endpoint_modgen write_endpoint ({write_endpoint_connections});\n"
                f"  shared_duplex_aggregator_modgen aggregator (\n{physical_connections},\n"
                f"{private_connections});\nendmodule\n"
                f"module shared_duplex_read_endpoint_modgen(\n{endpoint_port_list});\n"
                f"{endpoint_adapter_declarations}\n{endpoint_ports}\n{endpoint_assignments}\nendmodule\n"
                f"module shared_duplex_write_endpoint_modgen(\n{endpoint_port_list});\n"
                f"{endpoint_adapter_declarations}\n{endpoint_ports}\n{endpoint_assignments}\nendmodule\n"
                f"module shared_duplex_aggregator_modgen{aggregator_interface};\n"
                f"{aggregator_pin_declarations}\n{aggregator_private_ports}\n"
                f"{aggregator_cores}endmodule\n")

    def configure_activated_case(self, top: str, address_bits: int = 32, read_burst: int = 16,
                                 read_outstanding: int = 1, write_burst: int = 16,
                                 write_outstanding: int = 1) -> None:
        self.write_artifact(f"{top}.v", self.activated_rtl(top, address_bits, read_burst, read_outstanding,
                                                             write_burst, write_outstanding))
        self.write_artifact("panda-temp/architecture.xml",
                            self.architecture(top, read_burst, read_outstanding, write_burst, write_outstanding))
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw",
                            RAW.replace('strg: "same_base_pointwise"', f'strg: "{top}"'))
        self.write_artifact("panda-temp/after_Frontend::InterfaceInfer.raw",
                            self.post_interface_count_raw(top))
        self.write_artifact("stderr.log", f"---Activated same-base pointwise duplex m_axi bundle shared for root {top}\n" +
                            TRACE + "AXI_BURST_DUPLEX_PROOF " + json.dumps(RECORD, separators=(",", ":")) + "\n")
        self.write_artifact("provenance.txt", self.provenance().replace(
            "top=same_base_pointwise", f"top={top}", 1))

    @staticmethod
    def stderr(record: dict[str, object] | None = None, trace: str = TRACE) -> str:
        value = dict(RECORD if record is None else record)
        activation = ("---Activated same-base pointwise duplex m_axi bundle shared for root "
                      "same_base_pointwise\n" if "production_activated=1" in trace else "")
        return activation + trace + "AXI_BURST_DUPLEX_PROOF " + json.dumps(value, separators=(",", ":")) + "\n"

    def provenance(self) -> str:
        paths = {"source": self.source, "bambu": self.bambu, "plugin": self.plugin,
                 "checker": self.checker, "runner": self.runner}
        rows = ["top=same_base_pointwise"]
        for name, path in paths.items():
            rows.extend((f"{name}={path}", f"{name}_sha256={hashlib.sha256(path.read_bytes()).hexdigest()}"))
        raw = self.output / "panda-temp/before_Frontend::InterfaceInfer.raw"
        rows.extend((f"raw_ir={raw}", f"raw_ir_sha256={hashlib.sha256(raw.read_bytes()).hexdigest()}"))
        post_raw = self.output / "panda-temp/after_Frontend::InterfaceInfer.raw"
        if post_raw.is_file():
            rows.extend((f"post_interface_raw={post_raw}",
                         f"post_interface_raw_sha256={hashlib.sha256(post_raw.read_bytes()).hexdigest()}"))
        return "\n".join(rows) + "\n"

    def refresh_provenance(self) -> None:
        self.write_artifact("provenance.txt", self.provenance())

    def check(self, top: str = "same_base_pointwise") -> None:
        verify(self.output, top, self.source, self.bambu, self.plugin, self.checker, self.runner)

    def configure_fallback_case(self, top: str, reason: str, reads: int, writes: int,
                                read_fallback: str, write_fallback: str) -> None:
        trace = (f'AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=3 read_accesses={reads} '
                 f'write_accesses={writes} read_requested=1 read_recognized=0 '
                 f'read_fallback="{read_fallback}" write_requested=1 write_recognized=0 '
                 f'write_fallback="{write_fallback}" same_base_duplex_candidate=0 '
                 f'same_base_duplex_reason="{reason}" production_activated=0 '
                 f'activation_reason="bundle is not in the frozen activation plan"\n')
        record = {"bundle": "shared", "semantic_proof": False, "semantic_reason": reason,
                  "profile_eligible": True, "profile_reason": "read/write profile is eligible"}
        self.write_artifact(f"{top}.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture(top))
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw",
                            RAW.replace('strg: "same_base_pointwise"', f'strg: "{top}"'))
        self.write_artifact("stderr.log", self.stderr(record, trace))
        self.write_artifact("provenance.txt", self.provenance().replace(
            "top=same_base_pointwise", f"top={top}", 1))

    @staticmethod
    def post_interface_count_raw(top: str) -> str:
        names = (
            "shared_bambu_artificial_ParmMgr_configure_read",
            "shared_bambu_artificial_ParmMgr_burst_read",
            "shared_bambu_artificial_ParmMgr_configure_write",
            "shared_bambu_artificial_ParmMgr_store_write",
            "shared_bambu_artificial_ParmMgr_drain_write",
        )
        args = ((500, 501, 502, 503, 601), (504, 501, 502, 505, 502),
                (500, 501, 502, 503, 604), (506, 501, 507, 505, 502),
                (508, 501, 502, 503, 502))
        blocks = (0, 1, 0, 1, 2)
        lines = [
            '@1 function_val_node name: @2 body: @70',
            f'@2 identifier_node strg: "{top}"',
            '@70 statement_list_node bloc: 0 loop_id: 0 pred: ENTRY succ: 1 stmt: @200 stmt: @202 '
            'bloc: 1 loop_id: 1 pred: 0 succ: 2 stmt: @201 stmt: @203 '
            'bloc: 2 loop_id: 0 pred: 1 succ: EXIT stmt: @204',
            '@9 integer_ty_node bitsizealloc: 32 bitsize: 32 unsigned',
            '@10 integer_ty_node bitsizealloc: 1 bitsize: 1 unsigned',
            '@63 ssa_node type: @9', '@11 ssa_node type: @10',
            '@80 constant_int_val_node type: @9 value: 0',
            '@600 select_node type: @9 op: @11 op: @63 op: @80',
            '@601 ssa_node type: @9 def_stmt: @610',
            '@610 assign_stmt parent: @1 bb_index: 0 op: @601 op: @600',
            '@603 select_node type: @9 op: @11 op: @63 op: @80',
            '@604 ssa_node type: @9 def_stmt: @611',
            '@611 assign_stmt parent: @1 bb_index: 0 op: @604 op: @603',
        ]
        for index, (name, call_args, block) in enumerate(zip(names, args, blocks)):
            target = 1000 + index * 10
            statement = 1200 + index
            lines.extend((f'@{target} function_val_node name: @{target + 1} body: @{target + 2}',
                          f'@{target + 1} identifier_node strg: "{name}"',
                          f'@{target + 2} statement_list_node bloc: 0 loop_id: 0',
                          f'@{target + 3} call_node fn: @{target + 4}',
                          f'@{target + 4} addr_node op: @{target}',
                          f'@{statement} call_stmt parent: @1 bb_index: {block} op: @{target + 3} ' +
                          " ".join(f"arg: @{arg}" for arg in call_args)))
        for ref, value in ((500, 2), (501, 32), (502, 0), (504, 0),
                           (506, 1), (508, 3), (507, 0), (505, 700)):
            lines.append(f'@{ref} constant_int_val_node type: @9 value: {value}')
        return "\n".join(lines) + "\n"

    def replace_record(self, old: str, new: str, record: dict[str, object] | None = None) -> None:
        self.write_artifact("stderr.log", self.stderr(record).replace(old, new))

    def test_valid_structured_proof_passes(self) -> None:
        self.check()

    def test_distinct_base_proof_correlates_both_owners_and_restrict_metadata(self) -> None:
        verify_success_record(RECORD_DISTINCT, RAW_DISTINCT, "distinct_bases_source_first")
        architecture = ET.fromstring(self.architecture("distinct_bases_source_first"))
        verify_restrict_architecture_metadata(architecture, "distinct_bases_source_first", RAW_DISTINCT,
                                              RECORD_DISTINCT)
        with self.assertRaisesRegex(CheckError, "STORE base SSA does not name its declared owner argument"):
            verify_success_record(dict(RECORD_DISTINCT, write_base_ssa_index=9), RAW_DISTINCT,
                                  "distinct_bases_source_first")
        bad_architecture = ET.fromstring(self.architecture("distinct_bases_source_first").replace(
            'port="dst" index="1" source_restrict="1"',
            'port="dst" index="1" source_restrict="0"'))
        with self.assertRaisesRegex(CheckError, "source_restrict for distinct_bases_source_first.dst"):
            verify_restrict_architecture_metadata(bad_architecture, "distinct_bases_source_first", RAW_DISTINCT,
                                                  RECORD_DISTINCT)
        one_restrict_architecture = ET.fromstring(self.architecture("distinct_bases_one_restrict"))
        verify_restrict_architecture_metadata(one_restrict_architecture, "distinct_bases_one_restrict", RAW, None)

    def test_success_record_can_require_or_forbid_load_store_data_dependency(self) -> None:
        verify_success_record(RECORD_DISTINCT, RAW_DISTINCT, "distinct_bases_source_first")
        with self.assertRaisesRegex(CheckError, "dependency was found"):
            verify_success_record(RECORD_DISTINCT, RAW_DISTINCT, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        missing_load_value = RAW_DISTINCT.replace("@19 ssa_node type: @23\n", "")
        with self.assertRaisesRegex(CheckError, "dependency does not match"):
            verify_success_record(RECORD_DISTINCT, missing_load_value, "distinct_bases_source_first")
        independent_raw = RAW_DISTINCT.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35")
        verify_success_record(RECORD_DISTINCT, independent_raw, "distinct_bases_source_first",
                              require_load_store_data_dependency=False)
        with self.assertRaisesRegex(CheckError, "dependency does not match"):
            verify_success_record(RECORD_DISTINCT, independent_raw, "distinct_bases_source_first")
        xor_independent = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44").replace(
            "@44 add_node op: @43 op: @35", "@44 xor_node op: @42 op: @35")
        verify_success_record(RECORD_DISTINCT, xor_independent, "distinct_bases_source_first",
                              require_load_store_data_dependency=False)
        xor_dependent = xor_independent.replace("@44 xor_node op: @42 op: @35",
                                                "@44 xor_node op: @19 op: @35")
        with self.assertRaisesRegex(CheckError, "dependency was found"):
            verify_success_record(RECORD_DISTINCT, xor_dependent, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        sub_independent = xor_independent.replace("@44 xor_node op: @42 op: @35",
                                                 "@44 sub_node op: @42 op: @35")
        verify_success_record(RECORD_DISTINCT, sub_independent, "distinct_bases_source_first",
                              require_load_store_data_dependency=False)
        sub_dependent = sub_independent.replace("@44 sub_node op: @42 op: @35",
                                                "@44 sub_node op: @19 op: @35")
        with self.assertRaisesRegex(CheckError, "dependency was found"):
            verify_success_record(RECORD_DISTINCT, sub_dependent, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)

    def test_independent_store_proof_rejects_incomplete_dependency_traversal(self) -> None:
        independent_raw = RAW_DISTINCT.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35")
        missing_reference_raw = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44")
        missing_node = missing_reference_raw.replace("@44 add_node op: @43 op: @35\n", "")
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, missing_node, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        unsupported_node = missing_reference_raw.replace("@44 add_node op: @43 op: @35",
                                                          "@44 unsupported_value_node op: @35")
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, unsupported_node, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        unsupported_variable = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @66")
        unsupported_variable += '@66 ssa_node var: @67 type: @23\n@67 identifier_node strg: "not_an_argument"\n'
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, unsupported_variable, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        mismatched_phi_result = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44").replace(
            "@44 add_node op: @43 op: @35", "@44 ssa_node def_stmt: @32")
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, mismatched_phi_result, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        direct_assignment = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44").replace(
            "@44 add_node op: @43 op: @35", "@44 assign_stmt op: @45 op: @35")
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, direct_assignment, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        direct_phi = independent_raw.replace(
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35",
            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44").replace(
            "@44 add_node op: @43 op: @35", "@44 phi_stmt res: @44 def: @35 edge: 0")
        with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
            verify_success_record(RECORD_DISTINCT, direct_phi, "distinct_bases_source_first",
                                  require_load_store_data_dependency=False)
        with patch("check_duplex_effect_analysis_artifacts.OPERAND_DEPENDENCY_LIMIT", 1):
            with self.assertRaisesRegex(CheckError, "traversal is incomplete"):
                verify_success_record(RECORD_DISTINCT, independent_raw, "distinct_bases_source_first",
                                      require_load_store_data_dependency=False)

    def test_distinct_base_activation_requires_non_overlap_contract_warning(self) -> None:
        warning = ("WARNING: distinct-base duplex relies on the caller's __restrict contract; the caller must "
                   "ensure the accessed regions do not overlap (no runtime range validation is performed)\n")
        profile = {"max_read_burst_length": "16", "max_write_burst_length": "16"}
        for top in ("distinct_bases_source_first", "distinct_prefix_read_longer"):
            with self.subTest(top=top):
                activation = ("Activated distinct-base restricted pointwise duplex m_axi bundle shared for root "
                              f"{top}\n")
                rtl = self.activated_rtl(top)
                check_production_duplex(rtl, activation + warning, top, profile)
                with self.assertRaisesRegex(CheckError, "missing its caller non-overlap contract warning"):
                    check_production_duplex(rtl, activation, top, profile)
                if top == "distinct_prefix_read_longer":
                    stale_marker = activation.replace("distinct-base restricted", "same-base")
                    with self.assertRaisesRegex(CheckError, "production activation marker is missing"):
                        check_production_duplex(rtl, stale_marker + warning, top, profile)

    def test_distinct_base_64_profile_keeps_per_owner_recognition_expectations(self) -> None:
        self.assertEqual(expected_owner_recognition("distinct_bases_asymmetric_64", 1, 0), ("1", "0"))
        self.assertEqual(expected_owner_recognition("distinct_bases_asymmetric_64", 0, 1), ("0", "1"))

    def test_mismatched_bound_fixture_does_not_claim_comparison_guard_coverage(self) -> None:
        reason, owners, read_fallback, write_fallback, semantic, profile = CASES[
            "distinct_bases_mismatched_bounds"]
        self.assertEqual(reason, "effective bound is defined inside loop")
        self.assertEqual(owners, 2)
        self.assertEqual((read_fallback, write_fallback),
                         (FALLBACK_READ, FALLBACK_WRITE))
        self.assertFalse(semantic)
        self.assertTrue(profile)

    def test_option_c_case_inventory_pins_positive_and_veto_shapes(self) -> None:
        self.assertEqual(CASES["distinct_prefix_read_longer"][0],
                         "distinct-base restricted pointwise read-before-write candidate")
        self.assertEqual(CASES["distinct_prefix_write_longer"][0],
                         "distinct-base restricted pointwise read-before-write candidate")
        self.assertEqual(CASES["distinct_prefix_equal"][0],
                         "distinct-base restricted pointwise read-before-write candidate")
        self.assertEqual(CASES["distinct_prefix_signed_bound"][0],
                         "unsupported direction guard CFG")
        self.assertEqual(CASES["distinct_prefix_width_bound"][0],
                         "distinct-base restricted pointwise read-before-write candidate")
        self.assertTrue(CASES["distinct_prefix_width_bound"][4])
        self.assertEqual(CASES["distinct_prefix_width_bound"][1:],
                         (2, "-", "-", True, True))
        self.assertEqual(CASES["distinct_prefix_extra_cfg"][0], "unsupported direction guard CFG")
        self.assertEqual(CASES["distinct_prefix_guard_bypass"][0],
                         "unsupported direction guard CFG")
        self.assertEqual(CASES["distinct_bases_two_loops"][0],
                         "read/write accesses do not share one canonical loop")
        self.assertEqual(CASES["distinct_prefix_u64_bound"][0],
                         "unsupported direction guard CFG")
        self.assertEqual(CASES["distinct_prefix_u64_bound"][1:],
                         (2, FALLBACK_READ, FALLBACK_WRITE, False, True))
        self.assertEqual(CASES["shifted_index"][0],
                         "read/write accesses do not share one canonical loop")

    def test_prefix_trip_proof_uses_exact_latch_branch_and_cfg_edges(self) -> None:
        nodes = {
            1: ("integer_ty_node", "bitsizealloc: 32 bitsize: 32 unsigned"),
            2: ("integer_ty_node", "bitsizealloc: 1 bitsize: 1 unsigned"),
            9: ("ssa_node", "type: @1 def_stmt: @10"),
            10: ("phi_stmt", "parent: @90 bb_index: 2 res: @9 def: @16 edge: 3 def: @11 edge: 4"),
            11: ("ssa_node", "type: @1 def_stmt: @12"),
            12: ("assign_stmt", "parent: @90 bb_index: 4 op: @11 op: @13"),
            13: ("add_node", "type: @1 op: @9 op: @14"),
            14: ("constant_int_val_node", "type: @1 value: 1"),
            15: ("ssa_node", "type: @1"),
            16: ("constant_int_val_node", "type: @1 value: 0"),
            17: ("eq_node", "type: @2 op: @11 op: @15"),
            18: ("ssa_node", "type: @2 def_stmt: @19"),
            19: ("assign_stmt", "parent: @90 bb_index: 4 op: @18 op: @17"),
            20: ("multi_way_if_stmt", "parent: @90 bb_index: 4 op: @18 bloc: 7 bloc: 2"),
            22: ("ssa_node", "type: @2 def_stmt: @25"),
            23: ("constant_int_val_node", "type: @1 value: 0"),
            24: ("eq_node", "type: @2 op: @15 op: @23"),
            25: ("assign_stmt", "parent: @90 bb_index: 3 op: @22 op: @24"),
            26: ("multi_way_if_stmt", "parent: @90 bb_index: 3 op: @22 bloc: 7 bloc: 2"),
        }
        blocks = {
            2: (1, "", [], ["3", "4"], ["5", "6"]),
            3: (0, "", [25, 26], ["ENTRY"], ["2", "7"]),
            4: (1, "", [12, 19, 20], ["5"], ["7", "2"]),
            5: (1, "", [], ["2"], ["4"]),
            6: (1, "", [], ["2"], ["4"]),
            7: (0, "", [], ["3", "4"], ["EXIT"]),
        }
        record = {"read_header_bb": 2, "read_preheader_bb": 3, "write_exit_bb": 7,
                  "loop_trip_bound_ssa_index": 15, "loop_trip_compare_index": 18,
                  "read_induction_index": 9, "read_branch_index": 26, "read_guard_index": 22}
        self.assertEqual(verify_prefix_loop_trip_control(nodes, blocks, record, 1), (4, 18, 11))

        wrong_trip_bound = dict(nodes)
        wrong_trip_bound[17] = ("eq_node", "type: @2 op: @11 op: @14")
        with self.assertRaisesRegex(CheckError, "exact increment == trip bound"):
            verify_prefix_loop_trip_control(wrong_trip_bound, blocks, record, 1)
        wrong_increment = dict(nodes)
        wrong_increment[13] = ("sub_node", "type: @1 op: @9 op: @14")
        with self.assertRaisesRegex(CheckError, "induction plus one"):
            verify_prefix_loop_trip_control(wrong_increment, blocks, record, 1)
        wrong_branch = dict(nodes)
        wrong_branch[20] = ("multi_way_if_stmt", "parent: @90 bb_index: 4 op: @18 bloc: 2 bloc: 7")
        with self.assertRaisesRegex(CheckError, "map true to exit"):
            verify_prefix_loop_trip_control(wrong_branch, blocks, record, 1)
        wrong_cfg = dict(blocks)
        wrong_cfg[4] = (1, "", [12, 19, 20], ["5"], ["2", "6"])
        with self.assertRaisesRegex(CheckError, "CFG successors do not match"):
            verify_prefix_loop_trip_control(nodes, wrong_cfg, record, 1)

    def test_prefix_guard_rejects_mutated_header_bypass_edge(self) -> None:
        prefix_raw = '''@1 function_val_node name: @2 body: @20
@2 identifier_node strg: "prefix_cfg"
@11 multi_way_if_stmt parent: @1 bb_index: 2 op: @12 bloc: 3 bloc: 4
@12 ssa_node def_stmt: @13
@13 assign_stmt parent: @1 bb_index: 2 op: @12 op: @14
@14 lt_node op: @15 op: @16
@15 ssa_node
@16 ssa_node
@20 statement_list_node bloc: 1 loop_id: 5 pred: ENTRY succ: 2 bloc: 2 loop_id: 5 pred: 1 succ: 3 succ: 4 stmt: @11 bloc: 3 loop_id: 5 pred: 2 succ: 5 bloc: 4 loop_id: 5 pred: 2 succ: 6 bloc: 5 loop_id: 5 pred: 3 succ: 7 bloc: 6 loop_id: 5 pred: 4 succ: 7 bloc: 7 loop_id: 0 pred: 5 pred: 6 succ: EXIT
'''
        _, _, prefix_cfg, _ = function_graph(prefix_raw, "prefix_cfg")
        verify_direction_guard_dominance(prefix_cfg, "read", 1, 2, 3, 4, 5)

        bypass_raw = prefix_raw.replace("pred: ENTRY succ: 2 bloc: 2", "pred: ENTRY succ: 2 succ: 8 bloc: 2")
        bypass_raw = bypass_raw.replace("succ: EXIT\n", "succ: EXIT bloc: 8 loop_id: 5 pred: 1 succ: 5\n")
        _, _, bypass_cfg, _ = function_graph(bypass_raw, "prefix_cfg")
        with self.assertRaisesRegex(CheckError, "read effective-prefix guard does not dominate memory access"):
            verify_direction_guard_dominance(bypass_cfg, "read", 1, 2, 3, 4, 5)

    def test_prefix_count_proof_rejects_missing_or_inconsistent_recipe_fields(self) -> None:
        missing = dict(RECORD)
        del missing["pairing_domain_recipe"]
        with self.assertRaisesRegex(CheckError, "missing prefix-count proof fields"):
            verify_success_record(missing, RAW, "same_base_pointwise")
        with self.assertRaisesRegex(CheckError, "unguarded read count does not use"):
            verify_success_record(dict(RECORD, read_count_recipe="min_trip_and_effective_bound"),
                                  RAW, "same_base_pointwise")
        with self.assertRaisesRegex(CheckError, "read access BB does not match the original memory operation"):
            verify_success_record(dict(RECORD, read_access_bb=0), RAW, "same_base_pointwise")
        with self.assertRaisesRegex(CheckError, "loop trip comparison"):
            verify_success_record(dict(RECORD, loop_trip_compare_index=29), RAW,
                                  "same_base_pointwise")
        with self.assertRaisesRegex(CheckError, "read effective bound is not an unsigned-32"):
            verify_success_record(dict(RECORD, read_effective_compare_index=0,
                                       read_effective_bound_ssa_index=34,
                                       read_count_recipe="min_trip_and_effective_bound"),
                                  RAW, "same_base_pointwise")

    @staticmethod
    def configure_count_fixture() -> tuple[dict[str, tuple[str, str]], dict, dict[str, object]]:
        nodes = {
            "@100": ("ssa_node", "type: @1"),
            "@101": ("ssa_node", "type: @1"),
            "@102": ("ssa_node", "type: @1"),
            "@103": ("ssa_node", "type: @2"),
            "@104": ("lt_node", "op: @100 op: @101"),
            "@105": ("select_node", "op: @104 op: @100 op: @101"),
            "@106": ("select_node", "op: @103 op: @105 op: @110"),
            "@107": ("lt_node", "op: @100 op: @102"),
            "@108": ("select_node", "op: @107 op: @100 op: @102"),
            "@109": ("select_node", "op: @103 op: @108 op: @110"),
            "@110": ("constant_int_val_node", "value: 0"),
        }
        calls = {"configure_read": ("", ("", ""), ["@0", "@0", "@0", "@0", "@106"]),
                 "configure_write": ("", ("", ""), ["@0", "@0", "@0", "@0", "@109"])}
        record = {
            "loop_trip_bound_ssa_index": 100,
            "read_effective_bound_ssa_index": 101,
            "write_effective_bound_ssa_index": 102,
            "read_guard_index": 103,
            "write_guard_index": 103,
            "read_effective_compare_index": 201,
            "write_effective_compare_index": 202,
            "read_count_recipe": "min_trip_and_effective_bound",
            "write_count_recipe": "min_trip_and_effective_bound",
        }
        return nodes, calls, record

    def test_emitted_configure_counts_follow_distinct_clamped_prefix_recipes(self) -> None:
        nodes, calls, record = self.configure_count_fixture()
        verify_configure_count_expressions(nodes, calls, record)

    def test_emitted_configure_count_complements_zero_trip_entry_guard(self) -> None:
        nodes, calls, record = self.configure_count_fixture()
        nodes["@103"] = ("eq_node", "op: @100 op: @110")
        nodes["@111"] = ("ne_node", "op: @100 op: @110")
        nodes["@106"] = ("select_node", "op: @111 op: @105 op: @110")
        nodes["@109"] = ("select_node", "op: @111 op: @108 op: @110")
        verify_configure_count_expressions(nodes, calls, record)

        nodes["@106"] = ("select_node", "op: @103 op: @105 op: @110")
        with self.assertRaisesRegex(CheckError, "CONFIGURE_READ count does not implement"):
            verify_configure_count_expressions(nodes, calls, record)

    def test_emitted_configure_rejects_old_shared_count_reuse(self) -> None:
        nodes, calls, record = self.configure_count_fixture()
        calls["configure_write"] = (*calls["configure_write"][:2],
                                    ["@0", "@0", "@0", "@0", "@106"])
        with self.assertRaisesRegex(CheckError, "CONFIGURE_WRITE count does not implement"):
            verify_configure_count_expressions(nodes, calls, record)

    def test_emitted_configure_rejects_missing_or_inverted_min_clamp(self) -> None:
        nodes, calls, record = self.configure_count_fixture()
        nodes["@106"] = ("select_node", "op: @103 op: @100 op: @110")
        with self.assertRaisesRegex(CheckError, "CONFIGURE_READ count does not implement"):
            verify_configure_count_expressions(nodes, calls, record)
        nodes, calls, record = self.configure_count_fixture()
        nodes["@104"] = ("lt_node", "op: @101 op: @100")
        with self.assertRaisesRegex(CheckError, "CONFIGURE_READ count does not implement"):
            verify_configure_count_expressions(nodes, calls, record)

    def test_guard_proof_requires_exact_unsigned_prefix_predicate(self) -> None:
        nodes = {
            1: ("integer_ty_node", "bitsizealloc: 32 bitsize: 32 unsigned"),
            2: ("ssa_node", "type: @1"),
            3: ("ssa_node", "type: @1"),
            4: ("lt_node", "op: @2 op: @3"),
        }
        verify_unsigned_prefix_comparison(nodes, 4, 2, 3, "read")
        nodes[4] = ("gt_node", "op: @3 op: @2")
        verify_unsigned_prefix_comparison(nodes, 4, 2, 3, "read")
        nodes[4] = ("gt_node", "op: @2 op: @3")
        with self.assertRaisesRegex(CheckError, "not induction < its exact bound"):
            verify_unsigned_prefix_comparison(nodes, 4, 2, 3, "read")
        nodes[4] = ("lt_node", "op: @3 op: @2")
        with self.assertRaisesRegex(CheckError, "not induction < its exact bound"):
            verify_unsigned_prefix_comparison(nodes, 4, 2, 3, "read")

    def test_asymmetric_profile_and_64_bit_address_profile_pass(self) -> None:
        self.configure_activated_case("same_base_asymmetric_64", 64, 8, 2, 16, 1)
        self.check("same_base_asymmetric_64")

    def test_write_fifo_depth_must_match_architecture_profile(self) -> None:
        self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(
            ".MAX_OUTSTANDING(1), .FIFO_DEPTH(16), .PEER_FAULT_ENABLE(1)",
            ".MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .PEER_FAULT_ENABLE(1)"))
        with self.assertRaisesRegex(CheckError, "FIFO depth does not match architecture attribute write_fifo_depth"):
            self.check()

    def test_peer_fault_cross_connection_is_required(self) -> None:
        mutations = (
            ("wire read_peer_fault = write_core_fault;", "wire read_peer_fault = 1'b0;",
             "read_peer_fault must be cross-connected to write_core_fault"),
            ("wire read_peer_fault = write_core_fault;", "wire read_peer_fault = read_core_fault;",
             "read_peer_fault must be cross-connected to write_core_fault"),
            ("wire write_peer_fault = read_core_fault;", "wire write_peer_fault = write_core_fault;",
             "write_peer_fault must be cross-connected to read_core_fault"),
            (".peer_fault(read_peer_fault)", ".peer_fault(1'b0)",
             "read_core peer_fault port must connect to read_peer_fault"),
            (".fault(read_core_fault)", ".fault()",
             "read_core fault port must connect to read_core_fault"),
            (".PEER_FAULT_ENABLE(1)", ".PEER_FAULT_ENABLE(0)",
             "must enable peer fault propagation"),
        )
        for old, new, message in mutations:
            with self.subTest(mutation=new):
                self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(old, new, 1))
                with self.assertRaisesRegex(CheckError, message):
                    self.check()

    def test_activated_trace_requires_recognized_operations_without_fallback(self) -> None:
        self.configure_activated_case("same_base_pointwise")
        self.write_artifact("stderr.log", self.stderr().replace(
            "---Activated same-base pointwise duplex m_axi bundle",
            "Activated same-base pointwise duplex m_axi bundle"))
        self.check("same_base_pointwise")
        bad_trace = TRACE.replace('read_fallback="-"',
                                  'read_fallback="bundle contains stores"').replace(
            'write_fallback="-"',
            'write_fallback="bundle contains reads; same-bundle read/write requires duplex support"')
        self.write_artifact("stderr.log", self.stderr(trace=bad_trace))
        with self.assertRaisesRegex(CheckError, "read request/recognition/fallback"):
            self.check("same_base_pointwise")

    def test_partial_activation_is_rejected(self) -> None:
        self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(
            "wire fuselector_shared_duplex_read_endpoint_burst_read;\n", ""))
        with self.assertRaisesRegex(CheckError, "operation mapping is missing burst_read"):
            self.check()

    def test_activation_flag_cannot_be_downgraded_after_rtl_emission(self) -> None:
        self.write_artifact("stderr.log", self.stderr().replace("production_activated=1", "production_activated=0"))
        with self.assertRaisesRegex(CheckError, "production activation mismatch"):
            self.check()

    def test_activation_marker_is_required(self) -> None:
        self.write_artifact("stderr.log", self.stderr().replace(
            "---Activated same-base pointwise duplex m_axi bundle shared for root same_base_pointwise\n", ""))
        with self.assertRaisesRegex(CheckError, "production activation marker is missing"):
            self.check()

    def test_missing_drain_mapping_is_rejected(self) -> None:
        self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(
            "wire fuselector_shared_duplex_write_endpoint_drain_write;\n", ""))
        with self.assertRaisesRegex(CheckError, "operation mapping is missing drain_write"):
            self.check()

    def test_swapped_direction_mapping_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace("duplex_read_endpoint_burst_read",
                                           "duplex_write_endpoint_burst_read")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read_endpoint operation mapping is missing burst_read"):
            self.check()

    def test_legacy_adapter_coexistence_is_rejected(self) -> None:
        self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(
            "endmodule", "MinimalAXI4AdapterSingleBeat legacy_mem ();\nendmodule"))
        with self.assertRaisesRegex(CheckError, "coexists with a legacy memory adapter"):
            self.check()

    def test_missing_read_endpoint_is_rejected(self) -> None:
        self.write_artifact("same_base_pointwise.v", self.activated_rtl().replace(
            "  shared_duplex_read_endpoint_modgen read_endpoint (", ""))
        with self.assertRaisesRegex(CheckError, "exactly one duplex read_endpoint instance"):
            self.check()

    def test_endpoint_aggregator_command_crosswire_is_rejected(self) -> None:
        rtl = self.activated_rtl("datapath_matrix_candidate", aggregator_pins=True,
                                 external_top=False).replace(
            ".duplex_start(read_start_wire)", ".duplex_start(write_start_wire)")
        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as directory:
            path = Path(directory) / "top.v"
            path.write_text(rtl)
            with self.assertRaisesRegex(ValueError, "read start is not connected through one named net"):
                verify_backend_rtl(path, ("matrix_candidate",), {"matrix_candidate"})

    def test_missing_private_connection_on_both_instances_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace(".duplex_start(read_start_wire), ", "")
        rtl = rtl.replace(".read_start(read_start_wire),\n", "")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read start is missing on an endpoint or aggregator instance"):
            self.check()

    def test_matching_constants_are_not_private_nets(self) -> None:
        rtl = self.activated_rtl().replace(".duplex_start(read_start_wire)", ".duplex_start(1'b0)")
        rtl = rtl.replace(".read_start(read_start_wire)", ".read_start(1'b0)")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read start is not connected through one named net"):
            self.check()

    def test_dummy_output_net_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace("read_start_wire", "null_out_signal_read_start")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "dummy or constant"):
            self.check()

    def test_zero_aggregator_command_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace(".read_start(read_start_wire)", ".read_start(1'b0)")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read start is not connected through one named net"):
            self.check()

    def test_wrong_endpoint_or_aggregator_port_direction_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace("output  duplex_start;", "input  duplex_start;", 1)
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read endpoint duplex_start must be output"):
            self.check()

    def test_endpoint_start_must_use_start_port_bit_zero(self) -> None:
        rtl = self.activated_rtl().replace(
            "assign duplex_start = start_port[0];", "assign duplex_start = start;")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, r"read endpoint duplex_start must be assigned from start_port\[0\]"):
            self.check()

    def test_endpoint_done_must_drive_done_port(self) -> None:
        rtl = self.activated_rtl().replace(
            "assign done_port = duplex_done;", "assign done = duplex_done;")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read endpoint must assign done_port exactly once"):
            self.check()

    def test_endpoint_payload_mapping_must_be_correct(self) -> None:
        rtl = self.activated_rtl().replace(
            "assign duplex_data = in3;", "assign duplex_data = in2;")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "read endpoint duplex_data must be assigned from in3"):
            self.check()

    def test_disconnected_done_or_result_is_rejected(self) -> None:
        for port in ("read_done", "read_result"):
            with self.subTest(port=port):
                rtl = self.activated_rtl().replace(f".{port}(", f".{port}()")
                self.write_artifact("same_base_pointwise.v", rtl)
                with self.assertRaisesRegex(CheckError, "read (done|result) is not connected through one named net"):
                    self.check()

    def test_reused_private_net_is_rejected(self) -> None:
        rtl = self.activated_rtl().replace(".duplex_data(write_data_wire)",
                                           ".duplex_data(read_data_wire)")
        rtl = rtl.replace(".write_data(write_data_wire)", ".write_data(read_data_wire)")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "net .* is reused across links"):
            self.check()

    def test_private_address_wire_must_match_64_bit_profile(self) -> None:
        self.configure_activated_case("same_base_asymmetric_64", 64, 8, 2, 16, 1)
        rtl_path = self.output / "same_base_asymmetric_64.v"
        rtl_path.write_text(rtl_path.read_text().replace(
            "wire [63:0] read_address_wire;", "wire [31:0] read_address_wire;"), encoding="utf-8")
        with self.assertRaisesRegex(CheckError, "read address net width does not match the 64-bit profile"):
            self.check("same_base_asymmetric_64")

    def test_buser_must_remain_a_physical_input(self) -> None:
        rtl = self.activated_rtl().replace(
            "  input [0:0] m_axi_shared_buser;", "  output [0:0] m_axi_shared_buser;")
        self.write_artifact("same_base_pointwise.v", rtl)
        with self.assertRaisesRegex(CheckError, "external bundle pin buser is not declared as input"):
            self.check()

    def test_missing_structured_record_is_rejected(self) -> None:
        self.write_artifact("stderr.log", TRACE)
        with self.assertRaisesRegex(CheckError, "structured duplex proof record"):
            self.check()

    def test_stdout_echo_cannot_replace_compiler_stderr_witness(self) -> None:
        self.write_artifact("stderr.log", "compiler exited successfully\n")
        self.write_artifact("stdout.log", self.stderr())
        with self.assertRaisesRegex(CheckError, "preanalysis trace record"):
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
        with self.assertRaisesRegex(CheckError, "legacy base identity does not match the LOAD base"):
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
        raw = RAW.replace("@40 eq_node op: @36 op: @63", "@40 eq_node op: @36 op: @9")
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw", raw)
        self.refresh_provenance()
        with self.assertRaisesRegex(CheckError, "loop comparison is not equality"):
            self.check()

    def test_loop_relation_must_be_equality(self) -> None:
        raw = RAW.replace("@40 eq_node op: @36 op: @63", "@40 ne_node op: @36 op: @63")
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

    def test_unsigned_zero_guard_requires_exact_inverted_entry_branch(self) -> None:
        unsigned_zero = RAW.replace("@26 gt_node op: @47 op: @55", "@26 eq_node op: @63 op: @55")
        inverted_branch = unsigned_zero.replace(
            "@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 1 bloc: 2",
            "@10 multi_way_if_stmt parent: @1 bb_index: 0 op: @11 bloc: 2 bloc: 1")
        verify_success_record(RECORD, inverted_branch, "same_base_pointwise")
        wrong_targets = unsigned_zero
        with self.assertRaisesRegex(CheckError, "entry predicate is not signed declared-bound > 0"):
            verify_success_record(RECORD, wrong_targets, "same_base_pointwise")
        wrong_comparison = inverted_branch.replace("@26 eq_node op: @63 op: @55",
                                                  "@26 ne_node op: @63 op: @55")
        with self.assertRaisesRegex(CheckError, "entry predicate is not signed declared-bound > 0"):
            verify_success_record(RECORD, wrong_comparison, "same_base_pointwise")

    def test_entry_bound_conversion_must_be_exact_nop(self) -> None:
        raw = RAW.replace("@49 nop_node type: @23 op: @63",
                          "@49 add_node type: @23 op: @63 op: @57\n@57 constant_int_val_node type: @23 value: 1")
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
        shifted_reason = "read/write accesses do not share one canonical loop"
        record = {"bundle": "shared", "semantic_proof": False,
                  "semantic_reason": shifted_reason,
                  "profile_eligible": True, "profile_reason": "read/write profile is eligible"}
        trace = TRACE.replace("same_base_duplex_candidate=1", "same_base_duplex_candidate=0")
        trace = trace.replace(SUCCESS, shifted_reason)
        trace = trace.replace("read_recognized=1", "read_recognized=0").replace(
            "write_recognized=1", "write_recognized=0")
        trace = trace.replace('read_fallback="-"', f'read_fallback="{FALLBACK_READ}"').replace(
            'write_fallback="-"', f'write_fallback="{FALLBACK_WRITE}"')
        trace = trace.replace('production_activated=1 activation_reason="frozen application-wide proof and profile accepted"',
                              'production_activated=0 activation_reason="bundle is not in the frozen activation plan"')
        self.write_artifact("stderr.log", trace + "AXI_BURST_DUPLEX_PROOF " +
                            json.dumps(record, separators=(",", ":")) + "\n")
        self.write_artifact("shifted_index.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("shifted_index"))
        self.write_artifact("provenance.txt", self.provenance().replace(
            "top=same_base_pointwise", "top=shifted_index", 1))
        self.check("shifted_index")
        self.write_artifact("stderr.log", trace)
        with self.assertRaisesRegex(CheckError, "exactly one structured duplex proof record"):
            self.check("shifted_index")
        record["load_statement_index"] = 5
        self.write_artifact("stderr.log", trace + "AXI_BURST_DUPLEX_PROOF " +
                            json.dumps(record, separators=(",", ":")) + "\n")
        with self.assertRaisesRegex(CheckError, "success-only identity fields"):
            self.check("shifted_index")

    def test_cached_profile_promotion_is_rejected(self) -> None:
        record = dict(RECORD, profile_eligible=False, profile_reason="cache-enabled bundle")
        cache_trace = TRACE.replace('read_fallback="-"',
                                    'read_fallback="cache-enabled bundle"')
        cache_trace = cache_trace.replace(
            'write_fallback="-"',
            'write_fallback="cache-enabled bundle"')
        cache_trace = cache_trace.replace("read_recognized=1", "read_recognized=0").replace(
            "write_recognized=1", "write_recognized=0")
        cache_trace = cache_trace.replace(
            'production_activated=1 activation_reason="frozen application-wide proof and profile accepted"',
            'production_activated=0 activation_reason="cache-enabled bundle"')
        self.write_artifact("stderr.log", self.stderr(record, cache_trace))
        self.write_artifact("same_base_cached.v", "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("same_base_cached"))
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw",
                            RAW.replace('strg: "same_base_pointwise"', 'strg: "same_base_cached"'))
        (self.output / "panda-temp/after_Frontend::InterfaceInfer.raw").unlink()
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

    def test_wrong_access_counts_are_rejected(self) -> None:
        self.write_artifact("stderr.log", self.stderr().replace("read_accesses=1", "read_accesses=0"))
        with self.assertRaisesRegex(CheckError, "classified access counts"):
            self.check()

    def test_distinct_owner_access_counts_are_required(self) -> None:
        reason = "distinct-base duplex requires source restrict on both owners"
        trace = TRACE.replace("same_base_duplex_candidate=1", "same_base_duplex_candidate=0")
        trace = trace.replace(SUCCESS, reason)
        trace = trace.replace("read_recognized=1", "read_recognized=0").replace(
            "write_recognized=1", "write_recognized=0")
        trace = trace.replace('read_fallback="-"', f'read_fallback="{FALLBACK_READ}"').replace(
            'write_fallback="-"', f'write_fallback="{FALLBACK_WRITE}"')
        trace = trace.replace('production_activated=1 activation_reason="frozen application-wide proof and profile accepted"',
                              'production_activated=0 activation_reason="bundle has multiple owners"')
        read_trace = trace.replace("owner_argument_index=3", "owner_argument_index=4")
        read_trace = read_trace.replace("read_accesses=1 write_accesses=1", "read_accesses=1 write_accesses=0")
        write_trace = trace.replace("owner_argument_index=3", "owner_argument_index=5")
        write_trace = write_trace.replace("read_accesses=1 write_accesses=1", "read_accesses=0 write_accesses=1")
        record = {"bundle": "shared", "semantic_proof": False, "semantic_reason": reason,
                  "profile_eligible": True, "profile_reason": "read/write profile is eligible"}
        proof = "AXI_BURST_DUPLEX_PROOF " + json.dumps(record, separators=(",", ":")) + "\n"
        self.write_artifact("distinct_bases.v",
                            "module top; MinimalAXI4AdapterSingleBeat legacy_mem(); endmodule\n")
        self.write_artifact("panda-temp/architecture.xml", self.architecture("distinct_bases"))
        self.write_artifact("panda-temp/before_Frontend::InterfaceInfer.raw",
                            RAW.replace('strg: "same_base_pointwise"', 'strg: "distinct_bases"'))
        self.write_artifact("provenance.txt", self.provenance().replace(
            "top=same_base_pointwise", "top=distinct_bases", 1))
        self.write_artifact("stderr.log", read_trace + write_trace + proof)
        self.check("distinct_bases")
        bad_read_trace = read_trace.replace("read_accesses=1 write_accesses=0",
                                            "read_accesses=0 write_accesses=0")
        self.write_artifact("stderr.log", bad_read_trace + write_trace + proof)
        with self.assertRaisesRegex(CheckError, "classified access counts"):
            self.check("distinct_bases")

    def test_wrong_profile_reason_is_rejected(self) -> None:
        record = dict(RECORD, profile_reason="cache-enabled bundle")
        self.write_artifact("stderr.log", self.stderr(record))
        with self.assertRaisesRegex(CheckError, "structured profile reason"):
            self.check()

    def test_split_read_and_write_loops_remain_legacy(self) -> None:
        self.configure_fallback_case(
            "split_read_write_loops", "read/write accesses do not share one canonical loop", 1, 1,
            FALLBACK_READ, FALLBACK_WRITE)
        self.check("split_read_write_loops")

    def test_conditional_store_rejects_unsupported_guard_cfg(self) -> None:
        self.configure_fallback_case(
            "conditional_store", "unsupported direction guard CFG", 1, 1,
            FALLBACK_READ, FALLBACK_WRITE)
        self.check("conditional_store")

    def test_volatile_byte_side_store_is_counted_and_rejected(self) -> None:
        reason = "original effects are not fully classified ordinary accesses"
        read_fallback = "memory writes or opaque effects; prefetch ordering is not proven"
        write_fallback = "opaque, volatile, atomic, or unclassified memory effects"
        self.configure_fallback_case("same_loop_byte_side_store", reason, 1, 2,
                                      read_fallback, write_fallback)
        self.check("same_loop_byte_side_store")
        self.configure_fallback_case("same_loop_byte_side_store", reason, 1, 1,
                                      read_fallback, write_fallback)
        with self.assertRaisesRegex(CheckError, "classified access counts"):
            self.check("same_loop_byte_side_store")

    def test_opaque_call_prevents_semantic_proof(self) -> None:
        reason = "original effects are not fully classified ordinary accesses"
        self.configure_fallback_case(
            "same_loop_opaque_call", reason, 1, 1,
            "memory writes or opaque effects; prefetch ordering is not proven",
            "opaque, volatile, atomic, or unclassified memory effects")
        self.check("same_loop_opaque_call")

    def test_output_symlink_is_rejected(self) -> None:
        link = self.root / "case-link"
        link.symlink_to(self.output, target_is_directory=True)
        with self.assertRaisesRegex(CheckError, "invalid output directory"):
            verify(link, "same_base_pointwise", self.source, self.bambu, self.plugin, self.checker, self.runner)


if __name__ == "__main__":
    unittest.main()
