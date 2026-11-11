#!/usr/bin/env python3
"""Compile pragma and architecture-XML AXI latency metadata regression cases."""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import shlex
import subprocess
import sys
import xml.etree.ElementTree as ET

import compiler_test_support as compiler_support


TESTS = pathlib.Path(__file__).resolve().parent
FIXTURE = TESTS / "compiler_cases" / "latency_metadata.cpp"
CHECKER = TESTS / "check_compiler_artifacts.py"
BASE_ARGS = ["--compiler=I386_CLANG16", "--generate-interface=INFER",
             "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL",
             "--no-clean", "--extra-cc-options=-DNDEBUG", "--extra-cc-options=-DBAMBU"]


def prepare_output(path: pathlib.Path) -> pathlib.Path:
    """Create a fresh output directory before resolving it strictly."""
    path.mkdir(parents=True, exist_ok=False)
    return path.resolve(strict=True)


def select_case_defines(tops: str, reverse_root_order: bool = False) -> list[str]:
    """Expose only the intended source roots/pragmas to this frontend run."""
    defines = [f"--extra-cc-options=-DLATENCY_CASE_{top.strip().upper()}"
               for top in tops.split(",") if top.strip()]
    if reverse_root_order:
        defines.append("--extra-cc-options=-DLATENCY_REVERSE_ROOT_ORDER")
    return defines


def record(case: pathlib.Path, argv: list[str]) -> None:
    (case / "argv.json").write_text(json.dumps(argv, indent=2) + "\n", encoding="utf-8")
    (case / "argv.txt").write_text(shlex.join(argv) + "\n", encoding="utf-8")


