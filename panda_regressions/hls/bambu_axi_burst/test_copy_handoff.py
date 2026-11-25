#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import tempfile
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

import check_copy_handoff as checker

REPO_ROOT = Path(__file__).resolve().parents[3]
SCRATCH = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
SCRATCH.mkdir(parents=True, exist_ok=True)


def raw_fixture() -> str:
    rows = [
        '@1 function_val_node name: @2 body: @3', '@2 identifier_node strg: "copy_distinct_buffers"',
        '@3 statement_list_node bloc: 1 loop_id: 0 pred: ENTRY succ: 2 bloc: 2 loop_id: 1 pred: 1 succ: 2 succ: 3 bloc: 3 loop_id: 0 pred: 2 succ: EXIT',
    ]
    labels = {
        "CONFIGURE_READ": "input_bambu_artificial_ParmMgr_configure_read",
        "BURST_READ": "input_bambu_artificial_ParmMgr_burst_read",
        "CONFIGURE_WRITE": "output_bambu_artificial_ParmMgr_configure_write",
        "STORE_WRITE": "output_bambu_artificial_ParmMgr_store_write",
        "DRAIN_WRITE": "output_bambu_artificial_ParmMgr_drain_write",
    }
    nums = {k: i + 10 for i, k in enumerate(labels)}
    for index, (label, callee) in enumerate(labels.items()):
        fnode, ident, addr = 30 + index, 40 + index, 50 + index
        rows.extend((f'@{ident} identifier_node strg: "{callee}"',
                     f'@{fnode} function_val_node name: @{ident}',
                     f'@{addr} addr_node op: @{fnode}'))
        bb = 2 if label in ("BURST_READ", "STORE_WRITE") else (3 if label == "DRAIN_WRITE" else 1)
        args = " arg: @70" if label == "STORE_WRITE" else ""
        virtual = {"CONFIGURE_READ": " vdef: @80", "BURST_READ": " vuse: @80 vdef: @81",
                   "CONFIGURE_WRITE": " vdef: @82", "STORE_WRITE": " vuse: @82 vdef: @83",
                   "DRAIN_WRITE": " vuse: @84 vdef: @85"}[label]
        rows.append(f'@{nums[label]} call_stmt parent: @1 bb_index: {bb} fn: @{addr}{args}{virtual}')
    rows.append('@70 ssa_node def_stmt: @11')
    return "\n".join(rows) + "\n"


def wrapper_fixture() -> str:
    ports_read = ".start(start_port), .done(burst_done), .m_axi_arready(p_m_axi_input_arready), .m_axi_rdata(p_m_axi_input_rdata), .m_axi_arid(x)"
    ports_write = ".start(start_port), .done(burst_done), .m_axi_awaddr(burst_awaddr), .m_axi_awready(p_m_axi_output_awready), .m_axi_bready(burst_bready), .m_axi_awid(x)"
    return f'''module input_bambu_artificial_ParmMgr_modgen;
 wire burst_done, start_port, x; input [5:0] p_m_axi_input_rid;
 assign p_m_axi_input_arcache = 0; assign p_m_axi_input_awcache = 0; assign p_m_axi_input_bready = 0;
 wire burst_rid = |p_m_axi_input_rid;
 assign p_m_axi_input_arid = {{{{(BITSIZE_arid-1){{1'b0}}}}, burst_arid}};
 MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .BITSIZE_m_axi_arid(1)) e({ports_read});
endmodule
module output_bambu_artificial_ParmMgr_modgen;
 wire burst_done, burst_bready, start_port, x; wire [31:0] burst_awaddr; input [5:0] p_m_axi_output_bid;
 assign p_m_axi_output_awcache = 4'b0010; assign p_m_axi_output_arcache = 0;
 assign p_m_axi_output_awaddr = burst_awaddr;
 assign p_m_axi_output_bready = burst_bready;
 wire burst_bid = |p_m_axi_output_bid;
 MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .BITSIZE_in5(32), .BITSIZE_m_axi_awid(1)) e({ports_write});
endmodule
module datapath_copy_distinct_buffers;
 output [3:0] p_m_axi_output_awcache; output p_m_axi_output_bready;
 wire rstart, wstart, rdone, wdone;
 input_bambu_artificial_ParmMgr_modgen read_if(.start_port(rstart), .done_port(rdone));
 output_bambu_artificial_ParmMgr_modgen write_if(.start_port(wstart), .done_port(wdone));
endmodule
module copy_distinct_buffers;
 output [3:0] p_m_axi_output_awcache; output p_m_axi_output_bready;
endmodule
'''


