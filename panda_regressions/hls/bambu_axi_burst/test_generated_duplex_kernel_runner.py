from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
FAMILY = ROOT / "panda_regressions/hls/bambu_axi_burst"
RTL_TESTS = ROOT / "panda_regressions/verilog/axi_burst/rtl_tests"
RUNNER = FAMILY / "run_generated_duplex_kernel_tests.sh"
COPY_SOURCE = FAMILY / "bambu_axi_burst_duplex_copy.cpp"
PREFIX_SOURCE = FAMILY / "bambu_axi_burst_duplex_prefix.cpp"
OBSERVER = RTL_TESTS / "duplex_load_observer.sv"
OBSERVER_UNIT_TB = RTL_TESTS / "tb_duplex_load_observer.sv"
PARALLELISM_CHECKER = FAMILY / "check_duplex_parallelism_results.py"
MODELSIM_HELPERS = RTL_TESTS / "modelsim_helpers.sh"
MODELSIM_LOOKUP = ROOT / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
PROFILES = ("symmetric", "asymmetric")
KINDS = ("inplace", "copy")
PREFIX_CASES = ("READ_LONGER", "WRITE_LONGER", "ZERO_READ_WRITE_TAIL", "READ_ONLY", "ZERO_ZERO", "BOUNDS_CLAMP", "PAGE_SPLITS")
CASES = (
    "COUNTS_0_1_15_16_17", "COUNT38_SPLITS", "PAGE_BOUNDARY", "REINVOKE_DEPENDENT", "REINVOKE_BASE_COUNT",
    "LAST_B_WAIT", "RRESP_FAULT_RESET", "BRESP_FAULT_RESET",
)


class GeneratedDuplexKernelRunnerTests(unittest.TestCase):
    @staticmethod
    def _contains_canonical_path(record, expected):
        expected = Path(expected).resolve()
        if "  " in record and len(record.split("  ", 1)[0]) == 64:
            candidates = [record.split("  ", 1)[1]]
        else:
            command = record.split("|", 1)[1] if "|" in record else record
            candidates = shlex.split(command)
        for candidate in candidates:
            path = Path(candidate)
            if path.is_absolute() and path.exists() and path.resolve() == expected:
                return True
        return False

    def test_copy_carrier_requires_restrict_on_both_owners(self):
        import re

        source = COPY_SOURCE.read_text()
        signature = re.search(r'extern "C" void kernel\s*\(([^)]*)\)', source)
        self.assertIsNotNone(signature)
        parameters = signature.group(1)
        self.assertRegex(parameters, r"int\s*\*\s*__restrict\s+src\b")
        self.assertRegex(parameters, r"int\s*\*\s*__restrict\s+dst\b")
        self.assertEqual(parameters.count("__restrict"), 2)

    def setUp(self):
        SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = SCRATCH_ROOT / uuid.uuid4().hex
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)
        self.repo = self.scratch / "repo"
        self.family = self.repo / "panda_regressions/hls/bambu_axi_burst"
        self.rtl_tests = self.repo / "panda_regressions/verilog/axi_burst/rtl_tests"
        self.rtl_tests.mkdir(parents=True)
        self.family.mkdir(parents=True, exist_ok=True)
        shutil.copy2(RUNNER, self.family / RUNNER.name)
        shutil.copy2(FAMILY / "parallel_helpers.sh", self.family / "parallel_helpers.sh")
        shutil.copy2(MODELSIM_HELPERS, self.rtl_tests / MODELSIM_HELPERS.name)
        lookup = self.repo / "etc/libtech/backend/simulation/modelsim"
        lookup.mkdir(parents=True)
        shutil.copy2(MODELSIM_LOOKUP, lookup / MODELSIM_LOOKUP.name)
        (self.family / "bambu_axi_burst_duplex.cpp").write_text("// stub source\n")
        (self.family / "bambu_axi_burst_duplex_copy.cpp").write_text("// stub source\n")
        (self.family / "bambu_axi_burst_duplex_copy_tb.cpp").write_text("// stub golden\n")
        shutil.copy2(PREFIX_SOURCE, self.family / PREFIX_SOURCE.name)
        (self.rtl_tests / "tb_generated_duplex_kernel.sv").write_text("// stub testbench\n")
        (self.rtl_tests / OBSERVER.name).write_text("module duplex_load_observer; endmodule\n")
        (self.rtl_tests / OBSERVER_UNIT_TB.name).write_text("module tb_duplex_load_observer; endmodule\n")
        (self.family / PARALLELISM_CHECKER.name).write_text(self._checker_stub())

        self.prefix = self.scratch / "install"
        bambu = self.prefix / "bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text(self._bambu_stub())
        bambu.chmod(0o755)
        (self.prefix / "settings.sh").write_text(
            'export BAMBU_HLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\n'
        )
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("plugin stub\n")
        self.bin = self.scratch / "bin"
        self.bin.mkdir()
        for tool in ("vsim", "vlog", "vlib", "vmap"):
            self._stub(tool, self._modelsim_stub())
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.bin}:{self.env['PATH']}",
            "BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX": "1",
            "TMPDIR": str(self.scratch), "TMP": str(self.scratch), "TEMP": str(self.scratch),
            "SIM_JOBS": "2", "DUPLEX_G2_JOBS": "3",
            "SIM_MODE": "pass", "SIM_ACTIVE_FILE": str(self.scratch / "active"),
            "SIM_MAX_FILE": str(self.scratch / "max_active"),
            "SIM_CWD_FILE": str(self.scratch / "scenario-cwds"),
            "SIM_GEN_FILE": str(self.scratch / "generated-profiles"),
            "SIM_BUILD_FILE": str(self.scratch / "modelsim-build-args"),
            "SIM_SHARED_LIB_PARALLEL_FILE": str(self.scratch / "shared-library-parallelism"),
            "SIM_ARG_FILE": str(self.scratch / "scenario-args"),
            "SIM_CHECKER_FILE": str(self.scratch / "checker-invocations"),
            "SIM_UNIT_FILE": str(self.scratch / "observer-unit-invocations"),
            "CHECK_MODE": "pass",
        })

    @staticmethod
    def _bambu_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Bambu stub 1.0'; exit 0; fi
