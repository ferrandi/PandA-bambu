"""Deterministic comparison of independently validated PandA CI bundles.

The serialized comparison document intentionally separates primitive observations
from derived fields. Standalone validation can prove that classifications,
transitions, metric deltas, summaries, and policy decisions are internally
consistent with the serialized primitive observations. It cannot authenticate
that those primitive observations came from the original bundles, nor that the
primitive observations are correct if all of them are edited together.
"""

from __future__ import annotations

import hashlib
from pathlib import Path
from typing import Any

from .bundle import BundleValidationError, validate_bundle
from .constants import MULTI_TASK_SCHEMA_VERSION
from .hashing import sha256_file
from .schema import SchemaValidationError, SchemaValidator
from .serialization import (
    SerializationError,
    canonical_bytes,
    load_json,
    require_canonical,
    write_json,
)


COMPARISON_SCHEMA = "panda.ci.comparison"
COMPARISON_SCHEMA_VERSION = "1.0"
COMPARISON_SCHEMA_FILE = "comparison.schema.json"
REQUIRED_PROFILE_TOOLS = frozenset(
    {"ccache", "clang", "clangxx", "cmake", "gcc", "gxx", "llvm", "verilator"}
)

METRIC_SOURCES = (
    ("simulation.total-cycles", None, "cycles"),
    ("duration.hls-synthesis", "duration.hls-synthesis", "seconds"),
    ("duration.rtl-simulation", "duration.rtl-simulation", "seconds"),
    ("duration.regression-total", "duration.regression-total", "seconds"),
)

METRIC_DEFINITIONS = {
    metric_id: {"source_metric": source_metric, "unit": unit}
    for metric_id, source_metric, unit in METRIC_SOURCES
}
CORRECTNESS_FIELDS = ("synthesis", "simulation", "verification", "overall")
FAILURE_STAGE_FIELDS = ("synthesis", "simulation", "verification")
MISSING_IN_BASELINE_REASON = {"code": "task-added-in-candidate", "field": "task_id"}
MISSING_IN_CANDIDATE_REASON = {"code": "required-task-missing", "field": "task_id"}
ALLOWED_COMPARABILITY_REASONS = {
    "build-profile-differs",
    "configuration-differs",
    "required-task-missing",
    "task-added-in-candidate",
    "task-id-differs",
    "task-type-differs",
}
ALLOWED_POLICY_REASONS = {
    "baseline-build-incomplete",
    "baseline-regression-not-passing",
    "candidate-build-regression",
    "candidate-simulation-regression",
    "candidate-synthesis-regression",
    "candidate-verification-failure",
    "configuration-not-comparable",
    "cycle-information-unavailable",
    "new-candidate-regression",
    "required-regression-missing",
}

CONFIGURATION_PATHS = (
    "category",
    "input.source_path",
    "input.test_vector_kind",
    "input.test_vector_path",
    "input.test_vector_value",
    "input.top_function",
    "frontend.requested",
    "frontend.selected",
    "invocation.executable",
    "invocation.arguments",
    "options.compiler",
    "options.simulator",
    "options.simulate",
    "options.device",
    "options.clock_period",
    "options.interface",
    "options.language_standard",
    "options.optimization",
    "options.experimental_setup",
    "options.bambu_parameters",
    "options.expose_globals",
    "options.inline_max_cost",
    "options.parallel_backend",
)


class ComparisonError(ValueError):
    """Raised when comparison inputs or output are inconsistent."""


class ComparisonInputError(ComparisonError):
    """Raised when one named input bundle cannot be compared."""


def _value_at(value: Any, dotted_path: str) -> Any:
    current = value
    for part in dotted_path.split("."):
        if not isinstance(current, dict):
            return None
        current = current.get(part)
    return current


def _stage_outcome(task: dict[str, Any], stage_id: str) -> str:
    for stage in task.get("stages", []):
        if stage.get("stage_id") == stage_id:
            return str(stage.get("outcome", "unknown"))
    return "unknown"


