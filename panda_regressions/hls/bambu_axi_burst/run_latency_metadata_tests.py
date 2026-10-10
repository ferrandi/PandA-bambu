#!/usr/bin/env python3
"""Compile pragma and architecture-XML AXI latency metadata regression cases."""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import shlex
import subprocess
import sys
import xml.etree.ElementTree as ET


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


def run(case: pathlib.Path, argv: list[str]) -> subprocess.CompletedProcess[str]:
    case.mkdir(parents=True, exist_ok=False)
    record(case, argv)
    result = subprocess.run(argv, cwd=case, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False)
    (case / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def architecture(case: pathlib.Path) -> pathlib.Path:
    files = list(case.rglob("architecture.xml"))
    if len(files) != 1:
        raise RuntimeError(f"expected one architecture.xml under {case}, found {len(files)}")
    return files[0]


def bundle(path: pathlib.Path, symbol: str, name: str = "data") -> ET.Element:
    root = ET.parse(path).getroot()
    functions = [node for node in root.findall("function") if node.get("symbol") == symbol]
    if len(functions) != 1:
        raise RuntimeError(f"expected function symbol={symbol!r} in {path}, found {len(functions)}")
    matches = [node for node in functions[0].findall("./bundles/bundle") if node.get("name") == name]
    if len(matches) != 1:
        raise RuntimeError(f"expected one bundle {name!r} in {path}, found {len(matches)}")
    return matches[0]


def expected_latency(path: pathlib.Path, symbol: str, value: str) -> None:
    attrs = bundle(path, symbol).attrib
    if attrs.get("latency") != value:
        raise RuntimeError(f"noncanonical pragma serialization for {symbol}: expected latency={value}, got {attrs}")
    if attrs.get("mode") != "m_axi":
        raise RuntimeError(f"latency metadata fixture did not retain m_axi mode: {attrs}")


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
                      values: dict[str, str | None], burst_parameter: str) -> subprocess.CompletedProcess[str]:
    case = out / "shared-roots" / f"xml-{tag}"
    case.mkdir(parents=True, exist_ok=False)
    xml = case / "architecture.xml"
    set_root_latencies(seed_xml, xml, values)
    argv = [str(bambu), "--use-raw", str(seed_ir), f"--architecture-xml={xml}",
            # Keep these roots on the same legacy interface ABI: this matrix
            # isolates latency agreement from burst-resource eligibility.
            *BASE_ARGS, f"--top-fname={tops}", f"--bambu-parameter={burst_parameter}=0"]
    record(case, argv)
    result = subprocess.run(argv, cwd=case, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False)
    (case / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (case / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (case / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    return result


def invoke_xml(bambu: pathlib.Path, out: pathlib.Path, seed_ir: pathlib.Path,
               source_xml: pathlib.Path, symbol: str, value: str | None,
               burst_parameter: str, mode: str | None = None,
               tag: str | None = None, bundle_name: str | None = "data",
               remove_mode: bool = False) -> subprocess.CompletedProcess[str]:
    case = out / "xml" / (tag or ("omitted" if value is None else re.sub(r"\W+", "_", value).strip("_") or "empty"))
    if mode:
        case = out / "xml" / f"non-axi-{mode}"
    xml = case / "architecture.xml"
    case.mkdir(parents=True, exist_ok=False)
    write_xml(source_xml, xml, symbol, value, mode, bundle_name, remove_mode)
    argv = [str(bambu), "--use-raw", str(seed_ir), f"--architecture-xml={xml}",
            *BASE_ARGS, f"--top-fname={symbol}", f"--bambu-parameter={burst_parameter}=1"]
    record(case, argv)
    result = subprocess.run(argv, cwd=case, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False)
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
                    # The case pragma omits read_fifo_depth, so the effective
                    # depth is the derived next_pow2(max(2,1)*16) = 32.
                    "--expected-burst-max=16", "--expected-outstanding=1", "--expected-fifo-depth=32"]
    checker = subprocess.run(checker_argv, text=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, check=False)
    (case / "contract_checker_argv.json").write_text(json.dumps(checker_argv, indent=2) + "\n", encoding="utf-8")
    (case / "contract_checker.log").write_text(checker.stdout, encoding="utf-8")
    if checker.returncode:
        raise RuntimeError(f"B/O/D/RTL contract check failed; see {case}/contract_checker.log")


def assert_reject(result: subprocess.CompletedProcess[str], case: pathlib.Path) -> None:
    text = result.stdout + "\n" + result.stderr
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

    # Frontend pragma cases establish real .bambuir/XML artifacts and check
    # canonical decimal serialization before any direct architecture replay.
    pragma_cases = {
        "latency_omitted": "0", "latency_zero": "0", "latency_64": "64",
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
                            args.burst_parameter, tag=f"invalid-{case_name}")
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
    if non_axi_bundle.get("mode") != "array":
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
                            args.burst_parameter, tag=tag, bundle_name="a", remove_mode=remove_mode)
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
        result = run(case, argv)
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
                                      f"--bambu-parameter={args.burst_parameter}=0"])
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
                                       values, args.burst_parameter)
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
    stride_argv = [str(bambu), str(TESTS / "compiler_cases" / "unsupported_stride.cpp"), *BASE_ARGS,
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
                        args.burst_parameter, tag="unsupported-stride-invalid")
    assert_reject(result, case)
    results.append({"case": "xml/unsupported-stride-invalid", "expected": "reject",
                    "exit_code": result.returncode})

    # The pragma parser owns interface applicability and duplicate agreement.
    # Execute each top through the normal frontend and record concise evidence.
    for top, accepted in (("latency_shared_default_first", True),
                          ("latency_shared_zero_first", True),
                          ("latency_shared_default_conflict", False),
                          ("latency_shared_explicit_conflict", False),
                          ("latency_non_axi", False)):
        case = out / "pragma" / top
        argv = [str(bambu), str(FIXTURE), *BASE_ARGS, *select_case_defines(top), f"--top-fname={top}",
                f"--bambu-parameter={args.burst_parameter}=1"]
        result = run(case, argv)
        combined = result.stdout + "\n" + result.stderr
        if accepted and result.returncode != 0:
            raise RuntimeError(f"compatible/default latency pragma rejected for {top}; see {case}")
        if not accepted:
            if result.returncode == 0 or result.returncode < 0 or result.returncode >= 128:
                raise RuntimeError(f"invalid/conflicting latency pragma accepted or crashed for {top}; see {case}")
            if not re.search(r"latency.*(only valid|Conflicting)", combined, re.IGNORECASE):
                raise RuntimeError(f"{top} rejected without an explicit latency diagnostic; see {case}")
        results.append({"case": f"pragma/{top}", "expected": "accept" if accepted else "reject",
                        "exit_code": result.returncode})

    (out / "results.json").write_text(json.dumps({"status": "PASS", "results": results}, indent=2) + "\n",
                                       encoding="utf-8")
    print(f"LATENCY-METADATA PASS: {len(pragma_cases)} canonical pragma cases, "
          f"{len(positives)} direct XML accepts, 11 XML rejects, shared-root order, applicability/duplicates")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ET.ParseError) as error:
        print(f"LATENCY-METADATA FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
