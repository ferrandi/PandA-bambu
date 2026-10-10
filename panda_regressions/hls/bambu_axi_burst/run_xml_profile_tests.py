#!/usr/bin/env python3
"""Exercise strict AXI O/D validation through Bambu's .bambuir XML route."""
from __future__ import annotations

import argparse
import ctypes
import json
import pathlib
import re
import shlex
import subprocess
import sys
import xml.etree.ElementTree as ET


TESTS = pathlib.Path(__file__).resolve().parent
CHECKER = TESTS / "check_compiler_artifacts.py"
ULONG_MAX = (1 << (ctypes.sizeof(ctypes.c_ulong) * 8)) - 1


def profile_bundle(path: pathlib.Path, symbol: str) -> ET.Element:
    root = ET.parse(path).getroot()
    functions = [node for node in root.findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected exactly one function symbol={symbol!r} in {path}, found {len(functions)}")
    bundles = [node for node in functions[0].findall("./bundles/bundle")
               if node.get("name") == "data" and node.get("mode") == "m_axi"]
    if len(bundles) != 1:
        raise RuntimeError(f"expected exactly one bundle name='data' mode='m_axi' in {path}, found {len(bundles)}")
    return bundles[0]


def verify_seed(path: pathlib.Path, symbol: str) -> None:
    bundle = profile_bundle(path, symbol)
    if bundle.get("max_read_burst_length") != "16":
        raise RuntimeError(f"seed must have max_read_burst_length=16, got {bundle.attrib}")
    # The architecture format omits defaults; an explicit canonical value is
    # also semantically the same seed profile. With B=16 and one outstanding
    # burst the derived depth is next_pow2(max(2,1)*16) = 32.
    if bundle.get("num_read_outstanding", "1") != "1":
        raise RuntimeError(f"seed must have effective num_read_outstanding=1, got {bundle.attrib}")
    if bundle.get("read_fifo_depth", "32") != "32":
        raise RuntimeError(f"seed must have effective read_fifo_depth=32, got {bundle.attrib}")


def write_case_xml(seed: pathlib.Path, target: pathlib.Path, symbol: str,
                   values: dict[str, str | None]) -> None:
    tree = ET.parse(seed)
    root = tree.getroot()
    functions = [node for node in root.findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected exactly one function symbol={symbol!r} in {seed}, found {len(functions)}")
    bundles = [node for node in functions[0].findall("./bundles/bundle")
               if node.get("name") == "data" and node.get("mode") == "m_axi"]
    if len(bundles) != 1:
        raise RuntimeError(f"expected exactly one bundle name='data' mode='m_axi' in {seed}, found {len(bundles)}")
    bundle = bundles[0]
    for attribute in ("num_read_outstanding", "read_fifo_depth"):
        bundle.attrib.pop(attribute, None)
    for attribute, value in values.items():
        if value is not None:
            bundle.set(attribute, value)
    tree.write(target, encoding="utf-8", xml_declaration=True)


def record_argv(case_dir: pathlib.Path, argv: list[str]) -> None:
    (case_dir / "argv.json").write_text(json.dumps(argv, indent=2) + "\n", encoding="utf-8")
    (case_dir / "argv.txt").write_text(shlex.join(argv) + "\n", encoding="utf-8")


def invoke(bambu: pathlib.Path, case_dir: pathlib.Path, raw_ir: pathlib.Path,
           architecture: pathlib.Path, top: str, burst_parameter: str) -> subprocess.CompletedProcess[str]:
    argv = [str(bambu), "--use-raw", str(raw_ir), f"--architecture-xml={architecture}",
            f"--top-fname={top}", "--compiler=I386_CLANG16", "--generate-interface=INFER",
            "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL", "--no-clean",
            f"--bambu-parameter={burst_parameter}=1",
            "--extra-cc-options=-DNDEBUG", "--extra-cc-options=-DBAMBU"]
    record_argv(case_dir, argv)
    result = subprocess.run(argv, cwd=case_dir, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False)
    (case_dir / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case_dir / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case_dir / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def check_positive(case_dir: pathlib.Path, result: subprocess.CompletedProcess[str],
                   outstanding: int, depth: int) -> None:
    if result.returncode != 0:
        raise RuntimeError(f"positive XML profile exited {result.returncode}; see {case_dir}")
    argv = [sys.executable, str(CHECKER), "rtl", "--output", str(case_dir),
            "--require=MinimalAXI4MasterPipelined", "--configure-log", str(case_dir / "stderr.log"),
            "--configured-contract", "--expected-burst-max=16",
            f"--expected-outstanding={outstanding}", f"--expected-fifo-depth={depth}"]
    checked = subprocess.run(argv, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    (case_dir / "artifact_checker_argv.json").write_text(json.dumps(argv, indent=2) + "\n", encoding="utf-8")
    (case_dir / "artifact_checker.log").write_text(checked.stdout, encoding="utf-8")
    if checked.returncode != 0:
        raise RuntimeError(f"positive XML profile RTL check failed; see {case_dir}/artifact_checker.log")
    print(f"XML-PROFILE PASS: O={outstanding}/D={depth} ({case_dir.name})")


def check_negative(case_dir: pathlib.Path, result: subprocess.CompletedProcess[str],
                   attribute: str) -> None:
    if result.returncode == 0:
        raise RuntimeError(f"invalid {attribute} XML profile was accepted; see {case_dir}")
    if result.returncode < 0 or result.returncode >= 128:
        raise RuntimeError(f"invalid {attribute} XML profile crashed (exit {result.returncode}); see {case_dir}")
    combined = result.stdout + "\n" + result.stderr
    expected = re.compile(
        rf"Invalid AXI interface attribute '{re.escape(attribute)}' for bundle 'data'",
        re.IGNORECASE,
    )
    if not expected.search(combined):
        raise RuntimeError(
            f"invalid {attribute} XML profile failed without the explicit attribute+bundle diagnostic; "
            f"see {case_dir}"
        )
    print(f"XML-PROFILE reject PASS: {case_dir.name}")


def case_name(attribute: str, value: str) -> str:
    labels = {"": "empty", " ": "whitespace", "-1": "negative", "+1": "plus-one",
              "0": "zero", "3x": "suffix", "1x": "suffix", "18446744073709551616": "overflow"}
    safe = labels.get(value, re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_") or "empty")
    return f"invalid-{attribute}-{safe}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bambu", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("seed_ir", type=pathlib.Path)
    parser.add_argument("seed_xml", type=pathlib.Path)
    parser.add_argument("stride_ir", type=pathlib.Path)
    parser.add_argument("stride_xml", type=pathlib.Path)
    parser.add_argument("--burst-parameter", required=True)
    args = parser.parse_args()

    bambu = args.bambu.resolve(strict=True)
    output = args.output.resolve(strict=True)
    seed_ir = args.seed_ir.resolve(strict=True)
    seed_xml = args.seed_xml.resolve(strict=True)
    stride_ir = args.stride_ir.resolve(strict=True)
    stride_xml = args.stride_xml.resolve(strict=True)
    if not bambu.is_file() or not bambu.stat().st_mode & 0o111:
        raise RuntimeError(f"Bambu is not executable: {bambu}")
    if seed_ir.suffix != ".bambuir" or stride_ir.suffix != ".bambuir":
        raise RuntimeError("XML-profile tests require frontend-produced .bambuir files (debug .raw is not accepted)")
    verify_seed(seed_xml, "renamed_sum")
    verify_seed(stride_xml, "stride_two")
    # Record immutable frontend inputs and their provenance alongside the matrix.
    (output / "seed_inputs.json").write_text(json.dumps({
        "recognized_ir": str(seed_ir), "recognized_xml": str(seed_xml),
        "unsupported_stride_ir": str(stride_ir), "unsupported_stride_xml": str(stride_xml),
        "recognized_profile": {"max_read_burst_length": 16,
                                "num_read_outstanding": 1, "read_fifo_depth": 32},
        "unsupported_stride_profile": {"max_read_burst_length": 16,
                                        "num_read_outstanding": 1, "read_fifo_depth": 32},
    }, indent=2) + "\n", encoding="utf-8")

    positive = [
        # An omitted depth derives from the other two attributes:
        # next_pow2(max(2, O) * B) with B=16.
        ("defaults-omitted", {}, 1, 32),
        ("outstanding-only-3", {"num_read_outstanding": "3"}, 3, 64),
        ("depth-only-8", {"read_fifo_depth": "8"}, 1, 8),
        ("o1-d1", {"num_read_outstanding": "1", "read_fifo_depth": "1"}, 1, 1),
        ("o3-d8", {"num_read_outstanding": "3", "read_fifo_depth": "8"}, 3, 8),
        ("o16-d4096", {"num_read_outstanding": "16", "read_fifo_depth": "4096"}, 16, 4096),
        ("leading-zeroes-o03-d0008", {"num_read_outstanding": "03", "read_fifo_depth": "0008"}, 3, 8),
    ]
    for name, values, outstanding, depth in positive:
        case_dir = output / "cases" / name
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_case_xml(seed_xml, architecture, "renamed_sum", values)
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        check_positive(case_dir, result, outstanding, depth)

    invalid_values = ["", "-1", "+1", " ", "3x", "0", str(ULONG_MAX + 1)]
    matrix = [("num_read_outstanding", value) for value in invalid_values]
    matrix += [("num_read_outstanding", "17"), ("num_read_outstanding", "4294967297")]
    matrix += [("read_fifo_depth", value) for value in invalid_values]
    matrix += [("read_fifo_depth", "4097"), ("read_fifo_depth", "3"),
               ("read_fifo_depth", "4294967552")]
    for attribute, value in matrix:
        case_dir = output / "cases" / case_name(attribute, value)
        if case_dir.exists():
            case_dir = output / "cases" / f"{case_dir.name}-range"
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_case_xml(seed_xml, architecture, "renamed_sum", {attribute: value})
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        check_negative(case_dir, result, attribute)

    # The unsupported stride must still reject malformed architecture data;
    # it must not silently select the legacy fallback before parsing O/D.
    for attribute, value in (("num_read_outstanding", ""), ("read_fifo_depth", "3x")):
        name = f"unsupported-stride-{attribute}-malformed"
        case_dir = output / "cases" / name
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_case_xml(stride_xml, architecture, "stride_two", {attribute: value})
        result = invoke(bambu, case_dir, stride_ir, architecture, "stride_two", args.burst_parameter)
        check_negative(case_dir, result, attribute)
    print(f"XML-PROFILE suite PASS: {len(positive)} positive, {len(matrix) + 2} negative")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ET.ParseError) as error:
        print(f"XML-PROFILE FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
