"""Deterministic comparison of independently validated PandA CI bundles."""

from __future__ import annotations

import copy
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

METRIC_SOURCES = (
    ("simulation.total-cycles", None, "cycles"),
    ("duration.hls-synthesis", "duration.hls-synthesis", "seconds"),
    ("duration.rtl-simulation", "duration.rtl-simulation", "seconds"),
    ("duration.regression-total", "duration.regression-total", "seconds"),
)

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

BUILD_CONFIGURATION_FIELDS = (
    "configured_parallelism",
    "build_type",
    "release_enabled",
    "assertions_enabled",
    "warnings_as_errors",
    "selected_frontend",
    "synthesis_smoke_enabled",
    "cache_mode",
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


def _build_profile(documents: dict[str, dict[str, Any]]) -> dict[str, Any]:
    manifest = documents["manifest.json"]
    build_task = documents["tasks/open-build.json"]
    configuration = build_task["configuration"]
    return {
        "configuration": {
            field: copy.deepcopy(configuration.get(field))
            for field in BUILD_CONFIGURATION_FIELDS
        },
        "container": copy.deepcopy(manifest.get("container")),
        "runner": copy.deepcopy(manifest.get("runner")),
        "tools": copy.deepcopy(manifest.get("tools")),
        "workflow_file": _value_at(manifest, "workflow.file"),
    }


def _regression_tasks(
    documents: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    return {
        task["task_id"]: task
        for path, task in documents.items()
        if path.startswith("tasks/") and task.get("task_type") == "regression"
    }


def _configuration_reasons(
    baseline: dict[str, Any],
    candidate: dict[str, Any],
    build_profiles_equal: bool,
) -> list[dict[str, Any]]:
    reasons: list[dict[str, Any]] = []
    if baseline.get("task_id") != candidate.get("task_id"):
        reasons.append({"code": "task-id-differs", "field": "task_id"})
    if baseline.get("task_type") != candidate.get("task_type"):
        reasons.append({"code": "task-type-differs", "field": "task_type"})
    for path in CONFIGURATION_PATHS:
        baseline_value = _value_at(baseline.get("configuration", {}), path)
        candidate_value = _value_at(candidate.get("configuration", {}), path)
        if baseline_value != candidate_value:
            reasons.append({"code": "configuration-differs", "field": path})
    if not build_profiles_equal:
        reasons.append({"code": "build-profile-differs", "field": "build_profile"})
    return reasons


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


def _task_record(
    task_id: str,
    baseline: dict[str, Any] | None,
    candidate: dict[str, Any] | None,
    build_profiles_equal: bool,
) -> dict[str, Any]:
    if baseline is None:
        return {
            "classification": "missing-in-baseline",
            "comparability_reasons": [{"code": "task-added-in-candidate", "field": "task_id"}],
            "correctness": None,
            "evidence": [f"candidate:tasks/{task_id}.json"],
            "execution": None,
            "failure": None,
            "metrics": [],
            "task_id": task_id,
            "task_type": candidate["task_type"],
        }
    if candidate is None:
        return {
            "classification": "missing-in-candidate",
            "comparability_reasons": [{"code": "required-task-missing", "field": "task_id"}],
            "correctness": None,
            "evidence": [f"baseline:tasks/{task_id}.json"],
            "execution": None,
            "failure": None,
            "metrics": [],
            "task_id": task_id,
            "task_type": baseline["task_type"],
        }

    comparability_reasons = _configuration_reasons(
        baseline, candidate, build_profiles_equal
    )
    classification = "not-comparable" if comparability_reasons else "comparable"
    baseline_correctness = _correctness(baseline)
    candidate_correctness = _correctness(candidate)
    baseline_failure = _value_at(baseline, "failure.category")
    candidate_failure = _value_at(candidate, "failure.category")
    introduced_failure = any(
        baseline_correctness[field] == "pass" and candidate_correctness[field] == "fail"
        for field in ("synthesis", "simulation", "verification")
    )
    fixed_failure = any(
        baseline_correctness[field] == "fail" and candidate_correctness[field] == "pass"
        for field in ("synthesis", "simulation", "verification")
    )
    correctness = {
        field: {
            "baseline": baseline_correctness[field],
            "candidate": candidate_correctness[field],
            "transition": _transition(
                baseline_correctness[field], candidate_correctness[field]
            ),
        }
        for field in ("synthesis", "simulation", "verification", "overall")
    }
    return {
        "classification": classification,
        "comparability_reasons": comparability_reasons,
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
            "fixed_failure": fixed_failure,
            "introduced_failure": introduced_failure,
            "transition": _transition(baseline_failure, candidate_failure),
        },
        "metrics": _metric_comparison(task_id, baseline, candidate)
        if classification == "comparable"
        else [],
        "task_id": task_id,
        "task_type": baseline["task_type"],
    }


def _policy_reasons(
    baseline_documents: dict[str, dict[str, Any]],
    candidate_documents: dict[str, dict[str, Any]],
    task_records: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    reasons: list[dict[str, Any]] = []

    baseline_build = baseline_documents["tasks/open-build.json"]
    candidate_build = candidate_documents["tasks/open-build.json"]
    if baseline_build["outcome"] == "pass" and candidate_build["outcome"] != "pass":
        reasons.append(
            {
                "code": "candidate-build-regression",
                "decision": "reject",
                "evidence": [
                    "baseline:tasks/open-build.json#outcome",
                    "candidate:tasks/open-build.json#outcome",
                ],
                "task_id": "open-build",
            }
        )
    elif baseline_build["outcome"] != "pass":
        reasons.append(
            {
                "code": "baseline-build-incomplete",
                "decision": "manual-review",
                "evidence": ["baseline:tasks/open-build.json#outcome"],
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
            continue

        correctness = record["correctness"]
        assert correctness is not None
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
            metric for metric in record["metrics"] if metric["metric_id"] == "simulation.total-cycles"
        )
        if cycles["baseline_value"] is None or cycles["candidate_value"] is None:
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
    comparison_id = hashlib.sha256(
        canonical_bytes(
            {"baseline": baseline_identity, "candidate": candidate_identity}
        )
    ).hexdigest()
    baseline_tasks = _regression_tasks(baseline_documents)
    candidate_tasks = _regression_tasks(candidate_documents)
    build_profiles_equal = _build_profile(baseline_documents) == _build_profile(
        candidate_documents
    )
    task_records = [
        _task_record(
            task_id,
            baseline_tasks.get(task_id),
            candidate_tasks.get(task_id),
            build_profiles_equal,
        )
        for task_id in sorted(set(baseline_tasks) | set(candidate_tasks))
    ]
    reasons = _policy_reasons(
        baseline_documents, candidate_documents, task_records
    )
    decision = _decision(reasons)
    document = {
        "baseline": baseline_identity,
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
    """Validate comparison schema, canonical bytes, deltas, counts, and policy."""

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
    expected_summary = _summary(value["tasks"], value["policy"]["reasons"])
    if value["summary"] != expected_summary:
        errors.append("summary does not match task and policy records")
    expected_decision = _decision(value["policy"]["reasons"])
    if value["policy"]["decision"] != expected_decision:
        errors.append("policy decision does not match policy reasons")
    expected_outcome = {
        "accept": "pass",
        "manual-review": "manual-review",
        "reject": "regression",
    }[expected_decision]
    if value["overall_comparison_outcome"] != expected_outcome:
        errors.append("overall comparison outcome does not match policy decision")
    for task in value["tasks"]:
        if task["classification"] != "comparable" and task["metrics"]:
            errors.append(
                f"task {task['task_id']!r}: non-comparable task must not contain metric deltas"
            )
        for metric in task["metrics"]:
            baseline = metric["baseline_value"]
            candidate = metric["candidate_value"]
            expected_absolute = None
            expected_percentage = None
            if baseline is not None and candidate is not None:
                expected_absolute = candidate - baseline
                if baseline != 0:
                    expected_percentage = round((expected_absolute / baseline) * 100, 6)
            if metric["absolute_delta"] != expected_absolute:
                errors.append(
                    f"task {task['task_id']!r} metric {metric['metric_id']!r}: invalid absolute delta"
                )
            if metric["percentage_delta"] != expected_percentage:
                errors.append(
                    f"task {task['task_id']!r} metric {metric['metric_id']!r}: invalid percentage delta"
                )
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
