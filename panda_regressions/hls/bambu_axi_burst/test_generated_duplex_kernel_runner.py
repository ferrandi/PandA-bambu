from __future__ import annotations

import os
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
FAMILY = ROOT / "panda_regressions/hls/bambu_axi_burst"
RUNNER = FAMILY / "rtl_tests/run_generated_duplex_kernel_tests.sh"
COPY_SOURCE = FAMILY / "bambu_axi_burst_duplex_copy.cpp"
SCRATCH_ROOT = ROOT / "documentation/tmp/out_generated_duplex_kernel_runner_tests"
PROFILES = ("symmetric", "asymmetric")
KINDS = ("inplace", "copy")
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
        self.rtl_tests = self.family / "rtl_tests"
        self.rtl_tests.mkdir(parents=True)
        shutil.copy2(RUNNER, self.rtl_tests / RUNNER.name)
        shutil.copy2(FAMILY / "parallel_helpers.sh", self.family / "parallel_helpers.sh")
        (self.family / "bambu_axi_burst_duplex.cpp").write_text("// stub source\n")
        (self.family / "bambu_axi_burst_duplex_copy.cpp").write_text("// stub source\n")
        (self.family / "bambu_axi_burst_duplex_copy_tb.cpp").write_text("// stub golden\n")
        (self.rtl_tests / "tb_generated_duplex_kernel.sv").write_text("// stub testbench\n")

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
        })

    @staticmethod
    def _bambu_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Bambu stub 1.0'; exit 0; fi
profile=1
for arg in "$@"; do [[ $arg == --extra-cc-options=-DDUPLEX_PROFILE=* ]] && profile=${arg##*=}; done
kind=inplace
for arg in "$@"; do [[ $arg == */bambu_axi_burst_duplex_copy.cpp ]] && kind=copy; done
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
while (($#)); do
  if [[ $1 == --Mdir ]]; then mdir=$2; shift 2
  elif [[ $1 == -DCOPY_MODE ]]; then kind=copy; shift
  else shift; fi
done
mkdir -p "$mdir"
{
printf '#!/bin/bash\nset -euo pipefail\nkind=%q\n' "$kind"
cat <<'PROGRAM'
scenario=${1#*=}
profile=${0%/obj/Vtb_generated_duplex_kernel}; profile=${profile##*/}
[[ $kind == copy ]] && profile=${profile%_copy}
if [[ $kind == copy ]]; then marker=DUPLEX_KERNEL_COPY_G2; else marker=DUPLEX_KERNEL_G2; fi
mode=${SIM_MODE:-pass}
if [[ $mode == nonzero && $scenario == PAGE_BOUNDARY ]]; then exit 27; fi
if [[ $mode == absent && $scenario == PAGE_BOUNDARY ]]; then exit 0; fi
if [[ $mode == wrong && $scenario == COUNT38_SPLITS ]]; then echo "DUPLEX_KERNEL_G2 PASS CASE=WRONG PROFILE=$profile"; exit 0; fi
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
PROGRAM
} >"$mdir/Vtb_generated_duplex_kernel"
chmod +x "$mdir/Vtb_generated_duplex_kernel"
"""

    def _stub(self, name, content):
        path = self.bin / name
        path.write_text(content)
        path.chmod(0o755)

    def _run(self, mode="pass", output_name="fresh", outside=True, job_overrides=None):
        env = self.env.copy()
        env["SIM_MODE"] = mode
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
        self.assertIn("GENERATED_DUPLEX_KERNEL_G2 PASS: 8 scenarios x 2 profiles x 2 carriers", result.stdout)
        for profile in PROFILES:
            for kind in KINDS:
                profile_dir = output / (profile if kind == "inplace" else f"{profile}_copy")
                self.assertTrue((profile_dir / "generated/kernel.v").is_file())
                self.assertIn("DUPLEX_PROFILE=", (profile_dir / "compiler_argv.txt").read_text())
                marker = "DUPLEX_KERNEL_G2" if kind == "inplace" else "DUPLEX_KERNEL_COPY_G2"
                for scenario in CASES:
                    log = profile_dir / "scenarios" / f"{scenario}.log"
                    self.assertIn(f"{marker} PASS CASE={scenario} PROFILE={profile}", log.read_text())
        manifest = output / "scenario-sha256.txt"
        entries = manifest.read_text().splitlines()
        self.assertEqual(len(entries), len(PROFILES) * len(KINDS) * len(CASES))
        self.assertTrue(all((output / line.split("  ", 1)[1]).is_file() for line in entries))
        generated = (self.scratch / "generated-profiles").read_text().splitlines()
        self.assertCountEqual(generated, ("1:inplace", "1:copy", "2:inplace", "2:copy"))
        artifact_entries = (output / "artifact-sha256.txt").read_text().splitlines()
        self.assertEqual(len(artifact_entries), len(PROFILES) * len(KINDS) * (5 + len(CASES)))
        self.assertTrue(all((output / line.split("  ", 1)[1]).is_file() for line in artifact_entries))
        maximum = int((self.scratch / "max_active").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 3)
        cwd_entries = (self.scratch / "scenario-cwds").read_text().splitlines()
        self.assertEqual(len(set(cwd_entries)), len(PROFILES) * len(KINDS) * len(CASES))
        self.assertIn("scenario_jobs=3", (output / "provenance.txt").read_text())


if __name__ == "__main__":
    unittest.main()
