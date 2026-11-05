#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Offline integration checks for the issue #434 Mantis/ModelSim launcher."""

import importlib.util
import os
from contextlib import redirect_stderr
from io import StringIO
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import threading
import tempfile
import time
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
LAUNCHER = ROOT / "panda_regressions/hls/bambu_lp_array_channel.sh"
SUITE = ROOT / "panda_regressions/hls/bambu_lp_array_channel"
SCRATCH = ROOT / "documentation/tmp"
MANIFEST = ROOT / "panda_regressions/hls/bambu_lp_array_channel_list"
EVALUATION_CASES = {"lp_mul_array_vivado_verilog", "lp_mul_array_vivado_vhdl"}
CASE_COUNT = 15
HDL_CASE_COUNT = CASE_COUNT - len(EVALUATION_CASES)
_timed_capture_spec = importlib.util.spec_from_file_location(
    "timed_capture_preflight", SUITE / "test_timed_capture.py"
)
TIMED_CAPTURE = importlib.util.module_from_spec(_timed_capture_spec)
_timed_capture_spec.loader.exec_module(TIMED_CAPTURE)
_stallable_spec = importlib.util.spec_from_file_location(
    "stallable_multiplier_preflight", SUITE / "test_stallable_multiplier.py"
)
STALLABLE_MULTIPLIER = importlib.util.module_from_spec(_stallable_spec)
_stallable_spec.loader.exec_module(STALLABLE_MULTIPLIER)
_global_stall_spec = importlib.util.spec_from_file_location(
    "global_pipeline_stall_preflight", SUITE / "test_global_pipeline_stall.py"
)
GLOBAL_PIPELINE_STALL = importlib.util.module_from_spec(_global_stall_spec)
_global_stall_spec.loader.exec_module(GLOBAL_PIPELINE_STALL)


class MantisSimulationManifestTest(unittest.TestCase):
    def test_every_mantis_case_has_bambu_simulation_and_testbench(self):
        rows = [shlex.split(line) for line in MANIFEST.read_text(encoding="utf-8").splitlines()
                if line.strip() and not line.lstrip().startswith("#")]
        self.assertEqual(len(rows), CASE_COUNT)

        for tokens in rows:
            with self.subTest(benchmark=next(
                    (token.split("=", 1)[1] for token in tokens if token.startswith("--benchmark-name=")),
                    "missing-benchmark-name")):
                testbenches = [token.split("=", 1)[1] for token in tokens
                               if token.startswith("--generate-tb=")]
                self.assertEqual(len(testbenches), 1)
                relative = testbenches[0]
                self.assertTrue(relative.startswith("BENCHMARKS_ROOT/") and relative.endswith(".cpp"))
                self.assertTrue((SUITE / relative[len("BENCHMARKS_ROOT/"):]).is_file(), relative)

                benchmark = next(token.split("=", 1)[1] for token in tokens
                                 if token.startswith("--benchmark-name="))
                if benchmark not in EVALUATION_CASES:
                    continue

                evaluation = [token for token in tokens
                              if token == "--evaluation" or token.startswith("--evaluation=")]
                self.assertEqual(evaluation, ["--evaluation"])

                env_tokens = [token[len("BENCHMARK_ENV="):] for token in tokens
                              if token.startswith("BENCHMARK_ENV=")]
                self.assertEqual(len(env_tokens), 1)
                env = dict(part.split("=", 1) for part in env_tokens[0].split(",") if "=" in part)
                writer = "V" if benchmark.endswith("verilog") else "H"
                self.assertEqual(env.get("LP_EVALUATION_ONLY"), "1")
                self.assertIn(f"--writer={writer}", tokens)
                self.assertEqual(
                    shlex.split(env.get("MANTIS_POST_CHECK", "")),
                    ["python3", "BENCHMARKS_ROOT/check_vivado_evaluation.py", f"--writer={writer}"],
                )