def _metric_value(task: dict[str, Any], metric_id: str | None) -> int | float | None:
    if metric_id is None:
        value = _value_at(task, "results.simulation.total_cycles")
    else:
        value = next(
            (
                metric.get("value")
                for metric in task.get("metrics", [])
                if metric.get("metric_id") == metric_id
            ),
            None,
        )
    return value if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _transition(baseline: Any, candidate: Any) -> str:
    left = "unknown" if baseline is None else str(baseline)
    right = "unknown" if candidate is None else str(candidate)
    return f"{left} → {right}"


def _build_profile_identity(profile: dict[str, Any] | None) -> dict[str, Any] | None:
    if profile is None:
        return None
    return {
        "sha256": hashlib.sha256(canonical_bytes(profile)).hexdigest(),
        "tool_ids": sorted(
            item.get("tool_id")
            for item in profile.get("tool_versions", [])
            if isinstance(item, dict)
        ),
    }


def _identity(documents: dict[str, dict[str, Any]], bundle: Path) -> dict[str, Any]:
    manifest = documents["manifest.json"]
    return {
        "bundle_id": manifest["run_id"],
        "commit_sha": manifest["commit_sha"],
        "manifest_sha256": sha256_file(bundle / "manifest.json"),
        "protocol_version": manifest["schema_version"],
        "request_id": manifest["request_id"],
        "workflow_run_id": manifest["workflow_run_id"],
        "run_attempt": manifest["run_attempt"],
    }


def _build_profile(documents: dict[str, dict[str, Any]]) -> dict[str, Any] | None:
    profile = documents["manifest.json"].get("effective_build_profile")
    if not isinstance(profile, dict):
        return None

    def complete(value: Any) -> bool:
        if value is None:
            return False
        if isinstance(value, str):
            return bool(value.strip())
        if isinstance(value, dict):
            return bool(value) and all(complete(item) for item in value.values())
        if isinstance(value, list):
            return bool(value) and all(complete(item) for item in value)
        return True

    tools = profile.get("tool_versions")
    tool_ids = (
        {item.get("tool_id") for item in tools if isinstance(item, dict)}
        if isinstance(tools, list)
        else set()
    )
    return (
        profile
        if complete(profile)
        and len(tools) == len(REQUIRED_PROFILE_TOOLS)
        and tool_ids == REQUIRED_PROFILE_TOOLS
        else None
    )


