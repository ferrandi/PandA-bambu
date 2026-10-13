# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Assert vector-mask metadata and instantiated RTL after HLS, before cleanup.

Contracts cover the Clang 6/16 regression matrix. Library definitions, comments
and unreachable modules do not count as instances of the operations under test.
"""

import argparse
import pathlib
import re
import shlex
import sys


def comparator(operation, lanes, width=32, signed=False):
    return requirement(("" if signed else "ui_") + "vec_" + operation + "_node_FU",
                       BITSIZE_in1=width, NUM_ELEM_in1=lanes,
                       BITSIZE_in2=width, NUM_ELEM_in2=lanes,
                       BITSIZE_out1=1, NUM_ELEM_out1=lanes)


def requirement(name, **parameters):
    return {"name": name, "parameters": parameters}


def extract(lanes):
    return requirement("ui_extractelement_node_FU", BITSIZE_in1=1,
                       NUM_ELEM_in1=lanes, BITSIZE_out1=1)


def contract(case, compiler):
    if compiler not in ("I386_CLANG6", "I386_CLANG16"):
        raise ValueError("Unsupported compiler contract: " + compiler)
    modern = compiler == "I386_CLANG16"
    select = requirement("ui_vec_single_select_node_FU", BITSIZE_out1=32, NUM_ELEM_out1=2)
    shuffle = requirement("ui_shufflevector_node_FU", BITSIZE_in1=1, NUM_ELEM_in1=2,
                          BITSIZE_out1=1, NUM_ELEM_out1=2)
    if case == "ac_channel_nb_vector":
        # Clang 6 lowers the original scalar comparisons without a vector mask.
        return {"top": "nb_vector", "masks": [2] if modern else [],
                "vectors": [(2, 32)],
                "required": [select] + ([comparator("eq", 2), extract(2)] if modern else [])}
    match = re.fullmatch(r"vector_mask_(eq|ne|lt|gt|le|ge)_(unsigned|signed)", case)
    if match:
        operation, sign = match.groups()
        emitted = {"ne": "eq", "le": "gt", "ge": "lt"}.get(operation, operation) if modern else operation
        comparison = comparator(emitted, 2, signed=sign == "signed" and operation not in ("eq", "ne"))
        required = [comparison, shuffle if modern else extract(2)]
        if operation == "eq":
            required.append(requirement("ui_vec_and_node_FU", BITSIZE_in1=1, NUM_ELEM_in1=2,
                                        BITSIZE_in2=1, NUM_ELEM_in2=2, BITSIZE_out1=1, NUM_ELEM_out1=2)
                            if modern else select)
        elif modern:
            required.append(select)
        return {"top": "vector_mask_compare", "masks": [2], "required": required}

    top = case.replace("vector_mask_", "mask_", 1)
    if case == "vector_mask_qor":
        return {"top": "eq16", "masks": [16], "vectors": [(16, 32)],
                "required": [comparator("eq", 16),
                             requirement("ui_bitcast_node_FU", BITSIZE_in1=16, BITSIZE_out1=16)]}
    match = re.fullmatch(r"mask_lanes(2|4|16)", top)
    if match:
        lanes = int(match.group(1))
        return {"top": top, "masks": [lanes], "required": [comparator("ne", lanes), extract(lanes),
                requirement("ui_insertelement_node_FU", BITSIZE_out1=32, NUM_ELEM_out1=lanes)]}
    widths = {"mask_eq8": (8, 8), "mask_eq16": (16, 32), "mask_extract16": (16, 32),
              "mask_zext2": (2, 32), "mask_sext2": (2, 32), "mask_store_load4": (4, 32),
              "mask_store_load16": (16, 32)}
    if top in widths:
        lanes, width = widths[top]
        required = [comparator("eq", lanes, width)]
        result = {"top": top, "masks": [lanes], "required": required}
        if top == "mask_extract16":
            required.append(extract(lanes))
        if top in ("mask_zext2", "mask_sext2"):
            required.append(requirement("UUvector_converter_FU" if top == "mask_zext2" else "IIvector_converter_FU",
                                        BITSIZE_in1=1, NUM_ELEM_in1=2, BITSIZE_out1=32, NUM_ELEM_out1=2))
        if top in ("mask_eq8", "mask_eq16", "mask_store_load4", "mask_store_load16"):
            required.append(requirement("ui_bitcast_node_FU", BITSIZE_in1=lanes, BITSIZE_out1=lanes))
        if top in ("mask_store_load4", "mask_store_load16"):
            result["memory"] = True
        return result
    if top == "mask_const4":
        return {"top": top, "masks": [4], "constant": ("ui_vec_xor_node_FU", "in2", 4, 11),
                "required": [requirement("ui_vec_xor_node_FU", BITSIZE_in1=1, NUM_ELEM_in1=4,
                                         BITSIZE_in2=1, NUM_ELEM_in2=4, BITSIZE_out1=1, NUM_ELEM_out1=4),
                             requirement("ui_bitcast_node_FU", BITSIZE_in1=4, BITSIZE_out1=4)]}
    raise ValueError("Unknown HLS contract: " + case)


def integer(value):
    value = value.strip().replace("_", "")
    if re.fullmatch(r"\d+", value):
        return int(value)
    literal = re.fullmatch(r"(\d+)'[sS]?([bBdDhH])([0-9a-fA-F]+)", value)
    if literal:
        return int(literal.group(3), {"b": 2, "d": 10, "h": 16}[literal.group(2).lower()])
    return None


def parse_modules(text):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.DOTALL)
    modules = {}
    for match in re.finditer(r"\bmodule\s+(\w+)\b(.*?)\bendmodule\b", text, re.DOTALL):
        header, body = match.group(2).split(";", 1)
        defaults = dict(re.findall(r"\bparameter\s+(\w+)\s*=\s*([^,\n)]+)", header))
        instances = []
        pattern = r"\b(\w+)\s+(?:#\s*\((.*?)\)\s*)?(\w+)\s*\((.*?)\)\s*;"
        for item in re.finditer(pattern, body, re.DOTALL):
            parameters = dict(re.findall(r"\.(\w+)\s*\(([^()]*)\)", item.group(2) or ""))
            ports = dict(re.findall(r"\.(\w+)\s*\(([^()]*)\)", item.group(4)))
            instances.append({"kind": item.group(1), "name": item.group(3),
                              "parameters": parameters, "ports": ports, "owner": match.group(1)})
        modules[match.group(1)] = {"body": body, "defaults": defaults, "instances": instances}
    return modules


def reachable_instances(modules, top):
    if top not in modules:
        raise ValueError("Missing RTL top: " + top)
    pending, visited, instances = [top], set(), []
    while pending:
        name = pending.pop()
        if name in visited:
            continue
        visited.add(name)
        for item in modules[name]["instances"]:
            instances.append(item)
            if item["kind"] in modules:
                pending.append(item["kind"])
    return instances


def check_rtl(modules, spec):
    instances = reachable_instances(modules, spec["top"])
    errors = []
    for required in spec["required"]:
        candidates = [item for item in instances if item["kind"] == required["name"]]
        if not candidates:
            errors.append("missing instantiated FU " + required["name"])
            continue
        values = []
        for item in candidates:
            parameters = dict(modules.get(item["kind"], {}).get("defaults", {}))
            parameters.update(item["parameters"])
            values.append({key: integer(parameters.get(key, "")) for key in required["parameters"]})
        if required["parameters"] not in values:
            errors.append("FU {} dimensions: expected {}, got {}".format(required["name"], required["parameters"], values))
    if spec.get("memory") and not any(item["kind"].startswith("ARRAY_") for item in instances):
        errors.append("missing instantiated memory for the store/load round trip")
    if spec.get("constant"):
        kind, port, width, expected = spec["constant"]
        found = False
        for item in instances:
            if item["kind"] != kind:
                continue
            wire = item["ports"].get(port, "").strip()
            assignments = re.findall(r"\bassign\s+" + re.escape(wire) + r"\s*=\s*([^;]+);",
                                     modules[item["owner"]]["body"]) if wire else []
            found |= any(re.match(r"\s*" + str(width) + r"'", value) is not None and integer(value) == expected
                         for value in assignments)
        if not found:
            errors.append("missing packed {}-bit constant {} connected to {}.{}".format(width, expected, kind, port))
    if errors:
        raise ValueError("; ".join(errors))
    return sorted({item["kind"] for item in instances if any(
        required["name"] == item["kind"] for required in spec["required"])})


def check_bambuir(text, spec):
    nodes = {int(number): (kind, body) for number, kind, body in re.findall(
        r"^@(\d+)\s+(\w+)\s+(.*?)(?=^@\d+|\Z)", text, re.MULTILINE | re.DOTALL)}
    roots = []
    for number, (kind, body) in nodes.items():
        if kind == "function_val_node":
            name = re.search(r"\bname:\s*@(\d+)", body)
            identifier = nodes.get(int(name.group(1)), ("", ""))[1] if name else ""
            if re.search(r'\bstrg:\s*"' + re.escape(spec["top"]) + r'"', identifier):
                roots.append(number)
    if not roots:
        raise ValueError("Missing Bambu IR top: " + spec["top"])
    pending, visited, vectors = roots[:], set(), set()
    while pending:
        number = pending.pop()
        if number in visited or number not in nodes:
            continue
        visited.add(number)
        kind, body = nodes[number]
        if kind == "vector_ty_node":
            count = re.search(r"\bnelements:\s*(\d+)", body)
            if not count or int(count.group(1)) == 0:
                raise ValueError("Vector @{} missing nonzero nelements".format(number))
            element = re.search(r"\belts:\s*@(\d+)", body)
            element_body = nodes.get(int(element.group(1)), ("", ""))[1] if element else ""
            width = re.search(r"\bbitsize:\s*(\d+)", element_body)
            if width:
                vectors.add((int(count.group(1)), int(width.group(1))))
        body = re.sub(r'"(?:\\.|[^"\\])*"|\bparent:\s*@\d+', "", body)
        pending.extend(int(ref) for ref in re.findall(r"@(\d+)", body))
    expected = set(spec.get("vectors", [])) | {(lanes, 1) for lanes in spec["masks"]}
    if not expected <= vectors:
        raise ValueError("Missing reachable vector metadata {}: got {}".format(sorted(expected - vectors), sorted(vectors)))
    return sorted(vectors)


def effective_option(commands, name):
    result = None
    for index, value in enumerate(commands):
        if value == name and index + 1 < len(commands):
            result = commands[index + 1]
        elif value.startswith(name + "="):
            result = value.split("=", 1)[1]
    return result


def check_directory(directory, case, compiler):
    spec = contract(case, compiler)
    modules = {}
    for path in sorted(directory.glob("*.v")):
        modules.update(parse_modules(path.read_text()))
    fus = check_rtl(modules, spec)
    ir_files = list((directory / "panda-temp").glob("*.bambuir"))
    if len(ir_files) != 1:
        raise ValueError("Expected one plugin .bambuir, got {} (use --no-clean)".format(len(ir_files)))
    vectors = check_bambuir(ir_files[0].read_text(), spec)
    return "PASS HLS_STRUCTURE {} {}: vectors={} FUs={}".format(case, compiler, vectors, ",".join(fus))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument("--case")
    parser.add_argument("--compiler")
    args = parser.parse_args()
    try:
        compiler = args.compiler
        if not compiler:
            command = (args.directory / "execution.log").read_text().splitlines()[0]
            compiler = effective_option(shlex.split(command), "--compiler")
        print(check_directory(args.directory, args.case or args.directory.name, compiler))
    except (OSError, ValueError, IndexError) as error:
        print("FAIL HLS_STRUCTURE: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
