#!/usr/bin/env python3
"""Streaming, fail-closed VCD qualification for the burst T4B workflow.

JSON schema (version 1): ``schema_version``, ``mode``, ``scope``, ``status``,
``expected`` and ``metrics`` are always present on success. Metric values are
integers, ``null`` when a metric does not apply, or a nested histogram object.
Invalid
or incomplete traces exit non-zero and emit an ``INVALID`` JSON document when
``--json-out`` can be written.
"""

import argparse
import json
import re
import sys
from collections import deque
from pathlib import Path


CHANNEL = ("m_axi_arvalid", "m_axi_arready", "m_axi_araddr",
           "m_axi_arlen", "m_axi_arsize", "m_axi_arburst", "m_axi_arid",
           "m_axi_rvalid", "m_axi_rready", "m_axi_rlast",
           "m_axi_rid", "m_axi_rresp")
ENGINE = ("clock", "m_axi_arvalid", "m_axi_arready", "m_axi_araddr_reg",
          "m_axi_arlen_reg", "m_axi_arid", "m_axi_arsize", "m_axi_arburst",
          "m_axi_rvalid", "m_axi_rready", "m_axi_rlast", "m_axi_rid",
          "m_axi_rresp", "issued", "returned", "consumed", "region_count",
          "region_base", "fifo_count", "fault_reg", "load_pending",
          "load_pending_valid", "load_request_valid", "configure_complete",
          "configure_direct_capture", "cpu_load_done", "drain_complete",
          "m_axi_arvalid_reg", "m_axi_araddr_reg", "m_axi_arlen_reg",
          "remaining_beats", "next_address", "boundary_beats", "fifo_credit",
          "next_burst_length", "outstanding_after_reservation", "start", "in1",
          "in4", "in5", "reset_n")
COUNTERS = ("issued", "returned", "consumed", "region_count", "fifo_count")


def _bits(raw):
    raw = raw.lower().replace("_", "")
    if not raw or any(c not in "01xz" for c in raw):
        raise ValueError("malformed VCD value")
    if "x" in raw or "z" in raw:
        return None
    return int(raw, 2)


def _definitions(path):
    scopes, vars_by_scope = [], {}
    with open(path, encoding="ascii", errors="strict") as stream:
        for line in stream:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == "$scope" and len(parts) >= 4:
                scopes.append(parts[2])
            elif parts[0] == "$upscope":
                if not scopes:
                    raise ValueError("unbalanced $upscope in VCD header")
                scopes.pop()
            elif parts[0] == "$var":
                if len(parts) < 5:
                    raise ValueError("malformed $var declaration")
                try:
                    width = int(parts[2])
                except ValueError as exc:
                    raise ValueError("invalid VCD signal width") from exc
                # Questa emits zero-width parameter metadata. It carries no
                # waveform value and is irrelevant to signal qualification.
                if width == 0:
                    continue
                if width < 0:
                    raise ValueError("invalid negative-width VCD signal")
                if len(parts) < 6:
                    raise ValueError("malformed observable $var declaration")
                scope = tuple(scopes)
                name = parts[4]
                bit = None
                match = re.fullmatch(r"(.+)\[(\d+)\]", name)
                if not match and len(parts) >= 7:
                    match = re.fullmatch(r"\[(\d+)\]", parts[5])
                    if match:
                        bit = int(match.group(1))
                if match and width == 1:
                    if bit is None:
                        name, bit = match.group(1), int(match.group(2))
                vars_by_scope.setdefault(scope, {}).setdefault(name, []).append(
                    (parts[3], width, bit))
            elif parts[0] == "$enddefinitions":
                return vars_by_scope
    raise ValueError("VCD has no $enddefinitions")


