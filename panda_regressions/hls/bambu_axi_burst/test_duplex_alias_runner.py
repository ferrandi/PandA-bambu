#!/usr/bin/env python3
"""Offline integration tests for the duplex alias simulation runner."""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
os.environ.update(TMPDIR=str(TMP_ROOT), TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT))
RUNNER = HERE / "run_duplex_alias_tests.sh"
SCENARIOS = {1: "disjoint", 2: "forward-overlap", 3: "reverse-overlap", 4: "zero-trip",
             5: "nested-changing-base", 6: "nested-changing-count"}


class DuplexAliasRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="duplex-alias-runner-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.prefix = self.root / "install"
        (self.prefix / "bin").mkdir(parents=True)
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("offline stub plugin\n")
        (self.prefix / "settings.sh").write_text(f"export BAMBU_HLS='{self.prefix}'\n")
        self.bambu = self.prefix / "bin/bambu"
        self.bambu.write_text(self.compiler_stub())
        self.bambu.chmod(0o755)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        for tool in ("vsim", "vlib", "vlog", "vmap"):
            simulator_tool = self.bin / tool
            simulator_tool.write_text(self.modelsim_stub())
            simulator_tool.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(PATH=f"{self.bin}:{os.environ['PATH']}", TMPDIR=str(TMP_ROOT),
                        TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT), PYTHONDONTWRITEBYTECODE="1")

    def tearDown(self) -> None:
        self.temp.cleanup()

    @staticmethod
    def modelsim_stub() -> str:
        return r'''#!/usr/bin/env bash
set -eu
name=${0##*/}
case $name in
  vlib) mkdir -p work ;;
  vmap) printf 'modelsim.ini\n' > modelsim.ini ;;
  vlog) printf 'ModelSim vlog offline stub\n' ;;
  vsim)
    [[ ${1:-} != -version && ${1:-} != --version ]] || { echo 'QuestaSim offline stub'; exit 0; }
    case_name=
    for arg in "$@"; do [[ $arg != +CASE=* ]] || case_name=${arg#+CASE=}; done
    [[ -n $case_name ]]
    if [[ ${RANGE_PROBE_MODE:-good} == missing ]]; then exit 0; fi
    case $case_name in
      valid-disjoint) marker='DUPLEX_ALIAS_RANGE_PASS CASE=valid-disjoint ARLEN=15' ;;
      read-end-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=read-end-overflow ARLEN=0' ;;
      write-end-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=write-end-overflow ARLEN=0' ;;
      byte-count-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=byte-count-overflow ARLEN=0' ;;
      read-end-equals-2p32) marker='DUPLEX_ALIAS_RANGE_PASS CASE=read-end-equals-2p32 ARLEN=15' ;;
      high-bit-valid-disjoint) marker='DUPLEX_ALIAS_RANGE_PASS CASE=high-bit-valid-disjoint ARLEN=15 ARADDR=0000000080001000' ;;
      *) exit 3 ;;
    esac
    printf '# %s\n# DUPLEX_ALIAS_RANGE_GUARD_PASS CASES=1 NO_AXI_HANDSHAKES=1\n' "$marker"
    printf '$enddefinitions $end\n#0\n' > "range_guard_${case_name}.vcd"
    ;;
esac
'''

    @staticmethod
    def compiler_stub() -> str:
        return r'''#!/usr/bin/env bash
set -eu
if [[ ${1:-} == --version ]]; then echo "stub bambu 1"; exit 0; fi
if [[ ${FAKE_BAMBU_FAIL:-} == 1 ]]; then echo "forced compiler failure" >&2; exit 9; fi
profile= case_id= address_bits=
float_payload=0
for arg in "$@"; do
  case $arg in
    --extra-cc-options=-DDUPLEX_ALIAS_PROFILE=*) profile=${arg##*=};;
    --extra-cc-options=-DDUPLEX_ALIAS_CASE=*) case_id=${arg##*=};;
    --extra-cc-options=-DDUPLEX_ALIAS_FLOAT_BITS) float_payload=1;;
    --addr-bus-bitsize=*) address_bits=${arg#*=};;
  esac
done
[[ -n $profile && -n $case_id && -n $address_bits ]]
case ${SCENARIO_MARKER_MODE:-good} in
  wrong) scenario=unexpected ;;
  missing) scenario= ;;
  *) case $case_id in 1) scenario=disjoint;; 2) scenario=forward-overlap;; 3) scenario=reverse-overlap;; 4) scenario=zero-trip;; 5) scenario=nested-changing-base;; 6) scenario=nested-changing-count;; esac ;;
esac
if [[ $scenario == nested-changing-base || $scenario == nested-changing-count ]]; then
  [[ ${NESTED_FALLBACK_MODE:-good} != activation ]] ||
    printf '%s\n' '---Activated guarded distinct-base duplex for bundle shared (overlap uses serialized scalar transfers)'
else
  case ${ACTIVATION_MODE:-good} in
    missing) ;;
    replaced) printf '%s\n' 'Activated same-base pointwise duplex m_axi bundle shared for root kernel' ;;
    *) printf '%s\n' '---Activated guarded distinct-base duplex for bundle shared (overlap uses serialized scalar transfers)' ;;
  esac
fi
proof='{"bundle":"shared","semantic_proof":true,"runtime_range_guard":true,"proof_kind":"distinct_base_runtime_guarded_pointwise"}'
if [[ $scenario == nested-changing-base || $scenario == nested-changing-count ]]; then
  proof='{"bundle":"shared","semantic_proof":false,"semantic_reason":"runtime alias fallback requires one straight-line shared access block","runtime_range_guard":false}'
  [[ ${NESTED_FALLBACK_MODE:-good} != proof-success ]] ||
    proof='{"bundle":"shared","semantic_proof":true,"runtime_range_guard":true,"proof_kind":"distinct_base_runtime_guarded_pointwise"}'
fi
debug_proof=0
for arg in "$@"; do [[ $arg != --debug-classes=InterfaceInfer,BuildVirtualPhi ]] || debug_proof=1; done
if ((debug_proof)); then
  case ${PROOF_MODE:-good} in
    missing) ;;
    malformed) printf '%s\n' 'AXI_BURST_DUPLEX_PROOF {not-json}' ;;
    no-semantic) printf '%s\n' 'AXI_BURST_DUPLEX_PROOF {"bundle":"shared","semantic_proof":false,"runtime_range_guard":true,"proof_kind":"distinct_base_runtime_guarded_pointwise"}' ;;
    no-runtime-guard) printf '%s\n' 'AXI_BURST_DUPLEX_PROOF {"bundle":"shared","semantic_proof":true,"runtime_range_guard":false,"proof_kind":"distinct_base_runtime_guarded_pointwise"}' ;;
    wrong-kind) printf '%s\n' 'AXI_BURST_DUPLEX_PROOF {"bundle":"shared","semantic_proof":true,"runtime_range_guard":true,"proof_kind":"distinct_base_restricted_pointwise"}' ;;
    wrong-bundle) printf '%s\n' 'AXI_BURST_DUPLEX_PROOF {"bundle":"other","semantic_proof":true,"runtime_range_guard":true,"proof_kind":"distinct_base_runtime_guarded_pointwise"}' ;;
    duplicate) printf 'AXI_BURST_DUPLEX_PROOF %s\nAXI_BURST_DUPLEX_PROOF %s\n' "$proof" "$proof" ;;
    *) printf 'AXI_BURST_DUPLEX_PROOF %s\n' "$proof" ;;
  esac
fi
mkdir -p HLS_output/simulation
printf '<results/\n' > bambu_results.xml
read_outstanding=1; write_burst=16; write_fifo=32
[[ $profile != 2 ]] || { read_outstanding=2; write_burst=8; write_fifo=16; }
read_addr=$address_bits; write_addr=$address_bits
if [[ ${RTL_PROFILE_MODE:-good} == mismatch64 ]]; then read_addr=32; write_addr=32; fi
if [[ ($scenario == nested-changing-base || $scenario == nested-changing-count) && ${NESTED_FALLBACK_MODE:-good} != engine ]]; then
  printf 'module kernel; MinimalAXI4AdapterSingleBeat #(.BURST_TYPE(1)) scalar_adapter (); endmodule\n' > kernel.v
elif [[ ${RTL_PROFILE_MODE:-good} == split ]]; then
  printf 'module kernel; MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(%s), .FIFO_DEPTH(64), .BITSIZE_in4(32), .BITSIZE_m_axi_araddr(16), .BITSIZE_in5(32), .BITSIZE_m_axi_rdata(32)) read_profile_engine (); MinimalAXI4MasterPipelined #(.B_MAX(8), .MAX_OUTSTANDING(%s), .FIFO_DEPTH(64), .BITSIZE_in4(32), .BITSIZE_m_axi_araddr(32), .BITSIZE_in5(32), .BITSIZE_m_axi_rdata(32)) read_width_engine (); MinimalAXI4MasterPipelinedWrite #(.B_MAX(%s), .MAX_OUTSTANDING(1), .FIFO_DEPTH(%s), .BITSIZE_in4(32), .BITSIZE_m_axi_awaddr(32), .BITSIZE_in5(32), .BITSIZE_in3(32), .BITSIZE_m_axi_wdata(32)) write_engine (); endmodule\n' \
    "$read_outstanding" "$read_outstanding" "$write_burst" "$write_fifo" > kernel.v
else
  printf 'module kernel; MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(%s), .FIFO_DEPTH(64), .BITSIZE_in4(%s), .BITSIZE_m_axi_araddr(%s), .BITSIZE_in5(32), .BITSIZE_m_axi_rdata(32)) read_engine (); MinimalAXI4MasterPipelinedWrite #(.B_MAX(%s), .MAX_OUTSTANDING(1), .FIFO_DEPTH(%s), .BITSIZE_in4(%s), .BITSIZE_m_axi_awaddr(%s), .BITSIZE_in5(32), .BITSIZE_in3(32), .BITSIZE_m_axi_wdata(32)) write_engine (); endmodule\n' \
    "$read_outstanding" "$read_addr" "$read_addr" "$write_burst" "$write_fifo" "$write_addr" "$write_addr" > kernel.v
fi
printf 'module library_stub; endmodule\n' > panda_libtech.v
if [[ -n $scenario ]]; then
  count=64
  [[ $scenario != zero-trip ]] || count=0
  [[ $scenario != nested-changing-base && $scenario != nested-changing-count ]] || count=4
  marker="DUPLEX_ALIAS_GOLDEN_PASS CASE=$scenario profile=$profile N=$count"
  [[ $scenario != zero-trip ]] || marker+=" NO_DATA_CHANGE=1"
  [[ $scenario != nested-changing-base && $scenario != nested-changing-count ]] || marker+=" OUTER=4"
  [[ $float_payload == 0 ]] || marker+=" PAYLOAD=FLOAT32_BITS"
  printf '%s\n' "$marker" > HLS_output/simulation/testbench.log
  if [[ $scenario == zero-trip ]]; then
    case ${VCD_MODE:-good} in
      missing) ;;
      read-high) ar_value=1; aw_value=0 ;;
      write-high) ar_value=0; aw_value=1 ;;
      unknown) ar_value=x; aw_value=0 ;;
      *) ar_value=0; aw_value=0 ;;
    esac
    if [[ ${VCD_MODE:-good} != missing ]]; then
      cat > HLS_output/simulation/test.vcd <<EOF
\$scope module bambu_testbench \$end
\$var wire 1 ! m_axi_shared_arvalid \$end
\$var wire 1 \" m_axi_shared_awvalid \$end
\$upscope \$end
\$enddefinitions \$end
#0
$ar_value!
$aw_value\"
EOF
    fi
  fi
  if [[ $scenario == forward-overlap || $scenario == reverse-overlap ]]; then
    python3 - <<'PY'
import os
from pathlib import Path

mode = os.environ.get("VCD_MODE", "good")
clock_mode = os.environ.get("CLOCK_MODE", "good")
events = {cycle: [] for cycle in range(1, 5000)}
for index in range(64):
    base = index * 67 + 1
    ar_cycle = base - 2 if mode == "early-ar" and index == 1 else base
    events[ar_cycle].append("ar")
    events[base + 1].append("aw")
    events[base + 2].append("w")
    response = base + (5 if mode == "short-b-delay" else 66)
    events[response].append("b")

signals = {"ar": "!", "aw": "%", "w": "'", "b": ")"}
system_clocks = {
    "good": ['$var wire 1 # clock $end'],
    "missing-system": [],
    "ambiguous-system": ['$var wire 1 q clock $end', '$var wire 1 r clock $end'],
}[clock_mode]
lines = [
    "$version synthetic duplex alias testbench $end",
    "$timescale 1ps $end",
    "$scope module clocked_bambu_testbench $end",
    '$var reg 1 " clock $end',
    "$scope module bambu_testbench $end",
    "$var wire 1 # clock $end",
    "$scope module system $end",
    *system_clocks,
    "$var wire 1 ! sig_m_axi_shared_arvalid $end",
    "$var wire 1 $ sig_m_axi_shared_arready $end",
    "$var wire 1 % sig_m_axi_shared_awvalid $end",
    "$var wire 1 & sig_m_axi_shared_awready $end",
    "$var wire 1 ' sig_m_axi_shared_wvalid $end",
    "$var wire 1 ( sig_m_axi_shared_wready $end",
    "$var wire 1 ) sig_m_axi_shared_bvalid $end",
    "$var wire 1 + sig_m_axi_shared_bready $end",
    "$upscope $end",
    "$upscope $end",
    "$upscope $end",
    "$enddefinitions $end",
    "#0", "0#", '0"', "0!", "1$", "0%", "1&", "0'", "1(", "0)", "1+",
]
for cycle in range(1, max(events) + 1):
    time = cycle * 10
    active = events.get(cycle, [])
    lines.append(f"#{time}")
    lines.append("1#")
    lines.extend(f"1{signals[event]}" for event in active)
    lines.append(f"#{time + 1}")
    lines.extend(f"0{signals[event]}" for event in active)
    lines.append(f"#{time + 2}")
    lines.append("0#")
Path("HLS_output/simulation/test.vcd").write_text("\n".join(lines) + "\n")
PY
  fi
  if [[ $scenario != zero-trip && $scenario != forward-overlap && $scenario != reverse-overlap ]]; then
    : > HLS_output/simulation/test.vcd
  fi
else
  printf 'simulation completed without the expected marker\n' > HLS_output/simulation/testbench.log
fi
if ((debug_proof)) && [[ $case_id == 1 || $case_id == 2 || $case_id == 3 ]]; then
  mkdir -p panda-temp HLS_output/dot/kernel
  cat > panda-temp/after_Frontend::InterfaceInfer.raw <<'RAW'
@1 identifier_node strg: "kernel"
@2 function_val_node name: @1 body: @3
@3 statement_list_node bloc: 1 loop_id: 1 succ: 2 bloc: 2 loop_id: 1 succ: 1
@10 identifier_node strg: "shared_bambu_artificial_ParmMgr_configure_read"
@11 function_val_node name: @10
@12 addr_node op: @11
@13 identifier_node strg: "shared_bambu_artificial_ParmMgr_drain_write"
@14 function_val_node name: @13
@15 addr_node op: @14
@20 call_stmt parent: @2 bb_index: 1 vuse: @40 vdef: @41 fn: @12
@21 call_stmt parent: @2 bb_index: 2 vdef: @40 fn: @15
RAW
  cat > 'panda-temp/after_Frontend::BuildVirtualPhi::kernel(1)[1].raw' <<'RAW'
@1 identifier_node strg: "kernel"
@2 function_val_node name: @1 body: @3
@3 statement_list_node bloc: 3 loop_id: 0 succ: 2 bloc: 2 loop_id: 1 pred: 2 pred: 3 succ: 2
@10 identifier_node strg: "shared_bambu_artificial_ParmMgr_configure_read"
@11 function_val_node name: @10
@12 addr_node op: @11
@13 identifier_node strg: "shared_bambu_artificial_ParmMgr_drain_write"
@14 function_val_node name: @13
@15 addr_node op: @14
@20 call_stmt parent: @2 bb_index: 2 vuse: @40 vuse: @43 vdef: @41 fn: @12
@21 call_stmt parent: @2 bb_index: 2 vdef: @40 vover: @43 fn: @15
@22 phi_stmt parent: @2 bb_index: 2 res: @43 def: @42 edge: 3 def: @40 edge: 2 virtual
@40 ssa_node vers: 49 def_stmt: @21 use_stmt: @20 use_stmt: @22 virtual
@41 ssa_node def_stmt: @20 virtual
@42 ssa_node def_stmt: @44 use_stmt: @22 virtual
@43 ssa_node def_stmt: @22 use_stmt: @20 use_stmt: @21 virtual
@44 nop_stmt parent: @2 bb_index: 3
RAW
  cat > HLS_output/dot/kernel/OP_ADG.dot <<'DOT'
digraph G {
1 [label="kernel_2_20 - configure"];
2 [label="kernel_2_21 - drain"];
1->2 [color=green4,label="_49"];
}
DOT
  cat > HLS_output/dot/kernel/OP_FDFG.dot <<'DOT'
digraph G {
1 [label="kernel_2_20 - configure"];
2 [label="kernel_2_21 - drain"];
2->1 [color=lightblue,label="_49"];
}
DOT
fi
printf 'stub compiler stdout\n'
'''

    def invoke(self, out: Path | None = None, **extra_env: str) -> subprocess.CompletedProcess[str]:
        env = self.env.copy()
        env.update(extra_env)
        args = ["bash", str(RUNNER), str(self.bambu)]
        if out is not None:
            args.append(str(out))
        return subprocess.run(args, cwd=ROOT, env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def check_output(self, out: Path, address_bits: int,
                     proof_mode: str = "functional") -> subprocess.CompletedProcess[str]:
        args = ["python3", "-B", str(HERE / "check_duplex_alias_results.py"), str(out),
                "--source", str(HERE / "bambu_axi_burst_duplex_alias.cpp"),
                "--testbench", str(HERE / "bambu_axi_burst_duplex_alias_tb.cpp"),
                "--bambu", str(self.bambu), "--settings", str(self.prefix / "settings.sh"),
                "--plugin", str(self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"),
                "--simulator", str(self.bin / "vsim"), "--runner", str(RUNNER),
                "--range-probe-runner", str(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/run_duplex_alias_range_guard.sh"),
                "--range-probe-testbench", str(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/tb_duplex_alias_range_guard.sv"),
                "--addr-bits", str(address_bits), "--proof-mode", proof_mode]
        return subprocess.run(args, cwd=ROOT, env=self.env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_runs_exact_eight_case_matrix_with_isolated_dirs_and_digests(self) -> None:
        out = self.root / "fresh-output"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", DUPLEX_ALIAS_JOBS="3")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("DUPLEX_ALIAS_CHECK PASS PROFILES=2 SCENARIOS=12", result.stdout)
        self.assertEqual((out / "aggregate.log").read_text().splitlines()[0].split()[0], "DUPLEX_ALIAS_PASS")
        float_out = out / "float_payload"
        float_result = self.check_output(float_out, 32)
        self.assertEqual(float_result.returncode, 0, float_result.stdout)
        self.assertIn("DUPLEX_ALIAS_CHECK PASS PAYLOAD=FLOAT32_BITS PROFILES=1 SCENARIOS=3", float_result.stdout)
        self.assertIn("payload_bits=1", (float_out / "provenance.txt").read_text())
        directories = []
        for profile, profile_id in (("symmetric", "1"), ("asymmetric", "2")):
            for case_id, scenario in SCENARIOS.items():
                case_dir = out / profile / scenario
                directories.append(case_dir)
                count = 0 if scenario == "zero-trip" else 4 if scenario.startswith("nested-") else 64
                marker = f"DUPLEX_ALIAS_GOLDEN_PASS CASE={scenario} profile={profile_id} N={count}"
                if scenario == "zero-trip":
                    marker += " NO_DATA_CHANGE=1"
                if scenario.startswith("nested-"):
                    marker += " OUTER=4"
                self.assertEqual((case_dir / "HLS_output/simulation/testbench.log").read_text().strip(), marker)
                self.assertTrue((case_dir / "tmp").is_dir())
                provenance = (case_dir / "provenance.txt").read_text()
                self.assertIn("git_head=", provenance)
                self.assertIn("source_sha256=", provenance)
                self.assertIn("runner_sha256=", provenance)
                argv = shlex.split(next(line for line in provenance.splitlines() if line.startswith("argv="))[5:])
                self.assertIn("--simulator=MODELSIM", argv)
                self.assertNotIn("--debug-classes=InterfaceInfer,BuildVirtualPhi", argv)
                self.assertEqual(dict(line.split("=", 1) for line in provenance.splitlines()
                                      if "=" in line).get("proof_mode"), "functional")
                self.assertNotIn("AXI_BURST_DUPLEX_PROOF", "\n".join(
                    (case_dir / name).read_text() for name in ("stdout.log", "stderr.log")))
                self.assertIn("--tb-memory-mapping=SHARED", argv)
                self.assertIn("--memory-allocation-policy=NO_BRAM", argv)
                self.assertNotIn("--device-name", argv)
                self.assertNotIn("--target-file", argv)
                expected_vcd = scenario in ("forward-overlap", "reverse-overlap", "zero-trip")
                self.assertEqual("--generate-vcd" in argv, expected_vcd)
                self.assertEqual("--mem-delay-write=64" in argv,
                                 scenario in ("forward-overlap", "reverse-overlap"))
                for row in (case_dir / "artifact-sha256.txt").read_text().splitlines():
                    digest, relative = row.split("  ", 1)
                    self.assertEqual(hashlib.sha256((case_dir / relative).read_bytes()).hexdigest(), digest)
                vcd = case_dir / "HLS_output/simulation/test.vcd"
                self.assertTrue(vcd.is_file())
                self.assertEqual(vcd.stat().st_size > 0, expected_vcd)
        for scenario in ("disjoint", "forward-overlap", "reverse-overlap"):
            case_dir = float_out / "symmetric" / scenario
            marker = f"DUPLEX_ALIAS_GOLDEN_PASS CASE={scenario} profile=1 N=64 PAYLOAD=FLOAT32_BITS"
            self.assertEqual((case_dir / "HLS_output/simulation/testbench.log").read_text().strip(), marker)
            self.assertIn("payload_bits=1", (case_dir / "provenance.txt").read_text())
        self.assertEqual(len({path.resolve() for path in directories}), 12)
        self.assertTrue(all(path.is_dir() for path in directories))
        probe_dir = out / "rtl_range_guard"
        probe_log = (probe_dir / "probe.log").read_text()
        self.assertIn("CASE=read-end-overflow ARLEN=0", probe_log)
        probe_provenance = (probe_dir / "provenance.txt").read_text()
        probe_argv = shlex.split(next(line for line in probe_provenance.splitlines()
                                      if line.startswith("argv="))[5:])
        self.assertIn("-sv", probe_argv)
        self.assertIn(str((out / "symmetric/disjoint/kernel.v").resolve()), probe_argv)
        self.assertIn(str((out / "symmetric/disjoint/panda_libtech.v").resolve()), probe_argv)
        self.assertIn(str(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/tb_duplex_alias_range_guard.sv"), probe_argv)
        self.assertIn("DUPLEX_ALIAS_RANGE_PROBE PASS CASES=5 ADDR_BITS=32", (out / "rtl_range_guard_runner.log").read_text())
        for case_name in ("valid-disjoint", "read-end-overflow", "write-end-overflow",
                          "byte-count-overflow", "read-end-equals-2p32"):
            self.assertTrue((probe_dir / f"range_guard_{case_name}.vcd").is_file())

    def test_64_bit_address_bus_keeps_pointer_arguments_32_bit(self) -> None:
        out = self.root / "address-64"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                             DUPLEX_ALIAS_ADDRESS_BITS="64")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("DUPLEX_ALIAS_RANGE_PROBE PASS CASES=6 ADDR_BITS=64", result.stdout)
        root_fields = dict(line.split("=", 1) for line in (out / "provenance.txt").read_text().splitlines()
                           if "=" in line and not line.startswith("argv="))
        self.assertEqual(root_fields["addr_bits"], "64")
        for profile in ("symmetric", "asymmetric"):
            case_dir = out / profile / "disjoint"
            fields = dict(line.split("=", 1) for line in (case_dir / "provenance.txt").read_text().splitlines()
                          if "=" in line and not line.startswith("argv="))
            self.assertEqual(fields["addr_bits"], "64")
            argv = shlex.split(next(line for line in (case_dir / "provenance.txt").read_text().splitlines()
                                    if line.startswith("argv="))[5:])
            self.assertEqual(argv.count("--addr-bus-bitsize=64"), 1)
            rtl = (case_dir / "kernel.v").read_text()
            self.assertIn("BITSIZE_in4(64)", rtl)
            self.assertIn("BITSIZE_in5(32)", rtl)
            self.assertIn("BITSIZE_in3(32)", rtl)
        self.assertIn("CASE=high-bit-valid-disjoint ARLEN=15 ARADDR=0000000080001000",
                      (out / "rtl_range_guard/probe.log").read_text())

    def test_invalid_address_width_is_rejected_before_output_creation(self) -> None:
        out = self.root / "invalid-address-width"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                             DUPLEX_ALIAS_ADDRESS_BITS="48")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be 32 or 64", result.stdout)
        self.assertFalse(out.exists())

    def test_checker_rejects_address_width_provenance_and_argv_mismatch(self) -> None:
        out = self.root / "provenance-mismatch"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1")
        self.assertEqual(result.returncode, 0, result.stdout)
        scenario_file = out / "symmetric/disjoint/provenance.txt"
        original = scenario_file.read_text()
        scenario_file.write_text(original.replace("--addr-bus-bitsize=32", "--addr-bus-bitsize=64"))
        result = self.check_output(out, 32)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("must select AXI address width 32 exactly once", result.stdout)
        scenario_file.write_text(original)
        root_file = out / "provenance.txt"
        original_root = root_file.read_text()
        root_file.write_text(original_root.replace("addr_bits=32", "addr_bits=64"))
        result = self.check_output(out, 32)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("run provenance address width mismatch", result.stdout)

    def test_rtl_address_width_mismatch_and_split_profiles_fail_closed(self) -> None:
        for mode, selected_bits in (("mismatch64", "64"), ("split", "32")):
            with self.subTest(mode=mode):
                out = self.root / f"rtl-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                                     DUPLEX_ALIAS_ADDRESS_BITS=selected_bits, RTL_PROFILE_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                expected = "complete 64-bit profile" if mode == "mismatch64" else "exactly one read engine"
                self.assertIn(expected, result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_simulation_requires_explicit_outside_sandbox_opt_in(self) -> None:
        out = self.root / "must-not-exist"
        result = self.invoke(out)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run outside the sandbox", result.stdout)
        self.assertFalse(out.exists())

    def test_wrong_or_missing_scenario_marker_fails_closed(self) -> None:
        for mode in ("wrong", "missing"):
            with self.subTest(mode=mode):
                out = self.root / f"marker-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                                     SCENARIO_MARKER_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_golden_success_without_runtime_guard_activation_fails_closed(self) -> None:
        for mode in ("missing", "replaced"):
            with self.subTest(mode=mode):
                out = self.root / f"activation-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                                     DUPLEX_ALIAS_PROOF_MODE="development", ACTIVATION_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("runtime-guard activation witness", result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_golden_success_and_activation_without_valid_runtime_proof_fails_closed(self) -> None:
        expected_errors = {
            "missing": "expected exactly one well-formed duplex proof record",
            "malformed": "invalid duplex proof JSON",
            "no-semantic": "does not assert semantic_proof=true",
            "no-runtime-guard": "does not assert runtime_range_guard=true",
            "wrong-kind": "unexpected proof_kind",
            "wrong-bundle": "expected exactly one well-formed duplex proof record",
            "duplicate": "expected exactly one well-formed duplex proof record",
        }
        for mode, expected_error in expected_errors.items():
            with self.subTest(mode=mode):
                out = self.root / f"proof-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                                     DUPLEX_ALIAS_PROOF_MODE="development", PROOF_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(expected_error, result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_development_proof_mode_keeps_the_debug_proof_gate(self) -> None:
        out = self.root / "development-proof"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                             DUPLEX_ALIAS_PROOF_MODE="development", DUPLEX_ALIAS_FLOAT_BITS="1")
        self.assertEqual(result.returncode, 0, result.stdout)
        case_dir = out / "symmetric/disjoint"
        provenance = (case_dir / "provenance.txt").read_text()
        argv = shlex.split(next(line for line in provenance.splitlines() if line.startswith("argv="))[5:])
        self.assertIn("--debug-classes=InterfaceInfer,BuildVirtualPhi", argv)
        self.assertIn("--print-dot", argv)
        self.assertIn("--bambu-parameter=print-ir-manager=1", argv)
        compiler_output = (case_dir / "stdout.log").read_text() + (case_dir / "stderr.log").read_text()
        self.assertIn("AXI_BURST_DUPLEX_PROOF", compiler_output)
        self.assertIn("DUPLEX_ALIAS_CHECK PASS PAYLOAD=FLOAT32_BITS PROFILES=1 SCENARIOS=3", result.stdout)

    def test_nested_fallbacks_require_rejection_no_activation_and_no_burst_engines(self) -> None:
        out = self.root / "nested-fallbacks"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1")
        self.assertEqual(result.returncode, 0, result.stdout)
        for profile in ("symmetric", "asymmetric"):
            for scenario in ("nested-changing-base", "nested-changing-count"):
                case_dir = out / profile / scenario
                output = (case_dir / "stdout.log").read_text() + (case_dir / "stderr.log").read_text()
                self.assertNotIn("AXI_BURST_DUPLEX_PROOF", output)
                rtl = (case_dir / "kernel.v").read_text()
                self.assertNotIn("MinimalAXI4MasterPipelined #(", rtl)
                self.assertNotIn("MinimalAXI4MasterPipelinedWrite #(", rtl)
        for mode, expected_error in (("activation", "conservatively fall back"),
                                     ("proof-success", "must conservatively reject"),
                                     ("engine", "must not instantiate a burst read engine")):
            with self.subTest(mode=mode):
                bad_out = self.root / f"nested-fallback-{mode}"
                result = self.invoke(bad_out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                                     DUPLEX_ALIAS_PROOF_MODE="development", NESTED_FALLBACK_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(expected_error, result.stdout)
                self.assertFalse((bad_out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_development_proof_mode_checks_nested_fallback_records(self) -> None:
        out = self.root / "nested-development-proof"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1",
                             DUPLEX_ALIAS_PROOF_MODE="development")
        self.assertEqual(result.returncode, 0, result.stdout)
        for profile in ("symmetric", "asymmetric"):
            for scenario in ("nested-changing-base", "nested-changing-count"):
                case_dir = out / profile / scenario
                output = (case_dir / "stdout.log").read_text() + (case_dir / "stderr.log").read_text()
                self.assertEqual(output.count("AXI_BURST_DUPLEX_PROOF"), 1)
                self.assertIn('"semantic_proof":false', output)
                self.assertIn('"runtime_range_guard":false', output)

    def test_zero_trip_requires_known_low_arvalid_and_awvalid_in_vcd(self) -> None:
        for mode, expected_error in (("missing", "missing nonempty VCD for the AXI witness"),
                                     ("read-high", "shows nonzero/unknown"),
                                     ("write-high", "shows nonzero/unknown"),
                                     ("unknown", "shows nonzero/unknown")):
            with self.subTest(mode=mode):
                out = self.root / f"vcd-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", VCD_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(expected_error, result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_overlap_requires_delayed_b_and_prior_b_before_next_ar(self) -> None:
        cases = (("good", None),
                 ("early-ar", "next AR handshake precedes prior B completion"),
                 ("short-b-delay", "B completion delay is only 3 cycles"))
        for mode, expected_error in cases:
            with self.subTest(mode=mode):
                out = self.root / f"overlap-vcd-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", VCD_MODE=mode)
                if expected_error is None:
                    self.assertEqual(result.returncode, 0, result.stdout)
                    self.assertIn("overlap VCD proves delayed B serialization", result.stdout)
                else:
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn(expected_error, result.stdout)
                    self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_overlap_requires_one_system_clock_for_axi_signals(self) -> None:
        for mode in ("missing-system", "ambiguous-system"):
            with self.subTest(mode=mode):
                out = self.root / f"overlap-clock-{mode}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", CLOCK_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("must define exactly one effective system CLOCK signal", result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_checker_rejects_empty_required_vcds_with_matching_manifest_digest(self) -> None:
        for scenario, expected_error in (("zero-trip", "zero-trip simulation VCD is empty"),
                                         ("forward-overlap", "overlap AXI simulation VCD is empty")):
            with self.subTest(scenario=scenario):
                out = self.root / f"empty-required-vcd-{scenario}"
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1")
                self.assertEqual(result.returncode, 0, result.stdout)
                case_dir = out / "symmetric" / scenario
                vcd = case_dir / "HLS_output/simulation/test.vcd"
                vcd.write_bytes(b"")
                manifest = case_dir / "artifact-sha256.txt"
                rows = manifest.read_text().splitlines()
                entry = "HLS_output/simulation/test.vcd"
                replacement = f"{hashlib.sha256(b'').hexdigest()}  {entry}"
                for index, row in enumerate(rows):
                    if row.endswith(f"  {entry}"):
                        rows[index] = replacement
                        break
                else:
                    self.fail(f"missing VCD manifest entry for {scenario}")
                manifest.write_text("\n".join(rows) + "\n")
                checked = self.check_output(out, 32)
                self.assertNotEqual(checked.returncode, 0, checked.stdout)
                self.assertIn(expected_error, checked.stdout)

    def test_range_probe_missing_witness_fails_closed(self) -> None:
        out = self.root / "range-probe-missing"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", RANGE_PROBE_MODE="missing")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("range-guard probe missing VCD trace for valid-disjoint", result.stdout)
        self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

    def test_invalid_parallelism_and_preexisting_output_are_rejected(self) -> None:
        out = self.root / "invalid-jobs"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", DUPLEX_ALIAS_JOBS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(out.exists())
        existing = self.root / "existing"
        existing.mkdir()
        result = self.invoke(existing, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1")
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
