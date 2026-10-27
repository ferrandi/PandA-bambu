from __future__ import annotations

import os
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
FAMILY = ROOT / "panda_regressions/hls/bambu_axi_burst"
RTL_TESTS = ROOT / "panda_regressions/verilog/axi_burst/rtl_tests"
RUNNER = RTL_TESTS / "run_generated_duplex_kernel_tests.sh"
COPY_SOURCE = FAMILY / "bambu_axi_burst_duplex_copy.cpp"
PREFIX_SOURCE = FAMILY / "bambu_axi_burst_duplex_prefix.cpp"
OBSERVER = RTL_TESTS / "duplex_load_observer.sv"
OBSERVER_UNIT_TB = RTL_TESTS / "tb_duplex_load_observer.sv"
PARALLELISM_CHECKER = FAMILY / "check_duplex_parallelism_results.py"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
PROFILES = ("symmetric", "asymmetric")
KINDS = ("inplace", "copy")
PREFIX_CASES = ("READ_LONGER", "WRITE_LONGER", "ZERO_READ_WRITE_TAIL", "READ_ONLY", "ZERO_ZERO", "BOUNDS_CLAMP", "PAGE_SPLITS")
CASES = (
    "COUNTS_0_1_15_16_17", "COUNT38_SPLITS", "PAGE_BOUNDARY", "REINVOKE_DEPENDENT", "REINVOKE_BASE_COUNT",
    "LAST_B_WAIT", "RRESP_FAULT_RESET", "BRESP_FAULT_RESET",
)


class GeneratedDuplexKernelRunnerTests(unittest.TestCase):
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
        shutil.copy2(RUNNER, self.rtl_tests / RUNNER.name)
        shutil.copy2(FAMILY / "parallel_helpers.sh", self.family / "parallel_helpers.sh")
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
        self._stub("verilator", self._verilator_stub())
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
            "SIM_BUILD_FILE": str(self.scratch / "verilator-build-args"),
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
    def _verilator_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Verilator stub 5.0'; exit 0; fi
