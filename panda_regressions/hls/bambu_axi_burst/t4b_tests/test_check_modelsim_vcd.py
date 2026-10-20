#!/usr/bin/env python3
"""Deterministic parser regressions; no simulator installation required."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[3]
if not (REPO_ROOT / "panda_regressions").is_dir():
    raise RuntimeError(f"unexpected repository root for T4B tests: {REPO_ROOT}")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
CHECKER = HERE / "check_modelsim_vcd.py"
sys.path.insert(0, str(HERE))
import t4b_vcd
NAMES = ("issued", "returned", "consumed", "fifo_count",
         "burst_outstanding", "region_count")
IDS = dict(zip(NAMES, ("!", '"', "#", "$", "%", "&")))


def make_trace(final_updates, count=4, initial_unknown=True):
    lines = ["$scope module top $end", "$scope module burst_engine $end"]
    lines.extend(f"$var wire 32 {IDS[name]} {name} $end" for name in NAMES)
    lines.extend(("$upscope $end", "$upscope $end", "$enddefinitions $end", "$dumpvars"))
    if initial_unknown:
        lines.extend(f"bxxxxxxxx {IDS[name]}" for name in NAMES)
    lines.extend(("$end", "#1"))
    values = {"issued": count, "returned": count, "consumed": count,
              "fifo_count": 0, "burst_outstanding": 0, "region_count": count}
    lines.extend(f"b{value:032b} {IDS[name]}" for name, value in values.items())
    lines.append("#2")
    lines.extend(f"b{value} {IDS[name]}" for name, value in final_updates.items())
    return "\n".join(lines) + "\n"


class VcdCheckerTests(unittest.TestCase):
    def invoke(self, trace, n):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="vcd-check-", dir=TMP_ROOT) as temporary:
            root = Path(temporary)
            vcd, report = root / "trace.vcd", root / "coverage.tsv"
            vcd.write_text(trace, encoding="ascii")
            env = os.environ.copy()
            result = subprocess.run(
                [sys.executable, str(CHECKER), str(vcd), str(n), str(report)],
                text=True, capture_output=True, env=env, check=False)
            report_text = report.read_text(encoding="ascii") if report.exists() else ""
            return result, report_text

    def test_known_final_values_pass_after_uninitialized_x(self):
        final = {"issued": "100", "returned": "100", "consumed": "100",
                 "fifo_count": "0", "burst_outstanding": "0", "region_count": "100"}
        result, report = self.invoke(make_trace(final), 4)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("coverage_status=PASS", report)

    def test_zero_length_trace_passes(self):
        final = {name: "0" for name in NAMES}
        trace = make_trace(final, count=0)
        result, report = self.invoke(trace, 0)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("expected_count=0", report)

    def test_negative_n_legacy_cli_keeps_historical_zero_clamp(self):
        final = {name: "0" for name in NAMES}
        result, report = self.invoke(make_trace(final, count=0), -3)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("N=-3", report)
        self.assertIn("expected_count=0", report)

    def test_final_x_or_z_cannot_reuse_prior_expected_count(self):
        for unknown in ("x", "z"):
            with self.subTest(unknown=unknown):
                final = {"issued": "x" * 32, "returned": "z" * 32,
                         "consumed": "100", "fifo_count": "0",
                         "burst_outstanding": "0", "region_count": "100"}
                result, report = self.invoke(make_trace(final), 4)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("coverage_status=FAIL", report)
                self.assertIn("final_issued=None", report)
                self.assertIn("final_returned=None", report)

    def test_credit_limited_reservation_stays_frozen_when_credit_rises(self):
        pre = {"remaining_beats": 10, "next_address": 0x1ff0,
               "boundary_beats": 4, "fifo_credit": 2, "next_burst_length": 2}
        captured = {"m_axi_arvalid_reg": 1, "m_axi_araddr_reg": 0x1ff0,
                    "m_axi_arlen_reg": 1}
        reservation = t4b_vcd._check_reservation(pre, captured, 16)
        # FIFO credit rises while ARREADY is low. The registered request must
        # retain the original two-beat reservation rather than grow to four.
        stalled = {"m_axi_arvalid_reg": 1, "m_axi_araddr_reg": 0x1ff0,
                   "m_axi_arlen_reg": 1}
        self.assertEqual(reservation["beats"], 2)
        t4b_vcd._check_frozen(reservation, stalled)
        with self.assertRaisesRegex(ValueError, "changed while stalled"):
            t4b_vcd._check_frozen(reservation, {**stalled, "m_axi_arlen_reg": 3})

    def test_credit_reservation_is_checked_from_vcd_pre_edge_snapshot(self):
        names = ("clock", "remaining_beats", "next_address", "boundary_beats",
                 "fifo_credit", "next_burst_length", "m_axi_arvalid_reg",
                 "m_axi_araddr_reg", "m_axi_arlen_reg")
        widths = {"remaining_beats": 32, "next_address": 32, "boundary_beats": 32,
                  "fifo_credit": 32, "next_burst_length": 32,
                  "m_axi_araddr_reg": 32, "m_axi_arlen_reg": 8}
        ids = {name: chr(33 + i) for i, name in enumerate(names)}
        initial = {"clock": 0, "remaining_beats": 10, "next_address": 8176,
                   "boundary_beats": 4, "fifo_credit": 2, "next_burst_length": 2,
                   "m_axi_arvalid_reg": 0, "m_axi_araddr_reg": 0, "m_axi_arlen_reg": 0}
        lines = ["$scope module top $end", "$scope module burst_engine $end"]
        lines.extend(f"$var wire {widths.get(name, 1)} {ids[name]} {name} $end" for name in names)
        lines.extend(("$upscope $end", "$upscope $end", "$enddefinitions $end", "$dumpvars"))
        for name, value in initial.items():
            ident = ids[name]
            lines.append(f"{value}{ident}" if widths.get(name, 1) == 1 else f"b{value:b} {ident}")
        lines.extend(("$end", "#1", f"1{ids['clock']}", f"1{ids['m_axi_arvalid_reg']}",
                      f"b{8176:b} {ids['m_axi_araddr_reg']}", f"b1 {ids['m_axi_arlen_reg']}",
                      "#2", f"0{ids['clock']}", f"b100 {ids['fifo_credit']}",
                      "#3", f"1{ids['clock']}", "#4", f"0{ids['clock']}"))
        with tempfile.TemporaryDirectory(prefix="reservation-vcd-", dir=TMP_ROOT) as temporary:
            vcd = Path(temporary) / "reservation.vcd"
            vcd.write_text("\n".join(lines) + "\n", encoding="ascii")
            selected = {name: (ident, widths.get(name, 1)) for name, ident in ids.items()}
            edges = list(t4b_vcd._iter_edges(vcd, selected, {ident: name for name, ident in ids.items()}))
            self.assertEqual(len(edges), 2)
            _, _, pre, post = edges[0]
            reservation = t4b_vcd._check_reservation(pre, post, 16)
            self.assertEqual(reservation["beats"], 2)  # FIFO credit, not B/boundary, limited it.
            _, _, stalled_pre, stalled_post = edges[1]
            self.assertEqual(stalled_pre["fifo_credit"], 4)
            t4b_vcd._check_frozen(reservation, stalled_post)

    def test_first_wait_ignores_a_later_stall_after_immediate_hit(self):
        self.assertEqual(t4b_vcd._first_load_wait(7, 3, 7), 0)
        self.assertEqual(t4b_vcd._first_load_wait(7, 0, 15), 8)

    def test_generated_profile_uses_snapshot_edges_and_first_load_wait(self):
        names = (*t4b_vcd.ENGINE, "present_state", "start_port", "done_port")
        widths = {name: 32 for name in names}
        for name in ("clock", "m_axi_arvalid", "m_axi_arready", "m_axi_arid",
                     "m_axi_rvalid", "m_axi_rready", "m_axi_rlast", "m_axi_rid",
                     "fault_reg", "load_pending", "load_pending_valid", "load_request_valid",
                     "configure_complete", "configure_direct_capture", "cpu_load_done",
                     "drain_complete", "m_axi_arvalid_reg", "start", "reset_n",
                     "present_state", "start_port", "done_port"):
            widths[name] = 1
        widths.update({"m_axi_arlen_reg": 8, "m_axi_arsize": 3, "m_axi_arburst": 2,
                       "m_axi_rresp": 2, "in1": 2, "fifo_count": 3})
        ids = {name: chr(33 + i) for i, name in enumerate(names)}
        state = {name: 0 for name in names}
        lines = ["$scope module top $end", "$scope module system $end",
                 "$scope module DUT $end", "$scope module burst_engine $end"]
        lines.extend(f"$var wire {widths[name]} {ids[name]} {name} $end" for name in t4b_vcd.ENGINE)
        lines.extend(("$upscope $end", "$scope module Controller_i $end",
                      f"$var wire 1 {ids['present_state']} present_state $end", "$upscope $end",
                      f"$var wire 1 {ids['clock']} clock $end",
                      f"$var wire 1 {ids['start_port']} start_port $end",
                      f"$var wire 1 {ids['done_port']} done_port $end",
                      "$upscope $end", "$upscope $end", "$upscope $end",
                      "$enddefinitions $end", "$dumpvars"))
        for name, value in state.items():
            ident = ids[name]
            lines.append(f"{value}{ident}" if widths[name] == 1 else f"b{value:b} {ident}")
        lines.extend(("$end",))

        def emit(time, changes):
            lines.append(f"#{time}")
            for name, value in changes.items():
                state[name] = value
                ident = ids[name]
                lines.append(f"{value}{ident}" if widths[name] == 1 else f"b{value:b} {ident}")

        def edge(cycle, pre=None, post=None):
            tick = cycle * 10
            emit(tick, pre or {})
            emit(tick + 1, {"clock": 1, **(post or {})})
            emit(tick + 2, {"clock": 0})

        edge(1, {"reset_n": 0})
        edge(2, {"reset_n": 1, "start_port": 1})
        edge(3, {"start_port": 0, "start": 1, "in1": 2,
                 "configure_complete": 1, "configure_direct_capture": 1,
                 "region_count": 2, "region_base": 0})
        edge(4, {"start": 0, "in1": 0, "configure_complete": 0,
                 "configure_direct_capture": 0, "remaining_beats": 2,
                 "next_address": 0, "boundary_beats": 1024,
                 "fifo_credit": 2, "next_burst_length": 2,
                 "m_axi_arvalid_reg": 0, "m_axi_arsize": 2, "m_axi_arburst": 1,
                 "m_axi_arid": 0},
             {"m_axi_arvalid_reg": 1, "m_axi_araddr_reg": 0,
              "m_axi_arlen_reg": 1, "m_axi_arvalid": 1})
        edge(5, {"m_axi_arvalid": 1, "m_axi_arvalid_reg": 1,
                 "m_axi_arready": 1, "issued": 0},
             {"m_axi_arvalid_reg": 0, "m_axi_arvalid": 0, "issued": 2})
        edge(6, {"m_axi_arready": 0, "start": 1, "in1": 0,
                 "load_request_valid": 1, "fifo_count": 0,
                 "load_pending": 0, "load_pending_valid": 0,
                 "issued": 2, "returned": 0, "consumed": 0},
             {"load_pending": 1, "load_pending_valid": 1})
        edge(7, {"start": 0, "load_pending": 1, "load_pending_valid": 1,
                 "m_axi_rvalid": 1, "m_axi_rready": 1, "m_axi_rlast": 0,
                 "returned": 0, "fifo_count": 0},
             {"returned": 1, "fifo_count": 1})
        edge(8, {"m_axi_rvalid": 1, "m_axi_rready": 1, "m_axi_rlast": 1,
                 "m_axi_rid": 0, "m_axi_rresp": 0, "returned": 1,
                 "consumed": 0, "fifo_count": 1, "cpu_load_done": 1},
             {"returned": 2, "consumed": 1, "fifo_count": 1,
              "load_pending": 0, "load_pending_valid": 0})
        edge(9, {"m_axi_rvalid": 0, "cpu_load_done": 1, "start": 1,
                 "in1": 0, "load_request_valid": 1, "returned": 2,
                 "consumed": 1, "fifo_count": 1},
             {"consumed": 2, "fifo_count": 0})
        edge(10, {"start": 0, "cpu_load_done": 0, "returned": 2,
                 "consumed": 2, "issued": 2, "region_count": 2,
                 "fifo_count": 0}, {"drain_complete": 1})
        edge(11, {}, {"done_port": 1})

        with tempfile.TemporaryDirectory(prefix="generated-vcd-", dir=TMP_ROOT) as temporary:
            root = Path(temporary)
            vcd, report = root / "generated.vcd", root / "report.json"
            vcd.write_text("\n".join(lines) + "\n", encoding="ascii")
            result = subprocess.run(
                [sys.executable, str(HERE / "t4b_vcd.py"), "--mode", "generated_burst",
                 "--vcd", str(vcd), "--json-out", str(report), "--n", "2", "--b", "2",
                 "--o", "1", "--d", "2", "--declared-latency", "3", "--read-delay", "2"],
                text=True, capture_output=True, check=False, env=os.environ.copy())
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = __import__("json").loads(report.read_text(encoding="utf-8"))
            metrics = payload["metrics"]
            self.assertEqual(payload["status"], "PASS")
            self.assertEqual(metrics["first_wait_cycles"], 2)
            self.assertEqual(metrics["L_axi"], 2)
            self.assertEqual(metrics["declared_latency"], 3)
            self.assertEqual(metrics["read_delay"], 2)
            self.assertEqual(metrics["outstanding_peak"], 1)
            self.assertEqual(metrics["n_desc_min"], 1)
            self.assertEqual(metrics["first_ar_accept_cycle"], metrics["t_ar0"])
            self.assertEqual(metrics["last_rlast_accept_cycle"], metrics["t_final_rlast"])
            self.assertEqual(metrics["C_stream"],
                             metrics["last_rlast_accept_cycle"] - metrics["first_ar_accept_cycle"])
            self.assertIn("first_start_cycle", metrics)
            self.assertIn("first_done_cycle", metrics)
            self.assertLess(metrics["first_start_cycle"], metrics["configure_start_cycle"])
            self.assertGreater(metrics["first_done_cycle"], metrics["final_load_cycle"])
            self.assertIn("C_configure_drain", metrics)
            self.assertNotIn("C", metrics)

            fifo_vcd, fifo_report = root / "generated_fifo.vcd", root / "fifo_report.json"
            fifo_vcd.write_text("\n".join(lines).replace("cpu_load_done", "fifo_load_done") + "\n",
                                encoding="ascii")
            fifo_result = subprocess.run(
                [sys.executable, str(HERE / "t4b_vcd.py"), "--mode", "generated_burst",
                 "--vcd", str(fifo_vcd), "--json-out", str(fifo_report), "--n", "2", "--b", "2",
                 "--o", "1", "--d", "2", "--declared-latency", "3", "--read-delay", "2"],
                text=True, capture_output=True, check=False, env=os.environ.copy())
            self.assertEqual(fifo_result.returncode, 0, fifo_result.stderr)
            fifo_payload = __import__("json").loads(fifo_report.read_text(encoding="utf-8"))
            self.assertEqual(fifo_payload["metrics"], metrics)

    def test_engine_done_alias_is_fail_closed(self):
        def definitions(alias="cpu_load_done", width=1, duplicate=False,
                        second_scope=False, extra_scope_alias=False):
            signals = {name: [(name, 1, None)] for name in t4b_vcd.ENGINE
                       if name != "cpu_load_done"}
            if alias:
                signals[alias] = [(alias, width, None)]
                if duplicate:
                    signals[alias].append((alias + "_duplicate", width, None))
            result = {("top", "burst_engine"): signals}
            if second_scope:
                result[("top", "other", "burst_engine")] = dict(signals)
            if extra_scope_alias:
                result[("top", "other")] = {"fifo_load_done": [("other_fifo", 1, None)]}
            return result

        invalid = (
            ("neither alias", definitions(alias=None), "missing or ambiguous"),
            ("both aliases", {**definitions(), ("top", "burst_engine"): {
                **definitions()[("top", "burst_engine")],
                "fifo_load_done": [("fifo_load_done", 1, None)]}}, "missing or ambiguous"),
            ("wrong width", definitions(width=2), "invalid or duplicate"),
            ("duplicate declaration", definitions(duplicate=True), "invalid or duplicate"),
            ("multiple engine scopes", definitions(second_scope=True), "missing or ambiguous"),
            ("alias in another scope", definitions(extra_scope_alias=True), "outside burst_engine"),
            ("only in another scope", {
                ("top", "burst_engine"): definitions(alias=None)[("top", "burst_engine")],
                ("top", "other"): {"fifo_load_done": [("other_fifo", 1, None)]}},
             "missing or ambiguous"),
        )
        for label, defs, message in invalid:
            with self.subTest(label=label), self.assertRaisesRegex(ValueError, message):
                t4b_vcd._choose_scope(defs, "generated_burst")

    def test_strict_legacy_mode_streams_shared_axi_window(self):
        names = (*t4b_vcd.CHANNEL, "clock", "start_port", "done_port")
        ids = {name: chr(33 + i) for i, name in enumerate(names)}
        widths = {"m_axi_araddr": 32, "m_axi_arlen": 8, "m_axi_arsize": 3,
                  "m_axi_arburst": 2, "m_axi_rid": 1, "m_axi_rresp": 2}
        endpoint_ids = {"start_port": "S", "done_port": "D"}
        lines = ["$scope module top $end", "$scope module m_axi $end"]
        lines.extend(f"$var wire {widths.get(name, 1)} {ids[name]} {name} $end"
                     for name in (*t4b_vcd.CHANNEL, "clock"))
        lines.extend(("$upscope $end", "$scope module system $end", "$scope module DUT $end",
                      f"$var wire 1 {ids['clock']} clock $end",
                      "$var wire 1 S start_port $end", "$var wire 1 D done_port $end",
                      "$upscope $end", "$upscope $end", "$upscope $end",
                      "$enddefinitions $end", "$dumpvars"))
        lines.extend(f"b0 {ident}" for name, ident in ids.items()
                     if name in (*t4b_vcd.CHANNEL, "clock") and widths.get(name, 1) > 1)
        lines.extend(f"0{ident}" for name, ident in ids.items()
                     if name in (*t4b_vcd.CHANNEL, "clock") and widths.get(name, 1) == 1)
        lines.extend(("0S", "0D"))
        lines.extend(("$end",))

        def edge(t, **updates):
            lines.append(f"#{t}")
            for name, value in updates.items():
                ident = endpoint_ids[name] if name in endpoint_ids else ids[name]
                if widths.get(name, 1) == 1:
                    lines.append(f"{value}{ident}")
                else:
                    lines.append(f"b{value:b} {ident}")
            lines.extend((f"#{t + 1}", f"1{ids['clock']}"))
            lines.extend((f"#{t + 2}", f"0{ids['clock']}"))

        edge(1, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=0,
             m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0, start_port=1,
             m_axi_rvalid=0, m_axi_rready=1, m_axi_rlast=0,
             m_axi_rid=0, m_axi_rresp=0)
        edge(11, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=4,
             m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
             m_axi_rvalid=0, m_axi_rready=1, m_axi_rlast=0,
             m_axi_rid=0, m_axi_rresp=0)
        edge(21, m_axi_arvalid=0, m_axi_arready=1, m_axi_araddr=4,
             m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
             m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1,
             m_axi_rid=0, m_axi_rresp=0)
        edge(31, m_axi_arvalid=0, m_axi_arready=1, m_axi_araddr=4,
             m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
             m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1,
             m_axi_rid=0, m_axi_rresp=0, done_port=1)
        with tempfile.TemporaryDirectory(prefix="strict-vcd-", dir=TMP_ROOT) as temporary:
            root = Path(temporary)
            vcd, report = root / "trace.vcd", root / "report.json"
            vcd.write_text("\n".join(lines) + "\n", encoding="ascii")
            result = subprocess.run(
                [sys.executable, str(HERE / "t4b_vcd.py"), "--mode", "legacy",
                 "--vcd", str(vcd), "--json-out", str(report), "--n", "2"],
                text=True, capture_output=True, check=False, env=os.environ.copy())
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = __import__("json").loads(report.read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "PASS")
        self.assertEqual(payload["metrics"]["beats"], 2)
        self.assertEqual(payload["metrics"]["outstanding_peak"], 2)
        self.assertLess(payload["metrics"]["first_start_cycle"],
                        payload["metrics"]["first_done_cycle"])

    def run_strict_channel(self, trace, n=2, mode="legacy"):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="strict-channel-vcd-", dir=TMP_ROOT) as temporary:
            root = Path(temporary)
            vcd, report = root / "trace.vcd", root / "report.json"
            vcd.write_text(trace, encoding="ascii")
            result = subprocess.run(
                [sys.executable, str(HERE / "t4b_vcd.py"), "--mode", mode,
                 "--vcd", str(vcd), "--json-out", str(report), "--n", str(n)],
                text=True, capture_output=True, check=False, env=os.environ.copy())
            payload = __import__("json").loads(report.read_text(encoding="utf-8"))
            return result, payload

    def _scalarized_channel_trace(self, simultaneous=False, unknown_handshake=False):
        names = (*t4b_vcd.CHANNEL, "clock")
        widths = {"m_axi_araddr": 32, "m_axi_arlen": 8, "m_axi_arsize": 3,
                  "m_axi_arburst": 2, "m_axi_rid": 1, "m_axi_rresp": 2}
        ids = {}
        declarations = ["$scope module top $end", "$scope module m_axi $end"]
        counter = 0
        for name in names:
            width = widths.get(name, 1)
            if name in ("m_axi_araddr", "m_axi_arlen"):
                bits = {}
                for index in range(width):
                    ident = f"id{counter}"
                    counter += 1
                    bits[index] = ident
                    declarations.append(f"$var wire 1 {ident} {name} [{index}] $end")
                ids[name] = bits
            else:
                ident = f"id{counter}"
                counter += 1
                ids[name] = ident
                declarations.append(f"$var wire {width} {ident} {name} $end")
        declarations.append("$var parameter 0 unused_parameter $end")
        endpoint_ids = {"start_port": "start_ep", "done_port": "done_ep"}
        declarations.extend(("$upscope $end", "$scope module system $end",
                             "$scope module DUT $end",
                             f"$var wire 1 {ids['clock']} clock $end",
                             "$var wire 1 start_ep start_port $end",
                             "$var wire 1 done_ep done_port $end"))
        lines = declarations + ["$upscope $end", "$upscope $end", "$upscope $end",
                                "$enddefinitions $end", "$dumpvars"]

        def emit_value(name, value):
            if isinstance(ids[name], dict):
                if value is None:
                    lines.extend(f"x{ident}" for ident in ids[name].values())
                else:
                    lines.extend(f"{(value >> index) & 1}{ident}"
                                 for index, ident in ids[name].items())
            else:
                ident = ids[name]
                width = widths.get(name, 1)
                if value is None:
                    lines.append(("x" if width == 1 else "b" + "x" * width) +
                                 ("" if width == 1 else " " + ident))
                elif width == 1:
                    lines.append(f"{value}{ident}")
                else:
                    lines.append(f"b{value:b} {ident}")

        values = {name: 0 for name in names}
        values.update({"m_axi_araddr": None, "m_axi_arlen": None})
        for name, value in values.items():
            emit_value(name, value)
        lines.extend(("0start_ep", "0done_ep"))
        lines.append("$end")

        def edge(time, **updates):
            lines.append(f"#{time}")
            if unknown_handshake and time == 1:
                updates["m_axi_araddr"] = None
            for name, value in updates.items():
                values[name] = value
                if name in endpoint_ids:
                    lines.append(f"{value}{endpoint_ids[name]}")
                else:
                    emit_value(name, value)
            lines.extend((f"#{time + 1}", f"1{ids['clock']}",
                          f"#{time + 2}", f"0{ids['clock']}"))

        edge(1, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=0,
             m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
             start_port=1)
        if simultaneous:
            edge(11, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=4,
                 m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
                 m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1,
                 m_axi_rid=0, m_axi_rresp=0)
            edge(21, m_axi_arvalid=0, m_axi_rvalid=1, m_axi_rready=1,
                 m_axi_rlast=1, m_axi_rid=0, m_axi_rresp=0)
            edge(31, m_axi_rvalid=0, done_port=1)
        else:
            edge(11, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=4,
                 m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0)
            edge(21, m_axi_arvalid=0, m_axi_rvalid=1, m_axi_rready=1,
                 m_axi_rlast=1, m_axi_rid=0, m_axi_rresp=0)
            edge(31, m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1,
                 m_axi_rid=0, m_axi_rresp=0, done_port=1)
        return "\n".join(lines) + "\n"

    def test_scalarized_buses_and_zero_width_parameter_are_supported(self):
        result, report = self.run_strict_channel(self._scalarized_channel_trace())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(report["metrics"]["outstanding_peak"], 2)

    def test_scalarized_bus_with_missing_or_duplicate_index_is_rejected(self):
        trace = self._scalarized_channel_trace()
        for corrupted in (trace.replace("m_axi_araddr [5]", "m_axi_araddr [4]", 1),
                          trace.replace("m_axi_arlen [3]", "m_axi_arlen [2]", 1)):
            with self.subTest(corruption="duplicate bit index"):
                result, report = self.run_strict_channel(corrupted)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(report["status"], "INVALID")
                self.assertTrue(any("bit indices" in error for error in report["errors"]))

    def test_payload_unknown_is_ignored_idle_but_rejected_on_handshake(self):
        idle_trace = self._scalarized_channel_trace()
        self.assertIn("$var parameter 0 unused_parameter $end", idle_trace)
        # The initial X payload becomes a fully-known value before ARVALID.
        result, report = self.run_strict_channel(idle_trace)
        self.assertEqual(result.returncode, 0, result.stderr)
        bad_result, bad_report = self.run_strict_channel(
            self._scalarized_channel_trace(unknown_handshake=True))
        self.assertNotEqual(bad_result.returncode, 0)
        self.assertEqual(bad_report["status"], "INVALID")
        self.assertTrue(any("unknown" in error for error in bad_report["errors"]))

    def test_peak_occupancy_applies_simultaneous_ar_push_and_rlast_pop(self):
        result, report = self.run_strict_channel(
            self._scalarized_channel_trace(simultaneous=True))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["metrics"]["outstanding_peak"], 1)
        self.assertEqual(report["metrics"]["C_stream"],
                         report["metrics"]["last_rlast_accept_cycle"] -
                         report["metrics"]["first_ar_accept_cycle"])
        self.assertLessEqual(report["metrics"]["first_start_cycle"],
                             report["metrics"]["first_ar_accept_cycle"])
        self.assertGreater(report["metrics"]["first_done_cycle"],
                           report["metrics"]["last_rlast_accept_cycle"])


if __name__ == "__main__":
    unittest.main()
