import re
import os
import argparse
from collections import defaultdict

# Parameter values
parameters = {
    "scheduling_algorithm": ["LIST_BASED_SCHEDULING", "SDC_SCHEDULING"],
    "fu_binding_algorithm": ["CDFC_MODULE_BINDING", "UNIQUE_MODULE_BINDING"],
    "module_binding_algorithm": [
        "TS_WEIGHTED_CLIQUE_COVERING",
        "TTT_CLIQUE_COVERING_FAST",
        "TTT_CLIQUE_COVERING_FAST2",
        "TTT_CLIQUE_COVERING",
        "TTT_CLIQUE_COVERING2",
        "TS_CLIQUE_COVERING",
        "COLORING",
        "WEIGHTED_COLORING",
        "BIPARTITE_MATCHING",
    ],
    "fsm_algorithm": ["BUILD_FMS"],
    "liveness_algorithm": ["FSM_NI_SSA_LIVENESS"],
    "register_allocation_algorithm": [
        "WEIGHTED_CLIQUE_REGISTER_BINDING",
        "COLORING_REGISTER_BINDING",
        "CHORDAL_COLORING_REGISTER_BINDING",
        "UNIQUE_REGISTER_BINDING",
    ],
    "weighted_clique_register_algorithm": [
        "TS_WEIGHTED_CLIQUE_COVERING",
        "WEIGHTED_COLORING",
        "BIPARTITE_MATCHING",
        "TTT_CLIQUE_COVERING",
    ],
    "storage_value_insertion_algorithm": ["VALUES_SCHEME_STORAGE_VALUE_INSERTION"],
    "function_allocation_algorithm": ["DOMINATOR_FUNCTION_ALLOCATION"],
    "memory_allocation_algorithm": ["DOMINATOR_MEMORY_ALLOCATION"],
    "datapath_interconnection_algorithm": ["MUX_INTERCONNECTION_BINDING"],
    "interface_type": [
        "MINIMAL_INTERFACE_GENERATION",
        "INFERRED_INTERFACE_GENERATION",
        "WB4_INTERFACE_GENERATION",
    ],
    "datapath_architecture": ["CLASSIC_DATAPATH_CREATOR"],
    "hls_flow": ["STANDARD_HLS_FLOW"],
    "chaining_algorithm": ["SCHED_CHAINING"],
    "controller_type": ["FSM_CONTROLLER_CREATOR", "PIPELINE_CONTROLLER_CREATOR"],
}

# Base classes that are also valid steps
base_steps = {
    "minimal_interface": "MINIMAL_INTERFACE_GENERATION",
    "top_entity": "TOP_ENTITY_CREATION",
    "allocation": "ALLOCATION",
    "fun_dominator_allocation": "DOMINATOR_FUNCTION_ALLOCATION",
}

# Filenames to skip during parsing
skip_files = [
    "allocation_information.cpp",
    "application_frontend_flow_step.cpp",
    "frontend_flow_step_factory.cpp",
    "frontend_flow_step.cpp",
    "function_frontend_flow_step.cpp",
    "hls_flow_step_factory.cpp",
    "hls_function_step.cpp",
    "hls_step.cpp",
    "symbolic_application_frontend_flow_step.cpp",
    "Bit_Value_backward.cpp",
    "Bit_Value_forward.cpp",
    "IR_lowering_exec.cpp",
    "reg_binding_cs.cpp",
    "reg_binding.cpp",
    "generic_obj.cpp",
    "mux_obj.cpp",
]

# Constants for relationship types and colors
RELATIONSHIP_COLORS = {
    "DEPENDENCE": "black",
    "PRECEDENCE": "purple",
    "INVALIDATION": "red",
    "DYNAMIC": "black",
}

# Constants for step types and colors
STEP_COLORS = {
    "FrontendApplication": "orange",
    "FrontendFunction": "yellow",
    "HLSApplication": "blue",
    "HLSFunction": "green",
    "Technology": "grey",
    "Unknown": "white",
}


def ReplaceParameter(node):
    if "->" in node:
        for parm, values in parameters.items():
            if parm in node:
                return values
    return [node]


def ExtractPassNameInfo(file_content):
    """
    Extract class name, pass name, and pass type from pass implementation file
    """
    match = re.search(
        r"^(\w+)::[^\}]*?ApplicationFrontendFlowStep\(.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
        file_content,
        re.DOTALL | re.MULTILINE,
    )
    if match:
        return match.group(1).strip(), match.group(3).strip(), "FrontendApplication"
    match = re.search(
        r"^(\w+)::[^\}]*?FunctionFrontendFlowStep\(.*?,\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
        file_content,
        re.DOTALL | re.MULTILINE,
    )
    if match:
        return match.group(1).strip(), match.group(3).strip(), "FrontendFunction"
    match = re.search(
        r"^(\w+)::[^\}]*?HLS_step\(.*?,\s*.*?,\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
        file_content,
        re.DOTALL | re.MULTILINE,
    )
    if match:
        return match.group(1).strip(), match.group(3).strip(), "HLSApplication"
    match = re.search(
        r"^(\w+)::[^\}]*?HLSFunctionStep\(.*?,\s*.*?,\s*.*?,\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
        file_content,
        re.DOTALL | re.MULTILINE,
    )
    if match:
        return match.group(1).strip(), match.group(3).strip(), "HLSFunction"
    match = re.search(
        r"^(\w+)::[^\}]*?TechnologyFlowStep\(.*?,\s*.*?,\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
        file_content,
        re.DOTALL | re.MULTILINE,
    )
    if match:
        return match.group(1).strip(), match.group(3).strip(), "Technology"
    return None, None, None


def ExtractPassRelationshipsInfo(func_bodies):
    """
    Extract relatinoships information from pass implementation file
    """
    pattern = re.compile(r"(\s*)case\((\w+)_RELATIONSHIP\):\s*{(.*?)(\1)}", re.DOTALL)
    relationships = {key: [] for key in RELATIONSHIP_COLORS.keys()}

    for fbody in func_bodies:
        matches = pattern.findall(fbody)
        for _indent, relationship_type, body, _indent2 in matches:
            if relationship_type in relationships:
                for _code_line in body.split(";"):
                    code_line = _code_line.strip()
                    if len(code_line) <= 16:
                        continue
                    for reg_exp, rtype in [
                        (
                            re.compile(
                                r"insert\(\s*std::make_pair\(\s*(\w+::)?(\w+),\s*(\w+::)?(\w+)\)\s*\)",
                                re.DOTALL,
                            ),
                            None,
                        ),
                        (
                            re.compile(
                                r"insert\(\s*std::make_tuple\(\s*(\w+::)?(.*?),\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))",
                                re.DOTALL,
                            ),
                            None,
                        ),
                        (
                            re.compile(
                                r"ComputeSignature\(\s*(\w+::)?(\w+)", re.DOTALL
                            ),
                            "DYNAMIC",
                        ),
                        (
                            re.compile(r"insert\(\s*(\w+::)?(\w+)\)", re.DOTALL),
                            "TECHNOLOGY",
                        ),
                    ]:
                        match = reg_exp.search(code_line)
                        if match:
                            for v in ReplaceParameter(match.group(2)):
                                relationships[relationship_type].append(
                                    (v, rtype if rtype else match.group(4))
                                )
                            break
                        # print(f'  no match: {code_line}')
            else:
                print(
                    f"  Warning: Unrecognized relationship type '{relationship_type}' in the code."
                )

    return relationships


base_classes = defaultdict(tuple)


def ExtractPassInfo(file_path):
    """
    Extract information about a pass from pass implementation file
    """
    print(f"Analyzing file: {file_path}")
    with open(file_path, "r") as file:
        content = file.read()

    rels = ExtractPassRelationshipsInfo([])

    # Extract the pass name
    class_name, pass_name, pass_type = ExtractPassNameInfo(content)
    if not pass_name:
        # match = re.search(r": (\w+)\(.*?,\s*.*?,\s*.*?,\s*.*?,\s*(\w+::)?(.*?)\s*(?:,|\))", content, re.DOTALL)
        match = re.search(
            r"^(\w+)::[^\}]*?: (\w+)\((?:[^,)]*,\s*)*(?:\w+::)?([A-Z][A-Z0-9_]*)\s*(?:,|\))",
            content,
            re.DOTALL | re.MULTILINE,
        )
        if not match:
            print(f"  Could not determine the pass name in file: {file_path}")
            return None, None, {}
        class_name = match.group(1).strip()
        base_class = match.group(2).strip()
        pass_name = match.group(3).strip()
        print(f"  STEP NAME: {pass_name} (inherit from {base_class})")
        if base_class in base_classes:
            pass_type, rels = base_classes[base_class]
        else:
            base_filename = os.path.dirname(file_path) + f"/{base_class}.cpp"
            if os.path.exists(base_filename):
                ExtractPassInfo(base_filename)
            if base_class not in base_classes:
                pass_type = "Unknown"
                print(f"  Could not retrieve base pass in file: {file_path}")
            else:
                pass_type, rels = base_classes[base_class]
        # class_name = os.path.splitext(os.path.basename(file_path))[0]
        # match = re.search(f"^{class_name}::{class_name}\(", content, re.MULTILINE)
        # if not match:
        #     print(f'  Colud not determine class name in file: {file_path}')
        #     return pass_name, pass_type, rels

    method_pattern = re.compile(
        class_name + r"::Compute\w*Relationships\(.*?\)[^{]*^{(.*?)^}",
        re.DOTALL | re.MULTILINE,
    )

    if not pass_name[0].isupper():
        print(f"  STEP NAME : {pass_name}")
        print(f"  BASE STEP: {class_name}")
        matches = method_pattern.findall(content)
        if matches:
            rels = ExtractPassRelationshipsInfo(matches)
        else:
            print(f"  No relationships found in file: {file_path}")
        base_classes[class_name] = (pass_type, rels)
        return None, None, {}

    print(f"  CLASS NAME: {class_name}")
    print(f"  STEP NAME : {pass_name}")

    # Extract relationships
    matches = method_pattern.findall(content)
    if matches:
        class_rels = ExtractPassRelationshipsInfo(matches)
        for rtype, crels in class_rels.items():
            rels[rtype].extend(crels)
    else:
        print(f"  No relationships found in file: {file_path}")

    return pass_name, pass_type, rels


