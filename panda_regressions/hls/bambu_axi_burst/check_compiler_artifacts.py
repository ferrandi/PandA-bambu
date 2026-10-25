#!/usr/bin/env python3
"""Check burst regression fixture source or Bambu-generated artifacts.

The checker deliberately reads compiler outputs; it never synthesizes or
substitutes expected RTL. Artifact checks accept explicit patterns so the
compiler operation spelling can evolve without editing generated files.
"""
import argparse
import pathlib
import re
import sys


def balanced_end(text, opening):
    """Return the matching close-paren index, respecting Verilog strings."""
    depth = 0
    in_string = False
    escaped = False
    for index in range(opening, len(text)):
        char = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def verilog_instances(text):
    """Yield (module type, named-connection text) for parameterized instances."""
    for match in re.finditer(r"(?m)^\s*(?P<module>[A-Za-z_]\w*)\s*#\s*\(", text):
        parameter_open = text.find("(", match.start(), match.end())
        parameter_close = balanced_end(text, parameter_open)
        if parameter_close is None:
            continue
        cursor = parameter_close + 1
        instance = re.match(r"\s*[A-Za-z_]\w*\s*\(", text[cursor:])
        if not instance:
            continue
        connection_open = text.find("(", cursor + instance.start(), cursor + instance.end())
        connection_close = balanced_end(text, connection_open)
        if connection_close is not None:
            yield match.group("module"), text[connection_open + 1:connection_close]


def parameterized_instances(text, module_name):
    """Yield named parameters and ports for each exact module instance."""
    marker = re.compile(rf"\b{re.escape(module_name)}\s*#\s*\(")
    for match in marker.finditer(text):
        parameter_open = text.find("(", match.start(), match.end())
        parameter_close = balanced_end(text, parameter_open)
        if parameter_close is None:
            continue
        cursor = parameter_close + 1
        instance = re.match(r"\s*[A-Za-z_$][\w$]*\s*\(", text[cursor:])
        if not instance:
            continue
        connection_open = text.find("(", cursor + instance.start(), cursor + instance.end())
        connection_close = balanced_end(text, connection_open)
        if connection_close is None:
            continue
        parameter_text = text[parameter_open + 1:parameter_close]
        parameters = {name: value.strip() for name, value in re.findall(
            r"\.(\w+)\s*\(\s*([^()]*)\s*\)", parameter_text)}
        ports = text[connection_open + 1:connection_close]
        yield parameters, ports


def has_configured_engine(text, expected_parameters):
    """Require the expected profile and runtime-count ports on one engine."""
    for parameters, ports in parameterized_instances(text, "MinimalAXI4MasterPipelined"):
        if not all(parameters.get(name) == str(value)
                   for name, value in expected_parameters.items()):
            continue
        if (all(re.search(rf"\.in{i}\s*\(", ports) for i in range(1, 6)) and
                re.search(r"\.in5\s*\(\s*in5\s*\)", ports)):
            return True
    return False


def named_instance_ports(text, module_pattern):
    for module, connections in verilog_instances(text):
        if re.search(module_pattern, module):
            yield connections


def verilog_modules(text):
    """Return uniquely named module bodies from a generated Verilog file."""
    matches = list(re.finditer(r"(?m)^[ \t]*module\s+([A-Za-z_]\w*)\b", text))
    modules = {}
    duplicates = set()
    for index, match in enumerate(matches):
        end = text.find("endmodule", match.end())
        if end < 0:
            continue
        name = match.group(1)
        if name in modules:
            duplicates.add(name)
        modules[name] = text[match.end():end]
    return modules, duplicates


def count_named_instance(body, module_type, instance_name):
    """Count a bare named Verilog module instance in one module body."""
    pattern = re.compile(
        rf"(?m)^\s*{re.escape(module_type)}\s+{re.escape(instance_name)}\s*\("
    )
    return len(pattern.findall(body))