mdir=
kind=inplace
top=
args=$(printf '%q ' "$@")
while (($#)); do
  if [[ $1 == --Mdir ]]; then mdir=$2; shift 2
  elif [[ $1 == --top-module ]]; then top=$2; shift 2
  elif [[ $1 == -DCOPY_MODE ]]; then kind=copy; shift
  elif [[ $1 == -DPREFIX_MODE ]]; then kind=prefix; shift
  else shift; fi
done
printf '%s|%s\\n' "$mdir" "$args" >>"$SIM_BUILD_FILE"
mkdir -p "$mdir"
if [[ $top == tb_duplex_load_observer ]]; then
  cat >"$mdir/Vtb_duplex_load_observer" <<'PROGRAM'
#!/bin/bash
set -euo pipefail
printf '%s\\n' "$(printf '%q ' "$@")" >>"$SIM_UNIT_FILE"
echo 'DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2'
PROGRAM
  chmod +x "$mdir/Vtb_duplex_load_observer"
  exit 0
fi
{
printf '#!/bin/bash\nset -euo pipefail\nkind=%q\n' "$kind"
cat <<'PROGRAM'
scenario=${1#*=}
printf '%s|%s\\n' "$kind" "$(printf '%q ' "$@")" >>"$SIM_ARG_FILE"
profile=${0%/obj/Vtb_generated_duplex_kernel}; profile=${profile##*/}
[[ $kind == copy ]] && profile=${profile%_copy}
[[ $kind == prefix ]] && profile=${profile%_prefix}
if [[ $kind == copy ]]; then marker=DUPLEX_KERNEL_COPY_G2
elif [[ $kind == prefix ]]; then marker=DUPLEX_KERNEL_PREFIX_G2
else marker=DUPLEX_KERNEL_G2; fi
mode=${SIM_MODE:-pass}
if [[ $mode == nonzero && $scenario == PAGE_BOUNDARY ]]; then exit 27; fi
if [[ $mode == absent && $scenario == PAGE_BOUNDARY ]]; then exit 0; fi
if [[ $mode == wrong && $scenario == COUNT38_SPLITS ]]; then echo "DUPLEX_KERNEL_G2 PASS CASE=WRONG PROFILE=$profile"; exit 0; fi
if [[ $mode == prefix-absent && $kind == prefix && $scenario == READ_LONGER ]]; then exit 0; fi
if [[ $mode == absent-g3 && $scenario == COPY_PARALLELISM_256 ]]; then exit 0; fi
if [[ $mode == wrong-g3 && $scenario == COPY_PARALLELISM_256 ]]; then echo "DUPLEX_KERNEL_COPY_G2 PASS CASE=WRONG PROFILE=$profile"; exit 0; fi
exec 9>"${SIM_ACTIVE_FILE}.lock"
flock 9
active=$(cat "$SIM_ACTIVE_FILE" 2>/dev/null || echo 0); active=$((active + 1)); echo "$active" >"$SIM_ACTIVE_FILE"
max=$(cat "$SIM_MAX_FILE" 2>/dev/null || echo 0); (( active <= max )) || echo "$active" >"$SIM_MAX_FILE"
flock -u 9
pwd >>"$SIM_CWD_FILE"
sleep 0.03
flock 9
active=$(cat "$SIM_ACTIVE_FILE"); echo "$((active - 1))" >"$SIM_ACTIVE_FILE"
flock -u 9
echo "$marker PASS CASE=$scenario PROFILE=$profile"
if [[ $scenario == COPY_PARALLELISM_256 ]]; then echo "SIMULATOR_ARGS $(printf '%q ' "$@")"; fi
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
    case $scenario in
      READ_LONGER|ZERO_READ_WRITE_TAIL|BOUNDS_CLAMP) aw=3 ;;
      WRITE_LONGER|PAGE_SPLITS) aw=5 ;;
    esac
  fi
  [[ $mode != prefix-wrong-counts || $scenario != READ_LONGER ]] || reads=$((reads + 1))
  echo "DUPLEX_KERNEL_PREFIX_COUNTS CASE=$scenario PROFILE=$profile AR_BURSTS=$ar R_BEATS=$reads AW_BURSTS=$aw W_BEATS=$writes B_RESPONSES=$aw"
fi
PROGRAM
} >"$mdir/Vtb_generated_duplex_kernel"
chmod +x "$mdir/Vtb_generated_duplex_kernel"
"""

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
            ["/bin/bash", str(self.rtl_tests / RUNNER.name), str(self.prefix / "bin/bambu"), str(output)],
            cwd=self.repo, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
        )
        return result, output

    def test_outside_sandbox_gate_precedes_output_creation(self):
        result, output = self._run(outside=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("BURST_DUPLEX_G2_SIM_OUTSIDE_SANDBOX=1", result.stderr)
        self.assertFalse(output.exists())

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
        result, _ = self._run("nonzero", "nonzero-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("failed rc=27", result.stderr)

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
        self.assertEqual(len(artifact_entries), len(PROFILES) * (len(KINDS) * (5 + len(CASES)) + 5 + len(PREFIX_CASES)) + 8)
        self.assertTrue(all((output / line.split("  ", 1)[1]).is_file() for line in artifact_entries))
        self.assertTrue((output / "parallelism-check.log").is_file())
        self.assertEqual((self.scratch / "checker-invocations").read_text().strip(), str(output))
        self.assertEqual(len((self.scratch / "scenario-args").read_text().splitlines()), 48)
        g3_args = [line for line in (self.scratch / "scenario-args").read_text().splitlines()
                   if "COPY_PARALLELISM_256" in line]
        self.assertEqual(len(g3_args), 2)
        self.assertTrue(all("+CASE=COPY_PARALLELISM_256 +G3_METRICS" in line for line in g3_args))
        build_args = (self.scratch / "verilator-build-args").read_text().splitlines()
        self.assertEqual(len(build_args), 7)
        unit_builds = [line for line in build_args if "/observer-unit/obj" in line]
        self.assertEqual(len(unit_builds), 1)
        self.assertIn(str(self.rtl_tests / OBSERVER.name), unit_builds[0])
        self.assertIn(str(self.rtl_tests / OBSERVER_UNIT_TB.name), unit_builds[0])
        unit_invocations = (self.scratch / "observer-unit-invocations").read_text().splitlines()
        self.assertEqual(len(unit_invocations), 1)
        self.assertIn("+G3_METRICS +CASE=COPY_PARALLELISM_256", unit_invocations[0])
        unit_log = (output / "observer-unit/observer-unit.log").read_text()
        self.assertIn(
            "DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2",
            unit_log,
        )
        for mdir_and_args in build_args:
            mdir, args = mdir_and_args.split("|", 1)
            if "/observer-unit/obj" in mdir:
                continue
            self.assertIn(str(self.rtl_tests / OBSERVER.name), args)
            if "/symmetric" in mdir:
                self.assertIn("-GREAD_FIFO_DEPTH=64", args)
                self.assertIn("-GWRITE_FIFO_DEPTH=32", args)
            elif "/asymmetric" in mdir:
                self.assertIn("-GREAD_FIFO_DEPTH=64", args)
                self.assertIn("-GWRITE_FIFO_DEPTH=16", args)
            else:
                self.fail(f"unexpected profile build directory: {mdir}")
        self.assertTrue(any(str(self.rtl_tests / OBSERVER.name) in line for line in artifact_entries))
        self.assertTrue(any(str(self.rtl_tests / OBSERVER_UNIT_TB.name) in line for line in artifact_entries))
        self.assertTrue(any(str(self.family / PARALLELISM_CHECKER.name) in line for line in artifact_entries))
        maximum = int((self.scratch / "max_active").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 3)
        cwd_entries = (self.scratch / "scenario-cwds").read_text().splitlines()
        self.assertEqual(len(set(cwd_entries)), len(PROFILES) * (len(KINDS) * len(CASES) + len(PREFIX_CASES) + 1))
        self.assertIn("scenario_jobs=3", (output / "provenance.txt").read_text())


if __name__ == "__main__":
    unittest.main()