def _regression_tasks(
    documents: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    return {
        task["task_id"]: task
        for path, task in documents.items()
        if path.startswith("tasks/") and task.get("task_type") == "regression"
    }


def _comparability_checks(
    task_id: str,
    baseline: dict[str, Any],
    candidate: dict[str, Any],
    baseline_profile: dict[str, Any] | None,
    candidate_profile: dict[str, Any] | None,
) -> list[dict[str, Any]]:
    checks = [
        {
            "baseline": baseline.get("task_id"),
            "candidate": candidate.get("task_id"),
            "field": "task_id",
            "reason_code": "task-id-differs",
        },
        {
            "baseline": baseline.get("task_type"),
            "candidate": candidate.get("task_type"),
            "field": "task_type",
            "reason_code": "task-type-differs",
        },
    ]
    for path in CONFIGURATION_PATHS:
        checks.append(
            {
                "baseline": _value_at(baseline.get("configuration", {}), path),
                "candidate": _value_at(candidate.get("configuration", {}), path),
                "field": path,
                "reason_code": "configuration-differs",
            }
        )
    checks.append(
        {
            "baseline": _build_profile_identity(baseline_profile),
            "candidate": _build_profile_identity(candidate_profile),
            "field": "build_profile",
            "reason_code": "build-profile-differs",
        }
    )
    return checks


def _reasons_from_checks(checks: list[dict[str, Any]]) -> list[dict[str, Any]]:
    reasons = [
        {"code": check["reason_code"], "field": check["field"]}
        for check in checks
        if check["baseline"] != check["candidate"]
    ]
    return sorted(
        {canonical_bytes(reason): reason for reason in reasons}.values(),
        key=lambda item: (item["code"], item["field"]),
    )


def _metric_comparison(
    task_id: str,
    baseline: dict[str, Any],
    candidate: dict[str, Any],
) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for metric_id, source_metric, unit in METRIC_SOURCES:
        baseline_value = _metric_value(baseline, source_metric)
        candidate_value = _metric_value(candidate, source_metric)
        absolute_delta = None
        percentage_delta = None
        if baseline_value is not None and candidate_value is not None:
            absolute_delta = candidate_value - baseline_value
            if baseline_value != 0:
                percentage_delta = round((absolute_delta / baseline_value) * 100, 6)
        records.append(
            {
                "absolute_delta": absolute_delta,
                "baseline_value": baseline_value,
                "candidate_value": candidate_value,
                "evidence": [
                    f"baseline:tasks/{task_id}.json",
                    f"candidate:tasks/{task_id}.json",
                ],
                "metric_id": metric_id,
                "percentage_delta": percentage_delta,
                "unit": unit,
            }
        )
    return records


def _correctness(task: dict[str, Any]) -> dict[str, Any]:
    return {
        "overall": str(task.get("outcome", "unknown")),
        "simulation": _stage_outcome(task, "rtl-simulation"),
        "synthesis": _stage_outcome(task, "hls-synthesis"),
        "verification": _stage_outcome(task, "result-verification"),
    }


def _build_record(
    baseline_documents: dict[str, dict[str, Any]],
    candidate_documents: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    baseline = baseline_documents["tasks/open-build.json"]["outcome"]
    candidate = candidate_documents["tasks/open-build.json"]["outcome"]
    return {
        "baseline": baseline,
        "candidate": candidate,
        "evidence": [
            "baseline:tasks/open-build.json#outcome",
            "candidate:tasks/open-build.json#outcome",
        ],
        "transition": _transition(baseline, candidate),
    }


def _primitive_task_record(
    task_id: str,
    baseline: dict[str, Any] | None,
    candidate: dict[str, Any] | None,
    baseline_profile: dict[str, Any] | None,
    candidate_profile: dict[str, Any] | None,
) -> dict[str, Any]:
    if baseline is None:
        return {
            "correctness": None,
            "evidence": [f"candidate:tasks/{task_id}.json"],
            "execution": None,
            "failure": None,
            "metrics": [],
            "presence": {"baseline": False, "candidate": True},
            "task_id": task_id,
            "task_type": candidate["task_type"],
        }
    if candidate is None:
        return {
            "correctness": None,
            "evidence": [f"baseline:tasks/{task_id}.json"],
            "execution": None,
            "failure": None,
            "metrics": [],
            "presence": {"baseline": True, "candidate": False},
            "task_id": task_id,
            "task_type": baseline["task_type"],
        }

    baseline_correctness = _correctness(baseline)
    candidate_correctness = _correctness(candidate)
    baseline_failure = _value_at(baseline, "failure.category")
    candidate_failure = _value_at(candidate, "failure.category")
    correctness = {
        field: {
            "baseline": baseline_correctness[field],
            "candidate": candidate_correctness[field],
            "transition": _transition(
                baseline_correctness[field], candidate_correctness[field]
            ),
        }
        for field in CORRECTNESS_FIELDS
    }
    primitive = {
        "comparability_checks": _comparability_checks(
            task_id, baseline, candidate, baseline_profile, candidate_profile
        ),
        "correctness": correctness,
        "evidence": [
            f"baseline:tasks/{task_id}.json",
            f"candidate:tasks/{task_id}.json",
        ],
        "execution": {
            "baseline": _value_at(baseline, "execution.state"),
            "candidate": _value_at(candidate, "execution.state"),
            "transition": _transition(
                _value_at(baseline, "execution.state"),
                _value_at(candidate, "execution.state"),
            ),
        },
        "failure": {
            "baseline_category": baseline_failure,
            "candidate_category": candidate_failure,
            "fixed_failure": False,
            "introduced_failure": False,
            "transition": _transition(baseline_failure, candidate_failure),
        },
        "metrics": _metric_comparison(task_id, baseline, candidate),
        "presence": {"baseline": True, "candidate": True},
        "task_id": task_id,
        "task_type": baseline["task_type"],
    }
    return primitive


def _introduced_failure(correctness: dict[str, Any]) -> bool:
    return any(
        correctness[field]["baseline"] == "pass"
        and correctness[field]["candidate"] == "fail"
        for field in FAILURE_STAGE_FIELDS
    )


def _fixed_failure(correctness: dict[str, Any]) -> bool:
    return any(
        correctness[field]["baseline"] == "fail"
        and correctness[field]["candidate"] == "pass"
        for field in FAILURE_STAGE_FIELDS
    )


def _canonical_metric(task_id: str, metric: dict[str, Any]) -> dict[str, Any]:
    metric_id = metric["metric_id"]
    baseline = metric["baseline_value"]
    candidate = metric["candidate_value"]
    absolute_delta = None
    percentage_delta = None
    if baseline is not None and candidate is not None:
        absolute_delta = candidate - baseline
        if baseline != 0:
            percentage_delta = round((absolute_delta / baseline) * 100, 6)
    return {
        "absolute_delta": absolute_delta,
        "baseline_value": baseline,
        "candidate_value": candidate,
        "evidence": [
            f"baseline:tasks/{task_id}.json",
            f"candidate:tasks/{task_id}.json",
        ],
        "metric_id": metric_id,
        "percentage_delta": percentage_delta,
        "unit": METRIC_DEFINITIONS[metric_id]["unit"],
    }


def normalize_task_record(serialized_task: dict[str, Any]) -> dict[str, Any]:
    """Reconstruct derived task semantics from serialized primitive observations."""

    task = dict(serialized_task)
    task_id = task["task_id"]
    presence = task["presence"]
    baseline_present = presence["baseline"]
    candidate_present = presence["candidate"]
    if not baseline_present and candidate_present:
        classification = "missing-in-baseline"
        reasons = [MISSING_IN_BASELINE_REASON]
    elif baseline_present and not candidate_present:
        classification = "missing-in-candidate"
        reasons = [MISSING_IN_CANDIDATE_REASON]
    elif baseline_present and candidate_present:
        reasons = _reasons_from_checks(task["comparability_checks"])
        classification = "not-comparable" if reasons else "comparable"
    else:
        classification = "not-comparable"
        reasons = []

    evidence = (
        [f"candidate:tasks/{task_id}.json"]
        if classification == "missing-in-baseline"
        else [f"baseline:tasks/{task_id}.json"]
        if classification == "missing-in-candidate"
        else [f"baseline:tasks/{task_id}.json", f"candidate:tasks/{task_id}.json"]
    )
    correctness = None
    execution = None
    failure = None
    metrics: list[dict[str, Any]] = []
    if baseline_present and candidate_present:
        correctness = {
            field: {
                "baseline": task["correctness"][field]["baseline"],
                "candidate": task["correctness"][field]["candidate"],
                "transition": _transition(
                    task["correctness"][field]["baseline"],
                    task["correctness"][field]["candidate"],
                ),
            }
            for field in CORRECTNESS_FIELDS
        }
        execution = {
            "baseline": task["execution"]["baseline"],
            "candidate": task["execution"]["candidate"],
            "transition": _transition(
                task["execution"]["baseline"], task["execution"]["candidate"]
            ),
        }
        failure = {
            "baseline_category": task["failure"]["baseline_category"],
            "candidate_category": task["failure"]["candidate_category"],
            "fixed_failure": _fixed_failure(correctness),
            "introduced_failure": _introduced_failure(correctness),
            "transition": _transition(
                task["failure"]["baseline_category"],
                task["failure"]["candidate_category"],
            ),
        }
        if classification == "comparable":
            metrics = [_canonical_metric(task_id, metric) for metric in task["metrics"]]

    task.update(
        {
            "classification": classification,
            "comparability_reasons": reasons,
            "correctness": correctness,
            "evidence": evidence,
            "execution": execution,
            "failure": failure,
            "metrics": metrics,
        }
    )
    return task


def _task_record(
    task_id: str,
    baseline: dict[str, Any] | None,
    candidate: dict[str, Any] | None,
    build_profiles_equal: bool,
) -> dict[str, Any]:
    baseline_profile = {}
    candidate_profile = {} if build_profiles_equal else {"different": True}
    return normalize_task_record(
        _primitive_task_record(
            task_id, baseline, candidate, baseline_profile, candidate_profile
        )
    )


def _policy_reasons(
    build: dict[str, Any],
    task_records: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    reasons: list[dict[str, Any]] = []

    if build["baseline"] == "pass" and build["candidate"] != "pass":
        reasons.append(
            {
                "code": "candidate-build-regression",
                "decision": "reject",
                "evidence": build["evidence"],
                "task_id": "open-build",
            }
        )
    elif build["baseline"] != "pass":
        reasons.append(
            {
                "code": "baseline-build-incomplete",
                "decision": "manual-review",
                "evidence": [build["evidence"][0]],
                "task_id": "open-build",
            }
        )

    for record in task_records:
        task_id = record["task_id"]
        classification = record["classification"]
        if classification == "missing-in-candidate":
            reasons.append(
                {
                    "code": "required-regression-missing",
                    "decision": "reject",
                    "evidence": record["evidence"],
                    "task_id": task_id,
                }
            )
            continue
        if classification == "missing-in-baseline":
            reasons.append(
                {
                    "code": "new-candidate-regression",
                    "decision": "manual-review",
                    "evidence": record["evidence"],
                    "task_id": task_id,
                }
            )
            continue
        if classification == "not-comparable":
            reasons.append(
                {
                    "code": "configuration-not-comparable",
                    "decision": "manual-review",
                    "evidence": record["evidence"],
                    "task_id": task_id,
                }
            )

        correctness = record["correctness"]
        if correctness is None:
            continue
        baseline_overall = correctness["overall"]["baseline"]
        if baseline_overall != "pass":
            reasons.append(
                {
                    "code": "baseline-regression-not-passing",
                    "decision": "manual-review",
                    "evidence": [f"baseline:tasks/{task_id}.json#outcome"],
                    "task_id": task_id,
                }
            )
        for field, code in (
            ("synthesis", "candidate-synthesis-regression"),
            ("simulation", "candidate-simulation-regression"),
        ):
            if (
                correctness[field]["baseline"] == "pass"
                and correctness[field]["candidate"] == "fail"
            ):
                reasons.append(
                    {
                        "code": code,
                        "decision": "reject",
                        "evidence": record["evidence"],
                        "task_id": task_id,
                    }
                )
        if correctness["verification"]["candidate"] == "fail":
            reasons.append(
                {
                    "code": "candidate-verification-failure",
                    "decision": "reject",
                    "evidence": record["evidence"],
                    "task_id": task_id,
                }
            )
        cycles = next(
            (
                metric
                for metric in record["metrics"]
                if metric["metric_id"] == "simulation.total-cycles"
            ),
            None,
        )
        if cycles is not None and (
            cycles["baseline_value"] is None or cycles["candidate_value"] is None
        ):
            reasons.append(
                {
                    "code": "cycle-information-unavailable",
                    "decision": "manual-review",
                    "evidence": cycles["evidence"],
                    "task_id": task_id,
                }
            )
    return sorted(reasons, key=lambda item: (item["decision"], item["code"], item["task_id"]))


def _decision(reasons: list[dict[str, Any]]) -> str:
    decisions = {reason["decision"] for reason in reasons}
    if "reject" in decisions:
        return "reject"
    if "manual-review" in decisions:
        return "manual-review"
    return "accept"


def _summary(task_records: list[dict[str, Any]], reasons: list[dict[str, Any]]) -> dict[str, int]:
    comparable = [item for item in task_records if item["classification"] == "comparable"]
    return {
        "comparable_tasks": len(comparable),
        "correctness_improvements": sum(
            1 for item in comparable if item["failure"]["fixed_failure"]
        ),
        "correctness_regressions": sum(
            1 for item in comparable if item["failure"]["introduced_failure"]
        ),
        "manual_review_items": sum(
            1 for reason in reasons if reason["decision"] == "manual-review"
        ),
        "missing_candidate_tasks": sum(
            1 for item in task_records if item["classification"] == "missing-in-candidate"
        ),
    }


def compare_bundles(
    baseline_bundle: Path,
    candidate_bundle: Path,
    output: Path,
) -> dict[str, Any]:
    """Validate both bundles, compare them, write and revalidate canonical JSON."""

    baseline_path = baseline_bundle.resolve()
    candidate_path = candidate_bundle.resolve()
    try:
        baseline_documents = validate_bundle(baseline_path)
    except (BundleValidationError, SchemaValidationError, SerializationError, OSError, ValueError) as error:
        raise ComparisonInputError(f"baseline bundle validation failed: {error}") from error
    try:
        candidate_documents = validate_bundle(candidate_path)
    except (BundleValidationError, SchemaValidationError, SerializationError, OSError, ValueError) as error:
        raise ComparisonInputError(f"candidate bundle validation failed: {error}") from error

    for label, documents in (
        ("baseline", baseline_documents),
        ("candidate", candidate_documents),
    ):
        version = documents["manifest.json"].get("schema_version")
        if version != MULTI_TASK_SCHEMA_VERSION:
            raise ComparisonInputError(
                f"{label} bundle uses unsupported comparison protocol version {version!r}; "
                f"expected {MULTI_TASK_SCHEMA_VERSION!r}"
            )

    baseline_identity = _identity(baseline_documents, baseline_path)
    candidate_identity = _identity(candidate_documents, candidate_path)
    build = _build_record(baseline_documents, candidate_documents)
    comparison_id = hashlib.sha256(
        canonical_bytes(
            {"baseline": baseline_identity, "candidate": candidate_identity}
        )
    ).hexdigest()
    baseline_tasks = _regression_tasks(baseline_documents)
    candidate_tasks = _regression_tasks(candidate_documents)
    baseline_profile = _build_profile(baseline_documents)
    candidate_profile = _build_profile(candidate_documents)
    task_records = [
        normalize_task_record(
            _primitive_task_record(
                task_id,
                baseline_tasks.get(task_id),
                candidate_tasks.get(task_id),
                baseline_profile,
                candidate_profile,
            )
        )
        for task_id in sorted(set(baseline_tasks) | set(candidate_tasks))
    ]
    reasons = _policy_reasons(build, task_records)
    decision = _decision(reasons)
    document = {
        "baseline": baseline_identity,
        "build": build,
        "candidate": candidate_identity,
        "comparison_id": comparison_id,
        "generated_at": candidate_documents["manifest.json"].get("completed_at"),
        "overall_comparison_outcome": {
            "accept": "pass",
            "manual-review": "manual-review",
            "reject": "regression",
        }[decision],
        "policy": {"decision": decision, "reasons": reasons},
        "schema": COMPARISON_SCHEMA,
        "schema_version": COMPARISON_SCHEMA_VERSION,
        "summary": _summary(task_records, reasons),
        "tasks": task_records,
    }
    write_json(output, document)
    return validate_comparison(output)


def _expected_comparison_id(document: dict[str, Any]) -> str:
    return hashlib.sha256(
        canonical_bytes(
            {"baseline": document["baseline"], "candidate": document["candidate"]}
        )
    ).hexdigest()


def validate_comparison(path: Path) -> dict[str, Any]:
    """Validate canonical bytes and reconstructed comparison semantics."""

    value = load_json(path)
    require_canonical(path, value)
    if not isinstance(value, dict):
        raise ComparisonError("comparison document must be an object")
    repository = Path(__file__).resolve().parents[3]
    validator = SchemaValidator(repository / ".github" / "schemas" / "ci" / "v1")
    validator.validate(value, COMPARISON_SCHEMA_FILE)
    errors: list[str] = []
    if value["comparison_id"] != _expected_comparison_id(value):
        errors.append("comparison_id does not match baseline/candidate identities")
    task_ids = [task["task_id"] for task in value["tasks"]]
    if task_ids != sorted(task_ids) or len(task_ids) != len(set(task_ids)):
        errors.append("tasks must be unique and sorted by task_id")
    build = value["build"]
    if build["transition"] != _transition(build["baseline"], build["candidate"]):
        errors.append("build transition does not match build outcomes")
    if build["evidence"] != [
        "baseline:tasks/open-build.json#outcome",
        "candidate:tasks/open-build.json#outcome",
    ]:
        errors.append("build evidence does not use canonical provenance")
    normalized_tasks: list[dict[str, Any]] = []
    expected_check_fields = ["task_id", "task_type", *CONFIGURATION_PATHS, "build_profile"]
    for task in value["tasks"]:
        task_id = task["task_id"]
        presence = task["presence"]
        paired = presence["baseline"] and presence["candidate"]
        missing = presence["baseline"] != presence["candidate"]
        if not paired and not missing:
            errors.append(f"task {task_id!r}: invalid task presence")
            continue
        checks = task.get("comparability_checks", [])
        if paired:
            check_fields = [check["field"] for check in checks]
            if check_fields != expected_check_fields:
                errors.append(
                    f"task {task_id!r}: comparability checks are not canonical"
                )
            for check in checks:
                if check["reason_code"] not in ALLOWED_COMPARABILITY_REASONS:
                    errors.append(
                        f"task {task_id!r}: unknown comparability reason code"
                    )
        elif checks:
            errors.append(f"task {task_id!r}: missing task must not contain checks")
        reason_keys = [(item["code"], item["field"]) for item in task["comparability_reasons"]]
        if any(code not in ALLOWED_COMPARABILITY_REASONS for code, _field in reason_keys):
            errors.append(f"task {task_id!r}: unknown comparability reason")
        if len(reason_keys) != len(set(reason_keys)):
            errors.append(f"task {task_id!r}: duplicate comparability reason")
        if reason_keys != sorted(reason_keys):
            errors.append(f"task {task_id!r}: comparability reasons are not sorted")
        metric_ids = [metric["metric_id"] for metric in task["metrics"]]
        if len(metric_ids) != len(set(metric_ids)):
            errors.append(f"task {task_id!r}: duplicate metric ID")
        if any(metric_id not in METRIC_DEFINITIONS for metric_id in metric_ids):
            errors.append(f"task {task_id!r}: unknown metric ID")
        expected_metric_ids = [metric_id for metric_id, _source, _unit in METRIC_SOURCES]
        if task["classification"] == "comparable" and metric_ids != expected_metric_ids:
            errors.append(f"task {task_id!r}: metrics are not the canonical set")
        if task["classification"] != "comparable" and task["metrics"]:
            errors.append(
                f"task {task_id!r}: non-comparable task must not contain metric deltas"
            )
        if errors:
            continue
        try:
            normalized = normalize_task_record(task)
        except (KeyError, TypeError, ValueError) as error:
            errors.append(f"task {task_id!r}: cannot reconstruct task: {error}")
            continue
        normalized_tasks.append(normalized)
        for field in (
            "classification",
            "comparability_reasons",
            "correctness",
            "evidence",
            "execution",
            "failure",
        ):
            if task[field] != normalized[field]:
                errors.append(f"task {task_id!r}: {field} does not match reconstruction")
        for metric, expected_metric in zip(task["metrics"], normalized["metrics"]):
            if metric["absolute_delta"] != expected_metric["absolute_delta"]:
                errors.append(
                    f"task {task_id!r} metric {metric['metric_id']!r}: invalid absolute delta"
                )
            if metric["percentage_delta"] != expected_metric["percentage_delta"]:
                errors.append(
                    f"task {task_id!r} metric {metric['metric_id']!r}: invalid percentage delta"
                )
        if task["metrics"] != normalized["metrics"] and not any(
            "invalid absolute delta" in error or "invalid percentage delta" in error
            for error in errors
        ):
            errors.append(f"task {task_id!r}: metrics does not match reconstruction")
    if errors:
        raise ComparisonError("\n".join(errors))
    expected_reasons = _policy_reasons(value["build"], normalized_tasks)
    if any(reason["code"] not in ALLOWED_POLICY_REASONS for reason in value["policy"]["reasons"]):
        errors.append("policy contains unsupported reason")
    if value["policy"]["reasons"] != expected_reasons:
        errors.append("policy reasons do not match reconstructed comparison evidence")
    expected_summary = _summary(normalized_tasks, expected_reasons)
    if value["summary"] != expected_summary:
        errors.append("summary does not match task and policy records")
    expected_decision = _decision(expected_reasons)
    if value["policy"]["decision"] != expected_decision:
        errors.append("policy decision does not match policy reasons")
    expected_outcome = {
        "accept": "pass",
        "manual-review": "manual-review",
        "reject": "regression",
    }[expected_decision]
    if value["overall_comparison_outcome"] != expected_outcome:
        errors.append("overall comparison outcome does not match policy decision")
    if errors:
        raise ComparisonError("\n".join(errors))
    return value


def render_comparison(document: dict[str, Any]) -> str:
    """Render a human summary exclusively from a validated comparison document."""

    lines = [
        "| Task | Correctness | Baseline cycles | Candidate cycles | Delta |",
        "| --- | --- | ---: | ---: | ---: |",
    ]
    for task in document["tasks"]:
        if task["classification"] != "comparable":
            lines.append(
                f"| `{task['task_id']}` | {task['classification']} | n/a | n/a | n/a |"
            )
            continue
        correctness = task["correctness"]["overall"]["transition"]
        cycles = next(
            metric
            for metric in task["metrics"]
            if metric["metric_id"] == "simulation.total-cycles"
        )
        baseline = "n/a" if cycles["baseline_value"] is None else str(cycles["baseline_value"])
        candidate = "n/a" if cycles["candidate_value"] is None else str(cycles["candidate_value"])
        percentage = cycles["percentage_delta"]
        delta = "n/a" if percentage is None else f"{percentage:+.2f}%"
        lines.append(
            f"| `{task['task_id']}` | {correctness} | {baseline} | {candidate} | {delta} |"
        )
    summary = document["summary"]
    lines.extend(
        [
            "",
            f"Comparable tasks: {summary['comparable_tasks']}",
            f"Correctness regressions: {summary['correctness_regressions']}",
            f"Correctness improvements: {summary['correctness_improvements']}",
            f"Missing candidate tasks: {summary['missing_candidate_tasks']}",
            f"Manual-review items: {summary['manual_review_items']}",
            f"Policy decision: {document['policy']['decision']}",
            "",
        ]
    )
    return "\n".join(lines)


def render_comparison_file(path: Path) -> str:
    return render_comparison(validate_comparison(path))