def _choose_scope(defs, mode):
    engine_mode = mode in ("generated_burst", "engine_floor")
    if engine_mode:
        candidates = [(scope, sigs) for scope, sigs in defs.items()
                      if scope and scope[-1] == "burst_engine"]
        required = set(ENGINE)
        required.remove("cpu_load_done")
    elif mode == "slave":
        required = set(CHANNEL) | {"clock"}
        candidates = [(scope, sigs) for scope, sigs in defs.items()
                      if any(token.lower() in "/".join(scope).lower()
                             for token in ("testbenchmemaxi", "axi_slave", "systemmem"))]
    else:
        required = set(CHANNEL) | {"clock"}
        candidates = [(scope, sigs) for scope, sigs in defs.items()]
    if engine_mode:
        if len(candidates) != 1:
            raise ValueError("burst_engine scope is missing or ambiguous")
        scope, signals = candidates[0]
        logical_name, alternate_name = "cpu_load_done", "fifo_load_done"
        aliases = (logical_name, alternate_name)
        present = [name for name in aliases if name in signals]
        if len(present) != 1:
            raise ValueError("missing or ambiguous cpu_load_done/fifo_load_done VCD signal")
        actual_name = present[0]
        if len(signals[actual_name]) != 1 or signals[actual_name][0][1:] != (1, None):
            raise ValueError("invalid or duplicate cpu_load_done/fifo_load_done VCD signal")
        if any(name in other_signals for other_scope, other_signals in defs.items()
               if other_scope != scope for name in aliases):
            raise ValueError("cpu_load_done/fifo_load_done VCD signal outside burst_engine scope")
        signals[logical_name] = signals[actual_name]
        if actual_name != logical_name:
            del signals[actual_name]
        required.add(logical_name)
    qualified = [(s, d) for s, d in candidates if required.issubset(d)]
    if len(qualified) != 1:
        raise ValueError("signal scope is missing or ambiguous for mode " + mode)
    scope, signals = qualified[0]
    # ModelSim may scalarize packed buses while retaining explicit [index]
    # suffixes. Reassemble only a complete, unique bit range; never infer a
    # bus ordering from declaration order.
    for name in required:
        entries = signals[name]
        exact = [entry for entry in entries if entry[2] is None]
        indexed = [entry for entry in entries if entry[2] is not None]
        if exact and indexed:
            raise ValueError(f"inconsistent scalarized VCD signal {name}")
        if exact:
            if len(set(exact)) != 1:
                raise ValueError(f"duplicate VCD signal {name} in {'/'.join(scope)}")
        elif indexed:
            indices = [entry[2] for entry in indexed]
            if len(indices) != len(set(indices)) or set(indices) != set(range(max(indices) + 1)):
                raise ValueError(f"missing or duplicate bit indices for VCD signal {name}")
            if any(entry[1] != 1 for entry in indexed):
                raise ValueError(f"invalid scalarized width for VCD signal {name}")
        else:
            raise ValueError(f"missing VCD signal {name}")
    id_to_name = {}
    selected = {}
    for name in required:
        entries = signals[name]
        exact = [entry for entry in entries if entry[2] is None]
        if exact:
            ident, width, _ = exact[0]
            selected[name] = (ident, width)
            ids = [ident]
        else:
            indexed = sorted(entries, key=lambda entry: entry[2])
            width = len(indexed)
            selected[name] = (tuple((entry[2], entry[0]) for entry in indexed), width)
            ids = [entry[0] for entry in indexed]
        for ident in ids:
            if ident in id_to_name and id_to_name[ident] != name:
                prior = id_to_name[ident]
                prior_width = selected[prior][1]
                if prior_width != width:
                    raise ValueError("aliased VCD signals have conflicting widths")
            id_to_name.setdefault(ident, name)
    if mode == "generated_burst":
        controller_scopes = [(s, d) for s, d in defs.items()
                             if s and s[-1].startswith("Controller") and "present_state" in d]
        if len(controller_scopes) != 1:
            raise ValueError("present_state controller scope is missing or ambiguous")
        state_defs = controller_scopes[0][1]["present_state"]
        if len(state_defs) != 1 or state_defs[0][2] is not None:
            raise ValueError("present_state VCD signal is ambiguous")
        state_ident, state_width, _ = state_defs[0]
        selected["present_state"] = (state_ident, state_width)
        id_to_name.setdefault(state_ident, "present_state")
    return scope, selected, id_to_name