def check_shared_profile_rtl(text, roots, expected_parameters):
    """Prove each requested root reaches the one shared read-wrapper definition."""
    modules, duplicates = verilog_modules(text)
    if duplicates:
        return False, f"duplicate module definitions: {', '.join(sorted(duplicates))}"
    wrapper_type = "data_bambu_artificial_ParmMgr_modgen"
    wrapper_body = modules.get(wrapper_type)
    if wrapper_body is None:
        return False, f"missing shared resource wrapper module {wrapper_type}"

    engines = list(parameterized_instances(wrapper_body, "MinimalAXI4MasterPipelined"))
    if len(engines) != 1:
        return False, f"shared wrapper must contain exactly one read engine; found {len(engines)}"
    engine_parameters, engine_ports = engines[0]
    if not all(engine_parameters.get(name) == str(value)
               for name, value in expected_parameters.items()):
        return False, "shared wrapper read engine does not have the expected profile on one instance"
    if not all(re.search(rf"\.in{i}\s*\(", engine_ports) for i in range(1, 6)):
        return False, "shared wrapper read engine is missing required in1..in5 connections"

    required_axi_ports = {
        "p_m_axi_data_araddr", "p_m_axi_data_arvalid", "p_m_axi_data_rdata",
        "p_m_axi_data_rvalid", "p_m_axi_data_rready",
    }
    for root in roots:
        root_body = modules.get(root)
        pbi_type = f"PBI_{root}"
        pbi_instance = f"{pbi_type}_i0"
        pbi_body = modules.get(pbi_type)
        datapath_type = f"datapath_{root}"
        datapath_body = modules.get(datapath_type)
        if root_body is None or pbi_body is None or datapath_body is None:
            return False, f"missing root/PBI/datapath module in path for {root}"
        if count_named_instance(root_body, pbi_type, pbi_instance) != 1:
            return False, f"{root} does not instantiate exactly one {pbi_type}.{pbi_instance}"
        if count_named_instance(pbi_body, datapath_type, "Datapath_i") != 1:
            return False, f"{pbi_type} does not instantiate exactly one {datapath_type}.Datapath_i"

        wrappers = list(parameterized_instances(datapath_body, wrapper_type))
        if len(wrappers) != 1:
            return False, f"{datapath_type} must reach exactly one {wrapper_type}; found {len(wrappers)}"
        _, wrapper_ports = wrappers[0]
        connected_axi_ports = dict(re.findall(
            r"\.(p_m_axi_data_\w+)\s*\(\s*([A-Za-z_]\w*)\s*\)", wrapper_ports))
        missing = sorted(required_axi_ports - connected_axi_ports.keys())
        if missing:
            return False, f"{datapath_type} shared wrapper is missing AXI read wiring: {', '.join(missing)}"
        miswired = sorted(port for port in required_axi_ports
                          if connected_axi_ports[port] != port)
        if miswired:
            return False, f"{datapath_type} shared wrapper does not preserve the shared AXI bundle: {', '.join(miswired)}"
    return True, "both roots reach the shared wrapper module definition and its configured read engine"


