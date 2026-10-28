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
    # This source specifies B only. The serialized architecture XML omits O/D;
    # their effective values come from the defaults, O=16 and D=256.
    if "num_read_outstanding" in bundle.attrib or "read_fifo_depth" in bundle.attrib:
        raise RuntimeError(f"seed must omit default O/D attributes in serialized XML, got {bundle.attrib}")


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


def write_unbound_profile_xml(seed: pathlib.Path, target: pathlib.Path, symbol: str,
                              bundle_name: str, attributes: dict[str, str]) -> None:
    tree = ET.parse(seed)
    functions = [node for node in tree.getroot().findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected exactly one function symbol={symbol!r} in {seed}, found {len(functions)}")
    bundles = functions[0].find("bundles")
    if bundles is None:
        raise RuntimeError(f"function {symbol!r} has no bundles element in {seed}")
    bundles.append(ET.Element("bundle", {"mode": "m_axi", "name": bundle_name, **attributes}))
    tree.write(target, encoding="utf-8", xml_declaration=True)
    written_function = [node for node in ET.parse(target).getroot().findall("function")
                        if node.get("symbol") == symbol][0]
    matches = [node for node in written_function.findall("./bundles/bundle")
               if node.get("name") == bundle_name and node.get("mode") == "m_axi"]
    if len(matches) != 1:
        raise RuntimeError(f"expected exactly one unbound m_axi bundle {bundle_name!r} in {target}")
    parameter_bindings = [node for node in written_function.findall("./parameters/parameter")
                          if node.get("bundle") == bundle_name]
    if parameter_bindings:
        raise RuntimeError(f"test bundle {bundle_name!r} unexpectedly binds parameters in {target}")


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
                   attribute: str, bundle_name: str = "data") -> None:
    if result.returncode == 0:
        raise RuntimeError(f"invalid {attribute} XML profile was accepted; see {case_dir}")
    if result.returncode < 0 or result.returncode >= 128:
        raise RuntimeError(f"invalid {attribute} XML profile crashed (exit {result.returncode}); see {case_dir}")
    combined = result.stdout + "\n" + result.stderr
    expected = re.compile(
        rf"Invalid AXI interface attribute '{re.escape(attribute)}' for bundle '{re.escape(bundle_name)}'",
        re.IGNORECASE,
    )
    if not expected.search(combined):
        raise RuntimeError(
            f"invalid {attribute} XML profile failed without the explicit attribute+bundle diagnostic; "
            f"see {case_dir}"
        )
    print(f"XML-PROFILE reject PASS: {case_dir.name}")


def check_unbound_profile_accept(case_dir: pathlib.Path,
                                 result: subprocess.CompletedProcess[str],
                                 bundle_name: str,
                                 expected_profile: str) -> None:
    if result.returncode != 0:
        raise RuntimeError(f"valid unbound profile {bundle_name!r} was rejected "
                           f"(exit {result.returncode}); see {case_dir}")
    combined = result.stdout + "\n" + result.stderr
    if not re.search(rf"AXI_BURST_ANALYSIS root=.* bundle={re.escape(bundle_name)} "
                     r"enabled=1 profile=not-inferred reason=unbound(?: m_axi bundle)?(?:;|\s)", combined):
        raise RuntimeError(f"unbound profile {bundle_name!r} lacks the not-inferred diagnostic; "
                           f"see {case_dir}")
    if expected_profile not in combined:
        raise RuntimeError(f"unbound profile {bundle_name!r} lacks expected validated metadata "
                           f"{expected_profile!r}; see {case_dir}")
    rtl = [path for path in case_dir.rglob("*.v") if path.is_file() and path.name != "panda_libtech.v"]
    if not rtl:
        raise RuntimeError(f"unbound profile {bundle_name!r} emitted no RTL; see {case_dir}")
    for path in rtl:
        if bundle_name in path.read_text(encoding="utf-8", errors="replace"):
            raise RuntimeError(f"unbound profile {bundle_name!r} unexpectedly emitted a bundle resource "
                               f"in {path}")
    print(f"XML-PROFILE unbound metadata PASS: {case_dir.name}")