def _choose_endpoint_scope(defs):
    required = {"clock", "start_port", "done_port"}
    candidates = [(scope, sigs) for scope, sigs in defs.items()
                  if scope and scope[-1] == "DUT" and required.issubset(sigs)]
    if len(candidates) != 1:
        raise ValueError("DUT start_port/done_port endpoint scope is missing or ambiguous")
    scope, signals = candidates[0]
    selected = {}
    id_to_name = {}
    for name in required:
        entries = signals[name]
        exact = [entry for entry in entries if entry[2] is None]
        indexed = [entry for entry in entries if entry[2] is not None]
        if exact:
            if len(set(exact)) != 1 or indexed or exact[0][1] != 1:
                raise ValueError(f"invalid DUT endpoint declaration for {name}")
            ident, width, _ = exact[0]
            selected[name] = (ident, width)
        else:
            if (len(indexed) != 1 or indexed[0][2] != 0 or indexed[0][1] != 1):
                raise ValueError(f"invalid DUT endpoint declaration for {name}")
            ident = indexed[0][0]
            selected[name] = (((0, ident),), 1)
        id_to_name[ident] = name
    return scope, selected, id_to_name


def _endpoint_metrics(path, defs):
    scope, selected, ids = _choose_endpoint_scope(defs)
    start_cycle = done_cycle = None
    for cycle, _time, pre, post in _iter_edges(path, selected, ids):
        if start_cycle is None and (pre["start_port"] == 1 or post["start_port"] == 1):
            start_cycle = cycle
        if done_cycle is None and (pre["done_port"] == 1 or post["done_port"] == 1):
            done_cycle = cycle
    if start_cycle is None or done_cycle is None or done_cycle < start_cycle:
        raise ValueError("DUT start_port/done_port cycle endpoints are incomplete or out of order")
    return {"first_start_cycle": start_cycle, "first_done_cycle": done_cycle,
            "endpoint_scope": "/".join(scope)}


def _iter_edges(path, selected, id_to_name):
    ids = set(id_to_name)
    values = {ident: None for ident in ids}
    time = 0
    cycle = 0
    pending = None
    in_data = False

    def flush(group_time, old, changed):
        nonlocal cycle
        if group_time is None:
            return None
        def signal_value(name, snapshot):
            spec, width = selected[name]
            if isinstance(spec, tuple):
                bits = []
                for _index, ident in reversed(spec):
                    value = snapshot.get(ident)
                    bits.append("x" if value is None else str(value))
                return _bits("".join(bits))
            return snapshot.get(spec)

        if signal_value("clock", old) == 0 and signal_value("clock", values) == 1:
            cycle += 1
            pre = {name: signal_value(name, old) for name in selected}
            post = {name: signal_value(name, values) for name in selected}
            return cycle, group_time, pre, post
        return None

    with open(path, encoding="ascii", errors="strict") as stream:
        for line in stream:
            text = line.strip()
            if not text:
                continue
            if not in_data:
                if text.startswith("$enddefinitions"):
                    in_data = True
                continue
            if text.startswith("#"):
                new_time = int(text[1:])
                if new_time < time:
                    raise ValueError("VCD timestamps go backwards")
                if pending is not None:
                    event = flush(pending[0], pending[1], pending[2])
                    if event:
                        yield event
                time = new_time
                pending = (time, dict(values), {})
                continue
            if pending is None:
                pending = (time, dict(values), {})
            if text[0] in "bB":
                m = re.fullmatch(r"[bB]([01xXzZ_]+)\s+(\S+)", text)
                if not m:
                    continue
                raw, ident = m.groups()
            elif text[0] in "01xXzZ":
                raw, ident = text[0], text[1:]
            else:
                continue
            if ident in ids:
                value = _bits(raw)
                values[ident] = value
                pending[2][ident] = value
        if pending is not None:
            event = flush(pending[0], pending[1], pending[2])
            if event:
                yield event


def _need(snapshot, *names):
    out = {}
    for name in names:
        value = snapshot.get(name)
        if value is None:
            raise ValueError(f"unknown or missing value for {name}")
        out[name] = value
    return out


def _check_reservation(pre, post, burst_max):
    """Validate reservation arithmetic using pre-edge inputs and post-edge capture."""
    rem, addr, boundary, credit, next_len = _need(
        pre, "remaining_beats", "next_address", "boundary_beats", "fifo_credit",
        "next_burst_length").values()
    expected = min(burst_max, rem, boundary, credit)
    if expected <= 0 or next_len != expected:
        raise ValueError("reservation violates full min(B,remaining,boundary,credit) rule")
    valid, raddr, rlen = _need(post, "m_axi_arvalid_reg", "m_axi_araddr_reg",
                               "m_axi_arlen_reg").values()
    if valid != 1 or raddr is None or rlen is None or raddr != addr or rlen != expected - 1:
        raise ValueError("captured descriptor differs from pre-edge reservation inputs")
    return {"addr": raddr, "len": rlen, "beats": expected}