class ModelSimToolLookupTest(unittest.TestCase):
    def test_backend_path_resolves_vsim_directory_for_companion_tools(self):
        lookup = ROOT / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
        with tempfile.TemporaryDirectory() as temp_dir:
            backend = Path(temp_dir)
            modelsim_bin = backend / "modelsim" / "bin"
            modelsim_bin.mkdir(parents=True)
            for name in ("vsim", "vlib", "vlog", "vcom"):
                tool = modelsim_bin / name
                tool.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
                tool.chmod(0o755)

            env = os.environ.copy()
            env["PATH"] = "/usr/bin:/bin"
            env["BAMBU_HLS_BACKEND_PATH"] = str(backend)
            result = subprocess.run(
                ["bash", "-c",
                  'source "$1"; bambu_modelsim_resolve && export PATH="$BAMBU_MODELSIM_BIN:$PATH" '
                  '&& printf "%s\\n%s\\n%s\\n" "$BAMBU_MODELSIM_BIN" '
                  '"$(command -v vlib)" "$(command -v vcom)"',
                 "modelsim-lookup-test", str(lookup)],
                 cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                 stderr=subprocess.PIPE, check=False,
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip().splitlines(), [
            str(modelsim_bin), str(modelsim_bin / "vlib"), str(modelsim_bin / "vcom"),
        ])


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

    def write_evaluation_fixture(self, name, writer="V", xml_slack="0.250", report_slack="0.250",
                                 args_writer=None, simulation_return="0", include_simulation=True,
                                 include_run=True, cycles="23", total_cycles="23", include_slack=True,
                                 include_library=True, include_timing=True, design_state="Routed",
                                 include_status=True, routing_errors=0, malformed_xml=False,
                                 evaluation_count=1):
        case = self.root / name
        flow = case / "HLS_output/xilinx/flow_backend"
        flow.mkdir(parents=True)
        actual_writer = args_writer or writer
        root = ET.Element("application", {"args": f"bambu --writer={actual_writer}"})
        outputs = ET.SubElement(root, "outputs")
        ET.SubElement(outputs, "file").text = "mul_stage.v"
        library = "panda_libtech.v" if writer == "V" else "panda_libtech.vhd"
        if include_library:
            ET.SubElement(outputs, "file").text = library
            (case / library).write_text("library fixture\n", encoding="utf-8")
        timing = ET.SubElement(root, "timing")
        if include_simulation:
            simulation = ET.SubElement(timing, "simulation", {"return_value": simulation_return})
            if include_run:
                ET.SubElement(simulation, "run").text = "23"
        evaluation_values = {"CYCLES": cycles, "TOTAL_CYCLES": total_cycles}
        if include_slack:
            evaluation_values["CLOCK_SLACK"] = xml_slack
        for _ in range(evaluation_count):
            ET.SubElement(root, "evaluation", evaluation_values)
        results = case / "bambu_results.xml"
        if malformed_xml:
            results.write_text("<broken", encoding="utf-8")
        else:
            ET.ElementTree(root).write(results, encoding="unicode")
        if include_timing:
            state_line = f"| Design State : {design_state}\n" if design_state else ""
            (flow / "post_route_timing_summary.rpt").write_text(
                state_line + f"Setup : 0 Failing Endpoints, Worst Slack {report_slack}ns, Total Violation 0.000ns\n",
                encoding="utf-8",
            )
        if include_status:
            (flow / "post_route_status.rpt").write_text(
                f"# of nets with routing errors.......... : {routing_errors} :\n", encoding="utf-8"
            )
        return case

    def run_evaluation_checker(self, case, writer="V"):
        env = os.environ.copy()
        env.update(TMPDIR=str(self.root), TMP=str(self.root), TEMP=str(self.root))
        return subprocess.run(
            ["python3", str(SUITE / "check_vivado_evaluation.py"), "--writer=" + writer],
            cwd=case, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )

    def test_vivado_evaluation_checker_fails_closed(self):
        for writer in ("V", "H"):
            case = self.write_evaluation_fixture("checker-valid-" + writer, writer=writer)
            result = self.run_evaluation_checker(case, writer)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("routed setup slack 0.250 ns", result.stdout)

        invalid_cases = (
            ("zero_xml_slack", {"xml_slack": "0"}, "CLOCK_SLACK"),
            ("negative_xml_slack", {"xml_slack": "-0.1"}, "CLOCK_SLACK"),
            ("nan_xml_slack", {"xml_slack": "NaN"}, "CLOCK_SLACK"),
            ("infinite_xml_slack", {"xml_slack": "inf"}, "CLOCK_SLACK"),
            ("missing_xml_slack", {"include_slack": False}, "CLOCK_SLACK"),
            ("missing_evaluation", {"evaluation_count": 0}, "expected one evaluation record"),
            ("duplicate_evaluation", {"evaluation_count": 2}, "expected one evaluation record"),
            ("malformed_xml", {"malformed_xml": True}, "cannot parse"),
            ("wrong_writer", {"args_writer": "H"}, "command writer"),
            ("failed_simulation", {"simulation_return": "1"}, "did not return 0"),
            ("missing_simulation", {"include_simulation": False}, "simulation is missing"),
            ("missing_run", {"include_run": False}, "no recorded runs"),
            ("zero_cycles", {"cycles": "0"}, "CYCLES"),
            ("zero_total_cycles", {"total_cycles": "0"}, "TOTAL_CYCLES"),
            ("missing_library", {"include_library": False}, "expected generated"),
            ("missing_timing_report", {"include_timing": False}, "missing post-route evidence"),
            ("unrouted_design", {"design_state": "Synthesized"}, "does not identify a routed design"),
            ("zero_routed_slack", {"report_slack": "0"}, "routed setup Worst Slack"),
            ("negative_routed_slack", {"report_slack": "-0.1"}, "routed setup Worst Slack"),
            ("nan_routed_slack", {"report_slack": "NaN"}, "expected one routed setup Worst Slack value"),
            ("infinite_routed_slack", {"report_slack": "inf"}, "expected one routed setup Worst Slack value"),
            ("slack_mismatch", {"report_slack": "0.500"}, "differs from routed report"),
            ("missing_route_status", {"include_status": False}, "missing post-route evidence"),
            ("routing_error", {"routing_errors": 1}, "zero routing errors"),
        )
        for name, mutations, diagnostic in invalid_cases:
            with self.subTest(case=name):
                case = self.write_evaluation_fixture("checker-" + name, **mutations)
                result = self.run_evaluation_checker(case)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(diagnostic, result.stdout)

    def install_mocks(self):
        self.executable(
            "bambu",
            "import os, re, shlex, shutil, sys\n"
            "import xml.etree.ElementTree as ET\n"
            "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
            "ii=os.environ.get('MANTIS_EXPECT_LOOP_II','1')\n"
            "with open(os.environ['LP_INVOCATIONS'],'a') as f: f.write('bambu\\n')\n"
            "fn=os.environ.get('MANTIS_EXPECT_FUNCTION','f')\n"
            "eval_only=os.environ.get('LP_EVALUATION_ONLY') == '1'\n"
            "if '--simulate' in sys.argv and shutil.which('vsim') is None:\n"
            "  print('ModelSim vsim not found for requested Bambu simulation',file=sys.stderr); sys.exit(10)\n"
            "actual_ii='1' if mode == 'unexpected_pipeline' and ii == '0' else ii\n"
            "if mode != 'bad_schedule':\n"
            "  if ii == '0' and mode != 'unexpected_pipeline': print('Loop pipelining not possible')\n"
            "  else: print('II='+actual_ii+' solution found for loop (BB2)\\nInitiation Interval (II) for function '+fn+':BB2 = '+actual_ii)\n"
            "  latency=os.environ.get('LP_LATENCY')\n"
            "  if latency and mode != 'missing_latency': print('Iteration latency for function '+fn+':BB2 = '+latency)\n"
            "if mode == 'synth_nonzero': sys.exit(9)\n"
            "if mode != 'bad_schedule':\n"
            "  app=ET.Element('application', {'args':'bambu '+shlex.join(sys.argv[1:])})\n"
            "  function=ET.SubElement(ET.SubElement(app,'hls_results'),'function', {'name':fn})\n"
            "  if not (ii == '0' and mode != 'unexpected_pipeline'):\n"
            "   ET.SubElement(function,'loop_0', {'bb':'2','ii':actual_ii})\n"
            "  outputs=ET.SubElement(app,'outputs')\n"
            "  if eval_only:\n"
            "   writer=next((arg.split('=',1)[1] for arg in sys.argv if arg.startswith('--writer=')),'')\n"
            "   library='panda_libtech.v' if writer == 'V' else 'panda_libtech.vhd'\n"
            "   ET.SubElement(outputs,'file').text='mul_stage.v'\n"
            "   ET.SubElement(outputs,'file').text=library\n"
            "   open(library,'w').write('mock generated library\\n')\n"
            "  timing=ET.SubElement(app,'timing')\n"
            "  sim_status='1' if mode == 'bambu_sim_failure' else '0'\n"
            "  simulation=ET.SubElement(timing,'simulation', {'return_value':sim_status})\n"
            "  ET.SubElement(simulation,'run').text='23'\n"
            "  evaluation_values={'CYCLES':'23','TOTAL_CYCLES':'23','TIME':'115','TOTAL_TIME':'115'}\n"
            "  if eval_only:\n"
            "   slack='-0.1' if mode == 'negative_slack' else ('0' if mode == 'zero_slack' else '0.250')\n"
            "   evaluation_values.update({'CLOCK_SLACK':slack,'AREA':'1000','REGISTERS':'145','DSPS':'3','BRAMS':'0','PERIOD':'4.75','FREQUENCY':'210.5'})\n"
            "   flow=os.path.join('HLS_output','xilinx','flow_backend'); os.makedirs(flow,exist_ok=True)\n"
            "   open(os.path.join(flow,'post_route_timing_summary.rpt'),'w').write('| Design State : Routed\\nSetup : 0 Failing Endpoints, Worst Slack '+slack+'ns, Total Violation 0.000ns\\n')\n"
            "   open(os.path.join(flow,'post_route_status.rpt'),'w').write('# of nets with routing errors.......... : 0 :\\n')\n"
            "  ET.SubElement(app,'evaluation',evaluation_values)\n"
            "  ET.ElementTree(app).write('bambu_results.xml',encoding='unicode')\n"
            "if eval_only: sys.exit(0)\n"
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
            "  capture_capacity=os.environ.get('LP_EXPECT_CAPTURE_CAPACITY','')\n"
            "  if mode == 'missing_capture_capacity': capture_capacity=''\n"
            "  capture_capacity_arg=' .CAPACITY('+capture_capacity+'),' if capture_capacity else ''\n"
            "  capture_instance=('TIMED_CAPTURE #(.LATENCY('+capture+'),\\n'+capture_capacity_arg+' .BITSIZE_in1(32)) expected_capture();') if capture else ''\n"
            "  if os.environ.get('LP_EXPECT_CAPTURE_COUNT') and not capture:\n"
            "   capture_count=int(os.environ['LP_EXPECT_CAPTURE_COUNT'])\n"
            "   if mode == 'missing_capture_count': capture_count=0\n"
            "   elif mode == 'unexpected_capture_count' and '--benchmark-name=lp_mul_array' in sys.argv and capture_count == 0: capture_count += 1\n"
            "   capture_instance='\\n'.join('TIMED_CAPTURE #(.LATENCY(2), .BITSIZE_in1(32)) expected_capture_'+str(i)+'();' for i in range(capture_count))\n"
            "  stall=os.environ.get('LP_EXPECT_STALL','')\n"
            "  if mode == 'missing_stall': stall=''\n"
            "  stall_instance=('wire selector_stallable_fus;\\nselector_stallable_fus=1\\n.stall_port(selector_stallable_fus)') if stall else ''\n"
            "  hold=os.environ.get('LP_EXPECT_INPUT_HOLD','')\n"
            "  if mode == 'missing_input_hold': hold=''\n"
            "  hold_instance=('INPUT_HOLD #(.BITSIZE_in1(32)) expected_hold();') if hold else ''\n"
            "  fn=os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_'); dp='datapath_'+fn\n"
            "  phase_capture=bool(os.environ.get('LP_EXPECT_PHASE_CAPTURE'))\n"
            "  dynamic=bool(os.environ.get('LP_EXPECT_NO_MUL_CAPTURE') or os.environ.get('LP_EXPECT_MUL_II2_HOLD') or os.environ.get('LP_EXPECT_INPUT_HOLD') or phase_capture)\n"
            "  mul_name='fu_'+fn+'_930172_930199'; mul_pipe=os.environ.get('LP_EXPECT_PIPE_PARAMETER','2'); mul_pipe='' if mode == 'missing_pipeline' else mul_pipe; mul_params=('#(.PIPE_PARAMETER('+mul_pipe+'))' if mul_pipe else '')\n"
            "  mul='ui_mul_node_FU '+mul_params+' '+mul_name+' (.in1(a), .in2(b));'\n"
            "  if mode == 'missing_multiplier': mul=''\n"
            "  if mode == 'ambiguous_multiplier': mul+='\\nui_mul_node_FU '+mul_params+' fu_'+fn+'_930173_930200 (.in1(c), .in2(d));'\n"
            "  selector='selector_input_hold_fu_mock_930199' if os.environ.get('LP_EXPECT_MUL_II2_HOLD') else 'selector_timed_capture_'+mul_name[3:]\n"
            "  writer='out_bambu_artificial_ParmMgr_Write_fifo_modgen #(.BITSIZE_in1(1)) fu_'+fn+'_930172_930741 (.p_out_write(p_out_write), .start_port({selector_IN_UNBOUNDED_'+fn+'_930172_930741}));'\n"
            "  done=\"assign OUT_UNBOUNDED_\"+fn+\"_930172_930741 = 1'b0;\"\n"
            "  capture_name='timed_capture_'+mul_name[3:]\n"
            "  if mode == 'unexpected_mul_capture': mul+='\\nTIMED_CAPTURE #(.LATENCY(3), .BITSIZE_in1(32)) '+capture_name+'();'\n"
            "  phase_signals=('wire '+selector+'; wire '+selector+'_1; wire selector_IN_UNBOUNDED_'+fn+'_930172_930741;\\n'+writer+'\\n'+('TIMED_CAPTURE #(.LATENCY(2), .BITSIZE_in1(32)) '+capture_name+'();\\n' if mode != 'missing_capture' else '')) if phase_capture else ''\n"
            "  datapath=('module '+dp+'(clock); input clock;\\n'+mul+'\\n'+('wire '+selector+';\\n' if os.environ.get('LP_EXPECT_MUL_II2_HOLD') and mode != 'missing_input_hold' else '')+('wire selector_timed_capture_'+mul_name[3:]+'; wire selector_IN_UNBOUNDED_'+fn+'_930172_930741; wire OUT_UNBOUNDED_'+fn+'_930172_930741;\\n'+writer+'\\n'+done if os.environ.get('LP_EXPECT_INPUT_HOLD') else '')+phase_signals+'\\nendmodule') if dynamic else ''\n"
            "  open('mock.v','w').write('\\n'.join('module '+dut+'(); endmodule'+suffix for dut in duts)+'\\n'+unit_instance+'\\n'+capture_instance+'\\n'+hold_instance+'\\n'+stall_instance+'\\n'+datapath+'\\n')\n",
        )
        for tool in ("vlib", "vlog", "vsim"):
            body = (
                "import os, sys\n"
                "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
                "name=os.path.basename(sys.argv[0])\n"
                "if name == 'vlib': os.makedirs(sys.argv[-1],exist_ok=True)\n"
                "if name == 'vlog' and (os.environ.get('LP_EXPECT_NO_MUL_CAPTURE') or os.environ.get('LP_EXPECT_MUL_II2_HOLD') or os.environ.get('LP_EXPECT_INPUT_HOLD') or os.environ.get('LP_EXPECT_PHASE_CAPTURE')):\n"
                " required=['+define+LP_MUL_INSTANCE=fu_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930199']\n"
                " if os.environ.get('LP_EXPECT_MUL_II2_HOLD'): required += ['+define+LP_MUL_LAUNCH=selector_input_hold_fu_mock_930199']\n"
                " if os.environ.get('LP_EXPECT_INPUT_HOLD'): required += ['+define+LP_MUL_LAUNCH=selector_timed_capture_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930199','+define+LP_UNBOUNDED_LAUNCH=selector_IN_UNBOUNDED_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930741','+define+LP_UNBOUNDED_DONE=OUT_UNBOUNDED_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930741']\n"
                " if os.environ.get('LP_EXPECT_PHASE_CAPTURE'): required += ['+define+LP_MUL_LAUNCH=selector_timed_capture_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930199','+define+LP_MUL_CONSUME=selector_timed_capture_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930199_1','+define+LP_MUL_CAPTURE=timed_capture_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930199','+define+LP_UNBOUNDED_LAUNCH=selector_IN_UNBOUNDED_'+os.environ.get('MANTIS_EXPECT_FUNCTION','_mock').lstrip('_')+'_930172_930741']\n"
                " if any(not any(arg.startswith(want) for arg in sys.argv) for want in required): sys.exit('dynamic hierarchy macros missing: '+repr(required))\n"
                "if name == 'vlog':\n"
                " array_define='+define+LP_ARRAY_INTERFACE' in sys.argv\n"
                " if bool(os.environ.get('LP_EXPECT_ARRAY_INTERFACE')) != array_define: sys.exit('LP_ARRAY_INTERFACE define does not match case expectation')\n"
                "if name == 'vsim':\n"
                " bp=next((a.split('=',1)[1] for a in sys.argv if a.startswith('-gBP_MODE=')),'0')\n"
                " lib=sys.argv[sys.argv.index('-L')+1] if '-L' in sys.argv else ''\n"
                " if 'MUL_HOLD_MUT' in lib:\n"
                "  if mode == 'hold_mut_not_detected': print('FAIL [BP=0] errors=1 count=16 return=4280')\n"
                "  else: print('FAIL [BP=0] errors=1 count=16 return=4280\\nSTALLABLE_HOLD_MUT_DETECTED')\n"
                " elif 'HOLD_MUT' in lib:\n"
                "  if mode == 'hold_mut_not_detected': print('PASS [BP='+bp+'] return=4280')\n"
                "  else: print('FAIL [BP='+bp+'] errors=1 count=16 return=4280\\nHOLD_MUT_DETECTED return=4280')\n"
                " elif 'MUT' in lib:\n"
                "  if mode == 'mut_pass': print('PASS [BP='+bp+']')\n"
                "  elif mode == 'mut_inconclusive': print('INCONCLUSIVE [BP='+bp+'] no stall')\n"
                "  elif mode == 'mut_conflicting': print('FAIL [BP='+bp+'] corrupted input detected\\nPASS [BP='+bp+']')\n"
                "  elif mode != 'mut_no_fail': print('FAIL [BP='+bp+'] corrupted input detected')\n"
                " else:\n"
                "  print('PASS [BP='+bp+']')\n"
                "  if os.environ.get('LP_EXPECT_ARRAY_INTERFACE') and mode != 'array_oracle_missing': print('ARRAY_INTERFACE_MONITOR a_reads=16 b_reads=16 ce_errors=0 address_errors=0 q_errors=0 stall_cycles='+('0' if bp == '0' else '4'))\n"
                "  if os.environ.get('LP_EXPECT_CAPTURE_QUEUE_MIN') and bp == '2': print('CAPTURE_STALL_ORACLE mul_peak=2 required=2')\n"
                "  if os.environ.get('LP_EXPECT_STALL') and bp != '0': print('STALLABLE_STALL_ORACLE stall_cycles=4')\n"
                "  if os.environ.get('LP_EXPECT_MUL_II2_HOLD') and bp == '0': print('STALLABLE_II2_MONITOR launches=16 hold_checks=15 stable_errors=0 operand_changes=15 spacing_min=2 spacing_errors=0 stalled_hold_checks=0 stall_cycles=0')\n"
                "  if os.environ.get('LP_EXPECT_MUL_II2_HOLD') and bp != '0': print('STALLABLE_II2_MONITOR launches=16 hold_checks=15 stable_errors=0 operand_changes=15 spacing_min=2 spacing_errors=0 stalled_hold_checks=2 stall_cycles=4')\n"
                "  if os.environ.get('LP_EXPECT_INPUT_HOLD'):\n"
                "   operand_changes='0' if mode == 'operand_changes_zero' else '3'\n"
                "   if bp == '0':\n"
                "    print('FU_II2_MONITOR launches=4 hold_checks=3 stable_errors=0 operand_changes='+operand_changes+' spacing_min=2 spacing_errors=0 stalled_hold_checks=0')\n"
                "    inflight='0' if mode == 'ii2_no_overlap' else '3'; early='0' if mode == 'ii2_no_overlap' else '3'\n"
                "    print('UNBOUNDED_HANDSHAKE launches=16 inflight_launches='+inflight+' done_same_cycle=16 done_later=0 done_before_ii='+early+' pending=0 handshake_errors=0 accepted_writes=16')\n"
                "   else:\n"
                "    print('FU_II2_MONITOR launches=4 hold_checks=4 stable_errors=0 operand_changes='+operand_changes+' spacing_min=2 spacing_errors=0 stalled_hold_checks=2')\n"
                "    print('UNBOUNDED_HANDSHAKE launches=16 inflight_launches=3 done_same_cycle=2 done_later=14 done_before_ii=0 pending=0 handshake_errors=0 accepted_writes=16')\n"
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

    def test_timed_capture_parallelism_and_isolation(self):
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism([], {}), 1)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism([], {"J": "3"}), 3)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism(["-j24"], {"J": "3"}), 24)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism(["-j", "2"], {}), 2)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism(["--parallel=5"], {}), 5)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism(["--parallel", "7"], {}), 7)
        self.assertEqual(TIMED_CAPTURE.resolve_parallelism(["-j24", "-j", "8", "--parallel=4"], {"J": "2"}), 4)
        case_root = self.root / "timed-capture-cases"
        case_root.mkdir()
        barrier = threading.Barrier(2)
        lock = threading.Lock()
        active = 0
        maximum_active = 0
        directories = set()

        def mock_run(command, cwd, env):
            nonlocal active, maximum_active
            cwd = Path(cwd)
            with lock:
                first_command = cwd not in directories
                if first_command:
                    directories.add(cwd)
                    active += 1
                    maximum_active = max(maximum_active, active)
            if first_command:
                barrier.wait(timeout=10)
            if command[0] != "vsim":
                return ""
            top = command[5]
            language = "Verilog" if top.startswith("verilog_") else "VHDL"
            match = re.search(r"_l(\d+)_c(\d+)", top)
            marker = f"PASS {language} LATENCY={match.group(1)} CAPACITY={match.group(2)}"
            with lock:
                active -= 1
            if marker == "PASS VHDL LATENCY=3 CAPACITY=3":
                raise RuntimeError("injected simulator failure")
            if marker == "PASS Verilog LATENCY=15 CAPACITY=1":
                raise RuntimeError()
            return marker

        with patch.object(TIMED_CAPTURE, "run", side_effect=mock_run):
            results = TIMED_CAPTURE.run_timed_capture_cases(2, "/mock/modelsim/bin", case_root)

        self.assertEqual(len(results), 12)
        self.assertEqual(maximum_active, 2)
        self.assertEqual(len(directories), 12)
        self.assertTrue(all(not directory.exists() for directory in directories))
        failures = [(case, error) for case, marker, error in results if error is not None]
        self.assertEqual(len(failures), 2)
        self.assertEqual(failures[0][0], ("Verilog", 15, 1))
        self.assertEqual(failures[0][1], "RuntimeError: ")
        self.assertEqual(failures[1][0], ("VHDL", 3, 3))
        self.assertIn("injected simulator failure", failures[1][1])
        self.assertEqual(sum(marker is not None for _, marker, _ in results), 10)

        failed_case = (("Verilog", 0, 1), None, "")
        with patch.object(TIMED_CAPTURE, "resolve_modelsim_bin", return_value="/mock/modelsim/bin"), \
                patch.object(TIMED_CAPTURE.shutil, "which", return_value="/mock/modelsim/bin/tool"), \
                patch.object(TIMED_CAPTURE, "run_timed_capture_cases", return_value=[failed_case]), \
                redirect_stderr(StringIO()):
            self.assertEqual(TIMED_CAPTURE.main(["-j1"]), 1)

    def test_remaining_preflight_parallelism_and_failure_reporting(self):
        for name, preflight in (
                ("stallable", STALLABLE_MULTIPLIER),
                ("global", GLOBAL_PIPELINE_STALL)):
            with self.subTest(preflight=name):
                case_root = self.root / f"{name}-parallel-cases"
                case_root.mkdir()
                barrier = threading.Barrier(2)
                lock = threading.Lock()
                active = 0
                maximum_active = 0
                directories = set()

                def task(index):
                    def run_case(work):
                        nonlocal active, maximum_active
                        with lock:
                            active += 1
                            maximum_active = max(maximum_active, active)
                            directories.add(work)
                        barrier.wait(timeout=10)
                        time.sleep(0.01)
                        with lock:
                            active -= 1
                        return index
                    return run_case

                cases = [(f"case-{index}", task(index)) for index in range(2)]
                results = preflight.run_parallel_cases(cases, 2, case_root)
                self.assertEqual([result[0] for result in results], ["case-0", "case-1"])
                self.assertEqual([result[1] for result in results], [0, 1])
                self.assertEqual(maximum_active, 2)
                self.assertEqual(len(directories), 2)
                self.assertTrue(all(not directory.exists() for directory in directories))

                failed = preflight.run_parallel_cases(
                    [("blank_failure", lambda work: (_ for _ in ()).throw(RuntimeError()))],
                    1, case_root,
                )
                self.assertIsNone(failed[0][1])
                self.assertIn("RuntimeError", failed[0][2])
                with redirect_stderr(StringIO()), self.assertRaises(SystemExit) as exit_info:
                    preflight.report_case_results(failed)
                self.assertEqual(exit_info.exception.code, 1)

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
            results = ET.parse(case / "bambu_results.xml").getroot()
            command = shlex.split(results.get("args", ""))
            self.assertIn("--simulate", command, case.name)
            self.assertIn("--simulator=MODELSIM", command, case.name)
            simulation = results.find("./timing/simulation")
            self.assertIsNotNone(simulation, case.name)
            self.assertEqual(simulation.get("return_value"), "0", case.name)
            self.assertTrue(simulation.findall("./run"), case.name)
            evaluation = results.find("./evaluation")
            for metric in ("CYCLES", "TOTAL_CYCLES", "TIME", "TOTAL_TIME"):
                self.assertGreater(float(evaluation.get(metric, "0")), 0, f"{case.name}: {metric}")
            if case.name in EVALUATION_CASES:
                self.assertIn("--evaluation", command, case.name)
                self.assertGreater(float(evaluation.get("CLOCK_SLACK", "0")), 0, case.name)
                self.assertIn("BAMBU_EVALUATION_EXECUTED", (case / "execution.log").read_text())
                continue
            self.assertIn("PASS [BP=0]", (case / "vsim_BP0.log").read_text())
            self.assertIn("PASS [BP=1]", (case / "vsim_BP1.log").read_text())
            self.assertIn("PASS [BP=2]", (case / "vsim_BP2.log").read_text())
            mutant = (case / "vsim_MUT.log").read_text()
            self.assertIn("FAIL [BP=0]", mutant)
            self.assertNotRegex(mutant, r"(?:PASS|INCONCLUSIVE) \[BP=")
        mul_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array"
        self.assertIn("STALLABLE_STALL_ORACLE stall_cycles=4", (mul_case / "vsim_BP2.log").read_text())
        self.assertIn("ARRAY_INTERFACE_MONITOR a_reads=16 b_reads=16 ce_errors=0 address_errors=0 q_errors=0 stall_cycles=4",
                      (mul_case / "vsim_BP2.log").read_text())
        fallback_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array_capture_fallback"
        self.assertIn("ARRAY_INTERFACE_MONITOR a_reads=16 b_reads=16 ce_errors=0 address_errors=0 q_errors=0 stall_cycles=4",
                      (fallback_case / "vsim_BP2.log").read_text())
        self.assertIn("STALLABLE_II2_MONITOR launches=16", (self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array_stall_ii2" / "vsim_BP2.log").read_text())
        self.assertIn("STALLABLE_HOLD_MUT_DETECTED", (self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array_stall_ii2" / "vsim_MUL_HOLD_MUT.log").read_text())
        ii2 = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("HOLD_MUT_DETECTED return=4280", (ii2 / "vsim_HOLD_MUT.log").read_text())

    def test_nonpositive_vivado_slack_fails_both_evaluation_cases(self):
        result = self.run_suite("negative_slack", no_clean=False)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        failed = {case.name for case in self.case_dirs() if (case / "failure.log").exists()}
        self.assertEqual(failed, EVALUATION_CASES)
        for case in self.case_dirs():
            if case.name in EVALUATION_CASES:
                self.assertIn("CLOCK_SLACK must be finite and positive", (case / "failure.log").read_text())
                self.assertTrue((case / "HLS_output/xilinx/flow_backend/post_route_timing_summary.rpt").is_file())
            else:
                self.assertFalse((case / "vsim_BP0.log").exists(), case.name)
        self.cleanup_output()

    def test_restart_rechecks_cached_vivado_evidence(self):
        first = self.run_suite()
        self.assertEqual(first.returncode, 0, first.stdout)
        first_count = self.invocations.read_text().count("bambu\n")
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array_vivado_verilog"
        report = case / "HLS_output/xilinx/flow_backend/post_route_timing_summary.rpt"
        report.write_text("invalid cached timing evidence\n", encoding="utf-8")

        restarted = self.run_suite("ok", ("--restart",))
        self.assertEqual(restarted.returncode, 0, restarted.stdout)
        self.assertEqual(self.invocations.read_text().count("bambu\n"), first_count + 1)
        self.assertIn("Worst Slack 0.250ns", report.read_text())

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
                self.assertEqual(self.junit_failures(), HDL_CASE_COUNT)
                for case in self.case_dirs():
                    if case.name in EVALUATION_CASES:
                        self.assertFalse((case / "failure.log").exists())
                        continue
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
        self.assertEqual(self.junit_failures(), 7)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_mul_array"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(2)", (case / "failure.log").read_text())
        div_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_div_array"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(15)", (div_case / "failure.log").read_text())
        ii2_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(2)", (ii2_case / "failure.log").read_text())
        phase_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_phase_capture"
        self.assertIn("generated RTL lacks PIPE_PARAMETER(2)", (phase_case / "failure.log").read_text())
        self.cleanup_output()

    def test_stallable_rtl_contract_is_required(self):
        for mode, cases, message in (
            ("missing_stall", {"lp_mul_array", "lp_mul_array_capture_fallback", "lp_mul_array_stall_ii2", "lp_co_launch"},
             "generated RTL lacks shared stall selector"),
            ("unexpected_mul_capture", {"lp_mul_array", "lp_mul_array_capture_fallback", "lp_mul_array_stall_ii2", "lp_co_launch"},
             "generated RTL still captures selected multiplier result"),
        ):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                failed = {case.name for case in self.case_dirs() if (case / "failure.log").exists()}
                self.assertEqual(failed, cases)
                for case in self.case_dirs():
                    if case.name in cases:
                        self.assertIn(message, (case / "failure.log").read_text())
                self.cleanup_output()

    def test_array_interface_monitor_is_required(self):
        result = self.run_suite("array_oracle_missing")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        failed = {case.name for case in self.case_dirs() if (case / "failure.log").exists()}
        self.assertEqual(failed, {"lp_mul_array", "lp_mul_array_capture_fallback"})
        for case in self.case_dirs():
            if case.name in failed:
                self.assertIn("array interface hold/request oracle failed", (case / "failure.log").read_text())
        self.cleanup_output()

    def test_array_interface_monitor_is_optional_without_flag(self):
        case = self.root / "unflagged-array-case"
        case.mkdir()
        env = os.environ.copy()
        env.update(
            PATH=str(self.bin) + os.pathsep + env.get("PATH", ""),
            BAMBU_HLS_BACKEND_PATH="",
            LP_MOCK_MODE="ok",
            LP_INVOCATIONS=str(self.invocations),
            LP_SUITE=str(SUITE),
            LP_TB="tb_lp_mul_array.v",
            LP_TB_TOP="tb_lp_mul_array",
            MANTIS_EXPECT_LOOP_II="1",
            MANTIS_EXPECT_FUNCTION="_Z9mul_stageRA16_iS0_R10ac_channelIiE",
            LP_EXPECT_PIPE_PARAMETER="2",
            TMPDIR=str(SCRATCH),
            TMP=str(SCRATCH),
            TEMP=str(SCRATCH),
        )
        env.pop("LP_EXPECT_ARRAY_INTERFACE", None)
        result = subprocess.run(
            [str(SUITE / "run_case.sh"), "lp_mul_array.cpp", "--top-fname=mul_stage"],
            cwd=case, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("PASS [BP=0]", (case / "vsim_BP0.log").read_text())
        self.assertNotIn("ARRAY_INTERFACE_MONITOR", (case / "vsim_BP0.log").read_text())

    def test_dynamic_multiplier_identifiers_fail_closed(self):
        for mode in ("missing_multiplier", "ambiguous_multiplier"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                failures = {
                    case.name: (case / "failure.log").read_text()
                    for case in self.case_dirs()
                    if (case / "failure.log").exists()
                }
                self.assertEqual(
                    set(failures),
                    {"lp_mul_array", "lp_mul_array_capture_fallback", "lp_mul_array_stall_ii2", "lp_co_launch", "lp_co_launch_fu_ii2", "lp_phase_capture"},
                )
                expected = "expected one specialized multiplier instance" if mode == "missing_multiplier" else "found 2"
                for log in failures.values():
                    self.assertIn(expected, log)
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
        ii2_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("TIMED_CAPTURE instance with LATENCY(2)", (ii2_case / "failure.log").read_text())
        phase_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_phase_capture"
        self.assertIn("TIMED_CAPTURE instance with LATENCY(2)", (phase_case / "failure.log").read_text())
        self.cleanup_output()

    def test_array_capture_count_is_enforced(self):
        for mode, expected_case, message in (
            ("missing_capture_count", "lp_mul_array_capture_fallback", "expected 2 TIMED_CAPTURE instances, found 0"),
            ("unexpected_capture_count", "lp_mul_array", "expected 0 TIMED_CAPTURE instances, found 1"),
        ):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                failed = {case.name for case in self.case_dirs() if (case / "failure.log").exists()}
                self.assertEqual(failed, {expected_case})
                case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / expected_case
                self.assertIn(message, (case / "failure.log").read_text())
                self.cleanup_output()

    def test_multiplier_capture_capacity_is_required(self):
        result = self.run_suite("missing_capture_capacity")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 1)
        cases = {"lp_phase_capture"}
        for case in self.case_dirs():
            if case.name in cases:
                self.assertIn("LATENCY(2) and CAPACITY(3)", (case / "failure.log").read_text())
        self.assertEqual({case.name for case in self.case_dirs() if (case / "failure.log").exists()}, cases)
        self.cleanup_output()

    def test_ii2_requires_the_selected_fu_model_and_input_hold(self):
        for mode, expected in (("fu_ii1", "model ui_mul_node_FU_32_32_32_2 does not declare"),
                               ("missing_input_hold", "generated RTL lacks INPUT_HOLD")):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
                self.assertIn(expected, (case / "failure.log").read_text())
                if mode == "fu_ii1":
                    self.assertEqual(self.junit_failures(), 3)
                    phase_case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_phase_capture"
                    self.assertIn(expected, (phase_case / "failure.log").read_text())
                else:
                    self.assertEqual(self.junit_failures(), 2)
                self.cleanup_output()

    def test_hold_oracle_mutation_must_fail_without_corrupting_checksum(self):
        result = self.run_suite("hold_mut_not_detected")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 2)
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

    def test_ii2_monitor_requires_inflight_overlap_and_early_completion(self):
        result = self.run_suite("ii2_no_overlap")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 1)
        case = self.output_dir() / "LP-ARRAY-CHANNEL-MODELSIM" / "lp_co_launch_fu_ii2"
        self.assertIn("unbounded operation lacked an in-flight overlap or completion before FU II expiry",
                      (case / "failure.log").read_text())
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
                self.assertEqual(self.junit_failures(), HDL_CASE_COUNT)
                for case in self.case_dirs():
                    if case.name in EVALUATION_CASES:
                        self.assertFalse((case / "failure.log").exists())
                        continue
                    self.assertTrue((case / "failure.log").exists(), (case / "execution.log").read_text())
                    self.assertTrue((case / "mock.v").exists())
                    self.assertTrue((case / "vlog_BP0.log").exists())
                    self.assertTrue((case / "vsim_BP0.log").exists())
                self.cleanup_output()

    def test_missing_rtl_and_modelsim_are_failures(self):
        result = self.run_suite("missing_rtl")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), HDL_CASE_COUNT)
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
            self.assertIn("ModelSim vsim not found for requested Bambu simulation",
                          (case / "bambu.log").read_text())

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
        self.assertIn("assign launch_valid = launch && !(1RESET_VALUE);", verilog)
        self.assertIn("launch_delay <= (launch_delay << 1) | launch_valid;", verilog)
        self.assertIn("launch_delay <= launch_delay(LATENCY-2 downto 0) & launch_valid;", vhdl)
        self.assertIn('parameter name="CAPACITY">1</parameter>', ET.tostring(cell, encoding="unicode"))
        self.assertIn(
            'LIBRARY="TIMED_CAPTURE LATENCY CAPACITY in1 consume out1"',
            ET.tostring(model, encoding="unicode"),
        )
        self.assertIn("assign out1 = (LATENCY != 0 && capture_due) ? in1 : captured;", verilog)
        self.assertIn("wire bypass = capture_due && (count == 0);", verilog)
        self.assertIn("wire push = capture_valid && !(count == 0 && consume);", verilog)
        self.assertIn("out1 <= in1 when LATENCY /= 0 and capture_due = '1' else captured;", vhdl)
        self.assertIn("out1 <= in1 when capture_due = '1' and count = 0 else captured(read_ptr);", vhdl)
        self.assertIn("push := capture_valid = '1' and not (count = 0 and consume = '1');", vhdl)


if __name__ == "__main__":
    unittest.main()
