#!/usr/bin/env python3
"""Offline mutation tests for the same-bundle duplex handoff checker."""
from __future__ import annotations

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
os.environ["TMPDIR"] = str(TMP_ROOT)
os.environ["TMP"] = str(TMP_ROOT)
os.environ["TEMP"] = str(TMP_ROOT)
sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_duplex_handoff as checker
from test_duplex_effect_analysis_artifacts import RAW_DISTINCT, RECORD_DISTINCT

INDEPENDENT_SOURCE_RAW = RAW_DISTINCT.replace(
    "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44",
    "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35")

TOP = "distinct_bases_source_first"
IDS = {"CONFIGURE_READ": 101, "BURST_READ": 100, "CONFIGURE_WRITE": 102,
       "STORE_WRITE": 103, "DRAIN_WRITE": 104}
FUS = {"CONFIGURE_READ": "shared_bambu_artificial_ParmMgr_duplex_read_endpoint",
       "BURST_READ": "shared_bambu_artificial_ParmMgr_duplex_read_endpoint",
       "CONFIGURE_WRITE": "shared_bambu_artificial_ParmMgr_duplex_write_endpoint",
       "STORE_WRITE": "shared_bambu_artificial_ParmMgr_duplex_write_endpoint",
       "DRAIN_WRITE": "shared_bambu_artificial_ParmMgr_duplex_write_endpoint"}


def raw_ir(top: str = TOP, *, operations: bool = True, duplicate: str | None = None,
           sever_copy: bool = False, independent_dependency: bool = False,
           break_virtual: bool = False) -> str:
    body = (
        "bloc: 0 loop_id: 0 pred: ENTRY stmt: @101 stmt: @102 succ: 1 "
        "bloc: 1 loop_id: 1 pred: 0 stmt: @100 stmt: @103 succ: 2 "
        "bloc: 2 loop_id: 0 pred: 1 stmt: @104 succ: EXIT"
    ) if operations else "bloc: 0 loop_id: 0 pred: ENTRY succ: EXIT"
    lines = [f'@1 function_val_node name: @2 body: @3',
             f'@2 identifier_node strg: "{top}"',
             f'@3 statement_list_node {body}']
    if not operations:
        return "\n".join(lines) + "\n"
    names = {"CONFIGURE_READ": "configure_read", "BURST_READ": "burst_read",
             "CONFIGURE_WRITE": "configure_write", "STORE_WRITE": "store_write",
             "DRAIN_WRITE": "drain_write"}
    fn_ids = {label: 200 + index for index, label in enumerate(names)}
    addr_ids = {label: 210 + index for index, label in enumerate(names)}
    name_ids = {label: 220 + index for index, label in enumerate(names)}
    for label, name in names.items():
        lines.extend([f'@{name_ids[label]} identifier_node strg: "shared_bambu_artificial_ParmMgr_{name}"',
                      f'@{fn_ids[label]} function_val_node name: @{name_ids[label]}',
                      f'@{addr_ids[label]} addr_node op: @{fn_ids[label]}'])
    calls = {
        "CONFIGURE_READ": "@101 call_stmt parent: @1 bb_index: 0 fn: @210 arg: @1 arg: @2 vdef: @301",
        "BURST_READ": "@100 assign_stmt parent: @1 bb_index: 1 op: @400 op: @401 vuse: @301 vdef: @302",
        "CONFIGURE_WRITE": "@102 call_stmt parent: @1 bb_index: 0 fn: @212 arg: @1 arg: @2 vdef: @303",
        "STORE_WRITE": "@103 call_stmt parent: @1 bb_index: 1 fn: @213 arg: @1 arg: @2 arg: @PAYLOAD arg: @3 vuse: @303 vuse: @302 vdef: @304",
        "DRAIN_WRITE": "@104 call_stmt parent: @1 bb_index: 2 fn: @214 arg: @1 vuse: @304",
    }
    if sever_copy:
        calls["STORE_WRITE"] = calls["STORE_WRITE"].replace("@PAYLOAD", "@500")
    elif independent_dependency:
        calls["STORE_WRITE"] = calls["STORE_WRITE"].replace("@PAYLOAD", "@400")
    else:
        calls["STORE_WRITE"] = calls["STORE_WRITE"].replace("@PAYLOAD", "@400")
    if break_virtual:
        calls["BURST_READ"] = calls["BURST_READ"].replace("vuse: @301 ", "")
    lines.extend(calls.values())
    lines.extend(["@400 ssa_node def_stmt: @100", "@401 call_node fn: @211 arg: @2",
                  "@500 ssa_node var: @501", "@501 argument_val_node name: @502",
                  "@502 identifier_node strg: \"unrelated\""])
    if duplicate:
        original = calls[duplicate]
        dup_id = 600
        line = original.replace(f"@{IDS[duplicate]} ", f"@{dup_id} ", 1)
        lines.append(line)
        lines[2] = lines[2].replace("succ: 1", f"stmt: @{dup_id} succ: 1")
    return "\n".join(lines) + "\n"