def _check_frozen(reserved, post):
    addr, length, valid = _need(post, "m_axi_araddr_reg", "m_axi_arlen_reg",
                                "m_axi_arvalid_reg").values()
    if (addr, length, valid) != (reserved["addr"], reserved["len"], 1):
        raise ValueError("frozen AR descriptor changed while stalled")


def _first_load_wait(accept_cycle, accept_fifo_count, completion_cycle):
    if accept_fifo_count > 0:
        if completion_cycle != accept_cycle:
            raise ValueError("first load with FIFO data did not complete at its accept")
        return 0
    if completion_cycle is None or completion_cycle <= accept_cycle:
        raise ValueError("first load waiting on memory lacks a later completion")
    return completion_cycle - accept_cycle


def _channel_metrics(path, mode, scope, selected, ids, args):
    required = ("clock",) + CHANNEL
    if not set(required).issubset(selected):
        raise ValueError("channel window signals unavailable")
    descriptors = deque()
    outstanding = 0
    peak = 0
    beats = 0
    ar_first = rv_first = final_rlast = None
    last_rbeat = None
    ar_count = 0
    previous_end = None
    for cycle, _time, pre, post in _iter_edges(path, selected, ids):
        p = _need(pre, "m_axi_arvalid", "m_axi_rvalid")
        ar_ready = pre.get("m_axi_arready")
        r_ready = pre.get("m_axi_rready")
        if p["m_axi_arvalid"]:
            if ar_ready is None:
                raise ValueError("unknown ARREADY while ARVALID is asserted")
        elif ar_ready is None:
            ar_ready = 0
        if p["m_axi_rvalid"]:
            if r_ready is None:
                raise ValueError("unknown RREADY while RVALID is asserted")
        elif r_ready is None:
            r_ready = 0
        if p["m_axi_arvalid"] and ar_ready:
            payload = _need(pre, "m_axi_araddr", "m_axi_arlen", "m_axi_arid",
                            "m_axi_arsize", "m_axi_arburst")
            addr, length, rid = (payload["m_axi_araddr"], payload["m_axi_arlen"], payload["m_axi_arid"])
            if None in (addr, length, rid):
                raise ValueError("unknown AXI address, length, or ID")
            nbeats = length + 1
            if payload["m_axi_arsize"] != 2 or payload["m_axi_arburst"] != 1:
                raise ValueError("AXI descriptor is not 32-bit INCR")
            if nbeats < 1 or nbeats > 256:
                raise ValueError("AXI burst beat count outside 1..256")
            if addr // 4096 != (addr + nbeats * 4 - 1) // 4096:
                raise ValueError("AXI burst crosses a 4-KiB boundary")
            if previous_end is not None and addr != previous_end:
                raise ValueError("AXI descriptor address progression is not contiguous")
            previous_end = addr + nbeats * 4
            descriptors.append([addr, nbeats, 0, rid])
            outstanding += 1
            beats += nbeats
            ar_count += 1
            ar_first = cycle if ar_first is None else ar_first
        if p["m_axi_rvalid"]:
            rv_first = cycle if rv_first is None else rv_first
        if p["m_axi_rvalid"] and r_ready:
            if not descriptors:
                raise ValueError("R beat accepted without an outstanding descriptor")
            d = descriptors[0]
            rid, resp, last = _need(pre, "m_axi_rid", "m_axi_rresp", "m_axi_rlast").values()
            if None in (rid, resp, last):
                raise ValueError("unknown R ID/RESP/RLAST")
            if rid != d[3] or resp != 0:
                raise ValueError("AXI R ID or RESP mismatch")
            d[2] += 1
            expected_last = d[2] == d[1]
            if bool(last) != expected_last:
                raise ValueError("RLAST does not match descriptor beat count")
            last_rbeat = cycle
            if expected_last:
                descriptors.popleft()
                outstanding -= 1
                final_rlast = cycle
        peak = max(peak, outstanding)
    if ar_first is None or rv_first is None or final_rlast is None:
        raise ValueError("incomplete AXI channel window")
    if descriptors or outstanding != 0:
        raise ValueError("trace ended before AXI descriptors drained")
    if args.n is not None and beats != args.n:
        raise ValueError(f"accepted beats {beats} != expected N={args.n}")
    return {"t_ar0": ar_first, "first_ar_accept_cycle": ar_first,
            "t_rv0": rv_first, "L_axi": rv_first - ar_first,
            "t_final_rlast": final_rlast, "C_stream": final_rlast - ar_first,
            "last_rlast_accept_cycle": final_rlast,
            "beats": beats, "n_desc": ar_count, "outstanding_peak": peak,
            "outstanding_final": outstanding}


