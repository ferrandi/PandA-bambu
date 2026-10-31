#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Offline integration checks for the issue #434 Mantis/ModelSim launcher."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
LAUNCHER = ROOT / "panda_regressions/hls/bambu_lp_array_channel.sh"
SUITE = ROOT / "panda_regressions/hls/bambu_lp_array_channel"
SCRATCH = ROOT / "documentation/tmp"
CASE_COUNT = 10


class LPArrayChannelIntegrationTest(unittest.TestCase):
    def setUp(self):
        SCRATCH.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="lp-array-channel-test-", dir=SCRATCH)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.cwd = self.root / "caller cwd"
        self.cwd.mkdir()
        self.invocations = self.root / "invocations"
        self.install_mocks()

    def executable(self, name, body):
        path = self.bin / name
        path.write_text("#!/usr/bin/env python3\n" + body, encoding="utf-8")
        path.chmod(0o755)
        return path

    def install_mocks(self):
        self.executable(
            "bambu",
            "import os, re, sys\n"
            "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
            "ii=os.environ.get('MANTIS_EXPECT_LOOP_II','1')\n"
            "with open(os.environ['LP_INVOCATIONS'],'a') as f: f.write('bambu\\n')\n"
            "fn=os.environ.get('MANTIS_EXPECT_FUNCTION','f')\n"
            "actual_ii='1' if mode == 'unexpected_pipeline' and ii == '0' else ii\n"
            "if mode != 'bad_schedule':\n"
            "  if ii == '0' and mode != 'unexpected_pipeline': print('Loop pipelining not possible')\n"
            "  else: print('II='+actual_ii+' solution found for loop (BB2)\\nInitiation Interval (II) for function '+fn+':BB2 = '+actual_ii)\n"
            "  latency=os.environ.get('LP_LATENCY')\n"
            "  if latency and mode != 'missing_latency': print('Iteration latency for function '+fn+':BB2 = '+latency)\n"
            "if mode == 'synth_nonzero': sys.exit(9)\n"
            "if mode != 'bad_schedule':\n"
            "  loop_xml='' if ii == '0' and mode != 'unexpected_pipeline' else '<loop_0 bb=\"2\" ii=\"'+actual_ii+'\"/>'\n"
            "  open('bambu_results.xml','w').write('<root><hls_results><function name=\"'+fn+'\">'+loop_xml+'</function></hls_results></root>')\n"
            "if os.environ.get('LP_EXPECT_FU_II'):\n"
            "  model=os.environ['LP_EXPECT_FU_MODEL']; operation=os.environ['LP_EXPECT_FU_OPERATION']; latency=os.environ['LP_EXPECT_FU_LATENCY']; pipe=os.environ['LP_EXPECT_PIPE_PARAMETER']; vertex=fn+'_428912'\n"
            "  fu_ii='1' if mode == 'fu_ii1' else os.environ['LP_EXPECT_FU_II']\n"
            "  print('Processing operation: '+operation+' - '+vertex+' - %mock')\n"
            "  print('Analyzing operation '+vertex+' 6\\n  initiation_time '+fu_ii+'\\n  Analyzed operation '+vertex)\n"
            "  open('mock_scheduling.xml','w').write('<hls><scheduling><scheduling_constraints vertex_name=\"'+vertex+'\" fu_name=\"'+model+'\"/></scheduling></hls>')\n"
            "  open('technology_out.xml','w').write('<technology><library><cell><name>'+model+'</name><operation operation_name=\"'+operation+'\" cycles=\"'+latency+'\" initiation_time=\"'+fu_ii+'\" pipe_parameters=\"'+pipe+'\"/></cell></library></technology>')\n"
            "if mode != 'missing_rtl':\n"
            "  tb=os.environ.get('LP_TB','')\n"
            "  source=os.path.join(os.environ['LP_SUITE'],tb)\n"
            "  text=open(source).read()\n"
            "  duts=set(re.findall(r'^\\s*([A-Za-z_]\\w*)\\s+dut\\s*\\(',text,re.M))\n"
            "  if mode in ('shared_tb','missing_selected'): duts={'p_Z13dual_read_accRA16_iS0_R10ac_channelIiE'} if mode == 'missing_selected' else {'p_Z9dual_readRA16_iS0_R10ac_channelIiE'}\n"
            "  pipe=os.environ.get('LP_EXPECT_PIPE_PARAMETER','')\n"
            "  if mode == 'missing_pipeline': pipe=''\n"
            "  suffix='.PIPE_PARAMETER('+pipe+')' if pipe else ''\n"
            "  unit=os.environ.get('LP_EXPECT_UNIT','')\n"
            "  if mode == 'missing_expected_unit': unit=''\n"
            "  unit_instance=(unit+' expected_unit();') if unit else ''\n"
            "  capture=os.environ.get('LP_EXPECT_CAPTURE_LATENCY','')\n"
            "  if mode == 'missing_capture': capture=''\n"
            "  capture_instance=('TIMED_CAPTURE #(.LATENCY('+capture+'), .BITSIZE_in1(32)) expected_capture();') if capture else ''\n"
            "  hold=os.environ.get('LP_EXPECT_INPUT_HOLD','')\n"
            "  if mode == 'missing_input_hold': hold=''\n"
            "  hold_instance=('INPUT_HOLD #(.BITSIZE_in1(32)) expected_hold();') if hold else ''\n"
            "  open('mock.v','w').write('\\n'.join('module '+dut+'(); endmodule'+suffix for dut in duts)+'\\n'+unit_instance+'\\n'+capture_instance+'\\n'+hold_instance+'\\n')\n",
        )
        for tool in ("vlib", "vlog", "vsim"):
            body = (
                "import os, sys\n"
                "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
                "name=os.path.basename(sys.argv[0])\n"
                "if name == 'vlib': os.makedirs(sys.argv[-1],exist_ok=True)\n"
                "if name == 'vsim':\n"
                " bp=next((a.split('=',1)[1] for a in sys.argv if a.startswith('-gBP_MODE=')),'0')\n"
                " lib=sys.argv[sys.argv.index('-L')+1] if '-L' in sys.argv else ''\n"
                " if 'HOLD_MUT' in lib:\n"
                "  if mode == 'hold_mut_not_detected': print('PASS [BP='+bp+'] return=4280')\n"
                "  else: print('FAIL [BP='+bp+'] errors=1 count=16 return=4280\\nHOLD_MUT_DETECTED return=4280')\n"
                " elif 'MUT' in lib:\n"
                "  if mode == 'mut_pass': print('PASS [BP='+bp+']')\n"
                "  elif mode == 'mut_inconclusive': print('INCONCLUSIVE [BP='+bp+'] no stall')\n"
                "  elif mode == 'mut_conflicting': print('FAIL [BP='+bp+'] corrupted input detected\\nPASS [BP='+bp+']')\n"
                "  elif mode != 'mut_no_fail': print('FAIL [BP='+bp+'] corrupted input detected')\n"
                " else:\n"
                "  print('PASS [BP='+bp+']')\n"
                "  if os.environ.get('LP_EXPECT_INPUT_HOLD'):\n"
                "   operand_changes='0' if mode == 'operand_changes_zero' else '3'\n"
                "   if bp == '0':\n"
                "    print('FU_II2_MONITOR launches=4 hold_checks=3 stable_errors=0 operand_changes='+operand_changes+' spacing_min=2 spacing_errors=0 stalled_hold_checks=0')\n"
                "    print('UNBOUNDED_HANDSHAKE co_launches=3 done_same_cycle=3 done_later=0 pending=0 handshake_errors=0')\n"
                "   else:\n"
                "    print('FU_II2_MONITOR launches=4 hold_checks=4 stable_errors=0 operand_changes='+operand_changes+' spacing_min=2 spacing_errors=0 stalled_hold_checks=2')\n"
                "    print('UNBOUNDED_HANDSHAKE co_launches=3 done_same_cycle=0 done_later=3 pending=0 handshake_errors=0')\n"
                "  if mode == 'sim_conflicting': print('FAIL [BP='+bp+']')\n"
                " if mode == 'sim_nonzero_pass': sys.exit(7)\n"
            )
            self.executable(tool, body)

    def run_suite(self, mode="ok", extra_args=(), path=None, no_clean=True):
        env = os.environ.copy()
        env.update(
            PATH=path or (str(self.bin) + os.pathsep + env.get("PATH", "")),
            BAMBU_HLS_BACKEND_PATH="",
            LP_MOCK_MODE=mode,
            LP_INVOCATIONS=str(self.invocations),
            LP_SUITE=str(SUITE),
            TMPDIR=str(SCRATCH),
            TMP=str(SCRATCH),
            TEMP=str(SCRATCH),
        )
        return subprocess.run(
            [str(LAUNCHER), *extra_args, *( ["--no-clean"] if no_clean else [])], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120,
        )

    def output_dir(self):
        return self.cwd / "out_bambu_lp_array_channel_MODELSIM"

    def junit_failures(self):
        report = ET.parse(self.output_dir() / "test_report.xml").getroot()
        return len(report.findall(".//testcase/failure"))

    def case_dirs(self):
        return [path.parent for path in self.output_dir().rglob("execution.log")]

    def test_success_runs_all_cases_all_modes_and_mutation_oracles(self):
        result = self.run_suite()
        details = "\n".join(
            f"{case.name}: {(case / 'execution.log').read_text()}"
            for case in self.case_dirs()
            if (case / "execution.log").exists()
        )
        self.assertEqual(result.returncode, 0, result.stdout + "\n" + details)
        self.assertEqual((self.invocations.read_text().count("bambu\n")), CASE_COUNT)
        report = ET.parse(self.output_dir() / "test_report.xml").getroot()
        self.assertEqual(report.get("failures"), "0")
        self.assertEqual(report.get("tests"), str(CASE_COUNT))
        self.assertEqual(len(self.case_dirs()), CASE_COUNT)
        for case in self.case_dirs():
            self.assertIn("PASS [BP=0]", (case / "vsim_BP0.log").read_text())
            self.assertIn("PASS [BP=1]", (case / "vsim_BP1.log").read_text())
            self.assertIn("PASS [BP=2]", (case / "vsim_BP2.log").read_text())
            mutant = (case / "vsim_MUT.log").read_text()
            self.assertIn("FAIL [BP=0]", mutant)
            self.assertNotRegex(mutant, r"(?:PASS|INCONCLUSIVE) \[BP=")
        ii2 = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("HOLD_MUT_DETECTED return=4280", (ii2 / "vsim_HOLD_MUT.log").read_text())

    def test_synthesis_and_schedule_failures_override_plausible_messages(self):
        for mode in ("synth_nonzero", "bad_schedule"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                root = self.output_dir()
                self.assertEqual(self.junit_failures(), CASE_COUNT)
                for case in self.case_dirs():
                    self.assertTrue((case / "failure.log").exists(), (case / "execution.log").read_text())
                    self.assertTrue((case / "execution.log").exists())
                    self.assertTrue((case / "bambu.log").exists())
                self.cleanup_output()

    def test_mutation_oracle_rejects_missing_and_invalid_verdicts(self):
        for mode in ("mut_no_fail", "mut_pass", "mut_inconclusive", "mut_conflicting"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertEqual(self.junit_failures(), CASE_COUNT)
                for case in self.case_dirs():
                    verdict = (case / "vsim_MUT.log").read_text()
                    if mode == "mut_no_fail":
                        self.assertNotRegex(verdict, r"(?:PASS|FAIL|INCONCLUSIVE) \[BP=")
                    elif mode == "mut_pass":
                        self.assertIn("PASS [BP=0]", verdict)
                    elif mode == "mut_inconclusive":
                        self.assertIn("INCONCLUSIVE [BP=0]", verdict)
                    else:
                        self.assertIn("FAIL [BP=0]", verdict)
                        self.assertIn("PASS [BP=0]", verdict)
                self.cleanup_output()

    def test_multiplier_pipeline_parameter_is_required(self):
        result = self.run_suite("missing_pipeline")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 4)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(2)", (case / "failure.log").read_text())
        div_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_div_array"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(15)", (div_case / "failure.log").read_text())
        ii2_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(2)", (ii2_case / "failure.log").read_text())
        self.cleanup_output()

    def test_original_iteration_latency_is_required(self):
        result = self.run_suite("missing_latency")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 3)
        cases = {"lp_array_write", "lp_dual_array_read", "lp_array_write_acc"}
        for case in self.case_dirs():
            if case.name in cases:
                self.assertIn("missing expected BB2 iteration latency 2", (case / "failure.log").read_text())
        self.assertEqual({case.name for case in self.case_dirs() if (case / "failure.log").exists()}, cases)
        self.cleanup_output()

    def test_expected_capture_latency_is_required(self):
        result = self.run_suite("missing_capture")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 2)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch"
        self.assertIn("TIMED_CAPTURE instance with LATENCY(2)", (case / "failure.log").read_text())
        ii2_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("TIMED_CAPTURE instance with LATENCY(2)", (ii2_case / "failure.log").read_text())
        self.cleanup_output()

    def test_ii2_requires_the_selected_fu_model_and_input_hold(self):
        for mode, expected in (("fu_ii1", "model ui_mul_node_FU_32_32_32_2 does not declare"),
                               ("missing_input_hold", "generated RTL lacks INPUT_HOLD")):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertEqual(self.junit_failures(), 1)
                case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
                self.assertIn(expected, (case / "failure.log").read_text())
                self.cleanup_output()

    def test_hold_oracle_mutation_must_fail_without_corrupting_checksum(self):
        result = self.run_suite("hold_mut_not_detected")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 1)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("HOLD_MUT did not trigger the input stability oracle", (case / "failure.log").read_text())
        self.cleanup_output()

    def test_ii2_monitor_requires_operand_changes_between_launches(self):
        result = self.run_suite("operand_changes_zero")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 1)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("II=2 input stability or launch spacing failed for BP=0", (case / "failure.log").read_text())
        self.cleanup_output()

    def test_expected_memory_and_divider_units_must_be_instantiated(self):
        result = self.run_suite("missing_expected_unit")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 3)
        expected = {
            "lp_internal_rom_l2": "ARRAY_1D_STD_BRAM_NN_SDS",
            "lp_internal_rom_l3": "ARRAY_1D_STD_BRAM_NN_SDS_3",
            "lp_div_array": "ui_idiv_node_FU",
        }
        seen = set()
        for case in self.case_dirs():
            for name, unit in expected.items():
                if name in case.as_posix():
                    self.assertIn(f"generated RTL lacks instance of expected unit {unit}",
                                  (case / "failure.log").read_text())
                    seen.add(name)
                    break
        self.assertEqual(seen, set(expected))
        self.cleanup_output()

    def test_simulator_failure_and_conflicting_verdict_fail_junit(self):
        for mode in ("sim_nonzero_pass", "sim_conflicting"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertEqual(self.junit_failures(), CASE_COUNT)
                for case in self.case_dirs():
                    self.assertTrue((case / "failure.log").exists(), (case / "execution.log").read_text())
                    self.assertTrue((case / "mock.v").exists())
                    self.assertTrue((case / "vlog_BP0.log").exists())
                    self.assertTrue((case / "vsim_BP0.log").exists())
                self.cleanup_output()

    def test_missing_rtl_and_modelsim_are_failures(self):
        result = self.run_suite("missing_rtl")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), CASE_COUNT)
        self.cleanup_output()
        for name in ("vlib", "vlog", "vsim"):
            (self.bin / name).unlink()
        isolated = self.root / "path-without-modelsim"
        isolated.mkdir()
        for name in ("bambu", "python3", "timeout", "dirname", "readlink", "mkdir", "grep", "sed", "awk", "find", "rm"):
            target = shutil.which(name)
            if name == "bambu":
                target = str(self.bin / name)
            if target:
                (isolated / name).symlink_to(target)
        result = self.run_suite("ok", path=str(isolated))
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), CASE_COUNT)
        for case in self.case_dirs():
            self.assertIn(
                "ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH",
                (case / "failure.log").read_text(),
            )

    def test_shared_testbench_checks_only_selected_module_dut(self):
        env = os.environ.copy()
        env.update(
            PATH=str(self.bin) + os.pathsep + env.get("PATH", ""),
            BAMBU_HLS_BACKEND_PATH="",
            LP_MOCK_MODE="shared_tb",
            LP_INVOCATIONS=str(self.invocations),
            LP_SUITE=str(SUITE),
            LP_TB="tb_lp_dual_array.v",
            LP_TB_TOP="tb_lp_dual_read",
            MANTIS_EXPECT_LOOP_II="1",
            TMPDIR=str(SCRATCH), TMP=str(SCRATCH), TEMP=str(SCRATCH),
        )
        result = subprocess.run(
            [str(SUITE / "run_case.sh")], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )
        self.assertEqual(result.returncode, 0, result.stdout)

        env["LP_MOCK_MODE"] = "missing_selected"
        result = subprocess.run(
            [str(SUITE / "run_case.sh")], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("generated RTL lacks DUT module p_Z9dual_readRA16_iS0_R10ac_channelIiE", result.stdout)

    def test_restart_reexecutes_failed_case_and_refuses_existing_output(self):
        failed = self.run_suite("bad_schedule")
        self.assertNotEqual(failed.returncode, 0, failed.stdout)
        self.assertEqual(self.junit_failures(), CASE_COUNT)
        for case in self.case_dirs():
            self.assertTrue((case / "failure.log").exists())
            self.assertTrue((case / "bambu.log").exists())
            self.assertTrue((case / "execution.log").exists())
        first_count = self.invocations.read_text().count("bambu\n")
        self.assertEqual(first_count, CASE_COUNT)
        refused = self.run_suite("ok")
        self.assertNotEqual(refused.returncode, 0, refused.stdout)
        restarted = self.run_suite("ok", ("--restart",))
        self.assertEqual(restarted.returncode, 0, restarted.stdout)
        self.assertEqual(self.invocations.read_text().count("bambu\n"), first_count + CASE_COUNT)
        self.assertFalse(any((case / "failure.log").exists() for case in self.case_dirs()))

    def test_existing_output_is_preserved_on_refusal(self):
        target = self.cwd / "out_bambu_lp_array_channel_MODELSIM"
        target.mkdir()
        sentinel = target / "keep"
        sentinel.write_text("preserve", encoding="utf-8")
        result = self.run_suite()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(sentinel.read_text(), "preserve")

    def test_launcher_forwards_cleanup_choice(self):
        result = self.run_suite(no_clean=False)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertFalse(any((case / "vsim_BP0.log").exists() for case in self.case_dirs()))

    def cleanup_output(self):
        shutil.rmtree(self.output_dir())


class TimedCaptureTechnologyModelTest(unittest.TestCase):
    def test_timed_capture_uses_reset_free_srl_delay(self):
        technology = ET.parse(ROOT / "etc/libtech/NC_STD_IPs.xml").getroot()
        cell = next(cell for cell in technology.iter("cell") if cell.findtext("name") == "TIMED_CAPTURE")
        model = cell.find(".//NP_functionality")
        verilog = model.get("VERILOG_PROVIDED", "")
        vhdl = model.get("VHDL_PROVIDED", "")

        self.assertIn('SRL_STYLE = "srl"', verilog)
        self.assertIn('SRL_STYLE of launch_delay : signal is "srl"', vhdl)
        self.assertNotIn("1RESET_EDGE_FORCE", verilog)
        self.assertNotIn("process(clock, reset)", vhdl)
        self.assertNotIn("remaining", verilog)
        self.assertNotIn("active", verilog)
        self.assertIn("launch_delay <= (launch_delay << 1) | (launch && !(1RESET_VALUE));", verilog)
        self.assertIn("launch_delay <= launch_delay(LATENCY-2 downto 0) & launch_valid;", vhdl)
        self.assertIn("assign out1 = capture_due ? in1 : captured;", verilog)
        self.assertIn("out1 <= in1 when capture_due = '1' else captured;", vhdl)


if __name__ == "__main__":
    unittest.main()