def check_unbound_write_profile(case_dir: pathlib.Path,
                                result: subprocess.CompletedProcess[str],
                                bundle_name: str, outstanding: int | None,
                                expected_profile: str) -> None:
    """An unbound write bundle may ask for more than one burst in flight.

    The profile is read from the architecture XML, so this case proves the XML path accepts the
    attribute; the emitted engine generic is checked by the pragma-path cases in the write
    compiler suite, where the bundle is bound to a variable and an engine is actually emitted.
    """
    if result.returncode != 0:
        raise RuntimeError(f"unbound write profile {bundle_name!r} with outstanding={outstanding} "
                           f"was rejected (exit {result.returncode}); see {case_dir}")
    combined = result.stdout + "\n" + result.stderr
    if not re.search(rf"AXI_BURST_ANALYSIS root=.* bundle={re.escape(bundle_name)} "
                     r"enabled=1 profile=not-inferred reason=unbound(?: m_axi bundle)?(?:;|\s)", combined):
        raise RuntimeError(f"unbound write profile {bundle_name!r} lacks the not-inferred diagnostic; "
                           f"see {case_dir}")
    if expected_profile not in combined:
        raise RuntimeError(f"unbound write profile {bundle_name!r} lacks expected validated metadata "
                           f"{expected_profile!r}; see {case_dir}")
    refused = [line for line in combined.splitlines()
               if "write_burst_outstanding" in line and "only" in line]
    if refused:
        raise RuntimeError(f"unbound write profile {bundle_name!r} was refused despite succeeding: "
                           f"{refused[0]}; see {case_dir}")
    if not [path for path in case_dir.rglob("*.v") if path.is_file()]:
        raise RuntimeError(f"unbound write profile {bundle_name!r} emitted no Verilog under {case_dir}")
    for path in case_dir.rglob("*.v"):
        if path.is_file() and bundle_name in path.read_text(encoding="utf-8", errors="replace"):
            raise RuntimeError(f"unbound write profile {bundle_name!r} unexpectedly emitted a bundle resource "
                               f"in {path}")
    if outstanding is not None:
        (case_dir / "unbound_write_outstanding.txt").write_text(f"{outstanding}\n", encoding="utf-8")
    print(f"XML-PROFILE unbound write accept PASS: {case_dir.name} "
          f"write_burst_outstanding={outstanding if outstanding is not None else 'default'}")