def output_fixture(root: Path) -> None:
    pt = root / "panda-temp"
    dot = root / "HLS_output/dot/copy_distinct_buffers"
    pt.mkdir(parents=True)
    dot.mkdir(parents=True)
    pre = raw_fixture()
    before = "\n".join(line for line in pre.splitlines() if "call_stmt" not in line) + "\n"
    for name in ("before_Frontend::InterfaceInfer.raw", "after_Frontend::InterfaceInfer.raw",
                 "after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw"):
        (pt / name).write_text(before if name.startswith("before_") else pre)
    for name in ("OP_DFG.dot", "OP_ADG.dot", "OP_ODG.dot", "OP_FFLSAODG.dot", "HLS_scheduling.dot"):
        labels = list(checker.OPS.values())
        nodes = "\n".join(f'{index} [label="copy_distinct_buffers_1_{10 + index} - {label}"];'
                           for index, label in enumerate(labels))
        edges = "\n".join(f"{index} -> {index + 1};" for index in range(len(labels) - 1))
        (dot / name).write_text(f"digraph G {{\n{nodes}\n{edges}\n}}\n")
    rows = []
    for i, label in enumerate(("CONFIGURE_READ", "BURST_READ", "CONFIGURE_WRITE", "STORE_WRITE", "DRAIN_WRITE")):
        node = 10 + i
        cstep = (0, 1, 0, 2, 3)[i]
        rows.append(f'<scheduling_constraints vertex_name="copy_distinct_buffers_1_{node}" cstep="{cstep}" fu_name="{label}_FU" fu_index="{i}"/>')
    (root / "copy_distinct_buffers_scheduling.xml").write_text("<hls><scheduling>" + "".join(rows) + "</scheduling></hls>")
    (root / "copy_distinct_buffers.v").write_text(wrapper_fixture())