def _generated(path, scope, selected, ids, args):
    missing = set(ENGINE) - set(selected)
    if missing:
        raise ValueError("generated engine signals absent: " + ",".join(sorted(missing)))
    if args.n is None or args.b is None or args.o is None or args.d is None:
        raise ValueError("generated mode requires --n, --b, --o, and --d")
    queued = deque()
    reserved = None
    occupancy = peak = nbeats_total = n_desc = rbeats = 0
    t_ar0 = t_rv0 = t_pop0 = first_accept = first_done = None
    first_accept_fifo = None
    first_wait = exposed_wait = 0
    configure_seen = False
    config_cycle = configure_complete_cycle = final_load_cycle = drain_cycle = None
    prev_consumed = None
    first_consumed_cycle = None
    previous_counters = None
    reset_seen = False
    state_histogram = {}
    prior_state = None
    current_stall = max_stall = 0
    accepted_load_waiting = False
    reservation_count = 0
    last_address = None
    final_rlast_cycle = None
    payload_names = {"m_axi_araddr_reg", "m_axi_arlen_reg", "m_axi_arsize",
                     "m_axi_arburst", "m_axi_arid", "m_axi_rid", "m_axi_rresp",
                     "m_axi_rlast"}
    control_names = tuple(name for name in ENGINE if name not in payload_names)
    for cycle, _time, pre, post in _iter_edges(path, selected, ids):
        reset_value = pre.get("reset_n")
        if reset_value is None:
            if reset_seen:
                raise ValueError("reset_n became unknown after reset")
            continue
        if reset_value == 0:
            reset_seen = True
            continue
        reset_seen = True
        p = _need(pre, *control_names)
        q = _need(post, "m_axi_arvalid_reg", "m_axi_araddr_reg", "m_axi_arlen_reg",
                  "configure_complete", "drain_complete", "fault_reg", "issued",
                  "returned", "consumed", "region_count", "fifo_count", "fault_reg")
        if p["reset_n"] == 0:
            continue
        if p["fault_reg"] or q["fault_reg"]:
            raise ValueError("burst engine fault asserted")
        region_count, issued, returned, consumed, fifo = (p["region_count"], p["issued"],
                                                          p["returned"], p["consumed"], p["fifo_count"])
        if not (0 <= consumed <= returned <= issued <= region_count):
            raise ValueError("issued/returned/consumed invariant violated")
        if q["consumed"] < consumed or q["consumed"] > consumed + 1:
            raise ValueError("consumed counter changed by more than one beat")
        if q["consumed"] == consumed + 1 and not p["cpu_load_done"]:
            raise ValueError("consumed advanced without cpu_load_done")
        if q["consumed"] == consumed + 1 and first_consumed_cycle is None:
            first_consumed_cycle = cycle
        if q["consumed"] == args.n and p["cpu_load_done"]:
            final_load_cycle = cycle
        if previous_counters is not None:
            for name, prior in previous_counters.items():
                if p[name] < prior:
                    raise ValueError(f"{name} counter regressed")
        previous_counters = {name: p[name] for name in ("issued", "returned", "consumed")}
        if fifo != returned - consumed:
            raise ValueError("fifo_count differs from returned-consumed")
        if fifo > args.d:
            raise ValueError("fifo_count exceeds configured D")
        if prev_consumed is not None and consumed < prev_consumed:
            raise ValueError("consumed counter regressed")
        prev_consumed = consumed
        configure = bool(p["configure_complete"] or p["configure_direct_capture"])
        if configure and not configure_seen:
            configure_seen = True
            configure_complete_cycle = cycle
        if config_cycle is None and p["start"] and p["in1"] == 2:
            config_cycle = cycle
        if configure_seen and first_accept is None and p["start"] and p["in1"] == 0 and not p["load_pending"]:
            if not p["load_request_valid"]:
                raise ValueError("first load accept has load_request_valid=0")
            first_accept = cycle
            first_accept_fifo = fifo
            t_pop0 = cycle if p["cpu_load_done"] else None
            if fifo == 0:
                accepted_load_waiting = True
            else:
                first_wait = _first_load_wait(cycle, fifo,
                                              cycle if p["cpu_load_done"] else None)
                if p["cpu_load_done"]:
                    first_done = cycle
        if p["load_pending"] and p["load_pending_valid"] and not p["cpu_load_done"]:
            exposed_wait += 1
        if accepted_load_waiting and p["cpu_load_done"]:
            first_done = cycle
            first_wait = _first_load_wait(first_accept, first_accept_fifo, cycle)
            accepted_load_waiting = False
        if p["cpu_load_done"] and t_pop0 is None:
            t_pop0 = cycle
        ar_hs = bool(p["m_axi_arvalid"] and p["m_axi_arready"])
        if p["m_axi_arvalid"] and p["m_axi_arready"] is None:
            raise ValueError("unknown ARREADY while ARVALID is asserted")
        if ar_hs:
            payload = _need(pre, "m_axi_araddr_reg", "m_axi_arlen_reg", "m_axi_arsize",
                            "m_axi_arburst", "m_axi_arid")
            addr, length = payload["m_axi_araddr_reg"], payload["m_axi_arlen_reg"]
            if reserved is not None and (addr, length) != (reserved["addr"], reserved["len"]):
                raise ValueError("reserved ARADDR/ARLEN changed before handshake")
            if addr is None or length is None:
                raise ValueError("unknown accepted ARADDR/ARLEN")
            count = length + 1
            if reserved is not None and count != reserved["beats"]:
                raise ValueError("accepted ARLEN differs from reserved beat count")
            if count > args.b or count > 256:
                raise ValueError("accepted burst exceeds B or AXI 256-beat limit")
            if payload["m_axi_arsize"] != 2 or payload["m_axi_arburst"] != 1 or payload["m_axi_arid"] != 0:
                raise ValueError("accepted AR SIZE/BURST/ID violates INCR 32-bit ABI")
            if addr // 4096 != (addr + count * 4 - 1) // 4096:
                raise ValueError("accepted burst crosses 4-KiB boundary")
            if last_address is None:
                if addr != p["region_base"]:
                    raise ValueError("first accepted burst does not start at region_base")
            elif addr != last_address:
                raise ValueError("accepted burst address progression mismatch")
            if t_ar0 is None:
                t_ar0 = cycle
            last_address = addr + count * 4
            nbeats_total += count
            n_desc += 1
            occupancy += 1
            queued.append({"left": count, "id": payload["m_axi_arid"], "addr": addr})
            reserved = None
        expected_issued = issued + ((payload["m_axi_arlen_reg"] + 1) if ar_hs else 0)
        r_hs = bool(p["m_axi_rvalid"] and p["m_axi_rready"])
        if p["m_axi_rvalid"] and p["m_axi_rready"] is None:
            raise ValueError("unknown RREADY while RVALID is asserted")
        expected_returned = returned + (1 if r_hs else 0)
        if q["issued"] != expected_issued or q["returned"] != expected_returned:
            raise ValueError("counter changes disagree with accepted AXI handshakes")
        if p["m_axi_rvalid"]:
            if t_rv0 is None:
                t_rv0 = cycle
            if r_hs:
                if not queued:
                    raise ValueError("accepted R beat without descriptor")
                d = queued[0]
                rid, resp, last = _need(pre, "m_axi_rid", "m_axi_rresp", "m_axi_rlast").values()
                if None in (rid, resp, last) or rid != d["id"] or resp != 0:
                    raise ValueError("R ID/RESP unknown or invalid")
                d["left"] -= 1
                rbeats += 1
                if bool(last) != (d["left"] == 0):
                    raise ValueError("RLAST does not match reserved descriptor")
                if d["left"] == 0:
                    queued.popleft()
                    occupancy -= 1
                    final_rlast_cycle = cycle
        peak = max(peak, occupancy)
        if occupancy < 0 or occupancy > args.o:
            raise ValueError("outstanding occupancy outside [0,O]")
        # Reservation is identified by the registered valid rising after the
        # edge; all arithmetic inputs come from the pre-edge snapshot.
        if not p["m_axi_arvalid_reg"] and q["m_axi_arvalid_reg"]:
            rem, addr, boundary, credit, next_len = _need(
                p, "remaining_beats", "next_address", "boundary_beats", "fifo_credit",
                "next_burst_length").values()
            derived_remaining = p["region_count"] - p["issued"]
            derived_address = p["region_base"] + (p["issued"] << 2)
            derived_boundary = (4096 - (derived_address & 4095)) // 4
            derived_credit = args.d - (p["issued"] - p["consumed"])
            if (rem, addr, boundary, credit) != (derived_remaining, derived_address,
                                                  derived_boundary, derived_credit):
                raise ValueError("reservation inputs disagree with pre-edge region/FIFO state")
            reserved = _check_reservation(p, q, args.b)
            reservation_count += 1
        elif reserved is not None and not (p["m_axi_arvalid"] and p["m_axi_arready"]):
            _check_frozen(reserved, q)
        if q["drain_complete"]:
            drain_cycle = cycle
        state = pre.get("present_state")
        if state is not None:
            state_histogram[str(state)] = state_histogram.get(str(state), 0) + 1
            if prior_state is None or state != prior_state:
                current_stall = 1
            else:
                current_stall += 1
            max_stall = max(max_stall, current_stall)
            prior_state = state
    if config_cycle is None or first_accept is None or t_ar0 is None or t_rv0 is None:
        raise ValueError("trace lacks configure/load/AXI start events")
    if accepted_load_waiting:
        raise ValueError("first accepted load did not complete")
    if first_done is None:
        raise ValueError("first load completion was not observed")
    if first_consumed_cycle != first_done:
        raise ValueError("first consumed increment does not correspond to first accepted load")
    if final_load_cycle is None:
        raise ValueError("final CPU load completion was not observed")
    final = _need(post, "issued", "returned", "consumed", "region_count", "fifo_count",
                  "fault_reg") if 'post' in locals() else {}
    if final["fault_reg"] or final["issued"] != args.n or final["returned"] != args.n or \
            final["consumed"] != args.n or final["region_count"] != args.n or final["fifo_count"] != 0:
        raise ValueError("final engine counters do not match a drained N-beat region")
    if nbeats_total != args.n or rbeats != args.n:
        raise ValueError(f"AXI beat totals issued={nbeats_total} returned={rbeats} expected={args.n}")
    if occupancy or queued or reserved:
        raise ValueError("engine did not drain all descriptors")
    if drain_cycle is None:
        raise ValueError("drain_complete was never observed")
    latency = t_rv0 - t_ar0
    if args.declared_latency is None or args.read_delay is None:
        raise ValueError("generated mode requires --declared-latency and --read-delay")
    if args.read_delay != args.declared_latency - 1:
        raise ValueError("READ_DELAY must be declared latency minus one")
    if not (args.read_delay <= latency <= args.read_delay + 2):
        raise ValueError("measured AXI latency outside READ_DELAY..READ_DELAY+2")
    base = _need(post, "region_base")["region_base"]
    remaining = args.n
    address = base
    n_desc_min = 0
    while remaining:
        boundary = (4096 - (address & 4095)) // 4
        chunk = min(args.b, remaining, boundary)
        if chunk <= 0:
            raise ValueError("cannot derive geometric descriptor lower bound")
        remaining -= chunk
        address += chunk * 4
        n_desc_min += 1
    return {"t_ar0": t_ar0, "first_ar_accept_cycle": t_ar0,
            "t_rv0": t_rv0, "t_pop0": t_pop0,
            "L_axi": latency, "consume_lag": t_pop0 - t_rv0 if t_pop0 is not None else None,
            "first_load_accept_cycle": first_accept,
            "first_load_done_cycle": first_done, "first_wait_cycles": first_wait,
            "exposed_wait_cycles": exposed_wait,
            "state_stall_cycles": max_stall if state_histogram else None,
            "state_cycle_histogram": state_histogram,
            "outstanding_peak": peak, "outstanding_final": occupancy,
            "n_desc": n_desc, "n_desc_min": n_desc_min,
            "reservation_count": reservation_count,
            "beats_issued": nbeats_total, "beats_returned": rbeats,
            "t_final_rlast": final_rlast_cycle,
            "last_rlast_accept_cycle": final_rlast_cycle,
            "C_stream": final_rlast_cycle - t_ar0 if final_rlast_cycle is not None else None,
            "configure_start_cycle": config_cycle,
            "configure_cycle": config_cycle, "configure_complete_cycle": configure_complete_cycle,
            "final_load_cycle": final_load_cycle, "drain_cycle": drain_cycle,
            "configure_to_final_load_cycles": final_load_cycle - config_cycle,
            "configure_to_drain_cycles": drain_cycle - config_cycle,
            "C_configure_drain": drain_cycle - config_cycle,
            "declared_latency": args.declared_latency, "read_delay": args.read_delay,
            "fifo_final": 0, "engine_fault": False}