def check_xml_alias_rejection(case_dir: pathlib.Path,
                              result: subprocess.CompletedProcess[str],
                              reason: str) -> None:
    if result.returncode == 0 or result.returncode < 0 or result.returncode >= 128:
        raise RuntimeError(f"invalid XML write-outstanding profile was accepted or crashed "
                           f"(exit {result.returncode}); see {case_dir}")
    combined = result.stdout + "\n" + result.stderr
    if reason not in combined or "bundle 'alias_write'" not in combined:
        raise RuntimeError(f"invalid XML write-outstanding profile failed without its explicit "
                           f"bundle diagnostic; see {case_dir}")
    print(f"XML-PROFILE write alias reject PASS: {case_dir.name}")


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
        "recognized_profile": {
            "serialized": {"max_read_burst_length": "16",
                           "num_read_outstanding": None, "read_fifo_depth": None},
            "effective": {"max_read_burst_length": 16,
                          "num_read_outstanding": 16, "read_fifo_depth": 256}},
        "unsupported_stride_profile": {
            "serialized": {"max_read_burst_length": "16",
                           "num_read_outstanding": None, "read_fifo_depth": None},
            "effective": {"max_read_burst_length": 16,
                          "num_read_outstanding": 16, "read_fifo_depth": 256}},
    }, indent=2) + "\n", encoding="utf-8")

    positive = [
        # Defaults are O=16/D=256. An omitted depth with O=3 derives to 64.
        ("defaults-omitted", {}, 16, 256),
        ("outstanding-only-3", {"num_read_outstanding": "3"}, 3, 64),
        ("depth-only-8", {"read_fifo_depth": "8"}, 16, 8),
        ("o1-d1", {"num_read_outstanding": "1", "read_fifo_depth": "1"}, 1, 1),
        ("o3-d8", {"num_read_outstanding": "3", "read_fifo_depth": "8"}, 3, 8),
        ("o16-d4096", {"num_read_outstanding": "16", "read_fifo_depth": "4096"}, 16, 4096),
        ("leading-zeroes-o03-d0008", {"num_read_outstanding": "03", "read_fifo_depth": "0008"}, 3, 8),
        ("latency-one-metadata", {"latency": "1"}, 16, 256),
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
    matrix += [("max_read_burst_length", "")]
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

    # F2 unlocked write_burst_outstanding > 1: an unbound bundle asking for two bursts in
    # flight compiles and carries the value into the engine generic, so it is a positive case.
    write_case_dir = output / "cases" / "unbound-write-o2"
    write_case_dir.mkdir(parents=True, exist_ok=False)
    write_architecture = write_case_dir / "architecture.xml"
    write_attributes = {"max_write_burst_length": "16", "write_burst_outstanding": "2"}
    write_unbound_profile_xml(seed_xml, write_architecture, "renamed_sum", "unbound_write",
                              write_attributes)
    write_result = invoke(bambu, write_case_dir, seed_ir, write_architecture, "renamed_sum",
                          args.burst_parameter)
    check_unbound_write_profile(write_case_dir, write_result, "unbound_write", 2,
                                "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
                                "write_max=16(explicit) write_outstanding=2(explicit) write_fifo=32(derived)")

    # Vitis spelling, the established Bambu spelling, and FIFO metadata are
    # accepted without max_write_burst_length. Leading zeroes compare numerically.
    unbound_write_aliases = (
        ("unbound-write-vitis-alias", {"num_write_outstanding": "03"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
         "write_max=16(default) write_outstanding=3(explicit) write_fifo=32(derived)"),
        ("unbound-write-bambu-alias", {"write_burst_outstanding": "3"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
         "write_max=16(default) write_outstanding=3(explicit) write_fifo=32(derived)"),
        ("unbound-write-aliases-equal", {"num_write_outstanding": "03",
                                         "write_burst_outstanding": "3"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
         "write_max=16(default) write_outstanding=3(explicit) write_fifo=32(derived)"),
        ("unbound-write-fifo-only", {"write_fifo_depth": "32"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
         "write_max=16(default) write_outstanding=16(default) write_fifo=32(explicit)"),
        ("unbound-write-explicit-max", {"max_write_burst_length": "8"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=256(derived) "
         "write_max=8(explicit) write_outstanding=16(default) write_fifo=16(derived)"),
    )
    for name, attributes, expected_profile in unbound_write_aliases:
        case_dir = output / "cases" / name
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_unbound_profile_xml(seed_xml, architecture, "renamed_sum", "alias_write", attributes)
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        outstanding = None if "fifo-only" in name or "explicit-max" in name else 3
        check_unbound_write_profile(case_dir, result, "alias_write", outstanding, expected_profile)

    alias_negative = [
        ("zero", {"num_write_outstanding": "0"},
         "Invalid AXI interface attribute 'num_write_outstanding'"),
        ("seventeen", {"num_write_outstanding": "17"},
         "Invalid AXI interface attribute 'num_write_outstanding'"),
        ("overflow", {"num_write_outstanding": "18446744073709551616"},
         "Invalid AXI interface attribute 'num_write_outstanding'"),
        ("suffix", {"num_write_outstanding": "3x"},
         "Invalid AXI interface attribute 'num_write_outstanding'"),
        ("legacy-zero", {"write_burst_outstanding": "0"},
         "Invalid AXI interface attribute 'write_burst_outstanding'"),
        ("legacy-seventeen", {"write_burst_outstanding": "17"},
         "Invalid AXI interface attribute 'write_burst_outstanding'"),
        ("legacy-overflow", {"write_burst_outstanding": "18446744073709551616"},
         "Invalid AXI interface attribute 'write_burst_outstanding'"),
        ("legacy-nondecimal", {"write_burst_outstanding": "3x"},
         "Invalid AXI interface attribute 'write_burst_outstanding'"),
        ("conflict", {"num_write_outstanding": "3", "write_burst_outstanding": "4"},
         "Conflicting num_write_outstanding and write_burst_outstanding values"),
    ]
    for name, attributes, diagnostic in alias_negative:
        case_dir = output / "cases" / f"unbound-write-alias-{name}"
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_unbound_profile_xml(seed_xml, architecture, "renamed_sum", "alias_write", attributes)
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        check_xml_alias_rejection(case_dir, result, diagnostic)

    unbound_profiles = [
        ("unbound-read-outstanding-only", {"num_read_outstanding": "3"},
         "read_max=16(default) read_outstanding=3(explicit) read_fifo=64(derived) "
         "write_max=16(default) write_outstanding=16(default) write_fifo=32(derived)"),
        ("unbound-read-depth-only", {"read_fifo_depth": "8"},
         "read_max=16(default) read_outstanding=16(default) read_fifo=8(explicit) "
         "write_max=16(default) write_outstanding=16(default) write_fifo=32(derived)"),
        ("unbound-read-explicit-max", {"max_read_burst_length": "8"},
         "read_max=8(explicit) read_outstanding=16(default) read_fifo=128(derived) "
         "write_max=16(default) write_outstanding=16(default) write_fifo=32(derived)"),
    ]
    for name, attributes, expected_profile in unbound_profiles:
        case_dir = output / "cases" / name
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_unbound_profile_xml(seed_xml, architecture, "renamed_sum", "unbound_read", attributes)
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        check_unbound_profile_accept(case_dir, result, "unbound_read", expected_profile)

    unbound_invalid = [
        ("read-outstanding-zero", "unbound_read", {"num_read_outstanding": "0"},
         "num_read_outstanding"),
        ("read-outstanding-17", "unbound_read", {"num_read_outstanding": "17"},
         "num_read_outstanding"),
        ("read-depth-zero", "unbound_read", {"read_fifo_depth": "0"}, "read_fifo_depth"),
        ("read-depth-3", "unbound_read", {"read_fifo_depth": "3"}, "read_fifo_depth"),
        ("read-depth-4097", "unbound_read", {"read_fifo_depth": "4097"}, "read_fifo_depth"),
        ("write-depth-below-default-burst", "unbound_write",
         {"write_fifo_depth": "8"}, "write_fifo_depth"),
    ]
    for name, bundle_name, attributes, invalid_attribute in unbound_invalid:
        case_dir = output / "cases" / f"unbound-invalid-{name}"
        case_dir.mkdir(parents=True, exist_ok=False)
        architecture = case_dir / "architecture.xml"
        write_unbound_profile_xml(seed_xml, architecture, "renamed_sum", bundle_name, attributes)
        result = invoke(bambu, case_dir, seed_ir, architecture, "renamed_sum", args.burst_parameter)
        check_negative(case_dir, result, invalid_attribute, bundle_name)
    positive_count = len(positive) + 1 + len(unbound_write_aliases) + len(unbound_profiles)
    negative_count = len(matrix) + 2 + len(alias_negative) + len(unbound_invalid)
    print(f"XML-PROFILE suite PASS: {positive_count} positive, {negative_count} negative")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ET.ParseError) as error:
        print(f"XML-PROFILE FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
