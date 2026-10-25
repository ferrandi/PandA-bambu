#!/usr/bin/env python3
"""Offline fail-closed tests for the G3.3 static mapping and event schema."""
from __future__ import annotations

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
FAMILY = Path(__file__).resolve().parent
sys.path.insert(0, str(FAMILY))
import check_duplex_iteration_results as checker  # noqa: E402


def make_fixture(root: Path) -> Path:
    generated = root / "generated"
    dot = generated / "HLS_output/dot/kernel"
    raw = generated / "panda-temp"
    dot.mkdir(parents=True)
    raw.mkdir(parents=True)
    (generated / "bambu_results.xml").write_text(
        '<application><hls_results><function_1 name="kernel" function_ii="0">'
        '<loop_2 bb="2" ii="1"/></function_1></hls_results></application>\n')
    (generated / "kernel_scheduling.xml").write_text(
        '<hls><scheduling>' + ''.join(
            f'<scheduling_constraints vertex_name="kernel_100_{ident}" cstep="2" '
            f'fu_name="{name}" fu_index="0" library=""/>'
            for ident, name in ((1, "phi_stmt_FU"), (2, "gep_node_FU"),
                                (3, "read_endpoint"), (4, "gep_node_FU"),
                                (5, "write_endpoint"), (6, "shl_node_FU"), (7, "add_node_FU")))
        + '</scheduling></hls>\n')
    (raw / "after_CSROA.ll").write_text("""define void @kernel(i32* %0, i32* %1) {
3:                                                ; entry
  br label %2
2:                                                ; loop
  %idx = phi i32 [ %next, %2 ], [ 0, %3 ]
  %srcp = getelementptr i32, i32* %0, i32 %idx
  %value = load i32, i32* %srcp, align 4
  %dstp = getelementptr i32, i32* %1, i32 %idx
  store i32 %value, i32* %dstp, align 4
  %next = add i32 %idx, 1
  br label %2
}
""")
    (raw / "after_Frontend::BuildVirtualPhi::kernel(9)[2].raw").write_text("""@100 function_val_node name: @101 body: @102
@101 identifier_node strg: "kernel"
@102 statement_list_node bloc: 1 loop_id: 0 pred: ENTRY succ: 2 bloc: 2 loop_id: 1 pred: 1 pred: 2 succ: EXIT phi: @1 stmt: @2 stmt: @3 stmt: @4 stmt: @5 stmt: @6 stmt: @7
@1 phi_stmt parent: @100 bb_index: 2 res: @20 def: @21 edge: 2 def: @22 edge: 1
@2 assign_stmt parent: @100 bb_index: 2 op: @30 op: @40
@3 assign_stmt parent: @100 bb_index: 2 op: @31 op: @41
@4 assign_stmt parent: @100 bb_index: 2 op: @32 op: @42
@5 call_stmt parent: @100 bb_index: 2 fn: @55 arg: @31 arg: @33
@6 assign_stmt parent: @100 bb_index: 2 op: @34 op: @43
@7 assign_stmt parent: @100 bb_index: 2 op: @21 op: @44
@20 ssa_node def_stmt: @1
@21 ssa_node def_stmt: @7
@22 constant_int_val_node value: 0
@30 ssa_node def_stmt: @2
@31 ssa_node def_stmt: @3
@32 ssa_node def_stmt: @4
@33 ssa_node def_stmt: @5
@34 ssa_node def_stmt: @6
@40 gep_node
@41 call_node fn: @45
@42 gep_node
@43 shl_node
@44 add_node op: @20 op: @46
@45 addr_node op: @47
@46 constant_int_val_node value: 1
@47 function_val_node name: @48
@48 identifier_node strg: "shared_bambu_artificial_ParmMgr_burst_read"
@55 addr_node op: @56
@56 function_val_node name: @57
@57 identifier_node strg: "shared_bambu_artificial_ParmMgr_store_write"
""")
    (dot / "HLS_scheduling_BB2.dot").write_text(r'''digraph G {
3[shape=ellipse,label="kernel_100_1 - phi_stmt\n/* idx = phi_stmt(<next, BB2>, <0, BB3>) */\n"];
4[color=burlywood,shape=box,label="kernel_100_6 - shl_node\nindex_offset = idx << (2u);\n"];
5[color=burlywood,shape=box,label="kernel_100_2 - gep_node\nsrc_address = src + index_offset;\n"];
6[color=green,shape=box,label="kernel_100_3 - read_endpoint\nvalue = shared_bambu_artificial_ParmMgr_burst_read(src_address);\n"];
7[color=burlywood,shape=box,label="kernel_100_4 - gep_node\ndst_address = dst + index_offset;\n"];
8[color=green,shape=box,label="kernel_100_5 - write_endpoint\nshared_bambu_artificial_ParmMgr_store_write(value, dst_address);\n"];
9[color=burlywood,shape=box,label="kernel_100_7 - add_node\nnext = (unsigned int)(idx + (1u));\n"];
3->9 [label="idx"];
9->3 [label="next"];
}''')
    (dot / "HLS_FSM.dot").write_text('''digraph FSM {
S_0 [label=< S_0 | { kernel_100_1 [phi] kernel_100_6 [shift] kernel_100_3 [read] } | BB2>];
S_1 [label=< S_1 | { kernel_100_3 [read] } | BB2>];
S_2 [label=< S_2 | { kernel_100_5 [write] } | BB2>];
ENTRY -> S_0;
}''')
    (generated / "kernel.v").write_text("""module kernel(); PBI_kernel PBI_kernel_i0(); endmodule
module PBI_kernel();
controller_kernel Controller_i();
datapath_kernel Datapath_i();
endmodule
module controller_kernel();
parameter S_0=1, S_1=2, S_2=4;
reg [2:0] present_state;
always @(*) begin case (present_state) S_0:; S_1:; S_2:; endcase end
endmodule
module datapath_kernel();
ui_add_node_FU #(.WIDTH(32)) fu_kernel_100_7 (.out1(add_out), .in1(out_reg_0_reg_0), .in2(one));
ui_shl_node_FU #(.WIDTH(32)) fu_kernel_100_6 (.out1(shift_out), .in1(out_reg_0_reg_0), .in2(two));
ui_gep_node_FU #(.WIDTH(32)) fu_kernel_100_2 (.out1(src_address), .in1(src), .in2(shift_out));
ui_gep_node_FU #(.WIDTH(32)) fu_kernel_100_4 (.out1(dst_address), .in1(dst), .in2(shift_out));
register_SE #(.WIDTH(32)) reg_0 (.out1(out_reg_0_reg_0), .in1(add_out));
shared_bambu_artificial_ParmMgr_duplex_read_endpoint shared_bambu_artificial_ParmMgr_duplex_read_endpoint_15_i0 (.in4(src_address));
shared_bambu_artificial_ParmMgr_duplex_write_endpoint shared_bambu_artificial_ParmMgr_duplex_write_endpoint_16_i0 (.in4(dst_address));
endmodule
""")
    return generated


class DuplexIterationResultsTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = Path(tempfile.mkdtemp(prefix="g33_checker_", dir=TMP_ROOT))
        self.generated = make_fixture(self.scratch)

    def tearDown(self):
        import shutil
        shutil.rmtree(self.scratch)

    def test_positive_mapping_and_wait_state_exclusion(self):
        result = checker.verify(self.generated, "symmetric")
        self.assertEqual(result["scheduled_loop"], {"id": "loop_2", "bb": 2, "ii": 1, "function_ii": 0})
        self.assertEqual(result["event_definition"]["state_parameters"], ["S_0"])
        self.assertEqual(result["event_definition"]["excluded_load_wait_state_parameters"], ["S_1"])
        self.assertEqual(result["controller"]["hierarchical_present_state"],
                         "kernel.PBI_kernel_i0.Controller_i.present_state")
        self.assertEqual(result["controller"]["datapath_instance"], "Datapath_i")
        self.assertEqual(result["event_definition"]["source_address_signal"], "src_address")

    def test_wrong_loop_and_missing_mapping_fail(self):
        path = self.generated / "panda-temp/after_Frontend::BuildVirtualPhi::kernel(9)[2].raw"
        path.write_text(path.read_text().replace("bloc: 2 loop_id: 1", "bloc: 8 loop_id: 1"))
        with self.assertRaisesRegex(checker.CheckError, "PandA raw loop BB8 does not match scheduled loop BB2"):
            checker.verify(self.generated, "symmetric")

    def test_llvm_and_panda_block_numbers_are_independent(self):
        path = self.generated / "panda-temp/after_CSROA.ll"
        text = path.read_text().replace("2:", "8:").replace("%2", "%8")
        path.write_text(text)
        self.assertEqual(checker.verify(self.generated, "symmetric")["scheduled_loop"]["bb"], 2)

    def test_wrong_function_prefix_with_same_statement_suffix_fails(self):
        for path in (self.generated / "kernel_scheduling.xml",
                     self.generated / "HLS_output/dot/kernel/HLS_scheduling_BB2.dot"):
            path.write_text(path.read_text().replace("kernel_100_", "kernel_101_"))
        with self.assertRaisesRegex(checker.CheckError, "function identity does not match PandA raw function @100"):
            checker.verify(self.generated, "symmetric")

    def test_missing_or_different_panda_operation_identity_fails(self):
        path = self.generated / "HLS_output/dot/kernel/HLS_scheduling_BB2.dot"
        path.write_text(path.read_text().replace("kernel_100_1 - phi_stmt", "kernel_100_8 - phi_stmt"))
        with self.assertRaisesRegex(checker.CheckError, "scheduler XML is missing one or more mapped loop operation identities"):
            checker.verify(self.generated, "symmetric")

    def test_panda_statement_moved_to_different_block_fails(self):
        path = self.generated / "panda-temp/after_Frontend::BuildVirtualPhi::kernel(9)[2].raw"
        path.write_text(path.read_text().replace("@7 assign_stmt parent: @100 bb_index: 2",
                                                "@7 assign_stmt parent: @100 bb_index: 3"))
        with self.assertRaisesRegex(checker.CheckError, "absent from block 3"):
            checker.verify(self.generated, "symmetric")

    def test_altered_feedback_initial_value_or_increment_fails(self):
        path = self.generated / "panda-temp/after_Frontend::BuildVirtualPhi::kernel(9)[2].raw"
        original = path.read_text()
        mutations = [original.replace("def: @21 edge: 2", "def: @22 edge: 2"),
                     original.replace("@22 constant_int_val_node value: 0", "@22 constant_int_val_node value: 4"),
                     original.replace("@46 constant_int_val_node value: 1", "@46 constant_int_val_node value: 2")]
        for mutation in mutations:
            path.write_text(mutation)
            with self.assertRaisesRegex(checker.CheckError, "zero-based phi with unit feedback"):
                checker.verify(self.generated, "symmetric")

    def test_ambiguous_buildvirtualphi_raw_dumps_fail(self):
        source = self.generated / "panda-temp/after_Frontend::BuildVirtualPhi::kernel(9)[2].raw"
        (source.parent / "after_Frontend::BuildVirtualPhi::kernel(10)[3].raw").write_text(source.read_text())
        with self.assertRaisesRegex(checker.CheckError, "exactly one BuildVirtualPhi raw dump"):
            checker.verify(self.generated, "symmetric")

    def test_phi_load_state_without_index_shift_is_not_iteration_start(self):
        path = self.generated / "HLS_output/dot/kernel/HLS_FSM.dot"
        path.write_text(path.read_text().replace("kernel_100_6 [shift] ", ""))
        with self.assertRaisesRegex(checker.CheckError, "no loop state containing induction phi, index shift, and LOAD"):
            checker.verify(self.generated, "symmetric")

    def test_multiple_scheduled_loops_fail(self):
        path = self.generated / "bambu_results.xml"
        path.write_text(path.read_text().replace('</function_1>', '<loop_3 bb="3" ii="1"/></function_1>'))
        with self.assertRaisesRegex(checker.CheckError, "exactly one scheduled loop"):
            checker.verify(self.generated, "symmetric")

    def test_controller_case_label_must_match_declared_state(self):
        path = self.generated / "kernel.v"
        path.write_text(path.read_text().replace("S_2:;", "S_3:;"))
        with self.assertRaisesRegex(checker.CheckError, "state parameters and case labels differ"):
            checker.verify(self.generated, "symmetric")

    def test_ambiguous_load_mapping_fails(self):
        path = self.generated / "HLS_output/dot/kernel/HLS_scheduling_BB2.dot"
        with path.open("a") as stream:
            stream.write('\n10[color=green,shape=box,label="kernel_1_10 - burst_read\\nvalue2 = burst_read(src_address);\\n"];\n')
        with self.assertRaisesRegex(checker.CheckError, "exactly one LOAD op"):
            checker.verify(self.generated, "symmetric")

    def _mapping(self):
        return checker.verify(self.generated, "symmetric")

    def _events(self, rows, event_type="controller_iteration_start", source_base=0x2000):
        mapping = self._mapping()
        path = self.scratch / "events.json"
        path.write_text(json.dumps({"schema_version": 1, "mapping_id": mapping["mapping_id"],
                                    "event_type": event_type, "source_base_bytes": source_base,
                                    "events": rows}))
        return mapping, path

    @staticmethod
    def _valid_rows():
        return [{"iteration": index, "cycle": index + 10, "state": "S_0",
                 "loop_index": index, "source_address": 0x2000 + 4 * index}
                for index in range(256)]

    def test_valid_events_report_effective_ii(self):
        rows = self._valid_rows()
        rows[2]["cycle"] = 13
        for index in range(3, 256):
            rows[index]["cycle"] = rows[index - 1]["cycle"] + 1
        mapping, path = self._events(rows)
        result = checker.verify(self.generated, "symmetric", path)
        self.assertEqual(result["observed_events"]["intervals_cycles"][:2], [1, 2])
        self.assertEqual(result["observed_events"]["effective_ii_average_cycles"], 1 + 1 / 255)
        self.assertEqual(result["scheduled_loop"]["ii"], 1)

    def test_duplicate_missing_and_out_of_order_events_fail(self):
        duplicate = self._valid_rows()
        duplicate[1]["iteration"] = 0
        missing = self._valid_rows()[:-1]
        out_of_order = self._valid_rows()
        out_of_order[0], out_of_order[1] = out_of_order[1], out_of_order[0]
        cases = [duplicate, missing, out_of_order]
        for rows in cases:
            _, path = self._events(rows)
            with self.assertRaises(checker.CheckError):
                checker.verify(self.generated, "symmetric", path)

    def test_boolean_iteration_indices_fail(self):
        for field in ("iteration", "loop_index"):
            rows = self._valid_rows()
            rows[0][field] = False
            _, path = self._events(rows)
            with self.assertRaisesRegex(checker.CheckError, "non-boolean integer"):
                checker.verify(self.generated, "symmetric", path)

    def test_bad_timestamps_and_load_only_intervals_fail(self):
        rows = self._valid_rows()
        rows[1]["cycle"] = rows[0]["cycle"]
        _, path = self._events(rows)
        with self.assertRaisesRegex(checker.CheckError, "timestamp"):
            checker.verify(self.generated, "symmetric", path)
        _, path = self._events(self._valid_rows(), event_type="load_completion")
        with self.assertRaisesRegex(checker.CheckError, "not LOAD/transport completion"):
            checker.verify(self.generated, "symmetric", path)

    def test_wrong_address_state_and_source_base_fail(self):
        rows = self._valid_rows()
        rows[17]["source_address"] += 4
        _, path = self._events(rows)
        with self.assertRaisesRegex(checker.CheckError, "source_address"):
            checker.verify(self.generated, "symmetric", path)
        rows = self._valid_rows()
        rows[17]["state"] = "S_1"
        _, path = self._events(rows)
        with self.assertRaisesRegex(checker.CheckError, "outside the mapped iteration-start set"):
            checker.verify(self.generated, "symmetric", path)
        _, path = self._events(self._valid_rows(), source_base=0x3000)
        with self.assertRaisesRegex(checker.CheckError, "source_base_bytes"):
            checker.verify(self.generated, "symmetric", path)

    def test_load_completion_metrics_are_separate(self):
        log = self.scratch / "load.log"
        log.write_text("".join(
            f"DUPLEX_LOAD_EVENT CASE=COPY_PARALLELISM_256 INDEX={index} COUNT=256 CYCLE={index * 2} READ_B_MAX=16 READ_OUTSTANDING=1 READ_FIFO_DEPTH=64\n"
            for index in range(256)))
        result = checker.verify(self.generated, "symmetric", load_log=log)
        self.assertEqual(result["load_completion_metrics"]["event_count"], 256)
        self.assertEqual(result["load_completion_metrics"]["average_interval_cycles"], 2)
        self.assertNotIn("observed_events", result)


if __name__ == "__main__":
    unittest.main(verbosity=2)