def _arg_parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--mode", required=True, choices=("slave", "engine_floor", "generated_burst", "legacy"),
                   help="strict Bambu qualification profile")
    p.add_argument("--vcd", required=True, type=Path, help="input VCD streamed from disk")
    p.add_argument("--json-out", required=True, type=Path, help="write schema-version-1 JSON report")
    p.add_argument("--n", type=int, help="expected total read beats / region elements")
    p.add_argument("--b", type=int, help="expected maximum burst length in beats")
    p.add_argument("--o", type=int, help="expected maximum outstanding descriptors")
    p.add_argument("--d", type=int, help="expected read FIFO depth in beats")
    p.add_argument("--declared-latency", type=int, help="latency declared in compiler artifacts")
    p.add_argument("--read-delay", type=int, help="READ_DELAY value in the simulation model")
    return p


def main(argv=None):
    args = _arg_parser().parse_args(argv)
    report = {"schema_version": 1, "mode": args.mode, "scope": None,
              "status": "INVALID", "expected": {k: getattr(args, k) for k in
              ("n", "b", "o", "d", "declared_latency", "read_delay")}, "metrics": {}, "errors": []}
    try:
        if args.n is None or args.n <= 0:
            raise ValueError("strict modes require a positive --n")
        if args.mode in ("generated_burst", "engine_floor"):
            if args.b is None or not 1 <= args.b <= 256:
                raise ValueError("engine modes require --b in 1..256")
            if args.o is None or args.o <= 0 or args.d is None or args.d <= 0:
                raise ValueError("engine modes require positive --o and --d")
            if args.declared_latency is None or args.read_delay is None:
                raise ValueError("engine modes require --declared-latency and --read-delay")
        if args.mode == "slave" and (args.declared_latency is None or args.read_delay is None):
            raise ValueError("slave mode requires --declared-latency and --read-delay")
        defs = _definitions(args.vcd)
        scope, selected, ids = _choose_scope(defs, args.mode)
        report["scope"] = "/".join(scope)
        if args.mode in ("generated_burst", "engine_floor"):
            metrics = _generated(args.vcd, scope, selected, ids, args)
        else:
            metrics = _channel_metrics(args.vcd, args.mode, scope, selected, ids, args)
            metrics["first_wait_cycles"] = None
            metrics["n_desc_min"] = None
            if args.declared_latency is not None:
                if args.read_delay is None or args.read_delay != args.declared_latency - 1:
                    raise ValueError("declared latency and read delay disagree or are incomplete")
                metrics["declared_latency"] = args.declared_latency
                metrics["read_delay"] = args.read_delay
                if metrics["L_axi"] not in (args.read_delay, args.read_delay + 1,
                                             args.read_delay + 2):
                    raise ValueError("measured AXI latency outside READ_DELAY..READ_DELAY+2")
        if args.mode in ("generated_burst", "legacy"):
            metrics.update(_endpoint_metrics(args.vcd, defs))
        report["metrics"] = metrics
        report["status"] = "PASS"
    except Exception as exc:
        report["errors"].append(str(exc))
    try:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    except OSError as exc:
        print("cannot write JSON report: " + str(exc), file=sys.stderr)
        return 2
    if report["status"] != "PASS":
        print("T4B VCD qualification failed: " + "; ".join(report["errors"]), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