def run(case: pathlib.Path, argv: list[str],
        expected_failure: str | None = None,
        allow_success: bool = False) -> subprocess.CompletedProcess[str]:
    case.mkdir(parents=True, exist_ok=False)
    record(case, argv)
    result = compiler_support.run_compiler(
        argv, cwd=case, timeout=600, expected_failure=expected_failure,
        sandbox_markers=("Bad system call", "Operation not permitted", "Failed to create stream fd"),
        diagnostic_output_dir=case,
        allow_success=allow_success,
    )
    (case / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def architecture(case: pathlib.Path) -> pathlib.Path:
    files = list(case.rglob("architecture.xml"))
    if len(files) != 1:
        raise RuntimeError(f"expected one architecture.xml under {case}, found {len(files)}")
    return files[0]


def testbench_delays(path: pathlib.Path) -> tuple[int, int]:
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    instances = [index for index, line in enumerate(lines)
                 if re.search(r"\bif_m_axi_[A-Za-z0-9_]+\s+#\(", line)]
    if len(instances) != 1:
        raise RuntimeError(f"expected one generated m_axi testbench instance in {path}, found {len(instances)}")
    block = "\n".join(lines[instances[0]:instances[0] + 8])
    read = re.search(r"\.READ_DELAY\((\d+)\)", block)
    write = re.search(r"\.WRITE_DELAY\((\d+)\)", block)
    if read is None or write is None:
        raise RuntimeError(f"generated m_axi instance is missing explicit delay parameters: {path}")
    return int(read.group(1)), int(write.group(1))


def effective_model_write_delay(path: pathlib.Path, parameter: int) -> tuple[int, int]:
    """Evaluate the generated model clamp for its selected WRITE_DELAY branch."""
    text = path.read_text(encoding="utf-8", errors="replace")
    match = re.search(
        r"TestbenchMEMAXI\s+#\(\.index\(index\),\s*\.WRITE_DELAY\(\s*WRITE_DELAY\s*\?\s*"
        r"\(WRITE_DELAY\s*>\s*1\s*\?\s*WRITE_DELAY\s*-\s*1\s*:\s*1\)\s*:\s*(\d+)\s*\)",
        text,
    )
    if match is None:
        raise RuntimeError(f"generated TestbenchMEMAXI is missing the write-delay clamp: {path}")
    fallback = int(match.group(1))
    effective = parameter - 1 if parameter > 1 else (1 if parameter else fallback)
    return effective, fallback


def check_stimulus_independence(bambu: pathlib.Path, out: pathlib.Path,
                                burst_parameter: str) -> list[dict[str, object]]:
    source = TESTS / "bambu_axi_burst_sum_profile.cpp"
    tb = TESTS / "bambu_axi_burst_tb.cpp"
    cases = (("metadata64-cli7", source, 7, 5, []),
             ("metadata128-cli7", source, 7, 5, ["--extra-cc-options=-DBAMBU_AXI_SUM_LAT128"]),
             ("metadata128-cli9", source, 9, 6, ["--extra-cc-options=-DBAMBU_AXI_SUM_LAT128"]))
    results: list[dict[str, object]] = []
    for name, kernel_source, read_delay, write_delay, profile_args in cases:
        case = out / "stimulus" / name
        result = run(case, [str(bambu), str(kernel_source), *BASE_ARGS,
                            f"--top-fname=kernel", f"--generate-tb={tb}", "--tb-arg=1024",
                            *profile_args,
                            "--memory-allocation-policy=NO_BRAM",
                            "--tb-param-size=src:16384", f"--mem-delay-read={read_delay}",
                            f"--mem-delay-write={write_delay}",
                            f"--bambu-parameter={burst_parameter}=1"])
        if result.returncode:
            raise RuntimeError(f"stimulus testbench generation failed for {name}, exit={result.returncode}; see {case}")
        generated = list(case.rglob("bambu_testbench.v"))
        if len(generated) != 1:
            raise RuntimeError(f"expected one generated testbench in {case}, found {len(generated)}")
        actual = testbench_delays(generated[0])
        expected = (read_delay, write_delay)
        if actual != expected:
            raise RuntimeError(f"memory stimulus mismatch for {name}: expected {expected}, got {actual}; see {generated[0]}")
        results.append({"case": f"stimulus/{name}", "expected": list(expected), "actual": list(actual),
                        "testbench": str(generated[0])})
    if results[0]["actual"] != results[1]["actual"]:
        raise RuntimeError("changing only latency metadata changed generated READ_DELAY/WRITE_DELAY")
    if results[1]["actual"] == results[2]["actual"]:
        raise RuntimeError("changing --mem-delay-read/--mem-delay-write did not change generated stimulus")
    return results


def check_write_delay_clamp(bambu: pathlib.Path, out: pathlib.Path,
                            burst_parameter: str) -> list[dict[str, object]]:
    """Check both generated clamp branches at the minimum valid CLI delay."""
    source = TESTS / "bambu_axi_burst_sum_profile.cpp"
    tb = TESTS / "bambu_axi_burst_tb.cpp"
    case = out / "write-delay-clamp" / "global-fallback-cli-one"
    result = run(case, [str(bambu), str(source), *BASE_ARGS, "--top-fname=kernel",
                        f"--generate-tb={tb}", "--tb-arg=1024", "--tb-param-size=src:16384",
                        "--memory-allocation-policy=NO_BRAM",
                        "--mem-delay-read=2", "--mem-delay-write=2",
                        f"--bambu-parameter={burst_parameter}=1"])
    if result.returncode:
        raise RuntimeError(f"write-delay clamp testbench generation failed, "
                           f"exit={result.returncode}; see {case}")
    generated = list(case.rglob("bambu_testbench.v"))
    if len(generated) != 1:
        raise RuntimeError(f"expected one generated testbench in {case}, found {len(generated)}")
    _, actual_parameter = testbench_delays(generated[0])
    if actual_parameter != 2:
        raise RuntimeError(f"minimum CLI write delay was not passed to the generated interface: "
                           f"got {actual_parameter}; see {generated[0]}")
    results: list[dict[str, object]] = []
    for name, parameter in (("global-fallback-zero", 0), ("explicit-one", 1)):
        effective, fallback = effective_model_write_delay(generated[0], parameter)
        if fallback != 1 or effective < 1:
            raise RuntimeError(f"write-delay clamp failed for {name}: parameter={parameter}, "
                               f"fallback={fallback}, effective model delay={effective}; see {generated[0]}")
        results.append({"case": f"write-delay-clamp/{name}", "parameter": parameter,
                        "fallback": fallback, "effective_model_delay": effective,
                        "testbench": str(generated[0])})
    return results


def bundle(path: pathlib.Path, symbol: str, name: str = "data") -> ET.Element:
    root = ET.parse(path).getroot()
    functions = [node for node in root.findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected function symbol={symbol!r} in {path}, found {len(functions)}")
    matches = [node for node in functions[0].findall("./bundles/bundle") if node.get("name") == name]
    if len(matches) != 1:
        raise RuntimeError(f"expected one bundle {name!r} in {path}, found {len(matches)}")
    return matches[0]


def expected_latency(path: pathlib.Path, symbol: str, value: str | None) -> None:
    attrs = bundle(path, symbol).attrib
    if attrs.get("mode") != "m_axi":
        raise RuntimeError(f"latency metadata fixture did not retain m_axi mode: {attrs}")
    if value is None:
        if "latency" in attrs:
            raise RuntimeError(f"omitted pragma latency was serialized for {symbol}: {attrs}")
    elif attrs.get("latency") != value:
        raise RuntimeError(f"noncanonical pragma serialization for {symbol}: expected latency={value}, got {attrs}")


def write_xml(seed: pathlib.Path, target: pathlib.Path, symbol: str,
              value: str | None, mode: str | None = None,
              bundle_name: str | None = "data", remove_mode: bool = False) -> None:
    tree = ET.parse(seed)
    item = bundle_from_tree(tree, symbol, bundle_name)
    if value is None:
        item.attrib.pop("latency", None)
    else:
        item.set("latency", value)
    if mode is not None:
        item.set("mode", mode)
        if mode != "m_axi":
            item.attrib.pop("max_read_burst_length", None)
    if remove_mode:
        item.attrib.pop("mode", None)
    tree.write(target, encoding="utf-8", xml_declaration=True)


def bundle_from_tree(tree: ET.ElementTree, symbol: str, name: str | None = "data") -> ET.Element:
    functions = [node for node in tree.getroot().findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected function {symbol!r} in XML, found {len(functions)}")
    found = [node for node in functions[0].findall("./bundles/bundle")
             if name is None or node.get("name") == name]
    if len(found) != 1:
        raise RuntimeError(f"expected one bundle {name!r} in function {symbol}, found {len(found)}")
    return found[0]


def set_root_latencies(seed_xml: pathlib.Path, target: pathlib.Path,
                       values: dict[str, str | None]) -> None:
    tree = ET.parse(seed_xml)
    for symbol, value in values.items():
        item = bundle_from_tree(tree, symbol)
        if value is None:
            item.attrib.pop("latency", None)
        else:
            item.set("latency", value)
    tree.write(target, encoding="utf-8", xml_declaration=True)


def invoke_shared_xml(bambu: pathlib.Path, out: pathlib.Path, seed_ir: pathlib.Path,
                      seed_xml: pathlib.Path, tops: str, tag: str,
                      values: dict[str, str | None], burst_parameter: str,
                      expected_failure: str | None = None,
                      allow_success: bool = False) -> subprocess.CompletedProcess[str]:
    case = out / "shared-roots" / f"xml-{tag}"
    case.mkdir(parents=True, exist_ok=False)
    xml = case / "architecture.xml"
    set_root_latencies(seed_xml, xml, values)
    argv = [str(bambu), "--use-raw", str(seed_ir), f"--architecture-xml={xml}",
            # Keep these roots on the same legacy interface ABI: this matrix
            # isolates latency agreement from burst-resource eligibility.
            *BASE_ARGS, f"--top-fname={tops}", f"--bambu-parameter={burst_parameter}=0"]
    record(case, argv)
    result = compiler_support.run_compiler(
        argv, cwd=case, timeout=600, expected_failure=expected_failure,
        sandbox_markers=("Bad system call", "Operation not permitted", "Failed to create stream fd"),
        diagnostic_output_dir=case,
        allow_success=allow_success,
    )
    (case / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def invoke_xml(bambu: pathlib.Path, out: pathlib.Path, seed_ir: pathlib.Path,
               source_xml: pathlib.Path, symbol: str, value: str | None,
               burst_parameter: str, mode: str | None = None,
               tag: str | None = None, bundle_name: str | None = "data",
               remove_mode: bool = False,
               expected_failure: str | None = None) -> subprocess.CompletedProcess[str]:
    case = out / "xml" / (tag or ("omitted" if value is None else re.sub(r"\W+", "_", value).strip("_") or "empty"))
    if mode:
        case = out / "xml" / f"non-axi-{mode}"
    xml = case / "architecture.xml"
    case.mkdir(parents=True, exist_ok=False)
    write_xml(source_xml, xml, symbol, value, mode, bundle_name, remove_mode)
    argv = [str(bambu), "--use-raw", str(seed_ir), f"--architecture-xml={xml}",
            *BASE_ARGS, f"--top-fname={symbol}", f"--bambu-parameter={burst_parameter}=1"]
    record(case, argv)
    result = compiler_support.run_compiler(
        argv, cwd=case, timeout=600, expected_failure=expected_failure,
        sandbox_markers=("Bad system call", "Operation not permitted", "Failed to create stream fd"),
        diagnostic_output_dir=case,
    )
    (case / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def assert_accept(result: subprocess.CompletedProcess[str], case: pathlib.Path) -> None:
    if result.returncode:
        raise RuntimeError(f"expected accepted latency metadata, exit={result.returncode}; see {case}")
    checker_argv = [sys.executable, str(CHECKER), "rtl", "--output", str(case),
                    "--require=MinimalAXI4MasterPipelined", "--configured-contract",
                    "--configure-log", str(case / "stderr.log"),
                    # The case pragma specifies B=16 but omits O/D. The
                    # effective defaults are O=16 and derived D=256.
                    "--expected-burst-max=16", "--expected-outstanding=16", "--expected-fifo-depth=256"]
    checker = subprocess.run(checker_argv, text=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, check=False)
    (case / "contract_checker_argv.json").write_text(json.dumps(checker_argv, indent=2) + "\n", encoding="utf-8")
    (case / "contract_checker.log").write_text(checker.stdout, encoding="utf-8")
    if checker.returncode:
        raise RuntimeError(f"B/O/D/RTL contract check failed; see {case}/contract_checker.log")


def assert_reject(result: subprocess.CompletedProcess[str], case: pathlib.Path) -> None:
    cc_outputs = sorted((case / "panda-temp").glob("__cc_output*"))
    text = "\n".join((result.stdout, result.stderr,
                       *(path.read_text(encoding="utf-8", errors="replace") for path in cc_outputs)))
    if result.returncode == 0 or result.returncode < 0 or result.returncode >= 128:
        raise RuntimeError(f"invalid latency metadata was accepted or crashed (exit={result.returncode}); see {case}")
    if not re.search(r"Invalid AXI interface attribute 'latency' for bundle 'data'", text, re.IGNORECASE):
        raise RuntimeError(f"invalid XML latency failed without an explicit attribute/bundle diagnostic; see {case}")


def is_expected_multitop_backend_guard(result: subprocess.CompletedProcess[str]) -> bool:
    """Recognize only the downstream single-top backend limitation."""
    if result.returncode <= 0 or result.returncode >= 128:
        return False
    return re.search(r"Expected single top function name", result.stdout + "\n" + result.stderr,
                     re.IGNORECASE) is not None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bambu", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--burst-parameter", required=True)
    args = parser.parse_args()
    bambu = args.bambu.resolve(strict=True)
    out = prepare_output(args.output)
    if not bambu.is_file() or not bambu.stat().st_mode & 0o111:
        raise RuntimeError(f"Bambu is not executable: {bambu}")
    bambu_hls = pathlib.Path(os.environ.get("BAMBU_HLS", ""))
    compiler_support.validate_install(
        bambu, bambu_hls, bambu_hls / "lib/panda/clang-16/ASTAnalyzer.so")

    # Frontend pragma cases establish real .bambuir/XML artifacts and check
    # canonical decimal serialization before any direct architecture replay.
    pragma_cases = {
        "latency_omitted": None, "latency_zero": "0", "latency_64": "64",
        "latency_leading_zeroes": "64", "latency_uint32_max": "4294967295",
        "latency_duplicate_same": "64",
    }
    seeds: dict[str, tuple[pathlib.Path, pathlib.Path]] = {}
    results: list[dict[str, object]] = []
    for top, canonical in pragma_cases.items():
        case = out / "pragma" / top
        argv = [str(bambu), str(FIXTURE), *BASE_ARGS, *select_case_defines(top), f"--top-fname={top}",
                f"--bambu-parameter={args.burst_parameter}=1"]
        result = run(case, argv)
        if result.returncode:
            raise RuntimeError(f"pragma case {top} failed, exit={result.returncode}; see {case}")
        xml = architecture(case)
        expected_latency(xml, top, canonical)
        irs = list(case.rglob("*.bambuir"))
        if len(irs) != 1:
            raise RuntimeError(f"pragma seed {top} must emit one .bambuir, found {len(irs)}")
        seeds[top] = (irs[0], xml)
        results.append({"case": f"pragma/{top}", "expected": "accept", "exit_code": result.returncode,
                        "canonical_latency": canonical})

    seed_ir, seed_xml = seeds["latency_omitted"]
    (out / "seed_inputs.json").write_text(json.dumps({
        "source": str(FIXTURE), "seed_ir": str(seed_ir), "seed_xml": str(seed_xml),
        "seed_type": ".bambuir from regular frontend compilation (not debug .raw)",
        "pragma_canonical_values": pragma_cases,
    }, indent=2) + "\n", encoding="utf-8")

    positives = [(None, 0), ("0", 0), ("64", 64), ("00064", 64),
                 ("4294967295", 4294967295)]
    for value, _effective in positives:
        tag = "omitted" if value is None else re.sub(r"\W+", "_", value).strip("_") or "empty"
        case = out / "xml" / tag
        result = invoke_xml(bambu, out, seed_ir, seed_xml, "latency_omitted", value,
                            args.burst_parameter)
        assert_accept(result, case)
        results.append({"case": f"xml/{tag}", "expected": "accept", "exit_code": result.returncode,
                        "effective_latency": 0 if value is None else int(value, 10)})

    for index, invalid in enumerate(("", "+1", "-1", " 1", "1 ", "1x", "4294967296")):
        slug = re.sub(r"\W+", "_", invalid).strip("_") or "empty"
        case_name = f"{index}-{slug}"
        case = out / "xml" / f"invalid-{case_name}"
        result = invoke_xml(bambu, out, seed_ir, seed_xml, "latency_omitted", invalid,
                            args.burst_parameter, tag=f"invalid-{case_name}",
                            expected_failure="Invalid AXI interface attribute 'latency'")
        assert_reject(result, case)
        results.append({"case": f"xml/invalid-{case_name}", "expected": "reject", "exit_code": result.returncode})

    # Compile a true fixed-size C array through the frontend. Its inferred
    # architecture mode must be `array`, establishing a supported non-AXI
    # baseline before testing latency applicability on that same XML path.
    non_axi_symbol = "latency_non_axi_baseline"
    non_axi_seed_case = out / "pragma" / "non-axi-array-baseline"
    non_axi_seed_result = run(non_axi_seed_case, [
        str(bambu), str(FIXTURE), *BASE_ARGS, *select_case_defines(non_axi_symbol),
        f"--top-fname={non_axi_symbol}", f"--bambu-parameter={args.burst_parameter}=1"])
    if non_axi_seed_result.returncode:
        raise RuntimeError(f"supported fixed-array baseline failed; see {non_axi_seed_case}")
    non_axi_seed_xml = architecture(non_axi_seed_case)
    non_axi_bundle = bundle_from_tree(ET.parse(non_axi_seed_xml), non_axi_symbol, "a")
    if non_axi_bundle.get("mode") != "array" or "latency" in non_axi_bundle.attrib:
        raise RuntimeError(f"non-AXI baseline did not infer mode=array: {non_axi_bundle.attrib}")
    non_axi_seed_ir = list(non_axi_seed_case.rglob("*.bambuir"))
    if len(non_axi_seed_ir) != 1:
        raise RuntimeError(f"non-AXI baseline must emit one .bambuir; see {non_axi_seed_case}")
    results.append({"case": "pragma/non-axi-array-baseline", "expected": "accept",
                    "exit_code": non_axi_seed_result.returncode, "mode": "array"})

    # XML is a direct input surface too: reject both numeric values when the
    # bundle is non-AXI, and reject latency when mode is absent.
    for tag, value, remove_mode in (("non-axi-latency-64", "64", False),
                                    ("non-axi-latency-zero", "0", False),
                                    ("missing-mode-latency-zero", "0", True)):
        case = out / "xml" / tag
        result = invoke_xml(bambu, out, non_axi_seed_ir[0], non_axi_seed_xml, non_axi_symbol, value,
                            args.burst_parameter, tag=tag, bundle_name="a", remove_mode=remove_mode,
                            expected_failure="Invalid AXI interface attribute 'latency' for bundle 'a'")
        text = result.stdout + "\n" + result.stderr
        if result.returncode == 0 or result.returncode < 0 or result.returncode >= 128:
            raise RuntimeError(f"non-m_axi/missing-mode XML latency accepted or crashed (exit={result.returncode}); see {case}")
        if not re.search(r"Invalid AXI interface attribute 'latency' for bundle 'a'.*mode=m_axi", text,
                         re.IGNORECASE | re.DOTALL):
            raise RuntimeError(f"non-m_axi/missing-mode XML latency failed without the explicit applicability diagnostic; see {case}")
        results.append({"case": f"xml/{tag}", "expected": "reject", "exit_code": result.returncode})

    # Same-bundle roots must agree independent of the frontend's observed root
    # order. Verify the order from Bambu's own diagnostic rather than assuming
    # --top-fname preserves the requested spelling.
    root_pairs = [
        ("agree-order-a", "latency_shared_default_first,latency_zero", False),
        ("agree-order-b", "latency_shared_default_first,latency_zero", True),
    ]
    observed_orders = []
    root_names = {"latency_shared_default_first", "latency_zero"}
    for tag, tops, reverse_root_order in root_pairs:
        case = out / "shared-roots" / tag
        argv = [str(bambu), str(FIXTURE), *BASE_ARGS,
                *select_case_defines(tops, reverse_root_order), f"--top-fname={tops}",
                # Exercise metadata on the legacy m_axi path, independently of
                # whether either root can use the configured burst ABI.
                f"--bambu-parameter={args.burst_parameter}=0"]
        result = run(case, argv, expected_failure="Expected single top function name", allow_success=True)
        combined = result.stdout + "\n" + result.stderr
        seen = re.findall(r"Analyzing function ([^\s]+)", combined)
        if len(seen) < 2 or set(seen[:2]) != root_names:
            raise RuntimeError(f"{tag} did not analyze both intended roots; observed order was {seen}; see {case}")
        observed_orders.append({"case": tag, "requested": tops,
                                "reverse_declaration_order": reverse_root_order,
                                "observed": seen[:2]})
        if result.returncode != 0:
            if not is_expected_multitop_backend_guard(result):
                raise RuntimeError(f"compatible shared-root default/zero profile failed before the known multi-top backend guard; see {case}")
        results.append({"case": f"shared-roots/{tag}", "expected": "accept",
                        "exit_code": result.returncode,
                        "acceptance_stage": "backend-multitop-guard" if result.returncode else "complete"})
    (out / "shared-root-orders.json").write_text(json.dumps(observed_orders, indent=2) + "\n", encoding="utf-8")
    if len({tuple(item["observed"][:2]) for item in observed_orders}) != 2:
        raise RuntimeError(f"shared-root tests did not exercise both observed orders: {observed_orders}")

    # Re-run shared-bundle metadata through the compiled architecture XML path,
    # not just through pragma parsing. First obtain a compatible .bambuir seed
    # for each observed root order, then vary per-root values in its XML.
    xml_observed_orders = []
    for label, tops, reverse_root_order in (
            ("order-a", "latency_shared_default_first,latency_zero", False),
            ("order-b", "latency_shared_default_first,latency_zero", True)):
        seed_case = out / "shared-roots" / f"xml-seed-{label}"
        seed_result = run(seed_case, [str(bambu), str(FIXTURE), *BASE_ARGS,
                                      *select_case_defines(tops, reverse_root_order),
                                      f"--top-fname={tops}",
                                      # The seed and XML replay must have the
                                      # same legacy ABI so latency is isolated.
                                      f"--bambu-parameter={args.burst_parameter}=0"],
                          expected_failure="Expected single top function name", allow_success=True)
        observed = re.findall(r"Analyzing function ([^\s]+)", seed_result.stdout + "\n" + seed_result.stderr)
        if len(observed) < 2 or set(observed[:2]) != root_names:
            raise RuntimeError(f"shared-root XML seed did not analyze both roots for {label}: saw {observed}; see {seed_case}")
        if seed_result.returncode and not is_expected_multitop_backend_guard(seed_result):
            raise RuntimeError(f"shared-root XML seed failed before the known multi-top backend guard for {label}; see {seed_case}")
        xml_observed_orders.append(tuple(observed[:2]))
        seed_irs = list(seed_case.rglob("*.bambuir"))
        if len(seed_irs) != 1:
            raise RuntimeError(f"shared-root XML seed must emit one .bambuir; see {seed_case}")
        seed_arch = architecture(seed_case)
        default_root = "latency_shared_default_first"
        zero_root = "latency_zero"
        xml_cases = (
            ("agree-default-zero", {default_root: None, zero_root: "0"}, True),
            ("conflict-default-64", {default_root: None, zero_root: "64"}, False),
            ("conflict-explicit-zero-64", {default_root: "0", zero_root: "64"}, False),
        )
        for suffix, values, accepted in xml_cases:
            tag = f"{label}-{suffix}"
            case = out / "shared-roots" / f"xml-{tag}"
            result = invoke_shared_xml(bambu, out, seed_irs[0], seed_arch, tops, tag,
                                       values, args.burst_parameter,
                                       expected_failure=("Expected single top function name" if accepted else
                                                         "Incompatible AXI latency for bundle 'data'"),
                                       allow_success=accepted)
            combined = result.stdout + "\n" + result.stderr
            if accepted and result.returncode:
                if not is_expected_multitop_backend_guard(result):
                    raise RuntimeError(f"shared-root XML default/zero failed before the known multi-top backend guard for {label}; see {case}")
            if not accepted:
                if result.returncode == 0 or result.returncode < 0 or result.returncode >= 128:
                    raise RuntimeError(f"shared-root XML latency conflict accepted or crashed for {label}; see {case}")
                if not re.search(r"Incompatible AXI latency for bundle 'data'", combined, re.IGNORECASE):
                    raise RuntimeError(f"shared-root XML conflict lacked an explicit bundle diagnostic; see {case}")
            results.append({"case": f"shared-roots/xml-{tag}",
                            "expected": "accept" if accepted else "reject",
                            "exit_code": result.returncode,
                            "acceptance_stage": ("backend-multitop-guard" if accepted and result.returncode else
                                                 "complete" if accepted else "latency-conflict"),
                            "observed_seed_order": observed[:2]})
    if len(set(xml_observed_orders)) != 2:
        raise RuntimeError(f"shared-root XML tests did not exercise both observed orders: {xml_observed_orders}")

    # The malformed XML must be parsed and rejected even for a seed whose loop
    # is not eligible for burst conversion (before legacy fallback can win).
    stride_case = out / "stride-seed"
    stride_argv = [str(bambu), str(TESTS / "compiler_cases" / "axi_analysis_cases.cpp"), *BASE_ARGS,
                   "--extra-cc-options=-DAXI_ANALYSIS_CASE_08",
                   "--top-fname=stride_two", f"--bambu-parameter={args.burst_parameter}=1"]
    stride_result = run(stride_case, stride_argv)
    if stride_result.returncode:
        raise RuntimeError(f"could not generate unsupported-stride frontend seed; see {stride_case}")
    stride_ir = list(stride_case.rglob("*.bambuir"))
    if len(stride_ir) != 1:
        raise RuntimeError(f"unsupported-stride seed must emit one .bambuir, found {len(stride_ir)}")
    stride_xml = architecture(stride_case)
    case = out / "xml" / "unsupported-stride-invalid"
    result = invoke_xml(bambu, out, stride_ir[0], stride_xml, "stride_two", "1x",
                        args.burst_parameter, tag="unsupported-stride-invalid",
                        expected_failure="Invalid AXI interface attribute 'latency'")
    assert_reject(result, case)
    results.append({"case": "xml/unsupported-stride-invalid", "expected": "reject",
                    "exit_code": result.returncode})

    # The pragma parser owns interface applicability and duplicate agreement.
    # Execute each top through the normal frontend and record concise evidence.
    pragma_bundle_values: dict[str, str | None] = {
        "latency_shared_default_first": None,
        "latency_shared_zero_first": "0",
        "latency_shared_default_conflict": "64",
        "latency_shared_64_first": "64",
        "latency_non_axi": None,
    }
    for top, expected_value in pragma_bundle_values.items():
        case = out / "pragma" / top
        argv = [str(bambu), str(FIXTURE), *BASE_ARGS, *select_case_defines(top), f"--top-fname={top}",
                f"--bambu-parameter={args.burst_parameter}=1"]
        result = run(case, argv)
        if result.returncode != 0:
            raise RuntimeError(f"compatible/default latency pragma rejected for {top}; see {case}")
        if top == "latency_non_axi":
            attrs = bundle(architecture(case), top, "a").attrib
            if attrs.get("mode") != "array" or "latency" in attrs:
                raise RuntimeError(f"non-AXI pragma latency was not ignored: {attrs}")
        else:
            expected_latency(architecture(case), top, expected_value)
        results.append({"case": f"pragma/{top}", "expected": "accept",
                        "exit_code": result.returncode})

    conflict_top = "latency_shared_explicit_conflict"
    conflict_case = out / "pragma" / conflict_top
    conflict_result = run(conflict_case, [str(bambu), str(FIXTURE), *BASE_ARGS,
                                          *select_case_defines(conflict_top), f"--top-fname={conflict_top}",
                                          f"--bambu-parameter={args.burst_parameter}=1"],
                          expected_failure="Conflicting")
    conflict_text = conflict_result.stdout + "\n" + conflict_result.stderr
    if conflict_result.returncode == 0 or conflict_result.returncode < 0 or conflict_result.returncode >= 128:
        raise RuntimeError(f"conflicting explicit latency values were accepted or crashed; see {conflict_case}")
    if not re.search(r"latency.*Conflicting", conflict_text, re.IGNORECASE):
        raise RuntimeError(f"conflicting explicit latency values lacked a conflict diagnostic; see {conflict_case}")
    results.append({"case": f"pragma/{conflict_top}", "expected": "reject",
                    "exit_code": conflict_result.returncode})

    stimulus_results = check_stimulus_independence(bambu, out, args.burst_parameter)
    results.extend(stimulus_results)
    clamp_results = check_write_delay_clamp(bambu, out, args.burst_parameter)
    results.extend(clamp_results)

    (out / "results.json").write_text(json.dumps({"status": "PASS", "results": results}, indent=2) + "\n",
                                       encoding="utf-8")
    print(f"LATENCY-METADATA PASS: {len(pragma_cases)} canonical pragma cases, "
          f"{len(positives)} direct XML accepts, 11 XML rejects, shared-root order, applicability/duplicates, "
          f"stimulus checks={len(stimulus_results)}, write-delay clamp checks={len(clamp_results)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ET.ParseError) as error:
        print(f"LATENCY-METADATA FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