def dot(top: str, *, ids: dict[str, int] = IDS) -> str:
    names = {"CONFIGURE_READ": "configure_read", "BURST_READ": "burst_read",
             "CONFIGURE_WRITE": "configure_write", "STORE_WRITE": "store_write",
             "DRAIN_WRITE": "drain_write"}
    rows = ["digraph G {"]
    for index, (label, op_id) in enumerate(ids.items()):
        rows.append(f'  {index}[shape=box,label="{top}_1_{op_id} - shared_bambu_artificial_ParmMgr_{names[label]}\\noperation"];')
    rows.extend(["  0->1 [label=\"edge\"];", "}"])
    return "\n".join(rows) + "\n"


def create_fixture(root: Path, *, kind: str = "copy", top: str = TOP,
                   duplicate: str | None = None, sever_copy: bool = False,
                   independent_dependency: bool = False, break_virtual: bool = False) -> None:
    pt = root / "panda-temp"
    dot_dir = root / "HLS_output/dot" / top
    pt.mkdir(parents=True, exist_ok=True)
    dot_dir.mkdir(parents=True, exist_ok=True)
    source_raw = RAW_DISTINCT if kind == "copy" else INDEPENDENT_SOURCE_RAW
    if sever_copy:
        source_raw = INDEPENDENT_SOURCE_RAW
    (pt / "before_Frontend::InterfaceInfer.raw").write_text(source_raw)
    (pt / "after_Frontend::InterfaceInfer.raw").write_text(raw_ir(top, operations=True))
    (pt / f"after_Frontend::BuildVirtualPhi::{top}(1)[1].raw").write_text(
        raw_ir(top, operations=True, duplicate=duplicate,
               sever_copy=sever_copy or (kind == "independent" and not independent_dependency),
               independent_dependency=independent_dependency,
               break_virtual=break_virtual))
    xml = ET.Element("hls")
    scheduling = ET.SubElement(xml, "scheduling")
    for label, op_id in IDS.items():
        ET.SubElement(scheduling, "scheduling_constraints", {
            "vertex_name": f"{top}_1_{op_id}",
            "cstep": {"CONFIGURE_READ": "1", "CONFIGURE_WRITE": "1", "BURST_READ": "2",
                      "STORE_WRITE": "3", "DRAIN_WRITE": "4"}[label],
            "fu_name": FUS[label], "fu_index": "0"})
    ET.ElementTree(xml).write(root / f"{top}_scheduling.xml", encoding="unicode")
    for name in ("OP_DFG.dot", "OP_ADG.dot", "HLS_scheduling.dot"):
        (dot_dir / name).write_text(dot(top))
    (root / f"{top}.v").write_text(f"module {top}(input clk); endmodule\n")
    arch = ET.Element("architecture")
    function = ET.SubElement(arch, "function", {"name": top})
    bundles = ET.SubElement(function, "bundles")
    ET.SubElement(bundles, "bundle", {"name": "shared", "mode": "m_axi",
                                           "max_read_burst_length": "16",
                                           "max_write_burst_length": "16"})
    ET.ElementTree(arch).write(pt / "architecture.xml", encoding="unicode")
    (root / "stderr.log").write_text("AXI_BURST_DUPLEX_PROOF " + json.dumps(RECORD_DISTINCT) + "\n")
    (root / "stdout.log").write_text("activation warning\n")


class DuplexHandoffTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory(prefix="duplex-handoff-", dir=TMP_ROOT)
        self.root = Path(self.tmp.name)
        create_fixture(self.root)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def run_validated(self, kind: str = "copy") -> dict[str, object]:
        with mock.patch.object(checker, "verify_success_record", wraps=checker.verify_success_record) as proof, \
                mock.patch.object(checker, "check_production_duplex") as production:
            result = checker.validate(self.root, TOP, "symmetric", kind)
            self.assertEqual(proof.call_args.kwargs["require_load_store_data_dependency"], kind == "copy")
            production.assert_called_once()
            return result

    def test_valid_copy_reports_real_mapping_chains_and_proofs(self) -> None:
        result = self.run_validated()
        self.assertEqual(result["operations"]["BURST_READ"]["operation_id"], IDS["BURST_READ"])
        self.assertEqual(result["emitted_load_store_dependency"]["depends"], True)
        self.assertIn(("CONFIGURE_READ", "BURST_READ"),
                      {(edge["from"], edge["to"]) for edge in result["virtual_chains"]})
        self.assertIn("cross_direction_virtual_arcs", result)

    def test_independent_case_requires_full_absence_proof(self) -> None:
        create_fixture(self.root, kind="independent")
        result = self.run_validated("independent")
        self.assertFalse(result["emitted_load_store_dependency"]["depends"])
        self.assertTrue(result["emitted_load_store_dependency"]["traversal_complete"])

    def test_missing_duplicate_operation_and_wrong_top_fail(self) -> None:
        self.assertRejected(lambda: checker.validate(self.root, "not_kernel", "symmetric", "copy"),
                            "post-BuildVirtualPhi raw")
        create_fixture(self.root)
        for path in (self.root / "panda-temp/after_Frontend::InterfaceInfer.raw",
                     self.root / f"panda-temp/after_Frontend::BuildVirtualPhi::{TOP}(1)[1].raw"):
            path.write_text(path.read_text().replace(f'@2 identifier_node strg: "{TOP}"',
                                                     '@2 identifier_node strg: "other_top"'))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "exactly one selected function")
        create_fixture(self.root)
        for path in (self.root / "panda-temp/after_Frontend::InterfaceInfer.raw",
                     self.root / f"panda-temp/after_Frontend::BuildVirtualPhi::{TOP}(1)[1].raw"):
            text = path.read_text().replace("stmt: @103 ", "").replace("@103 call_stmt", "@603 call_stmt")
            path.write_text(text)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "exactly one of each duplex op")
        create_fixture(self.root, duplicate="STORE_WRITE")
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "exactly one")
        create_fixture(self.root)
        raw = self.root / f"panda-temp/after_Frontend::BuildVirtualPhi::{TOP}(1)[1].raw"
        raw.write_text(raw_ir(TOP, operations=True).replace("@103 call_stmt", "@603 call_stmt")
                       .replace("stmt: @103", "stmt: @603"))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "operation IDs disagree")

    def test_mismatched_dot_or_schedule_ids_fail(self) -> None:
        path = self.root / f"HLS_output/dot/{TOP}/OP_ADG.dot"
        path.write_text(path.read_text().replace(f"{TOP}_1_100", f"{TOP}_1_999"))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "map BURST_READ")
        create_fixture(self.root)
        sched = self.root / f"{TOP}_scheduling.xml"
        sched.write_text(sched.read_text().replace(f"{TOP}_1_100", f"{TOP}_1_999"))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "does not contain raw")

    def test_shared_or_swapped_fu_mappings_fail(self) -> None:
        sched = self.root / f"{TOP}_scheduling.xml"
        text = sched.read_text().replace(FUS["BURST_READ"], FUS["STORE_WRITE"])
        sched.write_text(text)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "distinct coherent")
        create_fixture(self.root)
        text = sched.read_text().replace(FUS["CONFIGURE_READ"], FUS["CONFIGURE_WRITE"])
        sched.write_text(text)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "distinct coherent")

    def test_global_read_write_fu_swap_is_rejected(self) -> None:
        sched = self.root / f"{TOP}_scheduling.xml"
        text = sched.read_text()
        read_name = "shared_bambu_artificial_ParmMgr_duplex_read_endpoint"
        write_name = "shared_bambu_artificial_ParmMgr_duplex_write_endpoint"
        text = text.replace(read_name, "__READ_ENDPOINT__").replace(write_name, read_name)
        sched.write_text(text.replace("__READ_ENDPOINT__", write_name))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "READ operations must map to the duplex read endpoint")

    def test_direction_fu_index_mismatch_is_rejected(self) -> None:
        sched = self.root / f"{TOP}_scheduling.xml"
        scheduling = ET.parse(sched)
        record = next(element for element in scheduling.getroot().iter("scheduling_constraints")
                      if element.get("vertex_name") == f"{TOP}_1_100")
        record.set("fu_index", "1")
        scheduling.write(sched, encoding="unicode")
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "distinct coherent function-unit instances")

    def test_copy_requires_load_scheduled_before_store(self) -> None:
        sched = self.root / f"{TOP}_scheduling.xml"
        sched.write_text(sched.read_text().replace(
            f'vertex_name="{TOP}_1_100" cstep="2"',
            f'vertex_name="{TOP}_1_100" cstep="4"'))
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "copy case must schedule BURST_READ before STORE_WRITE")

    def test_bad_configure_and_store_drain_scheduling_fail(self) -> None:
        sched = self.root / f"{TOP}_scheduling.xml"
        text = sched.read_text().replace(f'vertex_name="{TOP}_1_101" cstep="1"',
                                         f'vertex_name="{TOP}_1_101" cstep="5"')
        sched.write_text(text)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "CONFIGURE_READ is not scheduled")
        create_fixture(self.root)
        text = sched.read_text().replace(f'vertex_name="{TOP}_1_104" cstep="4"',
                                         f'vertex_name="{TOP}_1_104" cstep="2"')
        sched.write_text(text)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "STORE_WRITE is not scheduled")

    def test_broken_virtual_chain_and_copy_dependency_fail(self) -> None:
        create_fixture(self.root, break_virtual=True)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                            "broken virtual handoff")
        create_fixture(self.root, sever_copy=True)
        with mock.patch.object(checker, "verify_success_record"), \
                mock.patch.object(checker, "check_production_duplex"):
            self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"),
                                "no longer depends")

    def test_independent_case_rejects_introduced_dependency(self) -> None:
        create_fixture(self.root, kind="independent", independent_dependency=True)
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "independent"),
                            "independent case emitted a STORE_WRITE payload dependency on BURST_READ",
                            mock_proof=False)

    def test_missing_or_empty_required_artifacts_fail_closed(self) -> None:
        required = self.root / "panda-temp/before_Frontend::InterfaceInfer.raw"
        saved = required.read_text()
        required.unlink()
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "missing")
        required.write_text("")
        self.assertRejected(lambda: checker.validate(self.root, TOP, "symmetric", "copy"), "nonempty")
        required.write_text(saved)

    def assertRejected(self, callback, message: str, *, mock_proof: bool = True) -> None:
        proof_patch = mock.patch.object(checker, "verify_success_record") if mock_proof else mock.patch.object(
            checker, "verify_success_record", wraps=checker.verify_success_record)
        with proof_patch, mock.patch.object(checker, "check_production_duplex"), \
                self.assertRaisesRegex((ValueError, checker.CheckError), message):
            callback()


if __name__ == "__main__":
    unittest.main(verbosity=2)
