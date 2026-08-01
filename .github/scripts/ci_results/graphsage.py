"""Strict deterministic GraphSAGE RTL inventory parsing and comparison."""
from __future__ import annotations
from typing import Any

CASE_PREFIX = "GRAPHSAGE_CASE|"
VECTOR_LENGTHS = {"rows": 7, "neighbors": 12, "features": 18, "golden": 18, "observed": 18}
EXPECTED_CASE_IDS = {
    "regular", "irregular-zero-degree", "negative-signed-division", "duplicate-self-mixed",
}
EXPECTED_FIELDS = {"id", *VECTOR_LENGTHS, "count", "mismatches", "inputs_immutable"}

class GraphSAGEEvidenceError(ValueError):
    def __init__(self, message: str, *, report: dict[str, Any] | None = None):
        super().__init__(message)
        self.report = report

def _integers(value: str, key: str, length: int) -> list[int]:
    pieces = value.split(",") if value else []
    if len(pieces) != length:
        raise GraphSAGEEvidenceError(f"{key}: expected {length} integers, found {len(pieces)}")
    try:
        return [int(piece) for piece in pieces]
    except ValueError as error:
        raise GraphSAGEEvidenceError(f"{key}: malformed integer inventory") from error

def parse_inventory(text: str) -> dict[str, dict[str, Any]]:
    cases: dict[str, dict[str, Any]] = {}
    for line in text.splitlines():
        if not line.startswith(CASE_PREFIX):
            continue
        fields: dict[str, str] = {}
        for field in line[len(CASE_PREFIX):].split("|"):
            if "=" not in field:
                raise GraphSAGEEvidenceError("malformed case field")
            key, value = field.split("=", 1)
            if key in fields:
                raise GraphSAGEEvidenceError(f"duplicate field {key}")
            fields[key] = value
        missing = sorted(EXPECTED_FIELDS - fields.keys())
        if missing:
            raise GraphSAGEEvidenceError("missing fields: " + ", ".join(missing))
        extra = sorted(fields.keys() - EXPECTED_FIELDS)
        if extra:
            raise GraphSAGEEvidenceError("unexpected fields: " + ", ".join(extra))
        case_id = fields["id"]
        if not case_id or case_id in cases:
            raise GraphSAGEEvidenceError(f"invalid or duplicate case id {case_id!r}")
        case = {key: _integers(fields[key], key, length) for key, length in VECTOR_LENGTHS.items()}
        try:
            case.update(count=int(fields["count"]), mismatches=int(fields["mismatches"]),
                        inputs_immutable=int(fields["inputs_immutable"]))
        except ValueError as error:
            raise GraphSAGEEvidenceError("malformed scalar inventory") from error
        if (case["count"] != 18 or case["mismatches"] < 0
                or case["inputs_immutable"] not in {0, 1}):
            raise GraphSAGEEvidenceError(f"{case_id}: invalid count, mismatch, or input-immutability evidence")
        cases[case_id] = case
    if not cases:
        raise GraphSAGEEvidenceError("missing GraphSAGE output inventory")
    missing_cases = sorted(EXPECTED_CASE_IDS - cases.keys())
    extra_cases = sorted(cases.keys() - EXPECTED_CASE_IDS)
    if missing_cases or extra_cases:
        details = []
        if missing_cases:
            details.append("missing cases: " + ", ".join(missing_cases))
        if extra_cases:
            details.append("unexpected cases: " + ", ".join(extra_cases))
        raise GraphSAGEEvidenceError("invalid GraphSAGE case inventory (" + "; ".join(details) + ")")
    return cases

def compare_inventories(serial_text: str, sparta_text: str) -> dict[str, Any]:
    serial, sparta = parse_inventory(serial_text), parse_inventory(sparta_text)
    if serial.keys() != sparta.keys():
        raise GraphSAGEEvidenceError("serial and SPARTA case inventories differ")
    report_cases, mismatch_count, evidence_errors = [], 0, []
    for case_id in serial:
        left, right = serial[case_id], sparta[case_id]
        for key in ("rows", "neighbors", "features", "golden", "count"):
            if left[key] != right[key]:
                raise GraphSAGEEvidenceError(f"{case_id}: incompatible {key} inventory")
        gs = sum(a != b for a, b in zip(left["golden"], left["observed"]))
        gp = sum(a != b for a, b in zip(left["golden"], right["observed"]))
        ss = sum(a != b for a, b in zip(left["observed"], right["observed"]))
        mismatch_count += gs + gp + ss
        if left["mismatches"] != gs:
            evidence_errors.append(
                f"{case_id}: serial declared {left['mismatches']} mismatches but inventory has {gs}"
            )
        if right["mismatches"] != gp:
            evidence_errors.append(
                f"{case_id}: SPARTA declared {right['mismatches']} mismatches but inventory has {gp}"
            )
        if not left["inputs_immutable"] or not right["inputs_immutable"]:
            evidence_errors.append(f"{case_id}: input-immutability verification failed")
        report_cases.append({"case_id": case_id, "row_offsets": left["rows"],
            "neighbor_indices": left["neighbors"], "feature_values": left["features"],
            "golden": left["golden"], "serial": left["observed"], "sparta": right["observed"],
            "element_count": left["count"], "golden_serial_mismatches": gs,
            "golden_sparta_mismatches": gp, "serial_sparta_mismatches": ss,
            "serial_declared_mismatches": left["mismatches"],
            "sparta_declared_mismatches": right["mismatches"],
            "serial_input_immutable": bool(left["inputs_immutable"]),
            "sparta_input_immutable": bool(right["inputs_immutable"])})
    report = {"schema": "panda.ci.graphsage-comparison", "schema_version": "1.0",
            "arithmetic_contract": "signed C++ int sum and truncation-toward-zero division; zero degree yields zero",
            "case_count": len(report_cases), "element_comparison_count": len(report_cases) * 18 * 3,
            "mismatch_count": mismatch_count,
            "outcome": "fail" if mismatch_count or evidence_errors else "pass",
            "cases": report_cases}
    if mismatch_count or evidence_errors:
        message = (f"GraphSAGE comparison has {mismatch_count} element mismatches"
                   if mismatch_count else "GraphSAGE comparison evidence is invalid")
        if evidence_errors:
            message += ": " + "; ".join(evidence_errors)
        report["error"] = message
        raise GraphSAGEEvidenceError(message, report=report)
    return report
