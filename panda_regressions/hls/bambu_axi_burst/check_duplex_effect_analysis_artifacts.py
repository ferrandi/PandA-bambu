#!/usr/bin/env python3
"""Fail-closed checker for manual duplex effect and activation gates."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

from check_burst_results import _parameterized_instances

LEGACY_COMPONENT = re.compile(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b")
BURST_COMPONENTS = re.compile(r"\b(?:MinimalAXI4MasterPipelined|MinimalAXI4MasterPipelinedWrite)\b")
BURST_OPS = re.compile(r"fuselector_[A-Za-z0-9_]*(?:configure_read|burst_read|configure_write|store_write|drain_write)\b")
TRACE = re.compile(
    r'^\s*AXI_BURST_PREANALYSIS bundle=(\S+) owner_argument_index=(\d+) '
    r'read_accesses=(\d+) write_accesses=(\d+) read_requested=(\d) read_recognized=(\d) '
    r'read_fallback="([^"]*)" write_requested=(\d) write_recognized=(\d) '
    r'write_fallback="([^"]*)" same_base_duplex_candidate=(\d) '
    r'same_base_duplex_reason="([^"]*)" production_activated=(\d) '
    r'activation_reason="([^"]*)"\s*$', re.MULTILINE)
PROOF_PREFIX = "AXI_BURST_DUPLEX_PROOF "
SUCCESS = "same-base pointwise read-before-write candidate"
DISTINCT_SUCCESS = "distinct-base restricted pointwise read-before-write candidate"
FALLBACK_READ = "bundle contains stores"
FALLBACK_WRITE = "bundle contains reads; same-bundle read/write requires duplex support"
IDENTITY_FIELDS = {
    "owner_argument_index", "read_owner_argument_index", "write_owner_argument_index",
    "base_ssa_index", "read_base_ssa_index", "write_base_ssa_index",
    "load_statement_index", "store_statement_index",
    "read_preheader_bb", "read_header_bb", "read_branch_index", "read_guard_index",
    "read_bound_index", "read_induction_index", "write_preheader_bb", "write_header_bb",
    "write_exit_bb", "write_guard_index", "write_bound_index", "write_induction_index",
}
COUNT_PROOF_FIELDS = {
    "loop_trip_bound_ssa_index", "loop_trip_compare_index",
    "read_effective_bound_ssa_index", "write_effective_bound_ssa_index",
    "read_effective_compare_index", "write_effective_compare_index",
    "read_access_bb", "write_access_bb", "read_count_recipe", "write_count_recipe",
    "pairing_domain_recipe",
}
CASES: dict[str, tuple[str, int, str, str, bool, bool]] = {
    "same_base_pointwise": (SUCCESS, 1, "-", "-", True, True),
    "same_base_pointwise_reversed": (SUCCESS, 1, "-", "-", True, True),
    "same_base_asymmetric_64": (SUCCESS, 1, "-", "-", True, True),
    "same_base_cached": (SUCCESS, 1, "cache-enabled bundle", "cache-enabled bundle", True, False),
    "shifted_index": ("read/write accesses do not share one canonical loop", 1,
                      FALLBACK_READ, FALLBACK_WRITE, False, True),
    "stride_index": ("canonical contiguous loop recognition failed", 1, FALLBACK_READ, FALLBACK_WRITE, False, True),
    "conditional_store": ("unsupported direction guard CFG", 1, FALLBACK_READ, FALLBACK_WRITE, False, True),
    "residual_store": ("original effects are not fully classified ordinary accesses", 1,
                       "memory writes or opaque effects; prefetch ordering is not proven",
                       "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "volatile_effect": ("original effects are not fully classified ordinary accesses", 1,
                        "memory writes or opaque effects; prefetch ordering is not proven",
                        "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "separate_phase": ("original effects are not fully classified ordinary accesses", 1,
                       "memory writes or opaque effects; prefetch ordering is not proven",
                       "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "split_read_write_loops": ("read/write accesses do not share one canonical loop", 1,
                                FALLBACK_READ, FALLBACK_WRITE, False, True),
    "same_loop_byte_side_store": ("original effects are not fully classified ordinary accesses", 1,
                                  "memory writes or opaque effects; prefetch ordering is not proven",
                                  "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "same_loop_opaque_call": ("original effects are not fully classified ordinary accesses", 1,
                              "memory writes or opaque effects; prefetch ordering is not proven",
                              "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "distinct_bases": ("distinct-base duplex requires source restrict on both owners", 2,
                       FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_bases_source_first": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_bases_destination_first": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_bases_asymmetric_64": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_bases_mismatched_bounds": (
        "effective bound is defined inside loop", 2,
        FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_bases_two_loops": ("read/write accesses do not share one canonical loop", 2,
                                  FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_prefix_read_longer": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_prefix_write_longer": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_prefix_equal": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_prefix_signed_bound": ("unsupported direction guard CFG", 2,
                                     FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_prefix_width_bound": (DISTINCT_SUCCESS, 2, "-", "-", True, True),
    "distinct_prefix_u64_bound": ("unsupported direction guard CFG", 2,
                                   FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_prefix_extra_cfg": ("unsupported direction guard CFG", 2,
                                  FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_prefix_guard_bypass": ("unsupported direction guard CFG", 2,
                                     FALLBACK_READ, FALLBACK_WRITE, False, True),
    "distinct_bases_one_restrict": ("distinct-base duplex requires source restrict on both owners", 2,
                                    FALLBACK_READ, FALLBACK_WRITE, False, True),
}
ACCESS_COUNTS: dict[str, tuple[tuple[int, int], ...]] = {
    "same_base_pointwise": ((1, 1),),
    "same_base_pointwise_reversed": ((1, 1),),
    "same_base_asymmetric_64": ((1, 1),),
    "same_base_cached": ((1, 1),),
    "shifted_index": ((1, 1),),
    "stride_index": ((1, 1),),
    "conditional_store": ((1, 1),),
    "residual_store": ((1, 2),),
    "volatile_effect": ((1, 1),),
    "separate_phase": ((2, 2),),
    "split_read_write_loops": ((1, 1),),
    "same_loop_byte_side_store": ((1, 2),),
    "same_loop_opaque_call": ((1, 1),),
    "distinct_bases": ((0, 1), (1, 0)),
    "distinct_bases_source_first": ((1, 0), (0, 1)),
    "distinct_bases_destination_first": ((1, 0), (0, 1)),
    "distinct_bases_asymmetric_64": ((1, 0), (0, 1)),
    "distinct_bases_mismatched_bounds": ((1, 0), (0, 1)),
    "distinct_bases_two_loops": ((1, 0), (0, 1)),
    "distinct_prefix_read_longer": ((1, 0), (0, 1)),
    "distinct_prefix_write_longer": ((1, 0), (0, 1)),
    "distinct_prefix_equal": ((1, 0), (0, 1)),
    "distinct_prefix_signed_bound": ((1, 0), (0, 1)),
    "distinct_prefix_width_bound": ((1, 0), (0, 1)),
    "distinct_prefix_u64_bound": ((1, 0), (0, 1)),
    "distinct_prefix_extra_cfg": ((1, 0), (0, 1)),
    "distinct_prefix_guard_bypass": ((1, 0), (0, 1)),
    "distinct_bases_one_restrict": ((1, 0), (0, 1)),
}
ACTIVATED_CASES = {"same_base_pointwise", "same_base_pointwise_reversed", "same_base_asymmetric_64",
                   "distinct_bases_source_first", "distinct_bases_destination_first",
                   "distinct_bases_asymmetric_64", "distinct_prefix_read_longer",
                   "distinct_prefix_write_longer", "distinct_prefix_equal", "distinct_prefix_width_bound"}
DISTINCT_BASE_CASES = {"distinct_bases_source_first", "distinct_bases_destination_first",
                       "distinct_bases_asymmetric_64", "distinct_prefix_read_longer",
                       "distinct_prefix_write_longer", "distinct_prefix_equal",
                       "distinct_prefix_width_bound"}
PREFIX_COUNT_CASES = {"distinct_prefix_read_longer", "distinct_prefix_write_longer",
                      "distinct_prefix_equal", "distinct_prefix_width_bound"}
RESTRICT_METADATA_CASES = {
    "distinct_bases": {"src": "0", "dst": "0"},
    "distinct_bases_source_first": {"src": "1", "dst": "1"},
    "distinct_bases_destination_first": {"dst": "1", "src": "1"},
    "distinct_bases_asymmetric_64": {"src": "1", "dst": "1"},
    "distinct_bases_mismatched_bounds": {"src": "1", "dst": "1"},
    "distinct_bases_two_loops": {"src": "1", "dst": "1"},
    "distinct_prefix_read_longer": {"src": "1", "dst": "1"},
    "distinct_prefix_write_longer": {"src": "1", "dst": "1"},
    "distinct_prefix_equal": {"src": "1", "dst": "1"},
    "distinct_prefix_signed_bound": {"src": "1", "dst": "1"},
    "distinct_prefix_width_bound": {"src": "1", "dst": "1"},
    "distinct_prefix_u64_bound": {"src": "1", "dst": "1"},
    "distinct_prefix_extra_cfg": {"src": "1", "dst": "1"},
    "distinct_prefix_guard_bypass": {"src": "1", "dst": "1"},
    "distinct_bases_one_restrict": {"src": "1", "dst": "0"},
}
EXPECTED_ADDRESS_BITS = {"same_base_asymmetric_64": "64", "distinct_bases_asymmetric_64": "64"}
EXPECTED_PROFILES = {"same_base_asymmetric_64": ("8", "2", "16", "1"),
                     "distinct_bases_asymmetric_64": ("8", "2", "16", "1")}
READ_AXI_INPUTS = {"awready", "wready", "bid", "bresp", "buser", "bvalid",
                   "arready", "rid", "rdata", "rresp", "rlast", "ruser", "rvalid"}
AXI_SUFFIXES = {
    "awready", "wready", "bid", "bresp", "buser", "bvalid", "arready", "rid", "rdata", "rresp",
    "rlast", "ruser", "rvalid", "awid", "awaddr", "awlen", "awsize", "awburst", "awlock", "awcache",
    "awprot", "awqos", "awregion", "awuser", "awvalid", "wdata", "wstrb", "wlast", "wuser", "wvalid",
    "bready", "arid", "araddr", "arlen", "arsize", "arburst", "arlock", "arcache", "arprot", "arqos",
    "arregion", "aruser", "arvalid", "rready",
}


class CheckError(ValueError):
    pass


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def expected_owner_recognition(top: str, reads: int, writes: int) -> tuple[str, str]:
    if top in DISTINCT_BASE_CASES:
        return ("1" if reads == 1 else "0", "1" if writes == 1 else "0")
    recognized = "1" if top in ACTIVATED_CASES else "0"
    return recognized, recognized


PRIVATE_FIELDS = ("start", "opcode", "size", "data", "address", "count", "done", "result")


def verilog_modules(code: str) -> dict[str, str]:
    return {match.group(1): match.group(2) + "\n" + match.group(3)
            for match in re.finditer(
                r"(?ms)^\s*module\s+([A-Za-z_$][\w$]*)\s*\((.*?)\);(.*?)^\s*endmodule\b", code)}


def verilog_instances(body: str) -> list[tuple[str, str, dict[str, str]]]:
    instances = []
    pattern = re.compile(
        r"(?m)^\s*(?!module\b|endmodule\b|input\b|output\b|wire\b|reg\b)"
        r"([A-Za-z_$][\w$]*)\s*(?:#\s*\((?:[^()]|\([^()]*\))*\)\s*)?"
        r"([A-Za-z_$][\w$]*)\s*\(")
    for match in pattern.finditer(body):
        opening = match.end() - 1
        depth = 1
        closing = opening + 1
        while closing < len(body) and depth:
            depth += (body[closing] == "(") - (body[closing] == ")")
            closing += 1
        if depth:
            continue
        arguments = body[opening + 1:closing - 1]
        ports = {}
        for port_match in re.finditer(r"\.([A-Za-z_$][\w$]*)\s*\(", arguments):
            port_open = port_match.end() - 1
            port_depth = 1
            port_close = port_open + 1
            while port_close < len(arguments) and port_depth:
                port_depth += (arguments[port_close] == "(") - (arguments[port_close] == ")")
                port_close += 1
            if not port_depth:
                ports[port_match.group(1)] = arguments[port_open + 1:port_close - 1].strip()
        instances.append((match.group(1), match.group(2), ports))
    return instances


def verilog_decl(body: str, name: str, kind: str) -> tuple[str, int]:
    matches = list(re.finditer(
        rf"(?m)^\s*(input|output|inout|wire|logic|reg)\s+"
        rf"(?:signed\s+)?(\[[^\]]+\]\s*)?{re.escape(name)}\s*;?\s*$", body))
    if kind == "port":
        matches = [match for match in matches if match.group(1) in {"input", "output", "inout"}]
    else:
        matches = [match for match in matches if match.group(1) in {"wire", "logic", "reg"}]
    need(len(matches) == 1, f"expected exactly one {kind} declaration for {name}")
    match = matches[0]
    packed = match.group(2)
    if not packed:
        width = 1
    else:
        bounds = re.fullmatch(r"\[\s*(\d+)\s*:\s*(\d+)\s*\]", packed.strip())
        need(bounds is not None, f"{kind} {name} has unsupported width declaration {packed.strip()}")
        width = abs(int(bounds.group(1)) - int(bounds.group(2))) + 1
    return match.group(1), width


def verify_private_duplex_wiring(code: str, parent_body: str,
                                 endpoint_instances: dict[str, tuple[str, dict[str, str]]],
                                 aggregator_type: str, aggregator_args: dict[str, str],
                                 address_bits: int, data_bits: int) -> None:
    modules = verilog_modules(code)
    def module_matches(module_name: str, instance_type: str) -> bool:
        normalized_type = re.sub(r"_\d+_i\d+$", "_modgen", instance_type)
        return module_name == instance_type or module_name == normalized_type or \
            module_name.endswith("_" + instance_type) or module_name.endswith("_" + normalized_type)

    aggregator_modules = [name for name in modules if module_matches(name, aggregator_type)]
    need(len(aggregator_modules) == 1, "duplex aggregator module definition is missing or ambiguous")
    aggregator_body = modules[aggregator_modules[0]]
    wires_seen: set[str] = set()
    for direction in ("read", "write"):
        endpoint_type, endpoint_args = endpoint_instances[f"{direction}_endpoint"]
        endpoint_modules = [name for name in modules if module_matches(name, endpoint_type)]
        need(len(endpoint_modules) == 1, f"duplex {direction} endpoint module definition is missing or ambiguous")
        endpoint_body = modules[endpoint_modules[0]]
        expected_assignments = {
            "duplex_start": "start_port[0]",
            "duplex_opcode": "in1",
            "duplex_size": "in2",
            "duplex_data": "in3",
            "duplex_address": "in4",
            "duplex_count": "in5",
            "done_port": "duplex_done",
            "out1": "duplex_result",
        }
        assignments: dict[str, list[str]] = {}
        for assignment in re.finditer(
                r"\bassign\s+([A-Za-z_$][\w$]*)\s*=\s*([^;]+);", endpoint_body):
            lhs = assignment.group(1)
            rhs = re.sub(r"\s+", "", assignment.group(2))
            assignments.setdefault(lhs, []).append(rhs)
        for lhs, rhs in expected_assignments.items():
            values = assignments.get(lhs, [])
            need(len(values) == 1,
                 f"{direction} endpoint must assign {lhs} exactly once")
            need(values[0] == rhs,
                 f"{direction} endpoint {lhs} must be assigned from {rhs}")
        for field in PRIVATE_FIELDS:
            endpoint_port = f"duplex_{field}"
            aggregator_port = f"{direction}_{field}"
            endpoint_direction, endpoint_width = verilog_decl(endpoint_body, endpoint_port, "port")
            aggregator_direction, aggregator_width = verilog_decl(aggregator_body, aggregator_port, "port")
            expected_direction = "output" if field in {"start", "opcode", "size", "data", "address", "count"} else "input"
            need(endpoint_direction == expected_direction,
                 f"{direction} endpoint {endpoint_port} must be {expected_direction}")
            expected_aggregator_direction = "input" if expected_direction == "output" else "output"
            need(aggregator_direction == expected_aggregator_direction,
                 f"aggregator {aggregator_port} must be {expected_aggregator_direction}")
            expected_width = {"start": 1, "opcode": 2, "size": 32, "data": data_bits,
                              "address": address_bits, "count": 32, "done": 1,
                              "result": data_bits}[field]
            need(endpoint_width == aggregator_width == expected_width,
                 f"{direction} {field} port width does not match the {expected_width}-bit profile")

            endpoint_net = endpoint_args.get(endpoint_port)
            aggregator_net = aggregator_args.get(aggregator_port)
            need(endpoint_net is not None and aggregator_net is not None,
                 f"{direction} {field} is missing on an endpoint or aggregator instance")
            identifier = r"[A-Za-z_$][\w$]*"
            need(re.fullmatch(identifier, endpoint_net) and endpoint_net == aggregator_net,
                 f"{direction} {field} is not connected through one named net")
            need(not endpoint_net.startswith(("null_out_signal_", "out_const_")) and
                 "dummy" not in endpoint_net.lower(),
                 f"{direction} {field} is connected to a dummy or constant net")
            need(endpoint_net not in wires_seen, f"duplex private net {endpoint_net} is reused across links")
            wires_seen.add(endpoint_net)
            _wire_kind, wire_width = verilog_decl(parent_body, endpoint_net, "wire")
            need(wire_width == expected_width,
                 f"{direction} {field} net width does not match the {expected_width}-bit profile")


def check_production_duplex(rtl: str, compiler_log: str, top: str, attrs: dict[str, str]) -> None:
    bundle = "shared"
    is_distinct_base = top in DISTINCT_BASE_CASES
    marker_kind = "distinct-base restricted pointwise" if is_distinct_base else "same-base pointwise"
    marker = f"Activated {marker_kind} duplex m_axi bundle {bundle} for root {top}"
    matches = re.findall(rf"(?m)^\s*(?:---)?{re.escape(marker)}\s*$", compiler_log)
    need(len(matches) == 1, "production activation marker is missing or duplicated")
    if is_distinct_base:
        warning = ("WARNING: distinct-base duplex relies on the caller's __restrict contract; the caller must "
                   "ensure the accessed regions do not overlap (no runtime range validation is performed)")
        need(len(re.findall(rf"(?m)^\s*{re.escape(warning)}\s*$", compiler_log)) == 1,
             "distinct-base activation is missing its caller non-overlap contract warning")
    code = re.sub(r"(?s)/\*.*?\*/|//[^\n]*", "", rtl)
    modules = verilog_modules(code)
    instances_by_module = {name: verilog_instances(body) for name, body in modules.items()}
    for role in ("read_endpoint", "write_endpoint", "aggregator"):
        matches = [module for instances in instances_by_module.values() for module, _instance, _ports in instances
                   if re.search(rf"_duplex_{role}(?:_modgen)?$", module)]
        need(len(matches) == 1, f"expected exactly one duplex {role} instance, found {len(matches)}")

    operation_patterns = {
        "read_endpoint": ("configure_read", "burst_read"),
        "write_endpoint": ("configure_write", "store_write", "drain_write"),
    }
    for role, operations in operation_patterns.items():
        for operation in operations:
            pattern = (rf"\bfuselector_[A-Za-z0-9_$]*_duplex_{role}"
                       rf"(?:_\d+_i\d+)?(?:_[A-Za-z0-9_$]+)*_{operation}\b")
            need(re.search(pattern, code) is not None,
                 f"duplex {role} operation mapping is missing {operation}")
    need(not LEGACY_COMPONENT.search(code), "activated duplex RTL coexists with a legacy memory adapter")

    top_declarations = list(re.finditer(rf"(?m)^\s*module\s+{re.escape(top)}\s*\(", code))
    need(len(top_declarations) == 1, f"activated duplex RTL must contain exactly one selected top {top}")
    top_body = modules.get(top, "")
    need(bool(top_body), f"activated duplex RTL top {top} has no parseable module body")
    for suffix in AXI_SUFFIXES:
        direction = "input" if suffix in READ_AXI_INPUTS else "output"
        address_bits = EXPECTED_ADDRESS_BITS.get(top, "32")
        width = rf"\[{int(address_bits) - 1}:0\]\s+" if suffix in {"araddr", "awaddr"} else \
            r"(?:\[[^\]]+\]\s+)?"
        declaration = re.compile(
            rf"(?m)^\s*{direction}\s+{width}m_axi_shared_{suffix}\s*;?\s*$")
        need(declaration.search(top_body) is not None,
             f"external bundle pin {suffix} is not declared as {direction} on top {top}")
        need(len(re.findall(rf"\bp_m_axi_shared_{suffix}\b", code)) >= 3,
             f"physical bundle pin {suffix} is not connected through the aggregator")
    need(not re.search(r"(?m)^\s*(?:input|output)\b[^;\n]*\bduplex_", top_body),
         f"private duplex endpoint ports leaked into physical module interface {top}")
    expected_pbi_ports = {f"p_m_axi_shared_{suffix}": f"m_axi_shared_{suffix}"
                          for suffix in AXI_SUFFIXES}
    pbi_instances = [ports for module, _instance, ports in verilog_instances(top_body)
                     if module.startswith("PBI_") and set(expected_pbi_ports) <= ports.keys()]
    need(len(pbi_instances) == 1,
         f"top {top} must map every physical pin through exactly one PBI instance")
    for port, signal in expected_pbi_ports.items():
        need(pbi_instances[0].get(port) == signal,
             f"top {top} PBI mapping for {port} must connect to {signal}")

    internal_bundle_modules = []
    for module_name, body in modules.items():
        if module_name == top:
            continue
        present = {suffix for suffix in AXI_SUFFIXES
                   if re.search(rf"\bp_m_axi_shared_{suffix}\b", body)}
        if present == AXI_SUFFIXES:
            internal_bundle_modules.append((module_name, body))
    need(internal_bundle_modules,
         "activated duplex RTL lacks a complete internal p_m_axi bundle module")
    for module_name, body in internal_bundle_modules:
        for suffix in AXI_SUFFIXES:
            need(re.search(rf"(?m)^\s*(?:input|output|inout|wire|logic|reg)\s+"
                           rf"(?:\[[^\]]+\]\s*)?p_m_axi_shared_{suffix}\s*;?\s*$", body),
                 f"internal module {module_name} lacks declaration for p_m_axi_shared_{suffix}")

    read_instances = list(_parameterized_instances(code, "MinimalAXI4MasterPipelined"))
    write_instances = list(_parameterized_instances(code, "MinimalAXI4MasterPipelinedWrite"))
    need(len(read_instances) == 1, "activated duplex must instantiate exactly one read burst core")
    need(len(write_instances) == 1, "activated duplex must instantiate exactly one write burst core")
    read_profile = read_instances[0]
    write_profile = write_instances[0]
    expected = {
        "B_MAX": (attrs.get("max_read_burst_length"), attrs.get("max_write_burst_length")),
        "MAX_OUTSTANDING": (attrs.get("num_read_outstanding", "1"),
                            attrs.get("write_burst_outstanding", "1")),
        "BITSIZE_in4": (EXPECTED_ADDRESS_BITS.get(top, "32"), EXPECTED_ADDRESS_BITS.get(top, "32")),
    }
    for parameter, values in expected.items():
        need(values[0] is not None and values[1] is not None and
             read_profile.get(parameter) == values[0] and write_profile.get(parameter) == values[1],
             f"read/write cores do not independently preserve {parameter} profiles")
    for attr, profile in (("read_fifo_depth", read_profile), ("write_fifo_depth", write_profile)):
        if attr in attrs:
            need(profile.get("FIFO_DEPTH") == attrs[attr],
                 f"duplex core FIFO depth does not match architecture attribute {attr}")
    for profile in (read_profile, write_profile):
        need(profile.get("MAX_OUTSTANDING") is not None and profile.get("FIFO_DEPTH") is not None,
             "duplex burst core is missing outstanding/FIFO profile parameters")
        need(profile.get("PEER_FAULT_ENABLE") == "1",
             "duplex burst core must enable peer fault propagation")
    address_bits = EXPECTED_ADDRESS_BITS.get(top, "32")
    need(read_profile.get("BITSIZE_m_axi_araddr") == address_bits and
         write_profile.get("BITSIZE_m_axi_awaddr") == address_bits,
         "read/write core address widths do not match the selected AXI address profile")
    need(read_profile.get("BITSIZE_m_axi_rdata", "32") == write_profile.get("BITSIZE_m_axi_wdata", "32"),
         "read/write core data widths do not match the selected AXI data profile")

    parent_candidates = []
    for parent_name, body in modules.items():
        instances = verilog_instances(body)
        endpoints = {}
        aggregator = None
        for module_name, _instance_name, ports in instances:
            for role in ("read_endpoint", "write_endpoint"):
                if re.search(rf"_duplex_{role}(?:_modgen)?$", module_name):
                    endpoints[role] = (module_name, ports)
            if re.search(r"_duplex_aggregator(?:_modgen)?$", module_name):
                aggregator = (module_name, ports)
        if set(endpoints) == {"read_endpoint", "write_endpoint"} and aggregator:
            parent_candidates.append((parent_name, body, endpoints, aggregator))
    need(len(parent_candidates) == 1, "duplex endpoint and aggregator instances lack one unique parent module")
    _parent_name, parent_body, endpoints, (aggregator_type, aggregator_args) = parent_candidates[0]
    verify_private_duplex_wiring(code, parent_body, endpoints, aggregator_type, aggregator_args,
                                 int(address_bits), int(read_profile.get("BITSIZE_m_axi_rdata", "32")))
    aggregator_modules = [name for name in modules if name == aggregator_type or
                          name.endswith("_" + aggregator_type)]
    need(len(aggregator_modules) == 1,
         "duplex aggregator module definition is missing or ambiguous")
    aggregator_body = modules[aggregator_modules[0]]
    peer_assignments: dict[str, list[str]] = {}
    for assignment in re.finditer(
            r"\bassign\s+([A-Za-z_$][\w$]*)\s*=\s*([^;]+);|"
            r"\bwire\s+([A-Za-z_$][\w$]*)\s*=\s*([^;]+);", aggregator_body):
        lhs = assignment.group(1) or assignment.group(3)
        rhs = assignment.group(2) or assignment.group(4)
        peer_assignments.setdefault(lhs, []).append(re.sub(r"\s+", "", rhs))
    for peer, fault in (("read_peer_fault", "write_core_fault"),
                        ("write_peer_fault", "read_core_fault")):
        need(peer_assignments.get(peer) == [fault],
             f"duplex {peer} must be cross-connected to {fault}")
    core_instances = [instance for instance in verilog_instances(aggregator_body)
                      if instance[0] in {"MinimalAXI4MasterPipelined", "MinimalAXI4MasterPipelinedWrite"}]
    need(len(core_instances) == 2, "duplex aggregator must contain exactly one read and one write core")
    expected_core_ports = {
        "MinimalAXI4MasterPipelined": {"peer_fault": "read_peer_fault", "fault": "read_core_fault"},
        "MinimalAXI4MasterPipelinedWrite": {"peer_fault": "write_peer_fault", "fault": "write_core_fault"},
    }
    for module_name, instance_name, ports in core_instances:
        need(module_name in expected_core_ports,
             f"unexpected duplex core instance type {module_name}")
        for port, signal in expected_core_ports[module_name].items():
            need(ports.get(port) == signal,
                 f"duplex {instance_name} {port} port must connect to {signal}")


def verify_restrict_architecture_metadata(xml_root: ET.Element, top: str, raw_text: str,
                                          proof_record: dict[str, object] | None) -> None:
    expected = RESTRICT_METADATA_CASES.get(top)
    if expected is None:
        return
    functions = [node for node in xml_root.findall(".//function") if node.get("name") == top]
    need(len(functions) == 1, f"architecture XML must contain exactly one function named {top}")
    parameters = functions[0].find("./parameters")
    need(parameters is not None, f"architecture function {top} has no parameter metadata")
    by_port: dict[str, list[ET.Element]] = {}
    by_index: dict[int, list[ET.Element]] = {}
    for parameter in parameters.findall("./parameter"):
        port = parameter.get("port")
        index = parameter.get("index")
        if port:
            by_port.setdefault(port, []).append(parameter)
        if index is not None and index.isdecimal():
            by_index.setdefault(int(index), []).append(parameter)
    for port, expected_value in expected.items():
        entries = by_port.get(port, [])
        need(len(entries) == 1, f"expected exactly one architecture parameter {top}.{port}")
        need(entries[0].get("source_restrict") == expected_value,
             f"architecture source_restrict for {top}.{port} does not match source provenance")
    if proof_record is not None and proof_record.get("proof_kind") == "distinct_base_restricted_pointwise":
        nodes, func, _blocks, _reachable = function_graph(raw_text, top)
        args = [int(value) for value in fields(nodes[func]).get("arg", [])]
        read_owner = int(proof_record["read_owner_argument_index"])
        write_owner = int(proof_record["write_owner_argument_index"])
        need(read_owner in args and write_owner in args,
             "distinct proof owners are not both formal arguments in raw IR")
        read_parameter = by_index.get(args.index(read_owner), [])
        write_parameter = by_index.get(args.index(write_owner), [])
        need(len(read_parameter) == 1 and read_parameter[0].get("port") == "src" and
             read_parameter[0].get("source_restrict") == "1",
             "raw LOAD owner does not correlate to its restricted architecture parameter")
        need(len(write_parameter) == 1 and write_parameter[0].get("port") == "dst" and
             write_parameter[0].get("source_restrict") == "1",
             "raw STORE owner does not correlate to its restricted architecture parameter")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_file(path: Path, label: str) -> None:
    need(not path.is_symlink(), f"{label} must not be a symlink: {path}")
    need(path.is_file() and path.stat().st_size > 0, f"required non-empty {label} missing: {path}")


def parse_manifest(path: Path) -> dict[str, str]:
    safe_file(path, "provenance manifest")
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or "=" not in line:
            raise CheckError(f"malformed provenance line: {line!r}")
        key, value = line.split("=", 1)
        if key in fields or not value:
            raise CheckError(f"duplicate or empty provenance field: {key}")
        fields[key] = value
    required = {"top", "source", "source_sha256", "bambu", "bambu_sha256", "plugin", "plugin_sha256",
                "checker", "checker_sha256", "runner", "runner_sha256", "raw_ir", "raw_ir_sha256"}
    optional_post_fields = {"post_interface_raw", "post_interface_raw_sha256"}
    need(fields.keys() in (required, required | optional_post_fields),
         "provenance manifest fields are missing or unexpected")
    return fields


NODE_LINE = re.compile(r"^@(\d+) ([a-zA-Z0-9_]+)(?: (.*))?$")
REF = re.compile(r"(?:^|\s)([a-zA-Z_][a-zA-Z0-9_]*): @(\d+)")


def parse_raw(raw_text: str) -> dict[int, tuple[str, str]]:
    nodes: dict[int, tuple[str, str]] = {}
    for line in raw_text.splitlines():
        match = NODE_LINE.match(line)
        if match:
            index = int(match.group(1))
            need(index not in nodes, f"duplicate raw IR node @{index}")
            nodes[index] = (match.group(2), match.group(3) or "")
    need(bool(nodes), "raw IR contains no graph nodes")
    return nodes


def fields(node: tuple[str, str]) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for key, value in REF.findall(node[1]):
        result.setdefault(key, []).append(value)
    return result


def function_graph(raw_text: str, top: str) -> tuple[dict[int, tuple[str, str]], int, dict[int, tuple[int, str, list[int], list[str], list[str]]], set[int]]:
    nodes = parse_raw(raw_text)
    named = {index for index, (kind, attrs) in nodes.items()
             if kind == "identifier_node" and re.search(rf'\bstrg: "{re.escape(top)}"(?:\s|$)', attrs)}
    funcs = [index for index, (kind, attrs) in nodes.items()
             if kind == "function_val_node" and
             set(fields((kind, attrs)).get("name", [])) & {str(i) for i in named}]
    need(len(funcs) == 1, f"raw IR must contain exactly one selected function {top}")
    func = funcs[0]
    bodies = fields(nodes[func])["body"]
    need(len(bodies) == 1 and int(bodies[0]) in nodes and nodes[int(bodies[0])][0] == "statement_list_node",
         "selected function has no unique statement-list body")
    body_attrs = nodes[int(bodies[0])][1]
    block_matches = list(re.finditer(r"\bbloc: (\d+) loop_id: (\d+)(.*?)(?=\bbloc: \d+ loop_id: |$)",
                                     body_attrs))
    blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]] = {}
    for match in block_matches:
        block, loop_id = int(match.group(1)), int(match.group(2))
        stmt_ids = [int(value) for value in re.findall(r"\bstmt: @(\d+)", match.group(3))]
        preds = re.findall(r"\bpred: (ENTRY|\d+)", match.group(3))
        succs = re.findall(r"\bsucc: (EXIT|\d+)", match.group(3))
        need(block not in blocks, f"duplicate statement-list block {block}")
        blocks[block] = (loop_id, match.group(3), stmt_ids, preds, succs)
    need(bool(blocks), "selected function has no statement-list blocks")
    reachable: set[int] = set()
    pending = [func]
    while pending and len(reachable) < 1_000_000:
        index = pending.pop()
        if index in reachable or index not in nodes:
            continue
        reachable.add(index)
        node_fields = fields(nodes[index])
        for key in ("body", "name", "arg", "stmt", "phi", "op", "res", "def", "def_stmt"):
            pending.extend(int(ref) for ref in node_fields.get(key, []))
    return nodes, func, blocks, reachable


def node_in_function(nodes: dict[int, tuple[str, str]], func: int, reachable: set[int],
                     index: int, label: str) -> tuple[str, str]:
    need(index in nodes, f"{label} @{index} does not resolve in raw IR")
    node = nodes[index]
    parent_refs = fields(node).get("parent", [])
    need(index in reachable and node[0] not in {"function_val_node", "identifier_node", "statement_list_node"}
         and (not parent_refs or str(func) in parent_refs),
         f"{label} @{index} is not a node in selected function")
    return node


def statement_block(nodes: dict[int, tuple[str, str]], blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                    func: int, reachable: set[int], index: int, label: str) -> tuple[int, int, int]:
    node = node_in_function(nodes, func, reachable, index, label)
    attrs = node[1]
    bb_match = re.search(r"\bbb_index: (\d+)", attrs)
    need(bb_match is not None, f"{label} @{index} is not a statement with a block index")
    bb = int(bb_match.group(1))
    need(bb in blocks and index in blocks[bb][2], f"{label} @{index} is absent from block {bb}")
    return bb, blocks[bb][0], blocks[bb][2].index(index)


def operation_refs(node: tuple[str, str]) -> list[int]:
    return [int(value) for value in fields(node).get("op", [])]


def block_targets(node: tuple[str, str]) -> list[str]:
    return re.findall(r"\bbloc: (\d+)", node[1])


def conditional_branch(node: tuple[str, str], label: str) -> tuple[int, int, int]:
    conditions = operation_refs(node)
    targets = block_targets(node)
    need(node[0] == "multi_way_if_stmt" and len(conditions) == 1 and len(targets) == 2,
         f"{label} does not encode one true condition and one default branch")
    return conditions[0], int(targets[0]), int(targets[1])


def is_signed_32bit_value(nodes: dict[int, tuple[str, str]], index: int) -> bool:
    if index not in nodes:
        return False
    attrs = fields(nodes[index])
    type_ids = attrs.get("type", [])
    return (len(type_ids) == 1 and int(type_ids[0]) in nodes and
            nodes[int(type_ids[0])][0] == "integer_ty_node" and
            re.search(r"\bbitsize(?:alloc)?: 32(?:\s|$)", nodes[int(type_ids[0])][1]) is not None and
            not re.search(r"\bunsigned(?:\s|$)", nodes[int(type_ids[0])][1]))


def is_unsigned_32bit_value(nodes: dict[int, tuple[str, str]], index: int) -> bool:
    if index not in nodes:
        return False
    type_ids = fields(nodes[index]).get("type", [])
    return (len(type_ids) == 1 and int(type_ids[0]) in nodes and
            nodes[int(type_ids[0])][0] == "integer_ty_node" and
            re.search(r"\bbitsize(?:alloc)?: 32(?:\s|$)", nodes[int(type_ids[0])][1]) is not None and
            re.search(r"\bunsigned(?:\s|$)", nodes[int(type_ids[0])][1]) is not None)


def assignment_rhs(nodes: dict[int, tuple[str, str]], statement: tuple[str, str], label: str) -> int:
    operands = operation_refs(statement)
    need(statement[0] in {"assign_stmt", "gimple_assign"} and len(operands) == 2,
         f"{label} is not a two-operand assignment")
    return operands[1]


def operand_depends(nodes: dict[int, tuple[str, str]], root: int, target: int,
                    limit: int = 4096) -> bool:
    """Follow only value operands and SSA definitions, never ownership/use/type links."""
    pending, visited = [root], set()
    while pending and len(visited) < limit:
        index = pending.pop()
        if index == target:
            return True
        if index in visited or index not in nodes:
            continue
        visited.add(index)
        kind, _attrs = nodes[index]
        if kind == "ssa_node":
            defs = fields(nodes[index]).get("def_stmt", [])
            if len(defs) == 1 and int(defs[0]) in nodes:
                statement = nodes[int(defs[0])]
                if statement[0] in {"assign_stmt", "gimple_assign"}:
                    pending.extend(operation_refs(statement)[1:])
                elif statement[0] == "phi_stmt":
                    pending.extend(int(value) for value in fields(statement).get("def", []))
        elif kind in {"assign_stmt", "gimple_assign"}:
            pending.extend(operation_refs(nodes[index])[1:])
        else:
            pending.extend(operation_refs(nodes[index]))
    return False


def memory_operand(nodes: dict[int, tuple[str, str]], statement: tuple[str, str], role: str,
                   expected_width: int) -> tuple[int, int]:
    operands = operation_refs(statement)
    need(statement[0] in {"assign_stmt", "gimple_assign"} and len(operands) == 2,
         f"{role} is not a two-operand assignment")
    access_index = operands[1] if role == "LOAD" else operands[0]
    value_index = operands[0] if role == "LOAD" else operands[1]
    need(access_index in nodes and nodes[access_index][0] == "mem_access_node",
         f"{role} assignment has no memory access in the expected {'RHS' if role == 'LOAD' else 'LHS'} position")
    access_kind, access_attrs = nodes[access_index]
    need("ordinary_memory_access" in access_attrs and "volatile" not in access_attrs and "atomic" not in access_attrs,
         f"{role} is not an ordinary non-volatile memory access")
    type_refs = fields((access_kind, access_attrs)).get("type", [])
    need(len(type_refs) == 1 and int(type_refs[0]) in nodes,
         f"{role} has no unique access type")
    type_attrs = nodes[int(type_refs[0])][1]
    width = re.search(r"\b(?:bitsizealloc|bitsize): (\d+)", type_attrs)
    need(width is not None and int(width.group(1)) == expected_width,
         f"{role} access width is not the expected {expected_width} bits")
    address = operation_refs((access_kind, access_attrs))
    need(len(address) == 1, f"{role} has no unique address operand")
    return int(address[0]), value_index


def unsigned_32_ssa(nodes: dict[int, tuple[str, str]], index: int, label: str) -> None:
    node = nodes.get(index, ("", ""))
    need(node[0] == "ssa_node" and is_unsigned_32bit_value(nodes, index),
         f"{label} is not an unsigned-32 SSA value")


def comparison_expression(nodes: dict[int, tuple[str, str]], index: int,
                          label: str) -> tuple[str, tuple[int, ...]]:
    node = nodes.get(index, ("", ""))
    if node[0] == "ssa_node":
        definitions = fields(node).get("def_stmt", [])
        need(len(definitions) == 1 and int(definitions[0]) in nodes,
             f"{label} SSA has no unique definition")
        statement = nodes[int(definitions[0])]
        need(statement[0] in {"assign_stmt", "gimple_assign"}, f"{label} SSA definition is not an assignment")
        rhs = assignment_rhs(nodes, statement, label)
        node = nodes.get(rhs, ("", ""))
    operands = operation_refs(node)
    need(node[0] in {"lt_node", "gt_node", "eq_node"} and len(operands) == 2,
         f"{label} is not a supported binary comparison")
    return node[0], tuple(operands)


def verify_prefix_loop_trip_control(nodes: dict[int, tuple[str, str]],
                                    blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                                    record: dict[str, object], loop_id: int) -> tuple[int, int, int]:
    def statement_block_id(statement_id: int, label: str) -> int:
        statement = nodes.get(statement_id, ("", ""))
        block_index = re.search(r"\bbb_index: (\d+)", statement[1])
        need(statement[0] in {"assign_stmt", "gimple_assign"} and block_index is not None,
             f"{label} is not a block-indexed assignment")
        return int(block_index.group(1))

    header = int(record["read_header_bb"])
    preheader = int(record["read_preheader_bb"])
    exit_bb = int(record["write_exit_bb"])
    need(header in blocks and preheader in blocks and exit_bb in blocks,
         "prefix loop header, preheader, or exit block does not resolve")
    trip_bound = int(record["loop_trip_bound_ssa_index"])
    trip_compare = int(record["loop_trip_compare_index"])
    induction = int(record["read_induction_index"])
    unsigned_32_ssa(nodes, trip_bound, "loop trip bound")
    unsigned_32_ssa(nodes, induction, "prefix loop induction")
    compare_kind, compare_operands = comparison_expression(nodes, trip_compare, "loop trip comparison")
    need(compare_kind == "eq_node" and len(compare_operands) == 2,
         "prefix loop trip condition is not equality")

    induction_node = nodes.get(induction, ("", ""))
    induction_defs = fields(induction_node).get("def_stmt", [])
    need(induction_node[0] == "ssa_node" and len(induction_defs) == 1 and
         int(induction_defs[0]) in nodes and nodes[int(induction_defs[0])][0] == "phi_stmt",
         "prefix loop induction has no unique PHI definition")
    induction_phi = nodes[int(induction_defs[0])]
    phi_bb = re.search(r"\bbb_index: (\d+)", induction_phi[1])
    need(phi_bb is not None and int(phi_bb.group(1)) == header,
         "prefix loop induction PHI is not in the declared header")
    phi_incoming = re.findall(r"\bdef: @(\d+) edge: (\d+)", induction_phi[1])
    phi_by_edge = {int(edge): int(value) for value, edge in phi_incoming}
    need(len(phi_by_edge) == 2 and preheader in phi_by_edge,
         "prefix loop induction PHI does not have one entry and one backedge")
    latch_candidates = [edge for edge in phi_by_edge if edge != preheader]
    need(len(latch_candidates) == 1, "prefix loop induction PHI has an ambiguous backedge")
    latch = latch_candidates[0]
    need(latch in blocks and blocks[header][0] == loop_id and blocks[latch][0] == loop_id and
         blocks[preheader][0] == 0 and blocks[exit_bb][0] == 0,
         "prefix loop blocks do not match the declared header, preheader, latch, and exit")
    initial = nodes.get(phi_by_edge[preheader], ("", ""))
    need(initial[0] == "constant_int_val_node" and
         re.search(r"\bvalue: 0(?:\s|$)", initial[1]),
         "prefix loop induction PHI does not start at zero")
    increment = phi_by_edge[latch]
    unsigned_32_ssa(nodes, increment, "prefix loop increment")
    need(set(compare_operands) == {increment, trip_bound},
         "prefix loop trip comparison is not exact increment == trip bound")
    increment_node = nodes.get(increment, ("", ""))
    increment_defs = fields(increment_node).get("def_stmt", [])
    need(increment_node[0] == "ssa_node" and len(increment_defs) == 1 and
         int(increment_defs[0]) in nodes and statement_block_id(int(increment_defs[0]), "prefix induction increment") == latch and
         int(increment_defs[0]) in blocks[latch][2],
         "prefix induction increment is not defined in its recorded latch")
    increment_statement = nodes[int(increment_defs[0])]
    increment_expression = nodes.get(assignment_rhs(nodes, increment_statement,
                                                     "prefix induction increment"), ("", ""))
    increment_operands = operation_refs(increment_expression)
    need(increment_expression[0] == "add_node" and len(increment_operands) == 2 and
         induction in increment_operands and
         any(nodes.get(value, ("", ""))[0] == "constant_int_val_node" and
             re.search(r"\bvalue: 1(?:\s|$)", nodes[value][1]) for value in increment_operands),
         "prefix loop backedge is not the declared induction plus one")

    branch_matches = []
    for block, (block_loop_id, _attrs, statements, _preds, _succs) in blocks.items():
        if block_loop_id != loop_id:
            continue
        for statement_id in statements:
            statement = nodes.get(statement_id, ("", ""))
            if statement[0] != "multi_way_if_stmt":
                continue
            condition, true_target, default_target = conditional_branch(statement, "prefix trip branch")
            if condition == trip_compare:
                branch_matches.append((block, statement_id, statement, true_target, default_target))
    need(len(branch_matches) == 1,
         "recorded prefix trip comparison is not used by one unique loop branch")
    latch_bb, branch_id, _branch, true_target, default_target = branch_matches[0]
    need(latch_bb == latch,
         "prefix trip branch is not on the induction PHI backedge block")
    need(blocks[latch_bb][2][-1] == branch_id and
         sum(nodes[statement][0] == "multi_way_if_stmt" for statement in blocks[latch_bb][2]) == 1,
         "prefix trip branch is not the unique terminal branch in the latch")
    trip_condition_defs = fields(nodes.get(trip_compare, ("", ""))).get("def_stmt", [])
    need(len(trip_condition_defs) == 1 and int(trip_condition_defs[0]) in nodes and
         statement_block_id(int(trip_condition_defs[0]), "prefix trip comparison") == latch and
         int(trip_condition_defs[0]) in blocks[latch][2],
         "prefix trip comparison is not defined in its recorded latch")
    need(true_target == exit_bb and default_target == header,
         "prefix trip branch does not map true to exit and default to the loop backedge")
    need(set(blocks[latch_bb][4]) == {str(header), str(exit_bb)},
         "prefix trip branch CFG successors do not match its backedge and exit targets")
    need(set(blocks[header][3]) == {str(preheader), str(latch_bb)},
         "prefix loop header predecessors do not match entry and latch backedges")
    need(set(blocks[exit_bb][3]) == {str(preheader), str(latch_bb)},
         "prefix loop exit predecessors do not match zero-trip and latch exits")

    entry_branch_id = int(record["read_branch_index"])
    entry_branch = nodes.get(entry_branch_id, ("", ""))
    need(entry_branch[0] == "multi_way_if_stmt" and entry_branch_id in blocks[preheader][2],
         "prefix loop entry branch is not in the declared preheader")
    need(blocks[preheader][2][-1] == entry_branch_id and
         sum(nodes[statement][0] == "multi_way_if_stmt" for statement in blocks[preheader][2]) == 1,
         "prefix loop entry branch is not the unique terminal preheader branch")
    entry_condition, entry_true, entry_default = conditional_branch(entry_branch, "prefix loop entry branch")
    guard_id = int(record["read_guard_index"])
    need(entry_condition == guard_id and entry_true == exit_bb and entry_default == header,
         "prefix loop entry branch does not route zero-trip true to exit and nonzero default to header")
    need(set(blocks[preheader][4]) == {str(header), str(exit_bb)},
         "prefix loop preheader CFG successors do not match entry and zero-trip exit")
    guard_defs = fields(nodes.get(guard_id, ("", ""))).get("def_stmt", [])
    need(nodes.get(guard_id, ("", ""))[0] == "ssa_node" and len(guard_defs) == 1 and
         int(guard_defs[0]) in nodes and
         statement_block_id(int(guard_defs[0]), "prefix zero-trip guard") == preheader and
         int(guard_defs[0]) in blocks[preheader][2],
         "prefix zero-trip guard is not defined in its preheader")
    guard_kind, guard_operands = comparison_expression(nodes, guard_id, "prefix loop zero-trip guard")
    zero_operands = [value for value in guard_operands
                     if nodes.get(value, ("", ""))[0] == "constant_int_val_node" and
                     re.search(r"\bvalue: 0(?:\s|$)", nodes[value][1])]
    need(guard_kind == "eq_node" and len(guard_operands) == 2 and len(zero_operands) == 1 and
         set(guard_operands) == {trip_bound, zero_operands[0]},
         "prefix loop entry guard is not exact trip_bound == 0")
    return latch_bb, trip_compare, increment


def verify_unsigned_prefix_comparison(nodes: dict[int, tuple[str, str]], compare_id: int,
                                      induction: int, bound: int, label: str) -> None:
    compare_kind, operands = comparison_expression(nodes, compare_id, f"{label} comparison")
    exact_unsigned_less_than = ((compare_kind == "lt_node" and operands == (induction, bound)) or
                                (compare_kind == "gt_node" and operands == (bound, induction)))
    need(exact_unsigned_less_than,
         f"{label} effective comparison is not induction < its exact bound")
    unsigned_32_ssa(nodes, induction, f"{label} induction")
    unsigned_32_ssa(nodes, bound, f"{label} effective bound")
    induction_type = fields(nodes[induction]).get("type", [])
    bound_type = fields(nodes[bound]).get("type", [])
    need(induction_type and induction_type == bound_type,
         f"{label} induction and effective bound widths/types differ")


def block_reaches(blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                  start: int, target: int, loop_header: int) -> bool:
    pending = [start]
    visited: set[int] = set()
    while pending:
        current = pending.pop()
        if current == target:
            return True
        if current in visited or current == loop_header or current not in blocks:
            continue
        visited.add(current)
        pending.extend(int(value) for value in blocks[current][4] if value.isdigit())
    return False


def block_reaches_avoiding(blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                           start: int, target: int, loop_header: int, avoid_block: int) -> bool:
    pending = [(start, True)]
    visited: set[tuple[int, bool]] = set()
    while pending:
        current, first = pending.pop()
        state = (current, first)
        if state in visited or current == avoid_block or current not in blocks:
            continue
        visited.add(state)
        if current == target:
            return True
        if current == loop_header and not first:
            continue
        for successor in blocks[current][4]:
            if successor.isdigit():
                pending.append((int(successor), False))
    return False


def verify_direction_guard_dominance(blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                                     direction: str, loop_header: int, guard_bb: int,
                                     true_target: int, false_target: int, access_bb: int) -> None:
    need(block_reaches(blocks, true_target, access_bb, loop_header) and
         not block_reaches(blocks, false_target, access_bb, loop_header),
         f"{direction} memory access is not controlled by the true effective-prefix guard")
    need(not block_reaches_avoiding(blocks, loop_header, access_bb, loop_header, guard_bb),
         f"{direction} effective-prefix guard does not dominate memory access")


def verify_direction_count_proof(record: dict[str, object], nodes: dict[int, tuple[str, str]],
                                  blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                                  func: int, reachable: set[int], loop_header: int,
                                  access_blocks: dict[str, int], loop_comparison_index: int,
                                  loop_condition_index: int, loop_increment_index: int) -> None:
    need(COUNT_PROOF_FIELDS <= record.keys(), "semantic proof record is missing prefix-count proof fields")
    for key in COUNT_PROOF_FIELDS - {"read_count_recipe", "write_count_recipe", "pairing_domain_recipe"}:
        need(type(record[key]) is int, f"prefix-count proof field {key} is not numeric")
    trip_bound = int(record["loop_trip_bound_ssa_index"])
    unsigned_32_ssa(nodes, trip_bound, "loop trip bound")
    need(trip_bound == int(record["read_bound_index"]) == int(record["write_bound_index"]),
         "legacy bound identities do not alias the shared loop-trip bound")
    trip_compare_id = int(record["loop_trip_compare_index"])
    node_in_function(nodes, func, reachable, trip_compare_id, "loop trip comparison")
    trip_compare_kind, trip_compare_operands = comparison_expression(
        nodes, trip_compare_id, "loop trip comparison")
    need(trip_compare_id in {loop_comparison_index, loop_condition_index} and
         trip_compare_kind == "eq_node" and
         set(trip_compare_operands) == {loop_increment_index, trip_bound},
         "loop trip comparison is not the exact recorded loop-exit comparison")
    need(record["pairing_domain_recipe"] == "min_read_write_counts_prefix",
         "pairing domain is not the common read/write prefix")
    for direction in ("read", "write"):
        effective_bound = int(record[f"{direction}_effective_bound_ssa_index"])
        compare_id = int(record[f"{direction}_effective_compare_index"])
        need(int(record[f"{direction}_access_bb"]) == access_blocks[direction],
             f"{direction} access BB does not match the original memory operation")
        recipe = record[f"{direction}_count_recipe"]
        need(recipe in {"trip_count", "min_trip_and_effective_bound"},
             f"{direction} count recipe is unknown")
        unsigned_32_ssa(nodes, effective_bound, f"{direction} effective bound")
        if compare_id == 0:
            need(effective_bound == trip_bound and recipe == "trip_count",
                 f"unguarded {direction} count does not use the loop trip count")
            continue
        need(recipe == "min_trip_and_effective_bound",
             f"guarded {direction} count does not clamp to its effective bound")
        node_in_function(nodes, func, reachable, compare_id, f"{direction} effective comparison")
        induction = int(record[f"{direction}_induction_index"])
        verify_unsigned_prefix_comparison(nodes, compare_id, induction, effective_bound, direction)
        branches = []
        for block, (_loop, _attrs, statements, _preds, _succs) in blocks.items():
            for statement_id in statements:
                statement = nodes[statement_id]
                if statement[0] != "multi_way_if_stmt":
                    continue
                condition, true_target, false_target = conditional_branch(statement, f"{direction} prefix guard")
                condition_node = nodes.get(condition, ("", ""))
                condition_defs = fields(condition_node).get("def_stmt", []) if condition_node[0] == "ssa_node" else []
                rhs = None
                if len(condition_defs) == 1 and int(condition_defs[0]) in nodes:
                    rhs = assignment_rhs(nodes, nodes[int(condition_defs[0])], f"{direction} prefix guard")
                if rhs == compare_id or condition == compare_id:
                    branches.append((block, true_target, false_target))
        need(len(branches) == 1, f"{direction} effective comparison is not used by one exact CFG guard")
        guard_bb, true_target, false_target = branches[0]
        access_bb = access_blocks[direction]
        verify_direction_guard_dominance(blocks, direction, loop_header, guard_bb,
                                          true_target, false_target, access_bb)


def verify_configure_count_expressions(post_nodes: dict[str, tuple[str, str]], calls: dict,
                                       record: dict[str, object]) -> None:
    from check_duplex_activation_matrix import normalized_ssa_expression

    need("configure_read" in calls and "configure_write" in calls,
         "post-InterfaceInfer IR lacks read/write CONFIGURE calls")
    trip = str(int(record["loop_trip_bound_ssa_index"]))
    need(f"@{trip}" in post_nodes, "proof loop-trip bound does not resolve in post-InterfaceInfer IR")
    trip_expression = normalized_ssa_expression(post_nodes, f"@{trip}")
    zero_expression = ("constant_int_val_node", "0")
    expressions = {}
    for direction in ("read", "write"):
        bound = str(int(record[f"{direction}_effective_bound_ssa_index"]))
        guard = str(int(record[f"{direction}_guard_index"]))
        need(f"@{bound}" in post_nodes and f"@{guard}" in post_nodes,
             f"post-InterfaceInfer IR lacks the {direction} proof bound or entry guard")
        bound_expression = normalized_ssa_expression(post_nodes, f"@{bound}")
        guard_expression = normalized_ssa_expression(post_nodes, f"@{guard}")
        active_guard_expression = guard_expression
        guard_operands = guard_expression[1] if len(guard_expression) == 2 else ()
        is_zero_trip_guard = (guard_expression[0] == "eq_node" and len(guard_operands) == 2 and
                              ((guard_operands[0] == trip_expression and guard_operands[1] == zero_expression) or
                               (guard_operands[1] == trip_expression and guard_operands[0] == zero_expression)))
        if is_zero_trip_guard:
            active_guard_expression = ("ne_node", guard_operands)
        compare_index = int(record[f"{direction}_effective_compare_index"])
        if compare_index == 0:
            need(record[f"{direction}_count_recipe"] == "trip_count" and bound == trip,
                 f"unguarded {direction} CONFIGURE recipe is not the loop trip count")
            selected_count = trip_expression
        else:
            need(record[f"{direction}_count_recipe"] == "min_trip_and_effective_bound",
                 f"guarded {direction} CONFIGURE recipe omits the min clamp")
            minimum = ("select_node", (("lt_node", (trip_expression, bound_expression)),
                                        trip_expression, bound_expression))
            selected_count = minimum
        expressions[direction] = ("select_node", (active_guard_expression, selected_count, zero_expression))
    for direction in ("read", "write"):
        call_args = calls[f"configure_{direction}"][2]
        need(len(call_args) == 5, f"post-InterfaceInfer CONFIGURE_{direction.upper()} has the wrong arity")
        actual = normalized_ssa_expression(post_nodes, str(call_args[4]))
        need(actual == expressions[direction],
             f"post-InterfaceInfer CONFIGURE_{direction.upper()} count does not implement its recorded recipe")


def verify_emitted_configure_counts(output: Path, top: str,
                                    record: dict[str, object]) -> None:
    post_raw = output / "panda-temp/after_Frontend::InterfaceInfer.raw"
    safe_file(post_raw, "post-InterfaceInfer raw IR")
    need(not post_raw.parent.is_symlink(), "post-InterfaceInfer raw IR parent must not be a symlink")
    need(os.path.commonpath((str(post_raw.resolve()), str(output.resolve()))) == str(output.resolve()),
         "post-InterfaceInfer raw IR escapes compiler output directory")
    manifest = parse_manifest(output / "provenance.txt")
    need(Path(manifest.get("post_interface_raw", "")).resolve() == post_raw.resolve() and
         manifest.get("post_interface_raw_sha256") == sha256(post_raw),
         "post-InterfaceInfer raw IR is not anchored by the case provenance")
    from check_duplex_activation_matrix import require_raw, root_call_records

    post_nodes = require_raw(post_raw, post_raw.name)
    calls, _blocks = root_call_records(post_raw, top)
    verify_configure_count_expressions(post_nodes, calls, record)


def verify_success_record(record: dict[str, object], raw_text: str, top: str) -> None:
    need(IDENTITY_FIELDS <= record.keys(), "semantic proof record is missing identity fields")
    for key in IDENTITY_FIELDS:
        need(type(record[key]) is int, f"semantic identity field {key} is not numeric")
    proof_kind = record.get("proof_kind")
    need(proof_kind in {"same_base_pointwise", "distinct_base_restricted_pointwise"},
         "semantic proof record has an unknown proof kind")
    distinct_bases = proof_kind == "distinct_base_restricted_pointwise"
    read_owner = int(record["read_owner_argument_index"])
    write_owner = int(record["write_owner_argument_index"])
    read_base_index = int(record["read_base_ssa_index"])
    write_base_index = int(record["write_base_ssa_index"])
    need(int(record["owner_argument_index"]) == read_owner,
         "legacy owner identity does not match the LOAD owner")
    nodes, func, blocks, reachable = function_graph(raw_text, top)
    for label, owner, base_index in (("LOAD", read_owner, read_base_index),
                                     ("STORE", write_owner, write_base_index)):
        owner_node = node_in_function(nodes, func, reachable, owner, f"{label} owner argument")
        need(owner_node[0] == "argument_val_node", f"{label} owner identity is not a function argument")
        base = node_in_function(nodes, func, reachable, base_index, f"{label} base SSA")
        need(base[0] == "ssa_node", f"{label} base identity is not SSA")
        base_var = fields(base).get("var", [])
        need(str(owner) in base_var,
             f"{label} base SSA does not name its declared owner argument directly")
    need(int(record["base_ssa_index"]) == read_base_index,
         "legacy base identity does not match the LOAD base")
    need((read_owner != write_owner and read_base_index != write_base_index) if distinct_bases else
         (read_owner == write_owner and read_base_index == write_base_index),
         "proof kind does not match the declared owner/base identities")

    load_index, store_index = int(record["load_statement_index"]), int(record["store_statement_index"])
    load = node_in_function(nodes, func, reachable, load_index, "LOAD statement")
    store = node_in_function(nodes, func, reachable, store_index, "STORE statement")
    need(load[0] in {"assign_stmt", "gimple_assign"} and store[0] in {"assign_stmt", "gimple_assign"},
         "declared LOAD/STORE are not assignment statements")
    load_bb, load_loop, load_pos = statement_block(nodes, blocks, func, reachable, load_index, "LOAD")
    store_bb, store_loop, store_pos = statement_block(nodes, blocks, func, reachable, store_index, "STORE")
    need(load_loop != 0 and load_loop == store_loop,
         "LOAD and STORE are not in the same original loop block or common loop")
    if top in PREFIX_COUNT_CASES:
        need(COUNT_PROOF_FIELDS <= record.keys(), "semantic proof record is missing prefix-count proof fields")
        need(int(record["read_access_bb"]) == load_bb and int(record["write_access_bb"]) == store_bb,
             "prefix proof access-block identities do not match original LOAD/STORE blocks")
    else:
        need(load_bb == store_bb, "LOAD and STORE are not in the same original loop block")
        need(load_pos < store_pos, "LOAD does not precede STORE in the original iteration")
    load_address, load_value = memory_operand(nodes, load, "LOAD", 32)
    store_address, store_value = memory_operand(nodes, store, "STORE", 32)
    need((load_address != store_address) if distinct_bases else (load_address == store_address),
         "LOAD and STORE address identity does not match the proof kind")
    need(operand_depends(nodes, store_value, load_value),
         "STORE value is not derived from the preceding LOAD result")

    def address_gep(address_index: int, label: str) -> tuple[int, int]:
        address = node_in_function(nodes, func, reachable, address_index, f"{label} memory address")
        need(address[0] == "ssa_node", f"{label} memory address is not an SSA value")
        address_defs = fields(address).get("def_stmt", [])
        need(len(address_defs) == 1 and int(address_defs[0]) in nodes,
             f"{label} memory address has no unique defining assignment")
        address_def = nodes[int(address_defs[0])]
        address_expr = assignment_rhs(nodes, address_def, f"{label} memory address")
        gep = node_in_function(nodes, func, reachable, address_expr, f"{label} memory address expression")
        gep_ops = operation_refs(gep)
        need(gep[0] == "gep_node" and len(gep_ops) == 2,
             f"{label} memory address is not a canonical base-plus-index GEP")
        return gep_ops[0], gep_ops[1]

    gep_base, gep_offset = address_gep(load_address, "LOAD")
    need(gep_base == read_base_index,
         "LOAD address does not use the declared source base as its direct GEP base")
    write_gep_base, write_gep_offset = address_gep(store_address, "STORE")
    need(write_gep_base == write_base_index,
         "STORE address does not use the declared destination base as its direct GEP base")
    induction_id = int(record["read_induction_index"])
    def is_induction_byte_offset(offset_index: int, label: str) -> bool:
        offset = node_in_function(nodes, func, reachable, offset_index, f"{label} byte offset")
        offset_defs = fields(offset).get("def_stmt", []) if offset[0] == "ssa_node" else []
        need(len(offset_defs) == 1 and int(offset_defs[0]) in nodes,
             f"{label} byte offset has no unique defining assignment")
        offset_expr = nodes[int(offset_defs[0])]
        offset_rhs = assignment_rhs(nodes, offset_expr, f"{label} byte offset")
        scale = node_in_function(nodes, func, reachable, offset_rhs, f"{label} byte offset expression")
        scale_ops = operation_refs(scale)
        if scale[0] == "mul_node" and len(scale_ops) == 2:
            return ((scale_ops[0] == induction_id and
                     nodes.get(scale_ops[1], ("", ""))[0] == "constant_int_val_node" and
                     re.search(r"\bvalue: 4(?:\s|$)", nodes[scale_ops[1]][1])) or
                    (scale_ops[1] == induction_id and
                     nodes.get(scale_ops[0], ("", ""))[0] == "constant_int_val_node" and
                     re.search(r"\bvalue: 4(?:\s|$)", nodes[scale_ops[0]][1])))
        if scale[0] == "shl_node" and len(scale_ops) == 2:
            return (scale_ops[0] == induction_id and
                    nodes.get(scale_ops[1], ("", ""))[0] == "constant_int_val_node" and
                    re.search(r"\bvalue: 2(?:\s|$)", nodes[scale_ops[1]][1]))
        return False

    need(is_induction_byte_offset(gep_offset, "LOAD"),
         "LOAD byte offset is not the declared induction scaled by four bytes")
    if distinct_bases:
        need(is_induction_byte_offset(write_gep_offset, "STORE"),
             "STORE byte offset is not the declared induction scaled by four bytes")

    common = ("read_preheader_bb", "read_header_bb", "read_guard_index", "read_bound_index", "read_induction_index")
    loop_branch: tuple[str, str] | None = None
    loop_comparison: tuple[str, str] | None = None
    loop_comparison_index = 0
    prefix_latch_bb: int | None = None
    for side in ("read", "write"):
        header = int(record[f"{side}_header_bb"])
        preheader = int(record[f"{side}_preheader_bb"])
        need(header in blocks and preheader in blocks, f"{side} loop block identity does not resolve")
        need(blocks[header][0] != 0, f"{side} header is not marked as a loop block")
        need(blocks[header][0] == load_loop,
             f"{side} header does not identify the memory operations' canonical loop")
        if side == "read":
            branch_index = int(record["read_branch_index"])
            branch = node_in_function(nodes, func, reachable, branch_index, "read branch")
            need(branch[0] == "multi_way_if_stmt", "read branch identity is not a branch statement")
            branch_bb, branch_loop, _ = statement_block(nodes, blocks, func, reachable, branch_index, "read branch")
            need(branch_bb == preheader and branch_loop == blocks[preheader][0],
                 "read guard branch is not in the declared preheader")
            guard_id = int(record["read_guard_index"])
            bound_id = int(record["read_bound_index"])
            induction_id = int(record["read_induction_index"])
            branch_condition, _true_target, _default_target = conditional_branch(branch, "read entry branch")
            need(branch_condition == guard_id,
                 "read branch does not consume the declared guard")
            need(operand_depends(nodes, guard_id, bound_id),
                 "read guard does not depend on the declared bound")
            if top in PREFIX_COUNT_CASES:
                prefix_latch_bb, loop_comparison_index, _prefix_increment = verify_prefix_loop_trip_control(
                    nodes, blocks, record, load_loop)
                loop_condition = loop_comparison_index
                trip_condition_defs = fields(nodes[loop_comparison_index]).get("def_stmt", [])
                need(len(trip_condition_defs) == 1 and int(trip_condition_defs[0]) in nodes,
                     "prefix trip condition has no unique comparison definition")
                loop_comparison = nodes[assignment_rhs(
                    nodes, nodes[int(trip_condition_defs[0])], "prefix trip condition")]
                latch_branches = [statement for statement in blocks[prefix_latch_bb][2]
                                  if nodes[statement][0] == "multi_way_if_stmt" and
                                  conditional_branch(nodes[statement], "prefix trip branch")[0] ==
                                  loop_comparison_index]
                need(len(latch_branches) == 1, "prefix latch has no unique recorded trip branch")
                loop_branch = nodes[latch_branches[0]]
            else:
                loop_branches = [statement for statement in blocks[header][2]
                                 if nodes[statement][0] == "multi_way_if_stmt"]
                need(len(loop_branches) == 1, "loop header has no unique conditional exit branch")
                loop_branch = nodes[loop_branches[0]]
                loop_condition = operation_refs(loop_branch)
                need(len(loop_condition) == 1, "loop branch has no unique condition operand")
                condition = node_in_function(nodes, func, reachable, loop_condition[0], "loop condition")
                condition_defs = fields(condition).get("def_stmt", []) if condition[0] == "ssa_node" else []
                need(len(condition_defs) == 1 and int(condition_defs[0]) in nodes,
                     "loop condition has no unique defining assignment")
                loop_comparison_index = assignment_rhs(nodes, nodes[int(condition_defs[0])], "loop condition")
                comparison = node_in_function(nodes, func, reachable, loop_comparison_index, "loop comparison")
                comparison_operands = operation_refs(comparison)
                need(comparison[0] == "eq_node" and
                     len(comparison_operands) == 2 and
                     any(operand_depends(nodes, value, induction_id) for value in comparison_operands) and
                     any(operand_depends(nodes, value, bound_id) for value in comparison_operands),
                     "loop comparison is not equality over the declared bound and induction")
                loop_comparison = comparison
        guard = node_in_function(nodes, func, reachable, int(record[f"{side}_guard_index"]), f"{side} guard")
        bound = node_in_function(nodes, func, reachable, int(record[f"{side}_bound_index"]), f"{side} bound")
        induction = node_in_function(nodes, func, reachable, int(record[f"{side}_induction_index"]), f"{side} induction")
        need(guard[0] in {"ssa_node", "assign_stmt"}, f"{side} guard has an unexpected node kind")
        need(bound[0] in {"ssa_node", "argument_val_node", "constant_int_val_node"},
             f"{side} bound has an unexpected node kind")
        need(induction[0] == "ssa_node", f"{side} induction is not SSA")
        need(blocks[header][0] == load_loop, f"{side} loop context differs from memory operations")
    read_context = tuple(record[key] for key in common)
    write_context = (record["write_preheader_bb"], record["write_header_bb"], record["write_guard_index"],
                     record["write_bound_index"], record["write_induction_index"])
    need(read_context == write_context, "read and write proof regions do not share canonical loop context")
    preheader = int(record["read_preheader_bb"])
    header = int(record["read_header_bb"])
    exit_bb = int(record["write_exit_bb"])
    if top not in PREFIX_COUNT_CASES:
        need(str(preheader) in blocks[header][3] and str(header) in blocks[preheader][4],
             "declared preheader is not an actual predecessor/entry edge of the loop header")
        need(set(blocks[header][3]) == {str(preheader), str(header)},
             "loop header has unexpected CFG predecessors")
        need(str(header) in blocks[header][4] and str(header) in blocks[header][3],
             "loop header does not have the required self-edge")
        need(exit_bb in blocks and str(exit_bb) in blocks[header][4] and
             str(header) in blocks[exit_bb][3] and exit_bb != header,
             "declared write exit is not an actual loop-header successor with a matching predecessor")
        need(set(blocks[header][4]) == {str(header), str(exit_bb)},
             "loop header has unexpected CFG successors")
        need(set(blocks[preheader][4]) == {str(header), str(exit_bb)},
             "preheader entry branch does not have the declared header and exit CFG edges")
        entry_branches = [statement for statement in blocks[preheader][2]
                          if nodes[statement][0] == "multi_way_if_stmt"]
        need(len(entry_branches) == 1, "preheader has no unique loop-entry branch")
        entry_condition, entry_true_target, entry_default_target = conditional_branch(
            nodes[entry_branches[0]], "preheader branch")
        need(entry_condition == int(record["read_guard_index"]),
             "preheader branch condition differs from the declared signed-bound guard")
        need(entry_true_target == header and entry_default_target == exit_bb,
             "preheader branch polarity must map true to loop entry and default/false to exit")
        loop_condition, loop_true_target, loop_default_target = conditional_branch(loop_branch, "loop branch")
        need(loop_true_target == exit_bb and loop_default_target == header,
             "loop branch polarity must map true to exit and default/false to the backedge")
    comparison_operands = operation_refs(loop_comparison)
    induction_id = int(record["read_induction_index"])
    increment_defs = fields(nodes[induction_id]).get("def_stmt", [])
    need(len(increment_defs) == 1 and int(increment_defs[0]) in nodes and
         nodes[int(increment_defs[0])][0] == "phi_stmt",
         "declared induction has no unique PHI definition")
    phi_for_increment = nodes[int(increment_defs[0])]
    phi_incoming = re.findall(r"\bdef: @(\d+) edge: (\d+)", phi_for_increment[1])
    phi_by_edge = {int(edge): int(value) for value, edge in phi_incoming}
    backedge = prefix_latch_bb if top in PREFIX_COUNT_CASES else header
    increment_id = phi_by_edge.get(backedge)
    need(increment_id is not None and set(comparison_operands) ==
         {increment_id, int(record["read_bound_index"])},
         "loop predicate is not exactly increment == declared bound")
    if top not in PREFIX_COUNT_CASES:
        guard_defs = fields(nodes[entry_condition]).get("def_stmt", [])
        need(len(guard_defs) == 1 and int(guard_defs[0]) in nodes,
             "entry guard has no unique defining assignment")
        guard_comparison = nodes[assignment_rhs(nodes, nodes[int(guard_defs[0])], "entry guard")]
        guard_operands = operation_refs(guard_comparison)
        need(guard_comparison[0] == "gt_node" and len(guard_operands) == 2 and
             nodes.get(guard_operands[1], ("", ""))[0] == "constant_int_val_node" and
             re.search(r"\bvalue: 0(?:\s|$)", nodes[guard_operands[1]][1]) and
             is_signed_32bit_value(nodes, guard_operands[0]) and
             is_signed_32bit_value(nodes, guard_operands[1]),
             "entry predicate is not signed declared-bound > 0")
        signed_bound = guard_operands[0]
        signed_bound_defs = fields(nodes[signed_bound]).get("def_stmt", [])
        need(nodes[signed_bound][0] == "ssa_node" and len(signed_bound_defs) == 1 and
             int(signed_bound_defs[0]) in nodes,
             "entry signed-bound SSA has no unique defining assignment")
        cast_assignment = nodes[int(signed_bound_defs[0])]
        cast_operands = operation_refs(cast_assignment)
        need(cast_assignment[0] in {"assign_stmt", "gimple_assign"} and len(cast_operands) == 2 and
             cast_operands[0] == signed_bound and cast_operands[1] in nodes and
             nodes[cast_operands[1]][0] == "nop_node" and
             operation_refs(nodes[cast_operands[1]]) == [int(record["read_bound_index"])],
             "entry bound conversion is not a single nop of the declared bound")
        signed_type = fields(nodes[signed_bound]).get("type", [])
        conversion_type = fields(nodes[cast_operands[1]]).get("type", [])
        need(len(signed_type) == 1 and conversion_type == signed_type and
             is_signed_32bit_value(nodes, cast_operands[1]) and
             is_unsigned_32bit_value(nodes, int(record["read_bound_index"])),
             "entry bound conversion type is not unsigned32 to the matching signed32 guard type")
    body_node = nodes[int(fields(nodes[func])["body"][0])]
    phis = [int(value) for value in fields(body_node).get("phi", [])]
    induction_phis = [nodes[index] for index in phis if index in nodes and
                      nodes[index][0] == "phi_stmt" and
                      fields(nodes[index]).get("res", []) == [str(induction_id)]]
    need(len(induction_phis) == 1, "loop header lacks a unique induction PHI")
    phi = induction_phis[0]
    incoming = re.findall(r"\bdef: @(\d+) edge: (\d+)", phi[1])
    need(len(incoming) == 2, "induction PHI does not have exactly preheader and backedge values")
    by_edge = {int(edge): int(value) for value, edge in incoming}
    backedge = prefix_latch_bb if top in PREFIX_COUNT_CASES else header
    need(len(by_edge) == 2 and set(by_edge) == {preheader, backedge},
         "induction PHI edges do not match the declared preheader and loop backedge")
    initial = nodes.get(by_edge[preheader], ("", ""))
    need(initial[0] == "constant_int_val_node" and re.search(r"\bvalue: 0(?:\s|$)", initial[1]),
         "induction PHI does not start at zero on the preheader edge")
    increment = nodes.get(by_edge[backedge], ("", ""))
    increment_defs = fields(increment).get("def_stmt", []) if increment[0] == "ssa_node" else []
    need(len(increment_defs) == 1 and int(increment_defs[0]) in nodes,
         "induction PHI backedge is not a uniquely defined SSA increment")
    increment_rhs = nodes[int(increment_defs[0])]
    increment_node = nodes.get(assignment_rhs(nodes, increment_rhs, "induction increment"), ("", ""))
    increment_ops = operation_refs(increment_node)
    need(increment_node[0] == "add_node" and len(increment_ops) == 2 and
         any(value == induction_id for value in increment_ops) and
         any(nodes.get(value, ("", ""))[0] == "constant_int_val_node" and
             re.search(r"\bvalue: 1(?:\s|$)", nodes[value][1]) for value in increment_ops),
         "induction PHI backedge does not increment by one")
    verify_direction_count_proof(record, nodes, blocks, func, reachable, header,
                                 {"read": load_bb, "write": store_bb},
                                 loop_comparison_index,
                                 loop_condition, increment_id)


def parse_proof_records(stderr: str) -> list[dict[str, object]]:
    lines = [line.strip() for line in stderr.splitlines() if PROOF_PREFIX in line]
    records: list[dict[str, object]] = []
    for line in lines:
        need(line.startswith(PROOF_PREFIX), "duplex proof record is not a standalone debug record")
        try:
            def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
                result: dict[str, object] = {}
                for key, value in pairs:
                    need(key not in result, f"duplicate JSON field in duplex proof record: {key}")
                    result[key] = value
                return result

            value = json.loads(line[len(PROOF_PREFIX):], object_pairs_hook=unique_object)
        except json.JSONDecodeError as exc:
            raise CheckError(f"malformed duplex proof JSON record: {exc}") from exc
        need(isinstance(value, dict), "duplex proof JSON record is not an object")
        records.append(value)
    return records


def verify(output: Path, top: str, source: Path, bambu: Path, plugin: Path,
           checker: Path, runner: Path) -> None:
    need(top in CASES, f"unknown case: {top}")
    need(not output.is_symlink() and output.is_dir(), f"invalid output directory: {output}")
    manifest = parse_manifest(output / "provenance.txt")
    raw_path = Path(manifest["raw_ir"])
    expected_paths = {"source": source, "bambu": bambu, "plugin": plugin,
                      "checker": checker, "runner": runner}
    need(manifest["top"] == top, "provenance top does not match selected case")
    for name, path in expected_paths.items():
        safe_file(path, f"{name} provenance input")
        need(Path(manifest[name]).resolve() == path.resolve(), f"wrong {name} provenance path")
        need(manifest[f"{name}_sha256"] == sha256(path), f"wrong {name} provenance digest")
    safe_file(raw_path, "raw pre-InterfaceInfer IR")
    need(not raw_path.parent.is_symlink(), "raw IR parent directory must not be a symlink")
    need(manifest["raw_ir_sha256"] == sha256(raw_path), "wrong raw IR provenance digest")
    need(os.path.commonpath((str(raw_path.resolve()), str(output.resolve()))) == str(output.resolve()),
         "raw IR path escapes compiler output directory")
    need(raw_path.name == "before_Frontend::InterfaceInfer.raw" and raw_path.parent.name == "panda-temp",
         "raw IR input is not the pre-InterfaceInfer graph dump")

    rtl_path = output / f"{top}.v"
    xml_path = output / "panda-temp/architecture.xml"
    stderr_path = output / "stderr.log"
    stdout_path = output / "stdout.log"
    for artifact, label in ((rtl_path, "top RTL"), (xml_path, "architecture XML"),
                            (stderr_path, "compiler stderr log")):
        safe_file(artifact, label)
    rtl = rtl_path.read_text(encoding="utf-8", errors="replace")
    stderr = stderr_path.read_text(encoding="utf-8", errors="replace")
    stdout = stdout_path.read_text(encoding="utf-8", errors="replace") if stdout_path.is_file() else ""
    trace_lines = [line for line in stderr.splitlines() if "AXI_BURST_PREANALYSIS" in line]
    matches = list(TRACE.finditer(stderr))
    (expected_reason, expected_count, expected_read_fallback, expected_write_fallback,
     expected_semantic, expected_profile_eligible) = CASES[top]
    need(len(trace_lines) == len(matches), "malformed or ambiguous preanalysis trace line")
    need(len(matches) == expected_count,
         f"expected exactly {expected_count} preanalysis trace record(s), found {len(matches)}")
    owners: set[str] = set()
    access_counts: list[tuple[int, int]] = []
    for match in matches:
        (bundle, owner, reads, writes, read_requested, read_recognized, read_fallback,
         write_requested, write_recognized, write_fallback, candidate, reason,
         production_activated, activation_reason) = match.groups()
        need(bundle == "shared", f"trace names unexpected bundle {bundle}")
        need(owner not in owners, f"duplicate trace owner {owner}")
        owners.add(owner)
        access_counts.append((int(reads), int(writes)))
        expected_read_recognized, expected_write_recognized = expected_owner_recognition(
            top, int(reads), int(writes))
        need((read_requested, read_recognized, read_fallback) ==
             ("1", expected_read_recognized, expected_read_fallback),
             "read request/recognition/fallback does not match the exact preanalysis contract")
        need((write_requested, write_recognized, write_fallback) ==
             ("1", expected_write_recognized, expected_write_fallback),
             "write request/recognition/fallback does not match the exact preanalysis contract")
        expected_candidate = "1" if expected_semantic else "0"
        need(candidate == expected_candidate and reason == expected_reason,
             f"candidate outcome/reason mismatch: got {candidate}/{reason!r}")
        expected_activation = "1" if top in ACTIVATED_CASES else "0"
        need(production_activated == expected_activation,
             f"production activation mismatch: expected {expected_activation}, got {production_activated}")
        need(bool(activation_reason), "preanalysis trace has an empty activation reason")
        if expected_activation == "1":
            need(activation_reason == "frozen application-wide proof and profile accepted",
                 "activated trace lacks the frozen proof/profile acceptance reason")
    need(sorted(access_counts) == sorted(ACCESS_COUNTS[top]),
         f"classified access counts do not match the selected case: {access_counts!r}")

    proof_records = parse_proof_records(stderr)
    need(len(proof_records) == 1,
         "expected exactly one structured duplex proof record for the bundle")
    proof_by_owner: dict[int, dict[str, object]] = {}
    semantic_record: dict[str, object] | None = None
    for record in proof_records:
        required = {"bundle", "semantic_proof", "semantic_reason", "profile_eligible", "profile_reason"}
        need(required <= record.keys(), "duplex proof record is missing required contract fields")
        need(record["bundle"] == "shared", "duplex proof record names an unexpected bundle")
        need(type(record["semantic_proof"]) is bool and type(record["profile_eligible"]) is bool,
             "semantic/profile proof flags must be booleans")
        need(isinstance(record["semantic_reason"], str) and isinstance(record["profile_reason"], str),
             "semantic/profile reasons must be strings")
        semantic = bool(record["semantic_proof"])
        need(semantic == expected_semantic and record["semantic_reason"] == expected_reason,
             "structured semantic proof outcome/reason mismatch")
        if semantic:
            need(IDENTITY_FIELDS <= record.keys(), "semantic proof record is missing identity fields")
            expected_kind = ("distinct_base_restricted_pointwise" if expected_reason == DISTINCT_SUCCESS else
                             "same_base_pointwise")
            need(record.get("proof_kind") == expected_kind,
                 "structured proof kind does not match the selected candidate")
        else:
            need(not (IDENTITY_FIELDS & record.keys()) and "proof_kind" not in record,
                 "failed candidate exposes success-only identity fields")
        if semantic:
            owner_id = int(record["owner_argument_index"])
            need(str(owner_id) in owners, "structured semantic proof owner has no matching preanalysis trace")
            need(owner_id not in proof_by_owner, f"duplicate structured proof owner {owner_id}")
            proof_by_owner[owner_id] = record
            semantic_record = record
            verify_success_record(record, raw_path.read_text(encoding="utf-8", errors="replace"), top)
            if top in ACTIVATED_CASES:
                verify_emitted_configure_counts(output, top, record)
        need(record["profile_eligible"] is expected_profile_eligible,
             "structured profile eligibility does not match the selected profile")
        expected_profile_reason = "cache-enabled bundle" if top == "same_base_cached" else \
            "read/write profile is eligible"
        need(record["profile_reason"] == expected_profile_reason,
             "structured profile reason does not match the selected profile")

    root = ET.parse(xml_path).getroot()
    verify_restrict_architecture_metadata(root, top, raw_path.read_text(encoding="utf-8", errors="replace"),
                                           semantic_record)
    functions = [node for node in root.findall(".//function") if node.get("name") == top]
    need(len(functions) == 1, f"architecture XML must contain exactly one function named {top}")
    bundles = [node for node in functions[0].findall("./bundles/bundle") if node.get("name") == "shared"]
    need(len(bundles) == 1, "architecture XML must retain exactly one shared bundle")
    attrs = bundles[0].attrib
    read_burst, read_outstanding, write_burst, write_outstanding = EXPECTED_PROFILES.get(
        top, ("16", "1", "16", "1"))
    need(attrs.get("mode") == "m_axi" and attrs.get("max_read_burst_length") == read_burst and
         attrs.get("max_write_burst_length") == write_burst and
         attrs.get("write_burst_outstanding") == write_outstanding and
         attrs.get("num_read_outstanding", "1") == read_outstanding,
         "architecture XML does not preserve requested read/write m_axi profiles")
    if top in ACTIVATED_CASES:
        need(expected_semantic and expected_profile_eligible,
             "production activation requires semantic proof and an eligible profile")
        check_production_duplex(rtl, stdout + "\n" + stderr, top, attrs)
    else:
        need(not BURST_COMPONENTS.search(rtl), "analysis-only case unexpectedly emits a burst engine")
        need(not BURST_OPS.search(rtl), "analysis-only case unexpectedly emits a burst ABI operation")
        need(LEGACY_COMPONENT.search(rtl), "analysis-only case does not retain a legacy memory resource")
    print(f"DUPLEX EFFECT ANALYSIS CHECK PASS CASE={top} SEMANTIC={1 if expected_semantic else 0} "
          f"PROFILE={1 if expected_profile_eligible else 0} ACTIVATED={1 if top in ACTIVATED_CASES else 0}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("top", choices=tuple(CASES))
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--bambu", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    parser.add_argument("--checker", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        verify(args.output, args.top, args.source, args.bambu, args.plugin, args.checker, args.runner)
    except (OSError, ET.ParseError, CheckError, ValueError) as exc:
        print(f"DUPLEX EFFECT ANALYSIS CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
