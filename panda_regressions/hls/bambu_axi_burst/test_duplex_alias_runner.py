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
SCENARIOS = {1: "disjoint", 2: "forward-overlap", 3: "reverse-overlap", 4: "zero-trip"}


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
        verilator = self.bin / "verilator"
        verilator.write_text(self.verilator_stub())
        verilator.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(PATH=f"{self.bin}:{os.environ['PATH']}", TMPDIR=str(TMP_ROOT),
                        TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT), PYTHONDONTWRITEBYTECODE="1")

    def tearDown(self) -> None:
        self.temp.cleanup()

    @staticmethod
    def verilator_stub() -> str:
        return r'''#!/usr/bin/env bash
set -eu
if [[ ${1:-} == --version ]]; then echo 'Verilator 5.0 offline stub'; exit 0; fi
mdir=
for ((index = 1; index <= $#; ++index)); do
  if [[ ${!index} == --Mdir ]]; then
    next=$((index + 1))
    mdir=${!next}
    break
  fi
done
[[ -n $mdir ]]
mkdir -p "$mdir"
printf 'VERILATOR_PROBE_STUB_BUILD PASS argv=%s\n' "$*"
cat > "$mdir/Vtb_duplex_alias_range_guard" <<'EOF'
#!/usr/bin/env bash
set -eu
if [[ ${RANGE_PROBE_MODE:-good} != missing ]]; then
  case_name=${1#+CASE=}
  case ${1:-} in
    +CASE=valid-disjoint) marker='DUPLEX_ALIAS_RANGE_PASS CASE=valid-disjoint ARLEN=15' ;;
    +CASE=read-end-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=read-end-overflow ARLEN=0' ;;
    +CASE=write-end-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=write-end-overflow ARLEN=0' ;;
    +CASE=byte-count-overflow) marker='DUPLEX_ALIAS_RANGE_PASS CASE=byte-count-overflow ARLEN=0' ;;
    +CASE=read-end-equals-2p32) marker='DUPLEX_ALIAS_RANGE_PASS CASE=read-end-equals-2p32 ARLEN=15' ;;
    +CASE=high-bit-valid-disjoint) marker='DUPLEX_ALIAS_RANGE_PASS CASE=high-bit-valid-disjoint ARLEN=15 ARADDR=0000000080001000' ;;
    *) exit 3 ;;
  esac
  printf '%s\n%s\n' "$marker" 'DUPLEX_ALIAS_RANGE_GUARD_PASS CASES=1 NO_AXI_HANDSHAKES=1'
  printf '$enddefinitions $end\n#0\n' > "range_guard_${case_name}.vcd"
fi
EOF
chmod +x "$mdir/Vtb_duplex_alias_range_guard"
'''

    @staticmethod
    def compiler_stub() -> str:
        return r'''#!/usr/bin/env bash
set -eu
if [[ ${1:-} == --version ]]; then echo "stub bambu 1"; exit 0; fi
if [[ ${FAKE_BAMBU_FAIL:-} == 1 ]]; then echo "forced compiler failure" >&2; exit 9; fi
profile= case_id= address_bits=
for arg in "$@"; do
  case $arg in
    --extra-cc-options=-DDUPLEX_ALIAS_PROFILE=*) profile=${arg##*=};;
    --extra-cc-options=-DDUPLEX_ALIAS_CASE=*) case_id=${arg##*=};;
    --addr-bus-bitsize=*) address_bits=${arg#*=};;
  esac
done
[[ -n $profile && -n $case_id && -n $address_bits ]]
case ${SCENARIO_MARKER_MODE:-good} in
  wrong) scenario=unexpected ;;
  missing) scenario= ;;
  *) case $case_id in 1) scenario=disjoint;; 2) scenario=forward-overlap;; 3) scenario=reverse-overlap;; 4) scenario=zero-trip;; esac ;;
esac
case ${ACTIVATION_MODE:-good} in
  missing) ;;
  replaced) printf '%s\n' 'Activated same-base pointwise duplex m_axi bundle shared for root kernel' ;;
  *) printf '%s\n' '---Activated guarded distinct-base duplex for bundle shared (overlap uses serialized scalar transfers)' ;;
esac
proof='{"bundle":"shared","semantic_proof":true,"runtime_range_guard":true,"proof_kind":"distinct_base_runtime_guarded_pointwise"}'
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
mkdir -p HLS_output/simulation
printf '<results/\n' > bambu_results.xml
read_outstanding=1; write_burst=16; write_fifo=32
[[ $profile != 2 ]] || { read_outstanding=2; write_burst=8; write_fifo=16; }
read_addr=$address_bits; write_addr=$address_bits
if [[ ${RTL_PROFILE_MODE:-good} == mismatch64 ]]; then read_addr=32; write_addr=32; fi
if [[ ${RTL_PROFILE_MODE:-good} == split ]]; then
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
  marker="DUPLEX_ALIAS_GOLDEN_PASS CASE=$scenario profile=$profile N=$count"
  [[ $scenario != zero-trip ]] || marker+=" NO_DATA_CHANGE=1"
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
events = {cycle: [] for cycle in range(1, 5000)}
for index in range(64):
    base = index * 67 + 1
    ar_cycle = base - 2 if mode == "early-ar" and index == 1 else base
    events[ar_cycle].append("ar")
    events[base + 1].append("aw")
    events[base + 2].append("w")
    response = base + (5 if mode == "short-b-delay" else 66)
    events[response].append("b")

signals = {"ar": '"', "aw": "$", "w": "&", "b": "("}
lines = [
    "$version synthetic duplex alias testbench $end",
    "$timescale 1ps $end",
    "$scope module bambu_testbench $end",
    "$var wire 1 ! clock $end",
    "$var wire 1 ! clock $end",
    "$var wire 1 \" sig_m_axi_shared_arvalid $end",
    "$var wire 1 # sig_m_axi_shared_arready $end",
    "$var wire 1 $ sig_m_axi_shared_awvalid $end",
    "$var wire 1 % sig_m_axi_shared_awready $end",
    "$var wire 1 & sig_m_axi_shared_wvalid $end",
    "$var wire 1 ' sig_m_axi_shared_wready $end",
    "$var wire 1 ( sig_m_axi_shared_bvalid $end",
    "$var wire 1 ) sig_m_axi_shared_bready $end",
    "$upscope $end",
    "$enddefinitions $end",
    "#0", "0!", "0\"", "1#", "0$", "1%", "0&", "1'", "0(", "1)",
]
for cycle in range(1, max(events) + 1):
    time = cycle * 10
    active = events.get(cycle, [])
    lines.append(f"#{time}")
    lines.append("1!")
    lines.extend(f"1{signals[event]}" for event in active)
    lines.append(f"#{time + 1}")
    lines.extend(f"0{signals[event]}" for event in active)
    lines.append(f"#{time + 2}")
    lines.append("0!")
Path("HLS_output/simulation/test.vcd").write_text("\n".join(lines) + "\n")
PY
  fi
else
  printf 'simulation completed without the expected marker\n' > HLS_output/simulation/testbench.log
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

    def check_output(self, out: Path, address_bits: int) -> subprocess.CompletedProcess[str]:
        args = ["python3", "-B", str(HERE / "check_duplex_alias_results.py"), str(out),
                "--source", str(HERE / "bambu_axi_burst_duplex_alias.cpp"),
                "--testbench", str(HERE / "bambu_axi_burst_duplex_alias_tb.cpp"),
                "--bambu", str(self.bambu), "--settings", str(self.prefix / "settings.sh"),
                "--plugin", str(self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"),
                "--simulator", str(self.bin / "verilator"), "--runner", str(RUNNER),
                "--range-probe-runner", str(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/run_duplex_alias_range_guard.sh"),
                "--range-probe-testbench", str(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/tb_duplex_alias_range_guard.sv"),
                "--addr-bits", str(address_bits)]
        return subprocess.run(args, cwd=ROOT, env=self.env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_runs_exact_eight_case_matrix_with_isolated_dirs_and_digests(self) -> None:
        out = self.root / "fresh-output"
        result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", DUPLEX_ALIAS_JOBS="3")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("DUPLEX_ALIAS_CHECK PASS PROFILES=2 SCENARIOS=8", result.stdout)
        self.assertEqual((out / "aggregate.log").read_text().splitlines()[0].split()[0], "DUPLEX_ALIAS_PASS")
        directories = []
        for profile, profile_id in (("symmetric", "1"), ("asymmetric", "2")):
            for case_id, scenario in SCENARIOS.items():
                case_dir = out / profile / scenario
                directories.append(case_dir)
                count = 0 if scenario == "zero-trip" else 64
                marker = f"DUPLEX_ALIAS_GOLDEN_PASS CASE={scenario} profile={profile_id} N={count}"
                if scenario == "zero-trip":
                    marker += " NO_DATA_CHANGE=1"
                self.assertEqual((case_dir / "HLS_output/simulation/testbench.log").read_text().strip(), marker)
                self.assertTrue((case_dir / "tmp").is_dir())
                provenance = (case_dir / "provenance.txt").read_text()
                self.assertIn("git_head=", provenance)
                self.assertIn("source_sha256=", provenance)
                self.assertIn("runner_sha256=", provenance)
                argv = shlex.split(next(line for line in provenance.splitlines() if line.startswith("argv="))[5:])
                self.assertIn("--simulator=VERILATOR", argv)
                self.assertIn("--debug-classes=InterfaceInfer,BuildVirtualPhi", argv)
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
                self.assertEqual((case_dir / "HLS_output/simulation/test.vcd").is_file(), expected_vcd)
        self.assertEqual(len({path.resolve() for path in directories}), 8)
        self.assertTrue(all(path.is_dir() for path in directories))
        probe_dir = out / "rtl_range_guard"
        probe_log = (probe_dir / "probe.log").read_text()
        self.assertIn("CASE=read-end-overflow ARLEN=0", probe_log)
        probe_provenance = (probe_dir / "provenance.txt").read_text()
        probe_argv = shlex.split(next(line for line in probe_provenance.splitlines()
                                      if line.startswith("argv="))[5:])
        self.assertIn("--binary", probe_argv)
        self.assertIn("--timing", probe_argv)
        self.assertIn("--trace", probe_argv)
        self.assertIn("--top-module", probe_argv)
        self.assertIn(str(out / "symmetric/disjoint/kernel.v"), probe_argv)
        self.assertIn(str(out / "symmetric/disjoint/panda_libtech.v"), probe_argv)
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
                                     ACTIVATION_MODE=mode)
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
                result = self.invoke(out, BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX="1", PROOF_MODE=mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(expected_error, result.stdout)
                self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_ALIAS_PASS"))

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
