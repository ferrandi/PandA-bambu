"""Offline tests for profile generation and T4B runner fail-closed ordering."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
if not (ROOT / "panda_regressions").is_dir():
    raise RuntimeError(f"unexpected repository root for T4B tests: {ROOT}")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
PROFILE = HERE / "t4b_profile.py"
RUNNER = HERE / "run_t4b_matched.sh"
REAL_CHECKER = HERE / "t4b_vcd.py"
PARAM_CHECKER = HERE / "check_t4b_effective_params.py"


def parameter_vcd(parent_read=64, parent_write=64, child_read=63, child_write=63,
                  queue=33, *, missing=None, duplicate=None, ambiguous=False, decoy=False,
                  descriptor="present"):
    lines = ["$scope module clocked_bambu_testbench $end",
             "$scope module bambu_testbench $end", "$scope module system $end",
             "$scope module if_m_axi_gmem_fu $end"]
    params = [("p_read", "READ_DELAY", parent_read),
              ("p_write", "WRITE_DELAY", parent_write)]
    lines.extend(f"$var parameter 32 {ident} {name} $end" for ident, name, _ in params
                 if missing != name)
    if duplicate == "parent":
        lines.append("$var parameter 32 p_read_duplicate READ_DELAY $end")
    lines.extend(("$scope module axi4_tb $end",))
    child_params = [("c_read", "READ_DELAY", child_read),
                    ("c_write", "WRITE_DELAY", child_write),
                    ("c_queue", "QUEUE_SIZE", queue)]
    lines.extend(f"$var parameter 32 {ident} {name} $end" for ident, name, _ in child_params
                 if missing != name)
    if duplicate == "child":
        lines.append("$var parameter 32 c_read_duplicate READ_DELAY $end")
    lines.extend(("$upscope $end", "$upscope $end", "$upscope $end", "$upscope $end", "$upscope $end"))
    if decoy:
        lines.extend(("$scope module decoy $end", "$scope module axi4_tb $end",
                      "$var parameter 32 decoy_read READ_DELAY $end",
                      "$var parameter 32 decoy_write WRITE_DELAY $end",
                      "$var parameter 32 decoy_queue QUEUE_SIZE $end",
                      "$upscope $end", "$upscope $end"))
    if ambiguous:
        lines.extend(("$scope module second $end", "$scope module system $end",
                      "$scope module if_m_axi_gmem_fu $end",
                      "$var parameter 32 second_read READ_DELAY $end",
                      "$var parameter 32 second_write WRITE_DELAY $end",
                      "$scope module axi4_tb $end",
                      "$var parameter 32 second_child_read READ_DELAY $end",
                      "$var parameter 32 second_child_write WRITE_DELAY $end",
                      "$var parameter 32 second_queue QUEUE_SIZE $end",
                      "$upscope $end", "$upscope $end", "$upscope $end", "$upscope $end"))
    if descriptor != "absent":
        lines.extend(("$scope module top $end", "$scope module DUT $end",
                      "$scope module burst_engine $end",
                      "$var reg 5 dc descriptor_count $end"))
        if descriptor == "conflicting":
            lines.append("$var reg 4 dc2 descriptor_count $end")
        lines.extend(("$upscope $end", "$upscope $end", "$upscope $end"))
    lines.extend(("$enddefinitions $end", "$dumpvars"))
    for ident, name, value in (*params, *child_params):
        if missing != name:
            lines.append(f"b{value:b} {ident}")
    if duplicate == "parent":
        lines.append(f"b{parent_read:b} p_read_duplicate")
    if duplicate == "child":
        lines.append(f"b{child_read:b} c_read_duplicate")
    if decoy:
        lines.extend(("b0 decoy_read", "b0 decoy_write", "b1 decoy_queue"))
    if ambiguous:
        lines.extend((f"b{parent_read:b} second_read", f"b{parent_write:b} second_write",
                      f"b{child_read:b} second_child_read", f"b{child_write:b} second_child_write",
                      f"b{queue:b} second_queue"))
    if descriptor != "absent":
        lines.append("b1 dc")
        if descriptor == "conflicting":
            lines.append("b1 dc2")
    lines.append("$end")
    return "\n".join(lines) + "\n"


def add_parameter_hierarchy(path: Path):
    text = path.read_text(encoding="ascii")
    definitions, values = text.split("$enddefinitions $end", 1)
    hierarchy = parameter_vcd(descriptor="absent").split("$enddefinitions $end", 1)[0]
    hierarchy = hierarchy.replace("$scope module clocked_bambu_testbench $end\n", "", 1)
    hierarchy = hierarchy.rsplit("$upscope $end", 1)[0]
    definitions = definitions + "\n" + hierarchy
    dumpvars, remainder = values.split("$end", 1)
    extra_values = "\n".join(("b1000000 p_read", "b1000000 p_write", "b111111 c_read",
                               "b111111 c_write", "b100001 c_queue"))
    path.write_text(definitions + "$enddefinitions $end" + dumpvars + extra_values + "\n$end" + remainder,
                    encoding="ascii")


def write_real_checker_vcd(path: Path, mode: str) -> int:
    """Write a deterministic API fixture and return its inclusive DUT run cycles."""
    import t4b_vcd

    if mode == "generated_burst":
        names = (*t4b_vcd.ENGINE, "present_state", "start_port", "done_port")
        widths = {name: 32 for name in names}
        for name in ("clock", "m_axi_arvalid", "m_axi_arready", "m_axi_arid", "m_axi_rvalid",
                     "m_axi_rready", "m_axi_rlast", "m_axi_rid", "fault_reg", "load_pending",
                     "load_pending_valid", "load_request_valid", "configure_complete",
                     "configure_direct_capture", "cpu_load_done", "drain_complete",
                     "m_axi_arvalid_reg", "start", "reset_n", "present_state", "start_port", "done_port"):
            widths[name] = 1
        widths.update({"m_axi_arlen_reg": 8, "m_axi_arsize": 3, "m_axi_arburst": 2,
                       "m_axi_rresp": 2, "in1": 2, "fifo_count": 3})
        ids = {name: f"v{i}" for i, name in enumerate(dict.fromkeys(names))}
        state = {name: 0 for name in dict.fromkeys(names)}
        lines = ["$scope module top $end", "$scope module system $end", "$scope module DUT $end",
                 "$scope module burst_engine $end"]
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
        lines.append("$end")

        def edge(cycle, pre=None, post=None):
            tick = cycle * 10
            for name, value in (pre or {}).items():
                state[name] = value
            lines.append(f"#{tick}")
            for name, value in (pre or {}).items():
                ident = ids[name]
                lines.append(f"{value}{ident}" if widths[name] == 1 else f"b{value:b} {ident}")
            lines.append(f"#{tick + 1}")
            state["clock"] = 1
            for name, value in {"clock": 1, **(post or {})}.items():
                state[name] = value
                ident = ids[name]
                lines.append(f"{value}{ident}" if widths[name] == 1 else f"b{value:b} {ident}")
            lines.append(f"#{tick + 2}")
            state["clock"] = 0
            lines.append(f"0{ids['clock']}")

        edge(1, {"reset_n": 0})
        edge(2, {"reset_n": 1, "start_port": 1})
        edge(3, {"start": 1, "in1": 2, "configure_complete": 1,
                 "configure_direct_capture": 1, "region_count": 2, "region_base": 0})
        edge(4, {"start": 0, "in1": 0, "configure_complete": 0, "configure_direct_capture": 0,
                 "remaining_beats": 2, "next_address": 0, "boundary_beats": 1024,
                 "fifo_credit": 256, "next_burst_length": 2, "m_axi_arvalid_reg": 0,
                 "m_axi_arsize": 2, "m_axi_arburst": 1, "m_axi_arid": 0},
             {"m_axi_arvalid_reg": 1, "m_axi_araddr_reg": 0, "m_axi_arlen_reg": 1,
              "m_axi_arvalid": 1})
        edge(5, {"m_axi_arvalid": 1, "m_axi_arvalid_reg": 1, "m_axi_arready": 1, "issued": 0},
             {"m_axi_arvalid_reg": 0, "m_axi_arvalid": 0, "issued": 2})
        edge(6, {"m_axi_arready": 0, "start": 1, "in1": 0, "load_request_valid": 1,
                 "fifo_count": 0, "load_pending": 0, "load_pending_valid": 0, "issued": 2,
                 "returned": 0, "consumed": 0}, {"load_pending": 1, "load_pending_valid": 1})
        for cycle in range(7, 69):
            edge(cycle)
        edge(69, {"load_pending": 1, "load_pending_valid": 1, "m_axi_rvalid": 1,
                  "m_axi_rready": 1, "m_axi_rlast": 0, "returned": 0, "fifo_count": 0},
             {"returned": 1, "fifo_count": 1})
        edge(70, {"m_axi_rvalid": 1, "m_axi_rready": 1, "m_axi_rlast": 1, "m_axi_rid": 0,
                  "m_axi_rresp": 0, "returned": 1, "consumed": 0, "fifo_count": 1,
                  "cpu_load_done": 1},
             {"returned": 2, "consumed": 1, "fifo_count": 1,
              "load_pending": 0, "load_pending_valid": 0})
        edge(71, {"m_axi_rvalid": 0, "start": 1, "in1": 0, "load_request_valid": 1,
                  "returned": 2, "consumed": 1, "fifo_count": 1, "cpu_load_done": 1},
             {"consumed": 2, "fifo_count": 0})
        edge(72, {"cpu_load_done": 0, "returned": 2, "consumed": 2, "issued": 2,
                  "region_count": 2, "fifo_count": 0}, {"drain_complete": 1})
        edge(73, {}, {"done_port": 1})
        path.write_text("\n".join(lines) + "\n", encoding="ascii")
        add_parameter_hierarchy(path)
        return 72

    # Legacy no-engine trace: two scalar reads, delayed first response and DUT endpoints.
    names = (*t4b_vcd.CHANNEL, "clock")
    ids = {name: f"c{i}" for i, name in enumerate(names)}
    widths = {"m_axi_araddr": 32, "m_axi_arlen": 8, "m_axi_arsize": 3,
              "m_axi_arburst": 2, "m_axi_rid": 1, "m_axi_rresp": 2}
    endpoint = {"start_port": "S", "done_port": "D"}
    lines = ["$scope module top $end", "$scope module m_axi $end"]
    lines.extend(f"$var wire {widths.get(name, 1)} {ids[name]} {name} $end"
                 for name in (*t4b_vcd.CHANNEL, "clock"))
    lines.extend(("$upscope $end", "$scope module system $end", "$scope module DUT $end",
                  f"$var wire 1 {ids['clock']} clock $end", "$var wire 1 S start_port $end",
                  "$var wire 1 D done_port $end", "$upscope $end", "$upscope $end", "$upscope $end",
                  "$enddefinitions $end", "$dumpvars"))
    lines.extend(f"b0 {ident}" for name, ident in ids.items()
                 if name in (*t4b_vcd.CHANNEL, "clock") and widths.get(name, 1) > 1)
    lines.extend(f"0{ident}" for name, ident in ids.items()
                 if name in (*t4b_vcd.CHANNEL, "clock") and widths.get(name, 1) == 1)
    lines.extend(("0S", "0D", "$end"))
    state = {name: 0 for name in (*t4b_vcd.CHANNEL, "clock", "start_port", "done_port")}

    def edge(cycle, **updates):
        tick = cycle * 10
        lines.append(f"#{tick}")
        for name, value in updates.items():
            state[name] = value
            ident = endpoint[name] if name in endpoint else ids[name]
            lines.append(f"{value}{ident}" if widths.get(name, 1) == 1 else f"b{value:b} {ident}")
        lines.extend((f"#{tick + 1}", f"1{ids['clock']}", f"#{tick + 2}", f"0{ids['clock']}"))

    edge(1, start_port=1, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=0,
         m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0,
         m_axi_rvalid=0, m_axi_rready=1, m_axi_rlast=0, m_axi_rid=0, m_axi_rresp=0)
    edge(2, start_port=0, m_axi_arvalid=1, m_axi_arready=1, m_axi_araddr=4,
         m_axi_arlen=0, m_axi_arsize=2, m_axi_arburst=1, m_axi_arid=0)
    for cycle in range(3, 65):
        edge(cycle, m_axi_arvalid=0)
    edge(65, m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1)
    edge(66, m_axi_rvalid=1, m_axi_rready=1, m_axi_rlast=1)
    edge(67, m_axi_rvalid=0, done_port=1)
    path.write_text("\n".join(lines) + "\n", encoding="ascii")
    add_parameter_hierarchy(path)
    return 67


class EffectiveParameterVcdTests(unittest.TestCase):
    def test_accepts_effective_values_for_burst_and_legacy_source_forms(self):
        import check_t4b_effective_params as effective

        for mode in ("burst", "legacy"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(dir=TMP_ROOT) as td:
                vcd = Path(td) / "test.vcd"
                manifest = Path(td) / "manifest.json"
                text = parameter_vcd(decoy=True).replace(
                    "$scope module decoy $end\n", "$scope module decoy $end\n"
                    "$var wire 0 zero HALF_CLOCK_PERIOD $end\n$var real 64 float_id FLOAT $end\n")
                text = text.replace("$dumpvars\n", "$dumpvars\nr1 float_id\n", 1)
                vcd.write_text(text, encoding="ascii")
                manifest.write_text("{}\n", encoding="utf-8")
                effective.validate(vcd, manifest, 64, 32)
                result = json.loads(manifest.read_text(encoding="utf-8"))
                self.assertEqual(result["effective_hdl_parameters"]["values"], {
                    "clocked_bambu_testbench/bambu_testbench/system/if_m_axi_gmem_fu": {
                        "READ_DELAY": 64, "WRITE_DELAY": 64},
                    "clocked_bambu_testbench/bambu_testbench/system/if_m_axi_gmem_fu/axi4_tb": {
                        "READ_DELAY": 63, "WRITE_DELAY": 63, "QUEUE_SIZE": 33}})

    def test_rejects_wrong_missing_unknown_duplicate_and_ambiguous_values(self):
        import check_t4b_effective_params as effective

        cases = (
            {"parent_read": 63}, {"child_read": 64}, {"parent_write": 63},
            {"child_write": 64}, {"queue": 32}, {"missing": "READ_DELAY"},
            {"duplicate": "child"}, {"ambiguous": True},
        )
        for index, options in enumerate(cases):
            with self.subTest(options=options), tempfile.TemporaryDirectory(dir=TMP_ROOT) as td:
                vcd = Path(td) / "test.vcd"
                manifest = Path(td) / "manifest.json"
                vcd.write_text(parameter_vcd(**options), encoding="ascii")
                manifest.write_text("{}\n", encoding="utf-8")
                with self.assertRaises(effective.VcdParameterError):
                    effective.validate(vcd, manifest, 64, 32)

    def test_rejects_unknown_initial_parameter_value(self):
        import check_t4b_effective_params as effective

        with tempfile.TemporaryDirectory(dir=TMP_ROOT) as td:
            vcd = Path(td) / "test.vcd"
            manifest = Path(td) / "manifest.json"
            vcd.write_text(parameter_vcd().replace("b1000000 p_read", "bx000000 p_read"), encoding="ascii")
            manifest.write_text("{}\n", encoding="utf-8")
            with self.assertRaises(effective.VcdParameterError):
                effective.validate(vcd, manifest, 64, 32)


class T4BProfileTests(unittest.TestCase):
    def test_profile_copy_changes_only_the_interface_pragma_and_records_hashes(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-profile-test-", dir=TMP_ROOT) as td:
            out = Path(td)
            generated, manifest = out / "fixture.cpp", out / "profile.json"
            result = subprocess.run(
                ["python3", "-B", str(PROFILE), "--fixture", "independent", "--b", "16",
                 "--o", "8", "--d", "256", "--l", "64", "--output", str(generated),
                 "--manifest", str(manifest)],
                cwd=ROOT, text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(manifest.read_text(encoding="utf-8"))
            original = Path(data["source"]).read_text(encoding="utf-8")
            actual = generated.read_text(encoding="utf-8")
            self.assertEqual(actual.count("#pragma HLS interface"), 1)
            self.assertEqual(
                [line for line in original.splitlines() if not line.startswith("#pragma HLS interface")],
                [line for line in actual.splitlines() if not line.startswith("#pragma HLS interface")],
            )
            self.assertIn("max_read_burst_length=16", actual)
            self.assertIn("num_read_outstanding=8", actual)
            self.assertIn("read_fifo_depth=256", actual)
            self.assertIn("latency=64", actual)
            self.assertEqual(data["source_sha256"], hashlib.sha256(Path(data["source"]).read_bytes()).hexdigest())
            self.assertEqual(data["generated_sha256"], hashlib.sha256(generated.read_bytes()).hexdigest())
            self.assertEqual(data["diff_sha256"], hashlib.sha256(Path(data["diff_file"]).read_bytes()).hexdigest())

    def test_profile_rejects_malformed_numeric_values(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-profile-invalid-", dir=TMP_ROOT) as td:
            out = Path(td)
            result = subprocess.run(
                ["python3", "-B", str(PROFILE), "--fixture", "sum", "--b", "+16",
                 "--o", "4", "--d", "256", "--l", "64", "--output", str(out / "x.cpp"),
                 "--manifest", str(out / "x.json")],
                cwd=ROOT, text=True, capture_output=True, check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((out / "x.cpp").exists())


class T4BRunnerTests(unittest.TestCase):
    def test_runner_consumes_real_checker_endpoints_for_burst_and_legacy(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-real-checker-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, _ = self.prepare_mocks(root, event_log)
            for mode, fixture in (("burst", "generated_burst"), ("legacy", "legacy")):
                vcd = root / f"{mode}.vcd"
                total = write_real_checker_vcd(vcd, fixture)
                output = root / mode
                result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                            REAL_CHECKER, fixture="sum" if mode == "burst" else "independent",
                                            mode=mode, mock_vcd=vcd, total=total, n=2)
                checker_error = (output / "checker.stderr.log").read_text(encoding="utf-8") if (output / "checker.stderr.log").exists() else ""
                self.assertEqual(result.returncode, 0, result.stderr + checker_error)
                report = json.loads((output / "checker.json").read_text(encoding="utf-8"))
                metrics = report["metrics"]
                self.assertEqual(report["status"], "PASS")
                if mode == "burst":
                    self.assertLess(metrics["first_start_cycle"], metrics["configure_start_cycle"])
                    self.assertGreater(metrics["first_done_cycle"], metrics["final_load_cycle"])
                    self.assertIn("C_configure_drain", metrics)
                else:
                    self.assertLess(metrics["first_start_cycle"], metrics["first_done_cycle"])
                    self.assertNotIn("C_configure_drain", metrics)
                    self.assertTrue((output / "run/first_read_independent_work.v").is_file())
                    self.assertNotIn("MinimalAXI4MasterPipelined",
                                     (output / "run/first_read_independent_work.v").read_text())
                manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
                self.assertEqual(manifest["C_total"], total)
                self.assertEqual(manifest["C_total_vcd_inclusive_window"], total)
                self.assertEqual(manifest["C_total_xml_crosscheck"], total)

    def invoke_invalid(self, out: Path, *extra: str):
        env = os.environ.copy()
        env["TMPDIR"] = env["TMP"] = env["TEMP"] = str(TMP_ROOT)
        return subprocess.run(
            ["bash", str(RUNNER), "16", "4", "256", "64", "1024", "32", str(out), *extra],
            cwd=ROOT, env=env, text=True, capture_output=True, check=False,
        )

    def test_non_incr_and_invalid_numeric_inputs_fail_before_build(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-invalid-runner-", dir=TMP_ROOT) as td:
            root = Path(td)
            events = root / "events"
            bindir = root / "bin"
            bindir.mkdir()
            cmake = bindir / "cmake"
            cmake.write_text(f"#!/bin/sh\necho cmake >> '{events}'\n", encoding="utf-8")
            cmake.chmod(0o755)
            env_path = str(bindir) + os.pathsep + os.environ.get("PATH", "")
            base_env = os.environ.copy()
            base_env.update(PATH=env_path, TMPDIR=str(TMP_ROOT), TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT))
            for output, extra, replacement in (
                (root / "bad-type", ("--axi-burst-type", "FIXED"), ""),
                (root / "bad-q", (), "3"),
            ):
                argv = ["bash", str(RUNNER), "16", "4", "256", "64", "1024", replacement or "32", str(output), *extra]
                result = subprocess.run(argv, cwd=ROOT, env=base_env, text=True, capture_output=True, check=False)
                self.assertNotEqual(result.returncode, 0)
            huge = subprocess.run(
                ["bash", str(RUNNER), "999999999999999999999999999", "4", "256", "64", "1024", "32", str(root / "huge")],
                cwd=ROOT, env=base_env, text=True, capture_output=True, check=False,
            )
            self.assertNotEqual(huge.returncode, 0)
            self.assertFalse(events.exists(), "invalid request reached build/install")

    def make_fake_install(self, root: Path, event_log: Path):
        prefix = root / "fake-install"
        (prefix / "bin").mkdir(parents=True)
        settings = prefix / "settings.sh"
        settings.write_text(
            f"echo settings >> '{event_log}'\nexport T4B_SETTINGS_WAS_SOURCED=1\n",
            encoding="utf-8",
        )
        bambu = prefix / "bin" / "bambu"
        bambu.write_text(
            "#!/bin/bash\n"
            f"echo bambu:$* >> '{event_log}'\n"
            "[[ ${T4B_SETTINGS_WAS_SOURCED:-0} == 1 ]] || exit 85\n"
            "if [[ ${1:-} == --version ]]; then echo 'mock-bambu 1.0'; exit 0; fi\n"
            "mkdir -p HLS_output/simulation/modelsim_backend\n"
            "top=kernel; queue=32; burst=1\n"
            "for arg in \"$@\"; do case $arg in --top-fname=*) top=${arg#*=};; --tb-queue-size=*) queue=${arg#*=};; --bambu-parameter=experimental-m-axi-burst=0) burst=0;; esac; done\n"
            "echo '.READ_DELAY(READ_DELAY ? READ_DELAY - 1 : 63)' > HLS_output/simulation/bambu_testbench.v\n"
            "echo \".QUEUE_SIZE($((queue + 1))),\" >> HLS_output/simulation/bambu_testbench.v\n"
            "echo 'T4B_GOLDEN_PASS N=1024 result=1' > HLS_output/simulation/testbench.log\n"
            "echo 'Errors = 0' >> HLS_output/simulation/testbench.log\n"
            "echo '# End time: 10:00:00' > HLS_output/simulation/modelsim_backend/launch.log\n"
            "total=10; [[ ${T4B_MOCK_BAD_TOTAL:-0} != 1 ]] || total=11\n"
            "total=${T4B_MOCK_TOTAL:-$total}\n"
            "printf '<application><timing><simulation return_value=\"0\"><run>%s</run></simulation></timing></application>\\n' \"$total\" > HLS_output/simulation/modelsim_backend/bambu_results.xml\n"
            "echo \"  Total cycles             : $total cycles\" >&2\n"
            "for arg in \"$@\"; do if [[ $arg == --print-dot ]]; then echo mock-ir > profile.cpp.bambuir; echo mock-dot > profile.cpp.dot; fi; done\n"
            "if [[ ${T4B_MOCK_MISSING_VCD:-0} != 1 ]]; then if [[ -n ${T4B_MOCK_VCD:-} ]]; then cp \"$T4B_MOCK_VCD\" HLS_output/simulation/test.vcd; else "
            "{ echo '$scope module DUT $end'; if [[ ${T4B_MOCK_DESCRIPTOR_COUNT:-present} != absent ]]; then echo '$scope module burst_engine $end'; echo '$var reg 5 dc descriptor_count $end'; if [[ ${T4B_MOCK_DESCRIPTOR_COUNT:-present} == conflicting ]]; then echo '$var reg 4 dc2 descriptor_count $end'; fi; echo '$upscope $end'; fi; echo '$upscope $end'; echo '$enddefinitions $end'; } > HLS_output/simulation/test.vcd; fi; fi\n"
            "mkdir -p panda-temp\n"
            "latency=${T4B_MOCK_LATENCY:-64}; if [[ ${T4B_MOCK_MISSING_LATENCY:-0} != 1 ]]; then echo \"<architecture><bundle name=\\\"data\\\" mode=\\\"m_axi\\\" latency=\\\"$latency\\\" max_read_burst_length=\\\"16\\\" num_read_outstanding=\\\"4\\\" read_fifo_depth=\\\"256\\\"/></architecture>\" > panda-temp/architecture.xml; fi\n"
            "if [[ $burst == 1 ]]; then printf '%s\\n' \"${T4B_MOCK_ENGINE_RTL:-MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256), .READ_DELAY(63), ) burst_engine (clk)}\" > \"$top.v\"; else echo '// legacy no burst engine' > \"$top.v\"; fi\n",
            encoding="utf-8",
        )
        bambu.chmod(0o755)
        return prefix

    def prepare_mocks(self, root: Path, event_log: Path, modelsim_dir="modelsim"):
        bin_dir = root / "mock-bin"
        bin_dir.mkdir()
        cmake = bin_dir / "cmake"
        cmake.write_text(
            "#!/bin/bash\n"
            f"echo cmake:$* >> '{event_log}'\n"
            "[[ $TMPDIR == \"$T4B_EXPECTED_TMP\" && $TMP == $TMPDIR && $TEMP == $TMPDIR ]] || exit 84\n",
            encoding="utf-8",
        )
        cmake.chmod(0o755)
        modelsim = root / modelsim_dir / "questasim" / "bin"
        modelsim.mkdir(parents=True)
        vsim = modelsim / "vsim"
        vsim.write_text(
            "#!/bin/sh\n"
            "if [ \"${1:-}\" = '-version' ]; then echo 'mock-vsim 1.0'; fi\n"
            "exit 0\n",
            encoding="utf-8",
        )
        vsim.chmod(0o755)
        for tool in ("vlog", "vlib", "vmap"):
            companion = modelsim / tool
            companion.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            companion.chmod(0o755)
        checker = root / "fake_checker.py"
        checker.write_text(
            "import argparse, json, os\n"
            "p=argparse.ArgumentParser(); p.add_argument('--mode'); p.add_argument('--vcd'); p.add_argument('--json-out'); p.add_argument('--n'); p.add_argument('--b'); p.add_argument('--o'); p.add_argument('--d'); p.add_argument('--declared-latency'); p.add_argument('--read-delay'); a=p.parse_args()\n"
            "m={'L_axi':64,'first_wait_cycles':64,'exposed_wait_cycles':64,'outstanding_peak':4,'n_desc':64,'n_desc_min':64,'first_ar_accept_cycle':20,'last_rlast_accept_cycle':100,'C_stream':80,'first_start_cycle':2,'first_done_cycle':11,'endpoint_scope':'top/system/DUT'}\n"
            "if a.mode == 'generated_burst': m['C_configure_drain']=5\n"
            "if os.environ.get('T4B_MOCK_BAD_ENDPOINT') == 'missing': m.pop('first_start_cycle',None)\n"
            "if os.environ.get('T4B_MOCK_BAD_ENDPOINT') == 'negative': m['first_start_cycle']=-1\n"
            "json.dump({'schema_version':1,'mode':a.mode,'scope':'mock','status':'PASS','expected':{},'metrics':m,'errors':[]}, open(a.json_out,'w'))\n",
            encoding="utf-8",
        )
        return bin_dir, root / modelsim_dir, checker

    def invoke_mocked(self, output: Path, event_log: Path, prefix: Path, bindir: Path,
                      modelsim_root: Path, checker: Path, missing_vcd=False,
                      fixture="sum", mode="burst", bad_total=False,
                      missing_latency=False, latency="64", mock_vcd=None, total=None, n=1024,
                      bad_endpoint="", descriptor_count="present", engine_rtl=None,
                      backend_lookup=False, no_simulator=False, path_vsim=False,
                      bambu_hls=None):
        if mock_vcd is None and not missing_vcd:
            mock_vcd = output.parent / f"{output.name}-parameters.vcd"
            mock_vcd.write_text(parameter_vcd(descriptor=descriptor_count), encoding="ascii")
        env = os.environ.copy()
        path_entries = [str(bindir)]
        if backend_lookup or no_simulator:
            path_entries.append("/usr/bin:/bin")
        elif path_vsim:
            path_entries.extend((str(modelsim_root / "questasim" / "bin"), "/usr/bin:/bin"))
        else:
            path_entries.append(env.get("PATH", ""))
        env.update({
            "PATH": os.pathsep.join(path_entries),
            "TMPDIR": str(TMP_ROOT), "TMP": str(TMP_ROOT), "TEMP": str(TMP_ROOT),
            "BURST_MODELSIM_OUTSIDE_SANDBOX": "1",
            "T4B_TEST_MODE": "1", "T4B_TEST_INSTALL_PREFIX": str(prefix),
            "T4B_TEST_CHECKER": str(checker), "T4B_EVENT_LOG": str(event_log),
            "T4B_EXPECTED_TMP": str(output / "tmp"),
            "T4B_MOCK_MISSING_VCD": "1" if missing_vcd else "0",
            "T4B_MOCK_BAD_TOTAL": "1" if bad_total else "0",
            "T4B_MOCK_MISSING_LATENCY": "1" if missing_latency else "0",
            "T4B_MOCK_LATENCY": latency,
            "T4B_MOCK_VCD": str(mock_vcd) if mock_vcd else "",
            "T4B_MOCK_TOTAL": str(total) if total is not None else "",
            "T4B_MOCK_BAD_ENDPOINT": bad_endpoint,
            "T4B_MOCK_DESCRIPTOR_COUNT": descriptor_count,
            "T4B_MOCK_ENGINE_RTL": engine_rtl or "",
        })
        if backend_lookup:
            env.pop("MODELSIM_ROOT", None)
            env["BAMBU_HLS_BACKEND_PATH"] = str(modelsim_root)
        elif no_simulator:
            env.pop("MODELSIM_ROOT", None)
            env.pop("BAMBU_HLS_BACKEND_PATH", None)
        elif path_vsim:
            env.pop("MODELSIM_ROOT", None)
            env.pop("BAMBU_HLS_BACKEND_PATH", None)
            env["BAMBU_HLS"] = bambu_hls or ""
        else:
            env["MODELSIM_ROOT"] = str(modelsim_root)
        return subprocess.run(
            ["bash", str(RUNNER), "16", "4", "256", "64", str(n), "32", str(output),
             "--fixture", fixture, "--mode", mode],
            cwd=TMP_ROOT, env=env, text=True, capture_output=True, check=False,
        )

    def test_build_install_settings_precede_every_bambu_command(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-mock-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            output = root / "pass-out"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root, checker)
            self.assertEqual(result.returncode, 0, result.stderr)
            events = event_log.read_text(encoding="utf-8").splitlines()
            build = next(i for i, line in enumerate(events) if line == "cmake:--build build -j8")
            install = next(i for i, line in enumerate(events) if line == "cmake:--install build")
            settings = events.index("settings")
            bambu_calls = [i for i, line in enumerate(events) if line.startswith("bambu:")]
            self.assertLess(build, install)
            self.assertLess(install, settings)
            self.assertTrue(bambu_calls)
            self.assertTrue(all(settings < i for i in bambu_calls))
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["status"], "PASS")
            self.assertEqual(manifest["parameters"]["AXI_burst_type"], "INCR")
            self.assertEqual(manifest["parameters"]["mem_delay_read"], 64)
            self.assertEqual(manifest["read_delay_derived"], 63)
            params = manifest["effective_hdl_parameters"]
            self.assertEqual(params["scopes"], {
                "parent": "clocked_bambu_testbench/bambu_testbench/system/if_m_axi_gmem_fu",
                "axi4_tb": "clocked_bambu_testbench/bambu_testbench/system/if_m_axi_gmem_fu/axi4_tb"})
            self.assertEqual(params["values"][params["scopes"]["axi4_tb"]], {
                "READ_DELAY": 63, "WRITE_DELAY": 63, "QUEUE_SIZE": 33})
            self.assertEqual(manifest["C_total"], 10)
            self.assertEqual(manifest["C_stream"], 80)
            self.assertEqual(manifest["cycle_metrics"]["C_configure_drain"], 5)
            self.assertEqual(manifest["occupancy_source"], "axi_handshake_reconstruction")
            self.assertTrue(manifest["descriptor_count_dumped"])
            self.assertEqual(manifest["descriptor_count_width"], 5)
            self.assertEqual(manifest["descriptor_count_vcd_scope"], "burst_engine")
            self.assertEqual(manifest["generated_latency"]["values"][0]["value"], "64")
            self.assertEqual(manifest["testbench_parameter_sizes_bytes"], {})
            self.assertTrue(manifest["bambu_argv"])
            self.assertIn("--mem-delay-read=64", manifest["bambu_argv"])
            self.assertIn("--tb-queue-size=32", manifest["bambu_argv"])
            self.assertIn("--AXI-burst-type=INCREMENTAL", manifest["bambu_argv"])
            self.assertFalse(any("bus-pipelined" in arg for arg in manifest["bambu_argv"]))
            self.assertTrue((output / "profile.cpp.diff").is_file())

    def test_backend_lookup_without_vsim_on_path_passes_matching_mentor_root(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-backend-lookup-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            output = root / "backend-lookup-out"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root, checker,
                                        backend_lookup=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            selected_bin = modelsim_root / "questasim" / "bin"
            self.assertEqual(manifest["simulator"]["source"], "BAMBU_HLS_BACKEND_PATH")
            self.assertEqual(manifest["simulator"]["root"], str(selected_bin.parent))
            self.assertEqual(manifest["simulator"]["bin"], str(selected_bin))
            self.assertEqual(manifest["simulator"]["path"], str(selected_bin / "vsim"))
            self.assertIn(f"--mentor-root={selected_bin.parent}", manifest["bambu_argv"])

    def test_path_lookup_needs_no_bambu_settings_and_quotes_space_paths(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-path-lookup-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log, "model sim")
            poison_settings = root / "bambu settings must not load"
            poison_settings.mkdir()
            settings_marker = root / "unexpected-settings-source"
            (poison_settings / "settings.sh").write_text(
                f"printf 'early settings sourced\\n' >> '{settings_marker}'\n",
                encoding="utf-8",
            )
            output = root / "path-with-spaces-out"
            result = self.invoke_mocked(
                output, event_log, prefix, bindir, modelsim_root, checker,
                path_vsim=True, bambu_hls=str(poison_settings),
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertFalse(settings_marker.exists(), "PATH-selected ModelSim should not source Bambu settings")
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            selected_bin = modelsim_root / "questasim" / "bin"
            self.assertEqual(manifest["simulator"]["source"], "PATH")
            self.assertEqual(manifest["simulator"]["bin"], str(selected_bin))
            self.assertEqual(manifest["simulator"]["path"], str(selected_bin / "vsim"))
            self.assertIn("mock-vsim 1.0", manifest["simulator"]["version"])
            self.assertIn(f"--mentor-root={selected_bin.parent}", manifest["bambu_argv"])

    def test_rtl_only_models_sim_runners_take_path_before_bambu_settings(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="modelsim-path-first-", dir=TMP_ROOT) as td:
            root = Path(td)
            modelsim_bin = root / "model sim" / "bin"
            modelsim_bin.mkdir(parents=True)
            tool_log = root / "tools.log"
            for tool in ("vsim", "vlog", "vlib", "vmap"):
                exit_status = 19 if tool == "vlib" else 0
                (modelsim_bin / tool).write_text(
                    "#!/bin/sh\n"
                    f"printf '%s %s\\n' '{tool}' \"$*\" >> \"$TOOL_LOG\"\n"
                    f"exit {exit_status}\n",
                    encoding="utf-8",
                )
                (modelsim_bin / tool).chmod(0o755)
            poison_settings = root / "bambu settings must not load"
            poison_settings.mkdir()
            settings_marker = root / "settings-was-sourced"
            (poison_settings / "settings.sh").write_text(
                f"printf sourced > '{settings_marker}'\n", encoding="utf-8"
            )
            env = os.environ.copy()
            env.update({
                "PATH": str(modelsim_bin) + os.pathsep + "/usr/bin:/bin",
                "BAMBU_HLS": str(poison_settings),
                "BURST_MODELSIM_OUTSIDE_SANDBOX": "1",
                "TOOL_LOG": str(tool_log),
                "TMPDIR": str(TMP_ROOT), "TMP": str(TMP_ROOT), "TEMP": str(TMP_ROOT),
            })
            env.pop("BAMBU_HLS_BACKEND_PATH", None)
            env.pop("MODELSIM_ROOT", None)
            cases = (
                (ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/run_testbench_memaxi_model.sh",
                 ["characterize", str(root / "memaxi-out")], root / "memaxi-out"),
                (ROOT / "panda_regressions/verilog/axi_burst/t4b_tests/run_t4b_models.sh", [str(root / "t4b-out")], root / "t4b-out"),
            )
            for runner, args, output in cases:
                with self.subTest(runner=runner.name):
                    result = subprocess.run(
                        ["bash", str(runner), *args], cwd=ROOT, env=env,
                        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
                    )
                    self.assertEqual(result.returncode, 19, result.stdout)
                    self.assertTrue(output.is_dir(), result.stdout)
                    self.assertIn("vlib work", tool_log.read_text(encoding="utf-8"))
                    self.assertFalse(settings_marker.exists(), "PATH-selected ModelSim should not load Bambu settings")
                    tool_log.unlink()

    def test_missing_selected_companion_fails_before_build_or_output_creation(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-missing-tool-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            (modelsim_root / "questasim" / "bin" / "vlog").unlink()
            output = root / "missing-tool-out"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root, checker)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("ModelSim tool unavailable", result.stderr)
            self.assertFalse(output.exists())
            self.assertFalse(event_log.exists(), "missing simulator companion reached build/install")

    def test_scheduler_off_runs_without_any_models_installation(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-scheduler-off-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            output = root / "scheduler-off-out"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root, checker,
                                        mode="scheduler-off", no_simulator=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["status"], "PASS")
            self.assertEqual(manifest["simulator"]["source"], "not-used")
            self.assertIsNone(manifest["simulator"]["path"])

    def test_top_specific_rtl_and_burst_profile_for_all_witnesses(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-witnesses-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            for fixture, top in (("sum", "kernel"),
                                 ("independent", "first_read_independent_work"),
                                 ("mulxor", "first_read_mulxor_work")):
                output = root / fixture
                result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                            checker, fixture=fixture)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue((output / f"run/{top}.v").is_file())
                self.assertIn(f"--top-fname={top}", (output / "manifest.json").read_text())
                self.assertIn(".QUEUE_SIZE(33)",
                              (output / "run/HLS_output/simulation/bambu_testbench.v").read_text())
                manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
                self.assertEqual(manifest["testbench_parameter_sizes_bytes"],
                                 {"src": 16384} if fixture != "sum" else {})
                self.assertEqual(manifest["profile"], {"B": 16, "O": 4, "D": 256,
                                                       "declared_latency": 64})

    def test_burst_engine_profile_matcher_rejects_wrong_missing_duplicate_or_ambiguous_profiles(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-engine-profile-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            cases = {
                "wrong": "MinimalAXI4MasterPipelined #(.B_MAX(15), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256)) burst_engine (clk)",
                "missing": "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(4)) burst_engine (clk)",
                "duplicate": "MinimalAXI4MasterPipelined #(.B_MAX(16), .B_MAX(16), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256)) burst_engine (clk)",
                "wrong-instance": "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256)) other_engine (clk)",
                "ambiguous": "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256)) burst_engine (clk); MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(4), .FIFO_DEPTH(256)) burst_engine2 (clk)",
            }
            for name, rtl in cases.items():
                with self.subTest(name=name):
                    output = root / name
                    result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                                checker, engine_rtl=rtl)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("generated burst engine", result.stderr)
                    self.assertFalse((output / "checker.json").exists())

    def test_witness_param_size_is_used_and_recorded_in_every_mode(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-param-size-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            for fixture in ("independent", "mulxor"):
                for mode in ("burst", "legacy", "scheduler-off"):
                    with self.subTest(fixture=fixture, mode=mode):
                        output = root / f"{fixture}-{mode}"
                        previous_events = (event_log.read_text(encoding="utf-8").splitlines()
                                           if event_log.exists() else [])
                        result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                                    checker, fixture=fixture, mode=mode)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        events = event_log.read_text(encoding="utf-8").splitlines()[len(previous_events):]
                        build = events.index("cmake:--build build -j8")
                        install = events.index("cmake:--install build")
                        settings = events.index("settings")
                        bambu_calls = [i for i, line in enumerate(events) if line.startswith("bambu:")]
                        self.assertLess(build, install)
                        self.assertLess(install, settings)
                        self.assertTrue(bambu_calls)
                        self.assertTrue(all(settings < i for i in bambu_calls))
                        manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
                        self.assertEqual(manifest["testbench_parameter_sizes_bytes"], {"src": 16384})
                        self.assertIn("--tb-param-size=src:16384", manifest["bambu_argv"])
                        self.assertEqual(manifest["profile"], {"B": 16, "O": 4, "D": 256,
                                                               "declared_latency": 64})
                        if mode == "scheduler-off":
                            self.assertTrue((output / "run/profile.cpp.bambuir").is_file())
                            self.assertTrue((output / "run/profile.cpp.dot").is_file())

    def test_legacy_checks_top_specific_rtl_and_tool_total_must_match_vcd(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-integrity-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            output = root / "legacy"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                        checker, fixture="independent", mode="legacy")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((output / "run/first_read_independent_work.v").is_file())
            self.assertNotIn("MinimalAXI4MasterPipelined",
                             (output / "run/first_read_independent_work.v").read_text())
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertNotIn("C_configure_drain", manifest["cycle_metrics"])
            self.assertEqual(manifest["C_total_vcd_inclusive_window"], 10)
            bad_total = root / "bad-total"
            result = self.invoke_mocked(bad_total, event_log, prefix, bindir, modelsim_root,
                                        checker, bad_total=True)
            self.assertNotEqual(result.returncode, 0)
            manifest = json.loads((bad_total / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["status"], "INVALID_ENVIRONMENT_OR_CHECKER_FAILURE")

    def test_manifest_records_descriptor_count_presence_without_changing_occupancy_source(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-descriptor-count-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            for name, present, width in (("present", "present", 5), ("absent", "absent", None)):
                output = root / name
                result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                            checker, descriptor_count=present)
                self.assertEqual(result.returncode, 0, result.stderr)
                manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
                self.assertEqual(manifest["status"], "PASS")
                self.assertEqual(manifest["occupancy_source"], "axi_handshake_reconstruction")
                self.assertEqual(manifest["descriptor_count_dumped"], present == "present")
                self.assertEqual(manifest["descriptor_count_width"], width)
                self.assertEqual(manifest["descriptor_count_vcd_scope"],
                                 "burst_engine" if present == "present" else None)
            conflict = root / "conflicting"
            result = self.invoke_mocked(conflict, event_log, prefix, bindir, modelsim_root,
                                        checker, descriptor_count="conflicting")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("conflicting declarations", result.stderr)

    def test_generated_latency_metadata_is_required_and_must_match(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-latency-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            for name, options in (("missing", {"missing_latency": True}),
                                  ("mismatch", {"latency": "63"})):
                output = root / name
                result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                            checker, **options)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("generated AXI latency metadata", result.stderr)

    def test_missing_or_invalid_dut_endpoints_fail_closed(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-endpoint-integrity-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            for name, endpoint_error in (("missing", "missing"), ("negative", "negative")):
                output = root / name
                result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                            checker, bad_endpoint=endpoint_error)
                self.assertNotEqual(result.returncode, 0)
                manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
                self.assertEqual(manifest["status"], "INVALID_ENVIRONMENT_OR_CHECKER_FAILURE")

    def test_missing_vcd_fails_and_keeps_partial_run_artifacts(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-missing-vcd-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            output = root / "no-vcd-out"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root, checker, missing_vcd=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("required simulation artifact", result.stderr)
            self.assertTrue((output / "run/HLS_output/simulation/testbench.log").is_file())
            self.assertTrue((output / "bambu.exit_code").is_file())
            self.assertTrue((output / "manifest.json").is_file())
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["read_delay_derived"], 63)

    def test_effective_parameter_mismatch_fails_before_vcd_checker(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="t4b-runner-effective-params-", dir=TMP_ROOT) as td:
            root = Path(td)
            event_log = root / "events"
            prefix = self.make_fake_install(root, event_log)
            bindir, modelsim_root, checker = self.prepare_mocks(root, event_log)
            invalid_vcd = root / "wrong-child.vcd"
            invalid_vcd.write_text(parameter_vcd(child_read=64), encoding="ascii")
            output = root / "wrong-child"
            result = self.invoke_mocked(output, event_log, prefix, bindir, modelsim_root,
                                        checker, mock_vcd=invalid_vcd)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("effective AXI parameters in VCD", result.stderr)
            self.assertFalse((output / "checker.json").exists())
            self.assertIn("expected 63", (output / "effective-params.stderr.log").read_text())


if __name__ == "__main__":
    unittest.main()