def transitive_reduction(relationships):
    """
    Perform transitive reduction of pass flow graph
    """
    reduction = {
        node: set(
            [
                edge
                for rtype, trels in rels.items()
                for edge, _attr in trels
                if rtype != "INVALIDATION"
            ]
        )
        for node, _ptype, rels in relationships
    }
    for node, _ptype, rels in relationships:
        for tgt in list(reduction[node]):
            reduction[node].remove(tgt)

            # Check for alternative paths
            visited = set()
            stack = [node]
            found = False
            while stack:
                current = stack.pop()
                if current == tgt:
                    found = True
                    break
                if current not in visited and current in reduction:
                    visited.add(current)
                    stack.extend(reduction[current])

            # Restore the edge if no alternative path exists
            if not found:
                reduction[node].add(tgt)
        for rtype, trels in rels.items():
            if rtype == "INVALIDATION":
                continue
            trels[:] = [(tgt, attr) for tgt, attr in trels if tgt in reduction[node]]


def PrintPassGraph(relationships, output_file, reduce=False, no_feedback=False):
    """
    Print pass flow graph in DOT format
    """
    print(f"Generating DOT file: {output_file}")

    # Compute node degrees
    degree = defaultdict(int)
    for src, ptype, rels in relationships:
        if src not in degree:
            degree[src] = 0
        for relationship_type, edges in rels.items():
            for tgt, _attr in edges:
                if relationship_type == "INVALIDATION":
                    degree[src] += 1
                else:
                    degree[tgt] += 1

    if reduce:
        transitive_reduction(relationships)

    with open(output_file, "w") as dot_file:
        dot_file.write("digraph FrontendRelationships {\n")
        dot_file.write("    rankdir=LR;\n")

        for src, step_type, rels in relationships:
            size = 1.5 + degree[src]
            color = "black" if degree[src] else "red"
            fillcolor = STEP_COLORS[step_type]
            dot_file.write(
                f'    "{src}" [height={size}, shape=box3d, style=filled, color={color} fillcolor={fillcolor}];\n'
            )
            for relationship_type, edges in rels.items():
                if no_feedback and relationship_type == "INVALIDATION":
                    continue
                color = RELATIONSHIP_COLORS.get(relationship_type, "black")
                for tgt, attr in edges:
                    if relationship_type == "INVALIDATION":
                        dot_file.write(
                            f'    "{src}" -> "{tgt}" [color="{color}" label="{attr}"];\n'
                        )
                    else:
                        dot_file.write(
                            f'    "{tgt}" -> "{src}" [color="{color}" label="{attr}"];\n'
                        )

        dot_file.write("}\n")
    print(f"DOT file successfully generated: {output_file}")


def ParseDirectories(directories):
    all_relationships = []

    for directory in directories:
        print(f"Scanning directory: {directory}")
        for root, _, files in os.walk(directory):
            for file in sorted(files):
                if file.endswith(".cpp") and os.path.basename(file) not in skip_files:
                    file_path = os.path.join(root, file)
                    pass_name, pass_type, relationships = ExtractPassInfo(file_path)
                    if pass_name:
                        all_relationships.append((pass_name, pass_type, relationships))

    for base_class, pass_name in base_steps.items():
        if base_class in base_classes:
            pass_type, rels = base_classes[base_class]
            all_relationships.append((pass_name, pass_type, rels))

    return all_relationships


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate a DOT graph representing pass relationships."
    )
    parser.add_argument(
        "paths", nargs="+", help="Paths to directories containing passes' C++ files."
    )
    parser.add_argument(
        "-o", "--output", required=True, help="Path to the output DOT file."
    )
    parser.add_argument(
        "--transitive-reduce",
        action="store_true",
        default=False,
        help="Enable transitive reduction",
    )
    parser.add_argument(
        "--no-feedback",
        action="store_true",
        default=False,
        help="Remove invalidation arcs",
    )

    args = parser.parse_args()

    step_relationships = ParseDirectories(args.paths)

    PrintPassGraph(
        step_relationships, args.output, args.transitive_reduce, args.no_feedback
    )