def check_shared_profile_status(status, stdout, stderr):
    """Accept clean success or the exact known debug-only multi-top assertion."""
    if re.search(r"(?i)(segmentation fault|stack smashing|core dumped|assertion .* failed)",
                 stdout + "\n" + stderr):
        return False, "compiler output contains a crash/fatal marker"
    errors = re.findall(r"(?m)^error -> .*?$", stdout + "\n" + stderr)
    if status == 0:
        if errors:
            return False, "compiler returned success but emitted an error diagnostic"
        return True, "compiler exited successfully"
    expected = "error -> Expected single top function name (top_symbols.size() == 1)"
    expected_context = re.compile(
        r"(?m)^error -> Expected single top function name \(top_symbols\.size\(\) == 1\)\n"
        r"\s*void BackendWrapper::init\(const HLS_managerRef&\)\n"
        r"\s*[^\n]*BackendWrapper\.cpp:\d+\s*$"
    )
    if status != 1 or errors != [expected] or not expected_context.search(stderr):
        return False, f"compiler status {status} is not the known backend multi-top assertion"
    diagnostic_tail = [line for line in stderr.splitlines() if line.strip()][-3:]
    expected_tail = [
        expected,
        "\tvoid BackendWrapper::init(const HLS_managerRef&)",
    ]
    if (len(diagnostic_tail) != 3 or diagnostic_tail[:2] != expected_tail or
            not re.search(r"BackendWrapper\.cpp:\d+$", diagnostic_tail[2].strip())):
        return False, "known backend multi-top assertion is not the compiler's final diagnostic"
    return True, "compiler reported only the known backend multi-top assertion after RTL generation"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("ir", "rtl", "fallback", "shared-profile"))
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--require", action="append", default=[], help="required regex over compiler artifacts")
    parser.add_argument("--forbid", action="append", default=[], help="forbidden regex over compiler artifacts")
    parser.add_argument("--count-pattern", help="regex for the runtime count operand, required in the same IR artifact as configure")
    parser.add_argument("--configure-log", type=pathlib.Path,
                        help="compiler log that must contain InterfaceInfer's configured-region diagnostic")
    parser.add_argument("--configured-contract", action="store_true",
                        help="check generated RTL's five-input burst engine and runtime-count fan-in")
    parser.add_argument("--expected-burst-max", type=int,
                        help="expected B_MAX for --configured-contract")
    parser.add_argument("--expected-outstanding", type=int, default=1,
                        help="expected MAX_OUTSTANDING for --configured-contract (default: 1)")
    parser.add_argument("--expected-fifo-depth", type=int,
                        help="expected FIFO_DEPTH for --configured-contract "
                             "(default: derived as the next power of two of "
                             "max(2, MAX_OUTSTANDING) * B_MAX, the compiler rule "
                             "for a pragma that omits read_fifo_depth)")
    parser.add_argument("--fallback-log", type=pathlib.Path,
                        help="compiler log that must report retaining legacy m_axi reads")
    parser.add_argument("--compiler-status", type=int,
                        help="compiler exit status required by shared-profile mode")
    parser.add_argument("--stdout-log", type=pathlib.Path,
                        help="compiler stdout log for shared-profile mode")
    parser.add_argument("--stderr-log", type=pathlib.Path,
                        help="compiler stderr log for shared-profile mode")
    parser.add_argument("--shared-root", action="append", default=[],
                        help="root module to trace through PBI/datapath to the shared wrapper; repeat twice")
    args = parser.parse_args()
    if args.output is None or not args.output.is_dir():
        parser.error("artifact mode requires an existing --output directory")
    if args.mode == "shared-profile":
        if (len(args.shared_root) != 2 or len(set(args.shared_root)) != 2 or
                args.compiler_status is None or args.stdout_log is None or
                args.stderr_log is None or args.expected_burst_max is None or
                args.expected_outstanding is None or args.expected_fifo_depth is None):
            parser.error("shared-profile requires two --shared-root values, --compiler-status, "
                         "both compiler logs, and all three expected engine parameters")
        top = args.output / "top.v"
        try:
            rtl = top.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            print(f"SHARED PROFILE CHECK FAILED: cannot read generated top.v: {exc}", file=sys.stderr)
            return 1
        structural_ok, structural_reason = check_shared_profile_rtl(
            rtl, args.shared_root, {
                "B_MAX": args.expected_burst_max,
                "MAX_OUTSTANDING": args.expected_outstanding,
                "FIFO_DEPTH": args.expected_fifo_depth,
            })
        if not structural_ok:
            print(f"SHARED PROFILE CHECK FAILED: {structural_reason}", file=sys.stderr)
            return 1
        try:
            stdout = args.stdout_log.read_text(encoding="utf-8", errors="replace")
            stderr = args.stderr_log.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            print(f"SHARED PROFILE CHECK FAILED: cannot read compiler logs: {exc}", file=sys.stderr)
            return 1
        status_ok, status_reason = check_shared_profile_status(args.compiler_status, stdout, stderr)
        if not status_ok:
            print(f"SHARED PROFILE CHECK FAILED: {status_reason}", file=sys.stderr)
            return 1
        print(f"SHARED PROFILE CHECK PASS: {structural_reason}; {status_reason}")
        return 0
    if args.configured_contract and args.expected_fifo_depth is None:
        if args.expected_burst_max is None:
            parser.error("--configured-contract needs --expected-burst-max to derive FIFO_DEPTH")
        # Mirror ParseAXIReadBurstProfile: a pragma that omits read_fifo_depth
        # gets the next power of two of max(2, outstanding) * burst maximum.
        needed = max(2, args.expected_outstanding) * args.expected_burst_max
        derived_depth = 1
        while derived_depth < needed:
            derived_depth <<= 1
        args.expected_fifo_depth = derived_depth
    artifact_suffixes = {".v", ".sv", ".xml", ".log", ".txt", ".ll", ".bambuir", ".raw", ".ir", ".hls"}
    candidates = [p for p in args.output.rglob("*") if p.is_file() and p.suffix.lower() in artifact_suffixes]
    if not candidates:
        print(f"No compiler artifacts found below {args.output}", file=sys.stderr)
        return 2
    selected = candidates
    if args.mode == "ir":
        selected = [p for p in candidates if p.suffix.lower() in {".bambuir", ".raw", ".ir", ".ll"}]
    elif args.mode in ("rtl", "fallback"):
        selected = [p for p in candidates if p.suffix.lower() in {".v", ".sv"} and p.name != "panda_libtech.v"]
    if not selected:
        print(f"No {args.mode} compiler artifacts found below {args.output}", file=sys.stderr)
        return 2
    texts = []
    for path in selected:
        try:
            texts.append(path.read_text(encoding="utf-8", errors="replace"))
        except OSError:
            continue
    corpus = "\n".join(texts)
    required = list(args.require)
    forbidden = list(args.forbid)
    if args.mode in ("ir", "rtl") and not required:
        required = [r"(?i)(configure_read|burst_configure|MinimalAXI4MasterPipelined)"]
    if args.mode == "fallback" and not forbidden:
        forbidden = [r"MinimalAXI4MasterPipelined", r"configure_read"]
    if args.mode == "fallback" and not required:
        required = [r"(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)"]
    errors = []
    if args.configure_log:
        try:
            configure_log = args.configure_log.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            configure_log = ""
            errors.append(f"cannot read configure diagnostic log {args.configure_log}: {exc}")
        if not re.search(r"Configured read burst regions for bundle .*\(B=\d+\)", configure_log):
            errors.append(f"InterfaceInfer configured-region diagnostic absent from {args.configure_log}")
    if args.fallback_log:
        try:
            fallback_log = args.fallback_log.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            fallback_log = ""
            errors.append(f"cannot read fallback diagnostic log {args.fallback_log}: {exc}")
        if not re.search(r"Keeping legacy m_axi (?:reads|accesses) for bundle", fallback_log):
            errors.append(f"InterfaceInfer legacy-fallback diagnostic absent from {args.fallback_log}")
        if re.search(r"Configured read burst regions for bundle", fallback_log):
            errors.append(f"unexpected configured burst region reported in {args.fallback_log}")

    if args.configured_contract:
        if args.mode != "rtl":
            errors.append("--configured-contract is valid only in rtl mode")
        if args.expected_burst_max is None or not 1 <= args.expected_burst_max <= 256:
            errors.append("--configured-contract requires --expected-burst-max in [1,256]")
        if not 1 <= args.expected_outstanding <= 16:
            errors.append("--expected-outstanding must be in [1,16]")
        if not 1 <= args.expected_fifo_depth <= 4096 or args.expected_fifo_depth & (args.expected_fifo_depth - 1):
            errors.append("--expected-fifo-depth must be a power of two in [1,4096]")
        rtl_contract_ok = False
        for path in selected:
            text = path.read_text(encoding="utf-8", errors="replace")
            max_value = args.expected_burst_max
            if not has_configured_engine(text, {
                    "B_MAX": max_value,
                    "MAX_OUTSTANDING": args.expected_outstanding,
                    "FIFO_DEPTH": args.expected_fifo_depth,
            }):
                continue
            if not re.search(r"configure_read", text):
                continue
            if not re.search(r"\bBITSIZE_in5\s*=\s*32\b", text) or not re.search(
                    r"input\s+\[BITSIZE_in5-1:0\]\s+in5\s*;", text):
                continue
            if not re.search(r"wire\s+burst_rid\s*=\s*\|\s*p_m_axi_data_rid\s*;", text) or not re.search(
                    r"assign\s+p_m_axi_data_arid\s*=\s*\{\{\s*\(BITSIZE_arid-1\)\s*\{1'b0\}\s*\}\s*,\s*burst_arid\s*\}\s*;",
                    text):
                continue

            # Trace the resource wrapper's in5 operand into the datapath's
            # combinational fan-in. The bound must reach a select node whose
            # alternatives include the source-level n input and a zero value.
            datapath = re.search(r"module\s+datapath_\w+\s*\(.*?\);(?P<body>.*?)endmodule", text, re.S)
            if not datapath or not re.search(r"\bin_port_n\b", datapath.group("body")):
                continue
            body = datapath.group("body")
            # Build a conservative combinational fan-in graph from named
            # Verilog connections. Only out1 -> inN data paths are traversed.
            fanin = {}
            select_nodes = []
            for module, conn in verilog_instances(body):
                out = re.search(r"\.out1\s*\(\s*(\w+)\s*\)", conn)
                inputs = re.findall(r"\.in\d+\s*\(\s*(\w+)\s*\)", conn)
                if out:
                    fanin[out.group(1)] = inputs
                    if "select_node_FU" in module and "in_port_n" in inputs and any(
                            value in {"out_const_0", "out_conv_out_const_0_1_32"} for value in inputs):
                        select_nodes.append(out.group(1))
            resource_count_signals = []
            for conn in named_instance_ports(text, r"data_bambu_artificial_ParmMgr_modgen"):
                resource_count_signals.extend(re.findall(r"\.in5\s*\(\s*(\w+)\s*\)", conn))
            for count in resource_count_signals:
                reachable = set()
                pending = [count]
                while pending:
                    signal = pending.pop()
                    if signal in reachable:
                        continue
                    reachable.add(signal)
                    pending.extend(fanin.get(signal, ()))
                if any(signal in reachable for signal in select_nodes):
                    rtl_contract_ok = True
                    break
            if rtl_contract_ok:
                break
        if not rtl_contract_ok:
            errors.append("generated RTL lacks the configured five-input engine/B_MAX contract, RID mapping, or in5 fan-in from n guarded by zero")
    for pattern in required:
        if not re.search(pattern, corpus, re.MULTILINE):
            errors.append(f"required pattern absent: {pattern}")
    for pattern in forbidden:
        if re.search(pattern, corpus, re.MULTILINE):
            errors.append(f"forbidden pattern present: {pattern}")
    if args.mode == "ir":
        ir_suffixes = {".bambuir", ".raw", ".ir", ".ll"}
        ir_files = [p for p in selected if p.suffix.lower() in ir_suffixes]
        if not ir_files:
            errors.append("no Bambu IR artifact found (.bambuir/.raw/.ir/.ll); --no-clean may be missing")
        elif args.count_pattern:
            configure_patterns = required or [r"(?i)(configure_read|burst_configure)"]
            if not any(any(re.search(cp, text, re.M) for cp in configure_patterns) and
                       re.search(args.count_pattern, text, re.M)
                       for p in ir_files
                       for text in [p.read_text(encoding="utf-8", errors="replace")]):
                errors.append("configure operation and runtime count pattern were not found together in a Bambu IR artifact")
    if errors:
        print("ARTIFACT CHECK FAILED:", *errors, sep="\n  ", file=sys.stderr)
        print(f"Scanned {len(candidates)} compiler artifacts below {args.output}", file=sys.stderr)
        return 1
    print(f"ARTIFACT CHECK PASS ({args.mode}): scanned {len(selected)} generated files below {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
