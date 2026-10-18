#!/usr/bin/env python3
"""Fail-closed checker for default-device duplex alias simulations."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
import sys
from pathlib import Path


PROFILES = {"symmetric": "1", "asymmetric": "2"}
SCENARIOS = {"disjoint": "1", "forward-overlap": "2", "reverse-overlap": "3", "zero-trip": "4"}
ARTIFACT_SUFFIXES = {".v", ".sv", ".xml", ".log", ".vcd"}


class CheckError(ValueError):
    pass


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def safe_file(path: Path, label: str, nonempty: bool = True) -> Path:
    need(not path.is_symlink(), f"{label} is a symlink: {path}")
    need(path.is_file(), f"{label} is missing: {path}")
    if nonempty:
        need(path.stat().st_size > 0, f"{label} is empty: {path}")
    return path.resolve(strict=True)


def parse_fields(path: Path, label: str) -> dict[str, str]:
    safe_file(path, label)
    result: dict[str, str] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="strict").splitlines(), 1):
        need("=" in line, f"malformed {label} line {number}")
        key, value = line.split("=", 1)
        need(bool(key) and bool(value) and key not in result, f"duplicate/empty {label} field on line {number}")
        result[key] = value
    return result


def check_manifest(case_dir: Path) -> set[Path]:
    manifest = safe_file(case_dir / "artifact-sha256.txt", "artifact manifest")
    declared: dict[Path, str] = {}
    for number, line in enumerate(manifest.read_text(encoding="utf-8", errors="strict").splitlines(), 1):
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        need(match is not None, f"malformed artifact manifest line {number}")
        digest, raw = match.groups()
        path = Path(raw)
        if not path.is_absolute():
            path = case_dir / path
        resolved = safe_file(path, "manifest artifact", nonempty=path.suffix.lower() != ".log")
        try:
            resolved.relative_to(case_dir.resolve())
        except ValueError as exc:
            raise CheckError(f"artifact escapes scenario directory: {raw}") from exc
        need(resolved not in declared, f"duplicate artifact manifest entry: {raw}")
        need(sha256(resolved) == digest, f"artifact digest mismatch: {raw}")
        declared[resolved] = digest
    actual: set[Path] = set()
    for path in case_dir.rglob("*"):
        if path.is_symlink():
            need(path.suffix.lower() not in ARTIFACT_SUFFIXES,
                 f"generated artifact is a symlink: {path.relative_to(case_dir)}")
        elif path.is_file() and path.suffix.lower() in ARTIFACT_SUFFIXES:
            actual.add(path.resolve())
    need(actual == set(declared), "artifact manifest does not exactly cover generated RTL/XML/log files")
    return set(declared)


def check_runtime_proof(compiler_output: str, profile: str, scenario: str) -> None:
    records: list[dict[str, object]] = []
    for number, line in enumerate(compiler_output.splitlines(), 1):
        if "AXI_BURST_DUPLEX_PROOF" not in line:
            continue
        match = re.search(r"(?:^|\s)AXI_BURST_DUPLEX_PROOF (\{.*\})\s*$", line)
        need(match is not None, f"{profile}/{scenario}: malformed duplex proof record on compiler-output line {number}")
        try:
            record = json.loads(match.group(1))
        except json.JSONDecodeError as exc:
            raise CheckError(f"{profile}/{scenario}: invalid duplex proof JSON on compiler-output line {number}") from exc
        need(isinstance(record, dict),
             f"{profile}/{scenario}: duplex proof record on line {number} is not a JSON object")
        records.append(record)

    bundle_records = [record for record in records if record.get("bundle") == "shared"]
    need(len(bundle_records) == 1,
         f"{profile}/{scenario}: expected exactly one well-formed duplex proof record for bundle shared; "
         f"got {len(bundle_records)}")
    record = bundle_records[0]
    need(record.get("semantic_proof") is True,
         f"{profile}/{scenario}: bundle shared proof does not assert semantic_proof=true")
    need(record.get("runtime_range_guard") is True,
         f"{profile}/{scenario}: bundle shared proof does not assert runtime_range_guard=true")
    need(record.get("proof_kind") == "distinct_base_runtime_guarded_pointwise",
         f"{profile}/{scenario}: bundle shared proof has unexpected proof_kind")


def check_zero_trip_vcd(path: Path, profile: str) -> None:
    text = safe_file(path, "zero-trip simulation VCD").read_text(encoding="ascii", errors="strict")
    valid_ids: dict[str, set[str]] = {"arvalid": set(), "awvalid": set()}
    definitions_ended = False
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if line.startswith("$var "):
            fields = line.split()
            need(len(fields) >= 6 and fields[-1] == "$end", "malformed VCD variable declaration")
            name = fields[4]
            for channel in valid_ids:
                if re.search(rf"(?:^|[^A-Za-z0-9])m_axi_[A-Za-z0-9_]*_{channel}$", name):
                    valid_ids[channel].add(fields[3])
        elif line.startswith("$enddefinitions"):
            definitions_ended = True
            break
    need(definitions_ended, f"{profile}/zero-trip: VCD lacks $enddefinitions")
    for channel, identifiers in valid_ids.items():
        need(identifiers, f"{profile}/zero-trip: VCD lacks an m_axi {channel.upper()} signal")

    observed = {identifier: 0 for identifiers in valid_ids.values() for identifier in identifiers}
    for line in text.split("$enddefinitions", 1)[1].splitlines():
        scalar = re.fullmatch(r"([01xXzZ])(\S+)", line.strip())
        vector = re.fullmatch(r"[bB]([01xXzZ]+)\s+(\S+)", line.strip())
        if scalar:
            value, identifier = scalar.groups()
        elif vector:
            value, identifier = vector.groups()
        else:
            continue
        if identifier not in observed:
            continue
        observed[identifier] += 1
        need(value.lower() == "0", f"{profile}/zero-trip: VCD shows nonzero/unknown {identifier} activity")
    need(all(count > 0 for count in observed.values()),
         f"{profile}/zero-trip: VCD does not observe all m_axi ARVALID/AWVALID signals")


def parse_axi_vcd(path: Path, profile: str, scenario: str) -> tuple[dict[str, list[int]], int]:
    text = safe_file(path, "overlap AXI simulation VCD").read_text(encoding="ascii", errors="strict")
    signal_ids: dict[str, set[str]] = {"clock": set()}
    for signal in ("arvalid", "arready", "awvalid", "awready", "wvalid", "wready", "bvalid", "bready"):
        signal_ids[signal] = set()
    definitions_ended = False
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if line.startswith("$var "):
            fields = line.split()
            need(len(fields) >= 6 and fields[-1] == "$end", "malformed VCD variable declaration")
            name = fields[-2]
            if name in {"clock", "sig_clock"}:
                signal_ids["clock"].add(fields[3])
            elif name.startswith("sig_m_axi_shared_"):
                suffix = name.rsplit("_", 1)[-1]
                if suffix in signal_ids:
                    signal_ids[suffix].add(fields[3])
        elif line.startswith("$enddefinitions"):
            definitions_ended = True
            break
    need(definitions_ended, f"{profile}/{scenario}: VCD lacks $enddefinitions")
    for signal, identifiers in signal_ids.items():
        need(len(identifiers) == 1,
             f"{profile}/{scenario}: VCD must define exactly one shared-bundle {signal.upper()} signal")

    ids = {signal: next(iter(identifiers)) for signal, identifiers in signal_ids.items()}
    values = {identifier: "x" for identifier in ids.values()}
    events: dict[str, list[int]] = {name: [] for name in ("ar", "aw", "w", "b")}
    cycle = 0
    timestamp: int | None = None
    updates: dict[str, str] = {}

    def commit_timestamp() -> None:
        nonlocal cycle
        if timestamp is None:
            return
        old_clock = values[ids["clock"]]
        values.update(updates)
        if old_clock == "0" and values[ids["clock"]] == "1":
            cycle += 1
            for channel in events:
                if values[ids[f"{channel}valid"]] == "1" and values[ids[f"{channel}ready"]] == "1":
                    events[channel].append(cycle)

    for line in text.splitlines():
        if line.startswith("$enddefinitions"):
            continue
        if line.startswith("#"):
            commit_timestamp()
            timestamp = int(line[1:])
            updates = {}
            continue
        if timestamp is None:
            continue
        stripped = line.strip()
        scalar = re.fullmatch(r"([01xXzZ])(.+)", stripped)
        vector = re.fullmatch(r"[bB]([01xXzZ]+)\s+(\S+)", stripped)
        if scalar:
            value, identifier = scalar.groups()
        elif vector:
            value, identifier = vector.groups()
            value = value[-1]
        else:
            continue
        if identifier in values:
            updates[identifier] = value.lower()
    commit_timestamp()
    return events, cycle


def check_overlap_vcd(path: Path, profile: str, scenario: str) -> None:
    events, total_cycles = parse_axi_vcd(path, profile, scenario)
    expected = 64
    for channel, handshakes in events.items():
        need(len(handshakes) == expected,
             f"{profile}/{scenario}: expected {expected} {channel.upper()} handshakes, got {len(handshakes)}")

    minimum_b_delay_cycles = 32
    for index, (ar, aw, write, response) in enumerate(
            zip(events["ar"], events["aw"], events["w"], events["b"], strict=True)):
        need(ar < aw and ar < write and aw <= response and write < response,
             f"{profile}/{scenario}: iteration {index} AXI read/write/response ordering is invalid")
        delay = response - write
        need(delay >= minimum_b_delay_cycles,
             f"{profile}/{scenario}: iteration {index} B completion delay is only {delay} cycles; "
             f"need at least {minimum_b_delay_cycles}")
        if index + 1 < expected:
            need(events["ar"][index + 1] > response,
                 f"{profile}/{scenario}: next AR handshake precedes prior B completion at iteration {index}")
    need(total_cycles > events["b"][-1], f"{profile}/{scenario}: VCD does not extend beyond final B handshake")


def check_range_probe(out: Path, args: argparse.Namespace) -> None:
    probe_dir = out / "rtl_range_guard"
    need(probe_dir.is_dir() and not probe_dir.is_symlink(), "RTL range-guard probe output directory missing or symlink")
    probe_dir = probe_dir.resolve()
    kernel = safe_file(out / "symmetric/disjoint/kernel.v", "range-probe generated DUT RTL")
    libtech = safe_file(kernel.parent / "panda_libtech.v", "range-probe generated library RTL")
    fields = parse_fields(probe_dir / "provenance.txt", "range-probe provenance")
    expected = {
        "kernel": str(kernel),
        "libtech": str(libtech),
        "testbench": str(args.range_probe_testbench.resolve()),
        "runner": str(args.range_probe_runner.resolve()),
        "verilator": str(args.simulator.resolve()),
    }
    for key, value in expected.items():
        need(fields.get(key) == value, f"range probe provenance {key} mismatch")
    for key, path in (("kernel", kernel), ("libtech", libtech), ("testbench", args.range_probe_testbench),
                      ("runner", args.range_probe_runner), ("verilator", args.simulator)):
        need(fields.get(f"{key}_sha256") == sha256(path), f"range probe {key} digest mismatch")

    argv_text = fields.get("argv", "")
    need(bool(argv_text), "range probe invocation argv missing")
    argv = shlex.split(argv_text)
    need(argv and argv[0] == str(args.simulator.resolve()), "range probe executable does not match configured Verilator")
    need("--binary" in argv and "--timing" in argv and "--trace" in argv,
         "range probe does not build a timed Verilator simulation with diagnostic tracing")
    need(argv.count("--top-module") == 1 and
         argv[argv.index("--top-module") + 1] == "tb_duplex_alias_range_guard",
         "range probe top module mismatch")
    need(str(kernel) in argv and str(libtech) in argv and str(args.range_probe_testbench.resolve()) in argv,
         "range probe does not compile the symmetric/disjoint kernel and library RTL with the assigned testbench")
    need(not any(arg.startswith("--device-name") or arg.startswith("--target-file") for arg in argv),
         "range probe invocation unexpectedly overrides target")

    build_log = safe_file(probe_dir / "build.log", "range-probe Verilator build log")
    del build_log
    probe_log = safe_file(probe_dir / "probe.log", "range-probe simulation log")
    expected_markers = [
        "DUPLEX_ALIAS_RANGE_PASS CASE=valid-disjoint ARLEN=15",
        "DUPLEX_ALIAS_RANGE_PASS CASE=read-end-overflow ARLEN=0",
        "DUPLEX_ALIAS_RANGE_PASS CASE=write-end-overflow ARLEN=0",
        "DUPLEX_ALIAS_RANGE_PASS CASE=byte-count-overflow ARLEN=0",
        "DUPLEX_ALIAS_RANGE_PASS CASE=read-end-equals-2p32 ARLEN=15",
    ]
    lines = probe_log.read_text(encoding="utf-8", errors="strict").splitlines()
    observed = [line for line in lines if line.startswith("DUPLEX_ALIAS_RANGE_PASS CASE=")]
    need(observed == expected_markers, f"range probe scenario markers mismatch: {observed!r}")
    for case_name in ("valid-disjoint", "read-end-overflow", "write-end-overflow",
                      "byte-count-overflow", "read-end-equals-2p32"):
        safe_file(probe_dir / f"range_guard_{case_name}.vcd", f"range-probe {case_name} VCD")
    summary = "DUPLEX_ALIAS_RANGE_GUARD_PASS CASES=1 NO_AXI_HANDSHAKES=1"
    summaries = [line for line in lines if line.startswith("DUPLEX_ALIAS_RANGE_GUARD_PASS")]
    need(summaries == [summary] * 5, "range probe lacks one no-handshake marker per isolated case")
    runner_log = safe_file(out / "rtl_range_guard_runner.log", "range-probe runner log")
    expected_runner_marker = (f"DUPLEX_ALIAS_RANGE_PROBE PASS CASES=5 RTL_SHA256={sha256(kernel)} "
                              f"OUTPUT={probe_dir}")
    need(runner_log.read_text(encoding="utf-8", errors="strict").splitlines() == [expected_runner_marker],
         "range-probe runner log lacks exact success/provenance marker")


def check_case(out: Path, profile: str, scenario: str, args: argparse.Namespace) -> None:
    case_dir = out / profile / scenario
    need(case_dir.is_dir() and not case_dir.is_symlink(), f"scenario directory missing or symlink: {case_dir}")
    case_dir = case_dir.resolve()
    fields = parse_fields(case_dir / "provenance.txt", "scenario provenance")
    profile_id = PROFILES[profile]
    scenario_id = SCENARIOS[scenario]
    expected = {
        "profile": profile,
        "profile_id": profile_id,
        "scenario": scenario,
        "scenario_id": scenario_id,
        "feature": "1",
        "output": str(case_dir),
        "source": str(args.source.resolve()),
        "testbench": str(args.testbench.resolve()),
        "checker": str(Path(__file__).resolve()),
        "runner": str(args.runner.resolve()),
        "bambu": str(args.bambu.resolve()),
        "settings": str(args.settings.resolve()),
        "plugin": str(args.plugin.resolve()),
        "simulator": str(args.simulator.resolve()),
    }
    for key, value in expected.items():
        need(fields.get(key) == value, f"{profile}/{scenario}: provenance {key} mismatch")
    need(bool(re.fullmatch(r"[0-9a-fA-F]{7,40}", fields.get("git_head", ""))),
         f"{profile}/{scenario}: malformed git_head")
    for key, path in (("source", args.source), ("testbench", args.testbench), ("checker", Path(__file__).resolve()),
                      ("runner", args.runner), ("bambu", args.bambu), ("settings", args.settings),
                      ("plugin", args.plugin), ("simulator", args.simulator)):
        need(fields.get(f"{key}_sha256") == sha256(path), f"{profile}/{scenario}: {key} digest mismatch")

    count = 0 if scenario == "zero-trip" else 64
    marker = f"DUPLEX_ALIAS_GOLDEN_PASS CASE={scenario} profile={profile_id} N={count}"
    if scenario == "zero-trip":
        marker += " NO_DATA_CHANGE=1"
    need(fields.get("marker") == marker, f"{profile}/{scenario}: provenance marker mismatch")
    testbench_log = safe_file(case_dir / "HLS_output/simulation/testbench.log", "simulation testbench log")
    marker_lines = [line for line in testbench_log.read_text(encoding="utf-8", errors="replace").splitlines()
                    if line.startswith("DUPLEX_ALIAS_GOLDEN_PASS")]
    need(marker_lines == [marker], f"{profile}/{scenario}: expected exactly one golden marker; got {marker_lines!r}")

    activation = "Activated guarded distinct-base duplex for bundle shared (overlap uses serialized scalar transfers)"
    compiler_output = "\n".join((case_dir / name).read_text(encoding="utf-8", errors="replace")
                                 for name in ("stdout.log", "stderr.log"))
    need(compiler_output.count(activation) == 1,
         f"{profile}/{scenario}: expected exactly one runtime-range guard activation witness")
    check_runtime_proof(compiler_output, profile, scenario)

    vcd = case_dir / "HLS_output/simulation/test.vcd"
    if scenario == "zero-trip":
        check_zero_trip_vcd(vcd, profile)
    elif scenario in {"forward-overlap", "reverse-overlap"}:
        check_overlap_vcd(vcd, profile, scenario)

    required_artifacts = {
        safe_file(case_dir / "bambu_results.xml", "Bambu results XML"),
        safe_file(case_dir / "kernel.v", "generated kernel RTL"),
        safe_file(testbench_log, "simulation testbench log"),
        safe_file(case_dir / "stdout.log", "Bambu stdout log", nonempty=False),
        safe_file(case_dir / "stderr.log", "Bambu stderr log", nonempty=False),
    }
    if scenario == "zero-trip":
        required_artifacts.add(safe_file(vcd, "zero-trip simulation VCD"))
    elif scenario in {"forward-overlap", "reverse-overlap"}:
        required_artifacts.add(safe_file(vcd, "overlap AXI simulation VCD"))
    artifacts = check_manifest(case_dir)
    need(required_artifacts <= artifacts, f"{profile}/{scenario}: required evidence omitted from manifest")

    argv_text = fields.get("argv", "")
    need(bool(argv_text), f"{profile}/{scenario}: invocation argv missing")
    argv = shlex.split(argv_text)
    need(len(argv) > 2 and argv[0] == str(args.bambu.resolve()) and argv[1] == str(args.source.resolve()),
         f"{profile}/{scenario}: invocation executable/source mismatch")
    need("--simulate" in argv and "--simulator=VERILATOR" in argv,
         f"{profile}/{scenario}: invocation does not run Verilator simulation")
    need(argv.count("--memory-allocation-policy=NO_BRAM") == 1,
         f"{profile}/{scenario}: invocation must select NO_BRAM for delayed memory responses")
    need(argv.count("--debug-classes=InterfaceInfer,BuildVirtualPhi") == 1,
         f"{profile}/{scenario}: InterfaceInfer activation diagnostics were not enabled exactly once")
    needs_vcd = scenario == "zero-trip" or scenario in {"forward-overlap", "reverse-overlap"}
    need(argv.count("--generate-vcd") == (1 if needs_vcd else 0),
         f"{profile}/{scenario}: VCD generation does not match scenario")
    need(argv.count("--mem-delay-write=64") ==
         (1 if scenario in {"forward-overlap", "reverse-overlap"} else 0),
         f"{profile}/{scenario}: overlap case must use exactly 64 cycles of memory write delay")
    need(not any(arg.startswith("--device-name") or arg.startswith("--target-file") for arg in argv),
         f"{profile}/{scenario}: expected default device but invocation overrides target")
    need(argv.count("--bambu-parameter=experimental-m-axi-burst=1") == 1,
         f"{profile}/{scenario}: burst feature is not enabled exactly once")
    need(argv.count(f"--extra-cc-options=-DDUPLEX_ALIAS_PROFILE={profile_id}") == 1 and
         argv.count(f"--extra-cc-options=-DDUPLEX_ALIAS_CASE={scenario_id}") == 1,
         f"{profile}/{scenario}: invocation profile/scenario selection mismatch")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--testbench", required=True, type=Path)
    parser.add_argument("--bambu", required=True, type=Path)
    parser.add_argument("--settings", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    parser.add_argument("--simulator", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--range-probe-runner", required=True, type=Path)
    parser.add_argument("--range-probe-testbench", required=True, type=Path)
    args = parser.parse_args()
    out = args.output.resolve(strict=True)
    need(out.is_dir() and not args.output.is_symlink(), "output root missing or symlink")
    for path, label in ((args.source, "source"), (args.testbench, "testbench"), (args.bambu, "Bambu"),
                        (args.settings, "settings"), (args.plugin, "plugin"), (args.simulator, "simulator"),
                        (args.runner, "runner"), (args.range_probe_runner, "range-probe runner"),
                        (args.range_probe_testbench, "range-probe testbench")):
        safe_file(path, label)
    for profile in PROFILES:
        for scenario in SCENARIOS:
            check_case(out, profile, scenario, args)
    check_range_probe(out, args)
    expected_case_dirs = {str((out / profile / scenario).resolve()) for profile in PROFILES for scenario in SCENARIOS}
    actual_case_dirs = {str(path.resolve()) for profile_dir in out.iterdir() if profile_dir.name in PROFILES
                        for path in profile_dir.iterdir() if path.is_dir()}
    need(actual_case_dirs == expected_case_dirs, "scenario matrix contains missing or unexpected case directories")
    print("DUPLEX_ALIAS_CHECK PASS PROFILES=2 SCENARIOS=8 (zero-trip has no AR/AW; overlap VCD proves delayed B serialization)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except CheckError as exc:
        print(f"DUPLEX ALIAS CHECK FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc
