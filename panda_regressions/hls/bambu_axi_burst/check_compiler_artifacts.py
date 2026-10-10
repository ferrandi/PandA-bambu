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


def named_instance_ports(text, module_pattern):
    for module, connections in verilog_instances(text):
        if re.search(module_pattern, module):
            yield connections


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("ir", "rtl", "fallback"))
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
    args = parser.parse_args()
    if args.output is None or not args.output.is_dir():
        parser.error("artifact mode requires an existing --output directory")
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
            engine = re.search(
                rf"MinimalAXI4MasterPipelined\s*#\s*\(\s*\.B_MAX\s*\(\s*{max_value}\s*\)\s*,\s*"
                rf"\.MAX_OUTSTANDING\s*\(\s*{args.expected_outstanding}\s*\)\s*,\s*"
                rf"\.FIFO_DEPTH\s*\(\s*{args.expected_fifo_depth}\s*\)\s*"
                # The generator also passes the BITSIZE_<port> widths here; they
                # follow the profile parameters and must not break the match.
                rf"(?:,\s*\.\w+\s*\(\s*[^()]*\s*\)\s*)*\)\s*"
                r"(?P<instance>\w+)\s*\((?P<ports>.*?)\);", text, re.S)
            if not engine:
                continue
            ports = engine.group("ports")
            if not all(re.search(rf"\.in{i}\s*\(", ports) for i in range(1, 6)) or not re.search(
                    r"\.in5\s*\(\s*in5\s*\)", ports):
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