profile=1
for arg in "$@"; do [[ $arg == --extra-cc-options=-DDUPLEX_PROFILE=* ]] && profile=${arg##*=}; done
kind=inplace
for arg in "$@"; do
  [[ $arg == */bambu_axi_burst_duplex_copy.cpp ]] && kind=copy
  [[ $arg == */bambu_axi_burst_duplex_prefix.cpp ]] && kind=prefix
done
echo "$profile:$kind" >>"$SIM_GEN_FILE"
if [[ $profile == 1 ]]; then rb=16; ro=1; rf=64; wb=16; wo=1; wf=32
else rb=16; ro=2; rf=64; wb=8; wo=1; wf=16; fi
if [[ $kind == copy ]]; then
cat >kernel.v <<RTL
module kernel(input clock, input [31:0] src, dst, output done);
  wire p_m_axi_shared_arready, p_m_axi_shared_awready;
  MinimalAXI4MasterPipelined #(.B_MAX($rb), .MAX_OUTSTANDING($ro), .FIFO_DEPTH($rf)) read_engine (.m_axi_arready(p_m_axi_shared_arready));
  MinimalAXI4MasterPipelinedWrite #(.B_MAX($wb), .MAX_OUTSTANDING($wo), .FIFO_DEPTH($wf)) write_engine (.m_axi_awready(p_m_axi_shared_awready));
endmodule
RTL
elif [[ $kind == prefix ]]; then
cat >kernel.v <<RTL
module kernel(input clock, input [31:0] src, dst, read_bound, write_bound, n, output done);
  wire p_m_axi_shared_arready, p_m_axi_shared_awready;
  MinimalAXI4MasterPipelined #(.B_MAX($rb), .MAX_OUTSTANDING($ro), .FIFO_DEPTH($rf)) read_engine (.m_axi_arready(p_m_axi_shared_arready));
  MinimalAXI4MasterPipelinedWrite #(.B_MAX($wb), .MAX_OUTSTANDING($wo), .FIFO_DEPTH($wf)) write_engine (.m_axi_awready(p_m_axi_shared_awready));
endmodule
RTL
else
cat >kernel.v <<RTL
module kernel(input clock, input [31:0] data, n, output done);
  wire p_m_axi_shared_arready, p_m_axi_shared_awready;
  MinimalAXI4MasterPipelined #(.B_MAX($rb), .MAX_OUTSTANDING($ro), .FIFO_DEPTH($rf)) read_engine (.m_axi_arready(p_m_axi_shared_arready));
  MinimalAXI4MasterPipelinedWrite #(.B_MAX($wb), .MAX_OUTSTANDING($wo), .FIFO_DEPTH($wf)) write_engine (.m_axi_awready(p_m_axi_shared_awready));
endmodule
RTL
fi
cat >panda_libtech.v <<'RTL'
module MinimalAXI4MasterPipelined(); endmodule
module MinimalAXI4MasterPipelinedWrite(); endmodule
RTL
"""

    @staticmethod
    def _modelsim_stub():
        return r'''#!/usr/bin/env bash
set -euo pipefail
tool=${0##*/}
if [[ $tool == vsim && ${1:-} == -version ]]; then echo 'Questa fake 2026.1'; exit 0; fi
if [[ $tool == vmap ]]; then
  [[ ${1:-} != -c ]] || printf '[Library]\n' > modelsim.ini
  exit 0
fi
if [[ $tool == vlib ]]; then mkdir -p "${1:-work}"; exit 0; fi
if [[ $tool == vlog ]]; then
  printf '%s|%s\n' "$PWD" "$(printf '%q ' "$@")" >>"$SIM_BUILD_FILE"
  [[ ${SIM_MODE:-pass} != compile-fail ]] || { echo 'fake vlog compile error' >&2; exit 9; }
  exit 0
fi
[[ $tool == vsim ]] || exit 2
build=
top=
observer_cu_root=0
scenario=
for ((i=1; i<=$#; i++)); do
  arg=${!i}
  case $arg in
    -modelsimini) next=$((i + 1)); build=$(dirname "${!next}") ;;
    work.*)
      if [[ -z $top ]]; then top=${arg#work.}
      elif [[ ${arg#work.} == duplex_load_observer_cu ]]; then observer_cu_root=1
      fi ;;
    +CASE=*) scenario=${arg#+CASE=} ;;
  esac
done
if [[ $top == tb_generated_duplex_kernel && $observer_cu_root != 1 ]]; then
  echo 'generated kernel elaboration omitted work.duplex_load_observer_cu' >&2
  exit 42
fi
if [[ $top == tb_duplex_load_observer ]]; then
  printf '%q ' "$@" >>"$SIM_UNIT_FILE"; printf '\n' >>"$SIM_UNIT_FILE"
  echo 'DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2'
  exit 0
fi
instance=$(basename "$(dirname "$build")")
if [[ $instance == *_copy ]]; then profile=${instance%_copy}; kind=copy
elif [[ $instance == *_prefix ]]; then profile=${instance%_prefix}; kind=prefix
else profile=$instance; kind=inplace; fi
printf '%s|%s|%s|%s\n' "$profile" "$kind" "$scenario" "$(printf '%s ' "$@")" >>"$SIM_ARG_FILE"
printf '%s|%s|%s\n' "$profile" "$kind" "$scenario" >>"$SIM_CWD_FILE"
if [[ ${SIM_MODE:-pass} == nonzero && $scenario == PAGE_BOUNDARY ]]; then exit 27; fi
if [[ ${SIM_MODE:-pass} == timeout && $scenario == PAGE_BOUNDARY ]]; then echo 'fake timeout reached' >&2; exit 124; fi
if [[ ${SIM_MODE:-pass} == fatal-zero && $scenario == PAGE_BOUNDARY ]]; then echo '** Fatal: injected fatal with zero simulator exit'; exit 0; fi
if [[ ${SIM_MODE:-pass} == absent && $scenario == PAGE_BOUNDARY ]]; then exit 0; fi
if [[ ${SIM_MODE:-pass} == wrong && $scenario == COUNT38_SPLITS ]]; then echo "DUPLEX_KERNEL_G2 PASS CASE=WRONG PROFILE=$profile"; exit 0; fi
if [[ ${SIM_MODE:-pass} == prefix-absent && $kind == prefix && $scenario == READ_LONGER ]]; then exit 0; fi
if [[ ${SIM_MODE:-pass} == absent-g3 && $scenario == COPY_PARALLELISM_256 ]]; then exit 0; fi
if [[ ${SIM_MODE:-pass} == wrong-g3 && $scenario == COPY_PARALLELISM_256 ]]; then echo "DUPLEX_KERNEL_COPY_G2 PASS CASE=WRONG PROFILE=$profile"; exit 0; fi
fault_expected=0
[[ $scenario == RRESP_FAULT_RESET || $scenario == BRESP_FAULT_RESET ]] && fault_expected=1
fault_arg=0
for arg in "$@"; do [[ $arg == +AXI_BURST_EXPECT_FAULT ]] && fault_arg=1; done
[[ $fault_expected == $fault_arg ]] || { echo "unexpected expected-fault plusarg selection" >&2; exit 41; }
exec 9>"${SIM_ACTIVE_FILE}.lock"
flock 9
active=$(cat "$SIM_ACTIVE_FILE" 2>/dev/null || echo 0); active=$((active + 1)); echo "$active" >"$SIM_ACTIVE_FILE"
max=$(cat "$SIM_MAX_FILE" 2>/dev/null || echo 0); (( active <= max )) || echo "$active" >"$SIM_MAX_FILE"
lib_active=$(cat "$build/.active" 2>/dev/null || echo 0)
(( lib_active == 0 )) || echo "$build" >"$SIM_SHARED_LIB_PARALLEL_FILE"
echo "$((lib_active + 1))" >"$build/.active"
flock -u 9
sleep 0.03
flock 9
active=$(cat "$SIM_ACTIVE_FILE"); echo "$((active - 1))" >"$SIM_ACTIVE_FILE"
lib_active=$(cat "$build/.active"); echo "$((lib_active - 1))" >"$build/.active"
flock -u 9
if [[ $kind == copy ]]; then marker=DUPLEX_KERNEL_COPY_G2
elif [[ $kind == prefix ]]; then marker=DUPLEX_KERNEL_PREFIX_G2
else marker=DUPLEX_KERNEL_G2; fi
echo "$marker PASS CASE=$scenario PROFILE=$profile"
if [[ ${SIM_MODE:-pass} == missing-diagnostic && $scenario == RRESP_FAULT_RESET ]]; then exit 0; fi
case $scenario in
  RRESP_FAULT_RESET)
    echo 'AXI_BURST_FAULT engine=read reason=RRESP instance=kernel.read_engine.report_axi_read_diagnostic opcode=0 size=32 address=00002000 count=17 resp=10 id=0 last=0 descriptors=1 remaining=17'
    echo 'AXI_BURST_PEER_FAULT engine=write reason=peer instance=kernel.write_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00000000 count=0 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
    if [[ ${SIM_MODE:-pass} != missing-return-peer ]]; then
      echo 'AXI_BURST_PEER_FAULT engine=read reason=peer instance=kernel.read_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00002000 count=17 resp=00 id=0 last=0 descriptors=1 remaining=17'
    fi ;;
  BRESP_FAULT_RESET)
    echo 'AXI_BURST_FAULT engine=write reason=BRESP instance=kernel.write_engine.report_axi_write_diagnostic opcode=1 size=32 address=00002000 count=17 payload=00000000 resp=10 id=0 descriptors=1 remaining=17'
    echo 'AXI_BURST_PEER_FAULT engine=read reason=peer instance=kernel.read_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 descriptors=0 remaining=0'
    if [[ ${SIM_MODE:-pass} != missing-return-peer ]]; then
      echo 'AXI_BURST_PEER_FAULT engine=write reason=peer instance=kernel.write_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00002000 count=17 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
    fi ;;
esac
if [[ ${SIM_MODE:-pass} == bad-diagnostic && $scenario == COUNT38_SPLITS ]]; then
  echo 'AXI_BURST_FAULT engine=read reason=RRESP instance=kernel.read_engine.report_axi_read_diagnostic opcode=0 size=32 address=00002000 count=1 resp=10 id=0 last=0 descriptors=1 remaining=1'
fi
if [[ ${SIM_MODE:-pass} == extra-diagnostic && $scenario == COUNTS_0_1_15_16_17 ]]; then
  echo 'AXI_BURST_FAULT engine=read reason=RRESP instance=unexpected.report_axi_read_diagnostic opcode=0 size=32 address=00002000 count=1 resp=10 id=0 last=0 descriptors=1 remaining=1'
fi
if [[ $scenario == COPY_PARALLELISM_256 ]]; then echo "SIMULATOR_ARGS $(printf '%q ' "$@" | sed -E 's/.*(\+CASE=[^ ]+ \+G3_METRICS).*/\1/')"; fi
if [[ $kind == prefix ]]; then
  case $scenario in
    READ_LONGER) reads=38; writes=17; ar=3; aw=2 ;;
    WRITE_LONGER) reads=17; writes=38; ar=2; aw=3 ;;
    ZERO_READ_WRITE_TAIL) reads=0; writes=17; ar=0; aw=2 ;;
    READ_ONLY) reads=17; writes=0; ar=2; aw=0 ;;
    ZERO_ZERO) reads=0; writes=0; ar=0; aw=0 ;;
    BOUNDS_CLAMP) reads=17; writes=17; ar=2; aw=2 ;;
    PAGE_SPLITS) reads=23; writes=35; ar=3; aw=3 ;;
  esac
  if [[ $profile == asymmetric ]]; then
    case $scenario in READ_LONGER|ZERO_READ_WRITE_TAIL|BOUNDS_CLAMP) aw=3 ;; WRITE_LONGER|PAGE_SPLITS) aw=5 ;; esac
  fi
  [[ ${SIM_MODE:-pass} != prefix-wrong-counts || $scenario != READ_LONGER ]] || reads=$((reads + 1))
  echo "DUPLEX_KERNEL_PREFIX_COUNTS CASE=$scenario PROFILE=$profile AR_BURSTS=$ar R_BEATS=$reads AW_BURSTS=$aw W_BEATS=$writes B_RESPONSES=$aw"
fi
'''

    @staticmethod
    def _checker_stub():
        return """#!/usr/bin/env python3
import os
from pathlib import Path
import sys

output = Path(sys.argv[-1])
Path(os.environ["SIM_CHECKER_FILE"]).write_text(str(output) + "\\n")
if os.environ.get("CHECK_MODE") == "fail":
    print("DUPLEX PARALLELISM CHECK FAILED: injected failure")
    raise SystemExit(1)
for profile in ("symmetric", "asymmetric"):
    log = output / f"{profile}_copy/scenarios/COPY_PARALLELISM_256.log"
    if not log.is_file() or f"DUPLEX_KERNEL_COPY_G2 PASS CASE=COPY_PARALLELISM_256 PROFILE={profile}" not in log.read_text():
        print(f"missing exact scenario witness: {profile}")
        raise SystemExit(1)
    if "SIMULATOR_ARGS +CASE=COPY_PARALLELISM_256 +G3_METRICS" not in log.read_text():
        print(f"missing exact metrics plusargs: {profile}")
        raise SystemExit(1)
    print(f"DUPLEX PARALLELISM PASS PROFILE={profile}")
"""

    def _stub(self, name, content):
        path = self.bin / name
        path.write_text(content)
        path.chmod(0o755)

    def _run(self, mode="pass", output_name="fresh", outside=True, job_overrides=None, check_mode="pass"):
        env = self.env.copy()
        env["SIM_MODE"] = mode
        env["CHECK_MODE"] = check_mode
        env.update(job_overrides or {})
        if not outside:
            env.pop("BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX", None)
        output = self.scratch / output_name
        result = subprocess.run(
            ["/bin/bash", str(self.family / RUNNER.name), str(self.prefix / "bin/bambu"), str(output)],
            cwd=self.repo, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
        )
        return result, output

    def test_outside_sandbox_gate_precedes_output_creation(self):
        result, output = self._run(outside=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1", result.stderr)
        self.assertFalse(output.exists())

    def test_unexpected_normal_diagnostic_is_rejected(self):
        result, _ = self._run("extra-diagnostic", "unexpected-diagnostic")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_missing_fault_record_is_rejected(self):
        result, _ = self._run("missing-diagnostic", "missing-fault-record")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_missing_return_peer_record_is_rejected(self):
        result, _ = self._run("missing-return-peer", "missing-return-peer")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_invalid_parallelism_fails_before_output_creation(self):
        result, output = self._run(output_name="invalid-jobs", job_overrides={"DUPLEX_G2_JOBS": "0"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SIM_JOBS and DUPLEX_G2_JOBS must be positive integers", result.stderr)
        self.assertFalse(output.exists())

    def test_absent_or_wrong_marker_fails_with_zero_exit(self):
        for mode in ("absent", "wrong"):
            with self.subTest(mode=mode):
                result, _ = self._run(mode, f"{mode}-output")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("missing exact success marker", result.stderr)

    def test_prefix_marker_and_transaction_counts_fail_closed(self):
        for mode, message in (
            ("prefix-absent", "missing exact success marker"),
            ("prefix-wrong-counts", "missing or incorrect prefix transaction-count witness"),
        ):
            with self.subTest(mode=mode):
                result, _ = self._run(mode, f"{mode}-output")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)

    def test_parallelism_marker_and_checker_fail_closed(self):
        for mode, check_mode, message in (
            ("absent-g3", "pass", "missing exact success marker"),
            ("wrong-g3", "pass", "missing exact success marker"),
            ("pass", "fail", "duplex parallelism result check failed"),
        ):
            with self.subTest(mode=mode, check_mode=check_mode):
                result, _ = self._run(mode, f"g3-{mode}-{check_mode}", check_mode=check_mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)

    def test_nonzero_simulation_status_is_rejected(self):
        compile_failure, _ = self._run("compile-fail", "compile-failure-output")
        self.assertNotEqual(compile_failure.returncode, 0)
        self.assertIn("ModelSim compile failed", compile_failure.stderr)
        result, _ = self._run("nonzero", "nonzero-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("failed rc=27", result.stderr)
        fatal, _ = self._run("fatal-zero", "fatal-zero-output")
        self.assertNotEqual(fatal.returncode, 0)
        self.assertIn("failed rc=1", fatal.stderr)

    def test_manifest_mismatch_is_rejected(self):
        real_sha = shutil.which("sha256sum", path=self.env["PATH"].split(":", 1)[1])
        self._stub("sha256sum", f"""#!/bin/bash
if [[ ${{1:-}} == --check && ${{2:-}} == scenario-sha256.txt ]]; then
  echo 'scenario.log: FAILED' >&2
  exit 1
fi
exec {real_sha} \"$@\"
""")
        result, _ = self._run(output_name="manifest-mismatch")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("FAILED", result.stderr)

    def test_scenario_directories_manifests_and_bounded_parallelism(self):
        self.assertNotIn("verilator", (self.family / RUNNER.name).read_text().lower())
        result, output = self._run()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("GENERATED_DUPLEX_KERNEL_G2 PASS: 48 scenarios; includes two G3.1 parallelism scenarios", result.stdout)
        for profile in PROFILES:
            for kind in KINDS:
                profile_dir = output / (profile if kind == "inplace" else f"{profile}_copy")
                self.assertTrue((profile_dir / "generated/kernel.v").is_file())
                self.assertIn("DUPLEX_PROFILE=", (profile_dir / "compiler_argv.txt").read_text())
                marker = "DUPLEX_KERNEL_G2" if kind == "inplace" else "DUPLEX_KERNEL_COPY_G2"
                for scenario in CASES:
                    log = profile_dir / "scenarios" / f"{scenario}.log"
                    self.assertIn(f"{marker} PASS CASE={scenario} PROFILE={profile}", log.read_text())
            prefix_dir = output / f"{profile}_prefix"
            self.assertTrue((prefix_dir / "generated/kernel.v").is_file())
            for scenario in PREFIX_CASES:
                log = prefix_dir / "scenarios" / f"{scenario}.log"
                log_text = log.read_text()
                self.assertIn(f"DUPLEX_KERNEL_PREFIX_G2 PASS CASE={scenario} PROFILE={profile}", log_text)
                expected = {
                    "READ_LONGER": (3, 38, 2, 17),
                    "WRITE_LONGER": (2, 17, 3, 38),
                    "ZERO_READ_WRITE_TAIL": (0, 0, 2, 17),
                    "READ_ONLY": (2, 17, 0, 0),
                    "ZERO_ZERO": (0, 0, 0, 0),
                    "BOUNDS_CLAMP": (2, 17, 2, 17),
                    "PAGE_SPLITS": (3, 23, 3, 35),
                }[scenario]
                expected_ar, expected_reads, expected_aw, expected_writes = expected
                if profile == "asymmetric" and scenario in ("READ_LONGER", "ZERO_READ_WRITE_TAIL", "BOUNDS_CLAMP"):
                    expected_aw = 3
                if profile == "asymmetric" and scenario in ("WRITE_LONGER", "PAGE_SPLITS"):
                    expected_aw = 5
                counts = (
                    f"DUPLEX_KERNEL_PREFIX_COUNTS CASE={scenario} PROFILE={profile} "
                    f"AR_BURSTS={expected_ar} R_BEATS={expected_reads} AW_BURSTS={expected_aw} "
                    f"W_BEATS={expected_writes} B_RESPONSES={expected_aw}"
                )
                self.assertIn(counts, log_text)
        manifest = output / "scenario-sha256.txt"
        entries = manifest.read_text().splitlines()
        self.assertEqual(len(entries), len(PROFILES) * (len(KINDS) * len(CASES) + len(PREFIX_CASES) + 1) + 1)
        self.assertTrue(all((output / line.split("  ", 1)[1]).is_file() for line in entries))
        generated = (self.scratch / "generated-profiles").read_text().splitlines()
        self.assertCountEqual(generated, ("1:inplace", "1:copy", "1:prefix", "2:inplace", "2:copy", "2:prefix"))
        artifact_entries = (output / "artifact-sha256.txt").read_text().splitlines()
        self.assertEqual(len(artifact_entries), len(PROFILES) * (len(KINDS) * (5 + len(CASES)) + 5 + len(PREFIX_CASES)) + 57)
        self.assertTrue(all((output / line.split("  ", 1)[1]).is_file() for line in artifact_entries))
        self.assertTrue((output / "parallelism-check.log").is_file())
        checker_output = Path((self.scratch / "checker-invocations").read_text().strip())
        self.assertEqual(checker_output.resolve(), output.resolve())
        self.assertEqual(len((self.scratch / "scenario-args").read_text().splitlines()), 48)
        scenario_args = (self.scratch / "scenario-args").read_text().splitlines()
        self.assertTrue(all("work.duplex_load_observer_cu" in line for line in scenario_args))
        g3_args = [line for line in (self.scratch / "scenario-args").read_text().splitlines()
                   if "COPY_PARALLELISM_256" in line]
        self.assertEqual(len(g3_args), 2)
        self.assertTrue(all("+CASE=COPY_PARALLELISM_256 +G3_METRICS" in line for line in g3_args))
        build_args = (self.scratch / "modelsim-build-args").read_text().splitlines()
        self.assertEqual(len(build_args), 7)
        unit_builds = [line for line in build_args if "/observer-unit/sim" in line]
        self.assertEqual(len(unit_builds), 1)
        self.assertTrue(self._contains_canonical_path(unit_builds[0], self.rtl_tests / OBSERVER.name))
        self.assertTrue(self._contains_canonical_path(unit_builds[0], self.rtl_tests / OBSERVER_UNIT_TB.name))
        self.assertIn("-mfcu", unit_builds[0])
        self.assertIn("-cuname duplex_load_observer_unit_cu", unit_builds[0])
        self.assertNotIn("-cuname tb_duplex_load_observer", unit_builds[0])
        unit_args = shlex.split(unit_builds[0].split("|", 1)[1])
        resolved_unit_tb = (self.rtl_tests / OBSERVER_UNIT_TB.name).resolve()
        resolved_observer = (self.rtl_tests / OBSERVER.name).resolve()
        unit_tb_index = next(i for i, item in enumerate(unit_args)
                             if Path(item).is_absolute() and Path(item).exists() and Path(item).resolve() == resolved_unit_tb)
        observer_index = next(i for i, item in enumerate(unit_args)
                              if Path(item).is_absolute() and Path(item).exists() and Path(item).resolve() == resolved_observer)
        self.assertLess(unit_tb_index, observer_index)
        unit_invocations = (self.scratch / "observer-unit-invocations").read_text().splitlines()
        self.assertEqual(len(unit_invocations), 1)
        self.assertIn("-voptargs=+acc", unit_invocations[0])
        self.assertIn("+G3_METRICS +CASE=COPY_PARALLELISM_256", unit_invocations[0])
        unit_log = (output / "observer-unit/observer-unit.log").read_text()
        self.assertIn(
            "DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2",
            unit_log,
        )
        for build_and_args in build_args:
            build, args = build_and_args.split("|", 1)
            if "/observer-unit/sim" in build:
                continue
            self.assertTrue(self._contains_canonical_path(f"{profile}|{args}", self.rtl_tests / OBSERVER.name))
            self.assertIn("tb_generated_duplex_kernel.sv", args)
            self.assertIn("-mfcu", args)
            self.assertIn("-cuname duplex_load_observer_cu", args)
        self.assertEqual(sum("+define+COPY_MODE" in line for line in build_args), 2)
        self.assertEqual(sum("+define+PREFIX_MODE" in line for line in build_args), 2)
        self.assertEqual(sum("+define+COPY_MODE" not in line and "+define+PREFIX_MODE" not in line for line in build_args), 3)
        self.assertTrue(any(self._contains_canonical_path(line, self.rtl_tests / OBSERVER.name)
                            for line in artifact_entries))
        self.assertTrue(any(self._contains_canonical_path(line, self.rtl_tests / OBSERVER_UNIT_TB.name)
                            for line in artifact_entries))
        self.assertTrue(any(self._contains_canonical_path(line, self.family / PARALLELISM_CHECKER.name)
                            for line in artifact_entries))
        maximum = int((self.scratch / "max_active").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 3)
        cwd_entries = (self.scratch / "scenario-cwds").read_text().splitlines()
        self.assertEqual(len(set(cwd_entries)), 48)
        self.assertFalse((self.scratch / "shared-library-parallelism").exists(),
                         "scenarios sharing one ModelSim work library ran concurrently")
        self.assertIn("scenario_jobs=3", (output / "provenance.txt").read_text())
        invocations = (self.scratch / "scenario-args").read_text().splitlines()
        for line in invocations:
            profile, _kind, _scenario, args = line.split("|", 3)
            if profile == "symmetric":
                self.assertIn("-gREAD_OUTSTANDING=1", args)
                self.assertIn("-gWRITE_FIFO_DEPTH=32", args)
            else:
                self.assertIn("-gREAD_OUTSTANDING=2", args)
                self.assertIn("-gWRITE_FIFO_DEPTH=16", args)


if __name__ == "__main__":
    unittest.main()