class CopyHandoffTests(unittest.TestCase):
    def test_complete_fixture_reports_required_calls_and_copy_value(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            report = checker.validate(root)
            self.assertTrue(report["load_to_store"]["proved"])
            self.assertEqual(set(report["calls"]), set(checker.OPS))
            self.assertTrue(report["direction_chains"]["write"]["evidence"][-1]["store_precedes_drain_on_path"])

    def test_pre_interface_rejects_any_parmmgr_operation(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            path = root / "panda-temp/before_Frontend::InterfaceInfer.raw"
            path.write_text(path.read_text() + "@99 call_stmt parent: @1 bb_index: 1 fn: @50\n")
            with self.assertRaisesRegex(ValueError, "before InterfaceInfer unexpectedly"):
                checker.validate(root)

    def test_missing_post_transform_artifact_fails_closed(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            (root / "panda-temp/after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw").unlink()
            with self.assertRaisesRegex(ValueError, "post-BuildVirtualPhi"):
                checker.validate(root)

    def test_missing_load_to_store_value_fails(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            path = root / "panda-temp/after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw"
            path.write_text(path.read_text().replace("arg: @70", ""))
            with self.assertRaisesRegex(ValueError, "LOAD-to-STORE"):
                checker.validate(root)

    def test_required_configure_store_drain_disappearance_fails(self):
        removals = (("output_bambu_artificial_ParmMgr_configure_write", "CONFIGURE_WRITE"),
                    ("output_bambu_artificial_ParmMgr_store_write", "STORE_WRITE"),
                    ("output_bambu_artificial_ParmMgr_drain_write", "DRAIN_WRITE"))
        for callee, label in removals:
            with self.subTest(callee=callee), tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
                root = Path(temp)
                output_fixture(root)
                path = root / "panda-temp/after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw"
                path.write_text("\n".join(line for line in path.read_text().splitlines() if callee not in line))
                with self.assertRaisesRegex(ValueError, re.escape(f"expected one {label}")):
                    checker.validate(root)

    def test_required_virtual_edges_fail_when_only_the_edge_is_removed(self):
        for removed, replacement in ((" vdef: @80", ""), (" vuse: @82", "")):
            with self.subTest(removed=removed), tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
                root = Path(temp)
                output_fixture(root)
                path = root / "panda-temp/after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw"
                path.write_text(path.read_text().replace(removed, replacement))
                with self.assertRaisesRegex(ValueError, "virtual def/use edge"):
                    checker.validate(root)

    def test_store_to_drain_cfg_ordering_fails_when_exit_edge_is_removed(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            path = root / "panda-temp/after_Frontend::BuildVirtualPhi::copy_distinct_buffers.raw"
            path.write_text(path.read_text().replace("succ: 2 succ: 3 bloc: 3", "succ: 2 bloc: 3"))
            with self.assertRaisesRegex(ValueError, "path from STORE_WRITE"):
                checker.validate(root)

    def test_empty_dot_artifact_fails_closed(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            root = Path(temp)
            output_fixture(root)
            path = root / "HLS_output/dot/copy_distinct_buffers/OP_DFG.dot"
            path.write_text("digraph G {}\n")
            with self.assertRaisesRegex(ValueError, "no nodes or edges"):
                checker.validate(root)

    def test_invalid_load_to_store_schedule_fails_with_ir_and_edges_intact(self):
        schedules = (
            {"CONFIGURE_READ": 1, "BURST_READ": 2, "CONFIGURE_WRITE": 1,
             "STORE_WRITE": 0, "DRAIN_WRITE": 4},
            {"CONFIGURE_READ": 0, "BURST_READ": 1, "CONFIGURE_WRITE": 3,
             "STORE_WRITE": 2, "DRAIN_WRITE": 4},
            {"CONFIGURE_READ": 0, "BURST_READ": 3, "CONFIGURE_WRITE": 1,
             "STORE_WRITE": 2, "DRAIN_WRITE": 4},
        )
        for schedule in schedules:
            with self.subTest(schedule=schedule), tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
                root = Path(temp)
                output_fixture(root)
                path = root / "copy_distinct_buffers_scheduling.xml"
                xml = ET.parse(path)
                for row in xml.getroot().iter("scheduling_constraints"):
                    label = row.attrib["fu_name"][:-3]
                    row.set("cstep", str(schedule[label]))
                xml.write(path)
                with self.assertRaisesRegex(ValueError, "scheduled before"):
                    checker.validate(root)

    def test_wrapper_rejects_cross_wire_awcache_bready_and_split_profile(self):
        text = wrapper_fixture()
        mutations = (
            (text.replace(".start(start_port)", ".start(other_start)", 1),
             "start/done cross-wire or missing"),
            (text.replace(".done(burst_done)", ".done(other_done)", 1),
             "start/done cross-wire or missing"),
            (text.replace("assign p_m_axi_output_awcache = 4'b0010", "assign p_m_axi_output_awcache = 0"),
             "AWCACHE is not 0010"),
            (text.replace(".m_axi_bready(burst_bready)", ".m_axi_bready(1'b0)"),
             "BREADY is not driven by write engine"),
            (text.replace(".start_port(wstart)", ".start_port(rstart)"),
             "read/write start_port are missing or cross-connected at top-level"),
            (text.replace(".done_port(wdone)", ".done_port(rdone)"),
             "read/write done_port are missing or cross-connected at top-level"),
            (text.replace(".MAX_OUTSTANDING(1), .FIFO_DEPTH(32)", ".MAX_OUTSTANDING(2), .FIFO_DEPTH(32)", 1),
             "MinimalAXI4MasterPipelined same instance missing MAX_OUTSTANDING=1"),
            (text.replace(".B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)",
                          ".MAX_OUTSTANDING(1), .FIFO_DEPTH(32)", 1),
             "MinimalAXI4MasterPipelined same instance missing B_MAX=16"),
            (text.replace(".BITSIZE_in5(32)", ".BITSIZE_in5(1)"),
             "writer count input BITSIZE_in5 must be 32 bits"),
            (text.replace(".m_axi_awaddr(burst_awaddr)", ".m_axi_awaddr(unconnected_addr)"),
             "write engine AWADDR is not connected to burst_awaddr"),
            (text.replace("assign p_m_axi_output_awaddr = burst_awaddr;",
                          "assign p_m_axi_output_awaddr = 32'b0;"),
             "write AWADDR is not driven by write engine"),
        )
        for mutated, expected_message in mutations:
            with self.subTest(expected_message=expected_message), tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
                path = Path(temp) / "x.v"
                path.write_text(mutated)
                with self.assertRaisesRegex(ValueError, re.escape(expected_message)):
                    checker.check_wrapper(path)

    def test_wrapper_accepts_whitespace_variation_in_rtl_assignments(self):
        text = wrapper_fixture()
        text = text.replace("assign p_m_axi_output_awcache = 4'b0010;",
                            "assign\n  p_m_axi_output_awcache\t =\n    4'b0010 ;")
        text = text.replace("assign p_m_axi_output_awaddr = burst_awaddr;",
                            "assign p_m_axi_output_awaddr\n =\tburst_awaddr ;")
        text = text.replace("wire burst_rid = |p_m_axi_input_rid;",
                            "wire\n burst_rid\t=\n | p_m_axi_input_rid ;")
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            path = Path(temp) / "x.v"
            path.write_text(text)
            checker.check_wrapper(path)

    def test_wrapper_rejects_extra_unparameterized_writer_instance(self):
        text = wrapper_fixture().replace("module copy_distinct_buffers;",
                                         "module copy_distinct_buffers;\n MinimalAXI4MasterPipelinedWrite malformed();")
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            path = Path(temp) / "x.v"
            path.write_text(text)
            with self.assertRaisesRegex(ValueError, "exactly one writer engine"):
                checker.check_wrapper(path)

    def test_wrapper_rejects_disconnected_writer_awaddr_input(self):
        text = wrapper_fixture()
        mutated = text.replace(".m_axi_awaddr(burst_awaddr)", ".m_axi_awaddr(unconnected_addr)")
        self.assertIn("assign p_m_axi_output_awaddr = burst_awaddr;", mutated)
        with tempfile.TemporaryDirectory(dir=SCRATCH) as temp:
            path = Path(temp) / "x.v"
            path.write_text(mutated)
            with self.assertRaisesRegex(ValueError, "AWADDR is not connected"):
                checker.check_wrapper(path)


if __name__ == "__main__":
    unittest.main(verbosity=2)
