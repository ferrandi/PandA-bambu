#!/usr/bin/env python3
"""Extract the unchanged T4B hardware cells from PandA's XML libraries."""

from __future__ import annotations

import argparse
import hashlib
import html
import importlib.util
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path


def find_repo_root(start: Path) -> Path:
    """Find the checkout containing this script without assuming its depth."""
    for candidate in (start.resolve(), *start.resolve().parents):
        if (candidate / ".git").exists() and (candidate / "etc/libtech/NC_AXI_IPs.xml").is_file():
            return candidate
    raise RuntimeError(f"could not find repository root above {start}")


ROOT = find_repo_root(Path(__file__))
DEFAULT_AXI = ROOT / "etc/libtech/NC_AXI_IPs.xml"
DEFAULT_TB = ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml"

MEM_WIDTHS = {
    "clock": 1, "reset": 1,
    "arid": 1, "araddr": 32, "arlen": 8, "arsize": 3, "arburst": 2,
    "arlock": 1, "arcache": 4, "arprot": 3, "arqos": 4, "arregion": 4,
    "aruser": 1, "arvalid": 1, "arready": 1,
    "rid": 1, "rdata": 32, "rresp": 2, "ruser": 2, "rlast": 1,
    "rvalid": 1, "rready": 1,
    "awid": 1, "awaddr": 32, "awlen": 8, "awsize": 3, "awburst": 2,
    "awlock": 1, "awcache": 4, "awprot": 3, "awqos": 4, "awregion": 4,
    "awuser": 1, "awvalid": 1, "awready": 1,
    "wdata": 32, "wstrb": 4, "wlast": 1, "wvalid": 1, "wready": 1,
    "wuser": 1, "bid": 1, "bresp": 2, "buser": 2, "bvalid": 1, "bready": 1,
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_cell(xml_path: Path, target: str) -> tuple[ET.Element, str]:
    root = ET.parse(xml_path).getroot()
    cells = [cell for cell in root.findall(".//cell") if cell.findtext("name") == target]
    if len(cells) != 1:
        raise ValueError(f"expected exactly one XML cell named {target}, found {len(cells)}")
    cell = cells[0]
    module = cell.find("circuit/module_o")
    if module is None or module.get("id") != target:
        raise ValueError(f"unsupported XML structure for {target}: missing matching module_o")
    functionality = module.findall("NP_functionality")
    if len(functionality) != 1:
        raise ValueError(f"expected one NP_functionality for {target}, found {len(functionality)}")

    raw_xml = xml_path.read_text(encoding="utf-8")
    token = f"<name>{target}</name>"
    at = raw_xml.find(token)
    start = raw_xml.rfind("<cell", 0, at)
    end = raw_xml.find("</cell>", at)
    if at < 0 or start < 0 or end < 0:
        raise ValueError(f"could not locate raw XML cell source for {target}")
    match = re.search(r'\bVERILOG_PROVIDED="([^"]*)"', raw_xml[start:end], re.DOTALL)
    if match is None:
        raise ValueError(f"could not recover VERILOG_PROVIDED for {target}")
    body = html.unescape(match.group(1)).replace("\r\n", "\n")
    parsed = functionality[0].get("VERILOG_PROVIDED") or ""
    if " ".join(body.split()) != " ".join(parsed.split()):
        raise ValueError(f"raw and parsed VERILOG_PROVIDED content disagree for {target}")
    if not body.strip():
        raise ValueError(f"{target} has an empty HDL implementation")
    # These are Bambu's standard reset substitutions used by the read-core runner.
    body = body.replace("1RESET_EDGE", "").replace("1RESET_VALUE", "reset == 1'b0")
    if re.search(r"1[A-Z][A-Z0-9_]*", body):
        raise ValueError(f"unresolved HDL placeholder in {target}")
    return module, body


def module_text(module: ET.Element, body: str, *, memory: bool) -> str:
    target = module.get("id")
    params: list[tuple[str, str]] = []
    for parameter in module.findall("parameter"):
        name = parameter.get("name", "")
        value = (parameter.text or "").strip()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) or not re.fullmatch(r"[0-9]+", value):
            raise ValueError(f"unsupported parameter descriptor in {target}: {ET.tostring(parameter, encoding='unicode')}")
        params.append((name, value))

    ports: list[str] = []
    port_widths: list[tuple[str, int]] = []
    for port in module.findall("port_o"):
        name, direction = port.get("id", ""), port.get("dir", "")
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) or direction not in ("IN", "OUT"):
            raise ValueError(f"unsupported port descriptor in {target}: {ET.tostring(port, encoding='unicode')}")
        if memory:
            if name not in MEM_WIDTHS:
                raise ValueError(f"no reviewed AXI width for TestbenchMEMAXI port {name}")
            width = MEM_WIDTHS[name]
        else:
            descriptor = port.find("structural_type_descriptor")
            if descriptor is None:
                raise ValueError(f"missing type descriptor for {target}.{name}")
            if descriptor.get("type") == "BOOL" and descriptor.get("size") == "1":
                width = 1
            elif descriptor.get("type") == "VECTOR_BOOL" and descriptor.get("size") == "1":
                width = int(descriptor.get("vector_size", "0"))
                if width < 1:
                    raise ValueError(f"invalid vector width for {target}.{name}")
            else:
                raise ValueError(f"unsupported type for {target}.{name}")
        packed = "" if width == 1 else f" [{width - 1}:0]"
        ports.append(f"    {'input' if direction == 'IN' else 'output'} wire{packed} {name}")
        port_widths.append((name, width))
    if not ports or not params:
        raise ValueError(f"missing parameters or ports for {target}")

    param_text = ",\n".join(f"    parameter integer {name} = {value}" for name, value in params)
    localparams = "\n".join(
        f"localparam integer BITSIZE_{name} = {width};" for name, width in port_widths
    ) + "\n\n"
    type_declarations = ""
    if memory:
        # ptr_t is otherwise only provided at compilation-unit scope by the
        # test utility file, whose compile order may be after this module.
        # Keep this module self-contained and independent of compilation-unit
        # visibility rules (the harness contract uses 32-bit addresses).
        type_declarations = "typedef int unsigned ptr_t;\n\n"
    return f"// Extracted from {target}; body from XML with documented Questa compatibility rewrites.\nmodule {target} #(\n{param_text}\n) (\n" + ",\n".join(ports) + f"\n);\n{type_declarations}{localparams}{body}\nendmodule\n"


def normalize_questa_body(target: str, body: str) -> str:
    """Apply the two narrow Questa-compatible rewrites needed by this harness."""
    if target == "MinimalAXI4MasterPipelined":
        # Questa rejects the continuous assignment's implicit forward net and
        # then reports the explicit declaration below as a duplicate. Keep the
        # XML declarations and logic intact; place this assignment after the
        # declarations it references instead.
        assignments = list(re.finditer(
            r"(?m)^[ \t]*assign burst_outstanding = \(descriptor_count(?:_reg)? != 0\);\n", body))
        declarations = list(re.finditer(r"(?m)^[ \t]*wire burst_outstanding;\n", body))
        if len(assignments) != 1 or len(declarations) != 1:
            raise ValueError("unexpected burst_outstanding declaration/assignment in AXI cell")
        assignment = assignments[0].group()
        declaration = declarations[0].group()
        body = body.replace(assignment, "", 1)
        body = body.replace(declaration, declaration + assignment, 1)
    elif target == "TestbenchMEMAXI":
        # These two locals are not re-entrant or task/function variables. The
        # static block-local form is supported by Questa's parser, while its
        # `automatic ptr_t name;` form is rejected as a missing `var` keyword.
        body, count = re.subn(r"\bautomatic ptr_t (currAddr|endAddr);", r"ptr_t \1;", body)
        if count not in (0, 4):
            raise ValueError(f"expected four TestbenchMEMAXI local pointer declarations, found {count}")
    return body


def extract(axi_xml: Path, testbench_xml: Path, outdir: Path) -> dict[str, object]:
    outdir.mkdir(parents=True, exist_ok=True)
    engine_module, engine_body = find_cell(axi_xml, "MinimalAXI4MasterPipelined")
    mem_module, mem_body = find_cell(testbench_xml, "TestbenchMEMAXI")
    outputs = {
        "MinimalAXI4MasterPipelined.sv": module_text(
            engine_module, normalize_questa_body("MinimalAXI4MasterPipelined", engine_body), memory=False
        ),
        "TestbenchMEMAXI.sv": module_text(
            mem_module, normalize_questa_body("TestbenchMEMAXI", mem_body), memory=True
        ),
    }
    dependencies = (engine_module.find("NP_functionality").get("IP_COMPONENT") or "").split(",")
    if any(dependency.strip() for dependency in dependencies):
        extractor_path = Path(__file__).resolve().parents[1] / "rtl_tests/extract_write_engine.py"
        spec = importlib.util.spec_from_file_location("axi_dependency_extractor", extractor_path)
        extractor = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(extractor)
        prefix = "".join(extractor.extract(axi_xml, dependency.strip())
                         for dependency in dict.fromkeys(dependencies) if dependency.strip())
        outputs["MinimalAXI4MasterPipelined.sv"] = prefix + outputs["MinimalAXI4MasterPipelined.sv"]
    source_hashes = {str(path): sha256(path) for path in (axi_xml, testbench_xml)}
    output_hashes: dict[str, str] = {}
    for filename, text in outputs.items():
        path = outdir / filename
        path.write_text(text, encoding="utf-8")
        output_hashes[filename] = sha256(path)
    manifest = {
        "sources": source_hashes,
        "extracted_hdl": output_hashes,
        "reset_substitution": "1RESET_EDGE -> empty; 1RESET_VALUE -> (reset == 1'b0)",
        "memory_width_contract": MEM_WIDTHS,
    }
    (outdir / "extract_manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--axi-xml", type=Path, default=DEFAULT_AXI)
    parser.add_argument("--testbench-xml", type=Path, default=DEFAULT_TB)
    parser.add_argument("--outdir", type=Path, required=True)
    args = parser.parse_args()
    try:
        manifest = extract(args.axi_xml, args.testbench_xml, args.outdir)
    except (OSError, ET.ParseError, ValueError) as error:
        parser.error(str(error))
    for name, digest in manifest["sources"].items():
        print(f"source_sha256 {digest} {name}")
    for name, digest in manifest["extracted_hdl"].items():
        print(f"hdl_sha256 {digest} {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
