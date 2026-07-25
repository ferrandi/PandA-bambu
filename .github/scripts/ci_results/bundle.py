"""Schema and cross-document validation for a PandA CI result bundle."""

from __future__ import annotations

from pathlib import Path, PurePosixPath
from typing import Any, Iterable

from .constants import (
    ARTIFACT_IDS,
    CHECK_IDS,
    DOCUMENT_PATHS,
    METRIC_CONTRACTS,
    METRIC_IDS,
    RULE_IDS,
    SCHEMA_FILES,
    STAGE_IDS,
)
from .hashing import sha256_file
from .schema import SchemaValidationError, SchemaValidator
from .serialization import SerializationError, load_json, require_canonical


class BundleValidationError(ValueError):
    """Raised when one or more bundle invariants are violated."""

    def __init__(self, errors: Iterable[str]):
        self.errors = tuple(errors)
        super().__init__("\n".join(self.errors))


def _safe_relative_path(value: Any) -> bool:
    if not isinstance(value, str) or not value or "\\" in value:
        return False
    path = PurePosixPath(value)
    return not path.is_absolute() and ".." not in path.parts


def _unique_index(
    items: Any,
    key: str,
    location: str,
    errors: list[str],
) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    if not isinstance(items, list):
        return result
    for index, item in enumerate(items):
        if not isinstance(item, dict):
            continue
        identifier = item.get(key)
        if not isinstance(identifier, str):
            continue
        if identifier in result:
            errors.append(f"{location}[{index}]: duplicate {key} {identifier!r}")
        result[identifier] = item
    return result


def _exact_ids(
    actual: list[str],
    expected: tuple[str, ...],
    location: str,
    errors: list[str],
) -> None:
    if actual != list(expected):
        errors.append(
            f"{location}: expected stable order {list(expected)!r}, got {actual!r}"
        )


def _supported_version(document: Any, path: str, errors: list[str]) -> bool:
    if not isinstance(document, dict):
        errors.append(f"{path}: document must be an object")
        return False
    version = document.get("schema_version")
    if not isinstance(version, str) or "." not in version:
        errors.append(f"{path}.schema_version: expected major.minor string")
        return False
    major = version.split(".", 1)[0]
    if major != "1":
        errors.append(f"{path}.schema_version: unsupported major schema version {version!r}")
        return False
    return True


def _validate_evidence_reference(
    reference: Any,
    stages: dict[str, dict[str, Any]],
    metrics: dict[str, dict[str, Any]],
    checks: dict[str, dict[str, Any]],
    artifacts: dict[str, dict[str, Any]],
    errors: list[str],
    location: str,
) -> None:
    if not isinstance(reference, str) or "#" not in reference:
        errors.append(f"{location}: malformed evidence reference {reference!r}")
        return
    document, fragment = reference.split("#", 1)
    if document == "request.json" and fragment == "policy-profile":
        return
    if document == "tasks/open-build.json":
        if fragment == "outcome":
            return
        kind, separator, identifier = fragment.partition("/")
        targets = {"stage": stages, "metric": metrics, "check": checks}.get(kind)
        if separator and targets is not None and identifier in targets:
            return
    if document == "artifacts.json":
        kind, separator, identifier = fragment.partition("/")
        if separator and kind == "artifact" and identifier in artifacts:
            return
    errors.append(f"{location}: unresolved evidence reference {reference!r}")


def _failure_invariants(
    value: Any,
    outcome: Any,
    location: str,
    evidence_context: tuple[
        dict[str, dict[str, Any]],
        dict[str, dict[str, Any]],
        dict[str, dict[str, Any]],
        dict[str, dict[str, Any]],
    ],
    errors: list[str],
) -> None:
    if outcome == "fail" and value is None:
        errors.append(f"{location}: failed result requires a failure object")
    if outcome in {"pass", "skipped"} and value is not None:
        errors.append(f"{location}: {outcome} result must have null failure")
    if isinstance(value, dict):
        failure_stage = value.get("stage")
        if failure_stage is not None and failure_stage not in evidence_context[0]:
            errors.append(f"{location}: failure references unknown stage {failure_stage!r}")
        for index, reference in enumerate(value.get("evidence", [])):
            _validate_evidence_reference(
                reference,
                *evidence_context,
                errors,
                f"{location}.failure.evidence[{index}]",
            )


def validate_bundle(
    bundle_directory: Path,
    schema_directory: Path | None = None,
    require_deterministic: bool = True,
) -> dict[str, dict[str, Any]]:
    """Validate schemas, document identities, references, and policy semantics."""

    bundle = bundle_directory.resolve()
    repository = Path(__file__).resolve().parents[3]
    schemas = schema_directory or repository / ".github" / "schemas" / "ci" / "v1"
    errors: list[str] = []
    required_paths = ("manifest.json",) + DOCUMENT_PATHS
    documents: dict[str, dict[str, Any]] = {}

    for relative in required_paths:
        path = bundle / relative
        if not path.is_file():
            errors.append(f"{relative}: required document is missing")
            continue
        try:
            value = load_json(path)
            if require_deterministic:
                require_canonical(path, value)
        except (SerializationError, OSError) as error:
            errors.append(str(error))
            continue
        if isinstance(value, dict):
            documents[relative] = value
        else:
            errors.append(f"{relative}: top-level value must be an object")

    allowed_files = set(required_paths)
    allowed_directories = {"tasks"}
    if bundle.is_dir():
        for path in bundle.rglob("*"):
            relative = path.relative_to(bundle).as_posix()
            if path.is_symlink():
                errors.append(f"{relative}: symbolic links are not allowed in the bundle")
            elif path.is_dir() and relative not in allowed_directories:
                errors.append(f"{relative}: unexpected directory in result bundle")
            elif path.is_file() and relative not in allowed_files:
                errors.append(f"{relative}: unexpected file in result bundle")

    schema_validator = SchemaValidator(schemas)
    for relative, document in documents.items():
        if not _supported_version(document, relative, errors):
            continue
        marker = document.get("schema")
        schema_file = SCHEMA_FILES.get(marker)
        if schema_file is None:
            errors.append(f"{relative}.schema: unknown schema marker {marker!r}")
            continue
        try:
            schema_validator.validate(document, schema_file)
        except SchemaValidationError as error:
            errors.extend(f"{relative}: {line}" for line in str(error).splitlines())

    if any(relative not in documents for relative in required_paths):
        raise BundleValidationError(errors)

    manifest = documents["manifest.json"]
    request = documents["request.json"]
    task = documents["tasks/open-build.json"]
    artifact_document = documents["artifacts.json"]
    verdict = documents["verdict.json"]

    stage_list = task.get("stages", [])
    metric_list = task.get("metrics", [])
    check_list = task.get("checks", [])
    artifact_list = artifact_document.get("artifacts", [])
    rule_list = verdict.get("rules", [])
    stages = _unique_index(stage_list, "stage_id", "tasks/open-build.json.stages", errors)
    metrics = _unique_index(metric_list, "metric_id", "tasks/open-build.json.metrics", errors)
    checks = _unique_index(check_list, "check_id", "tasks/open-build.json.checks", errors)
    artifacts = _unique_index(
        artifact_list, "artifact_id", "artifacts.json.artifacts", errors
    )
    rules = _unique_index(rule_list, "rule_id", "verdict.json.rules", errors)
    context = (stages, metrics, checks, artifacts)

    _exact_ids([item.get("stage_id") for item in stage_list], STAGE_IDS, "stages", errors)
    _exact_ids([item.get("metric_id") for item in metric_list], METRIC_IDS, "metrics", errors)
    _exact_ids([item.get("check_id") for item in check_list], CHECK_IDS, "checks", errors)
    _exact_ids(
        [item.get("artifact_id") for item in artifact_list], ARTIFACT_IDS, "artifacts", errors
    )
    _exact_ids([item.get("rule_id") for item in rule_list], RULE_IDS, "rules", errors)

    document_refs = manifest.get("documents", [])
    ref_index = _unique_index(document_refs, "document_id", "manifest.json.documents", errors)
    expected_refs = {
        "artifacts": ("artifacts.json", "panda.ci.artifact-index"),
        "open-build": ("tasks/open-build.json", "panda.ci.task-result"),
        "request": ("request.json", "panda.ci.request"),
        "verdict": ("verdict.json", "panda.ci.verdict"),
    }
    if [item.get("document_id") for item in document_refs] != sorted(expected_refs):
        errors.append("manifest.json.documents: references must be sorted by document_id")
    if set(ref_index) != set(expected_refs):
        errors.append("manifest.json.documents: must reference every required result document")
    for document_id, (relative, marker) in expected_refs.items():
        reference = ref_index.get(document_id)
        if reference is None:
            continue
        if reference.get("path") != relative or not _safe_relative_path(reference.get("path")):
            errors.append(f"manifest document {document_id!r}: broken path reference")
        target = documents.get(relative)
        if target is None:
            continue
        if reference.get("schema") != marker or target.get("schema") != marker:
            errors.append(f"manifest document {document_id!r}: schema marker mismatch")
        if reference.get("schema_version") != target.get("schema_version"):
            errors.append(f"manifest document {document_id!r}: schema version mismatch")
        actual_hash = sha256_file(bundle / relative)
        if reference.get("sha256") != actual_hash:
            errors.append(f"manifest document {document_id!r}: SHA-256 mismatch")

    identity_fields = ("repository", "base_sha", "ref", "pull_request_number")
    for field in identity_fields:
        if manifest.get(field) != request.get(field):
            errors.append(f"manifest/request identity mismatch for {field}")
    if manifest.get("request_id") != request.get("request_id"):
        errors.append("manifest/request request_id mismatch")
    if request.get("requested_task_ids") != ["open-build"]:
        errors.append("request.json.requested_task_ids must contain only open-build")
    if request.get("requested_artifact_ids") != list(ARTIFACT_IDS):
        errors.append(
            "request.json.requested_artifact_ids must contain every v1 artifact in stable order"
        )
    request_tasks = request.get("tasks", [])
    if request_tasks != [{"task_id": "open-build", "task_type": "build"}]:
        errors.append("request.json.tasks does not match the open-build task result")
    if task.get("task_id") != "open-build" or task.get("task_type") != "build":
        errors.append("tasks/open-build.json: task identity mismatch")
    request_parameters = request.get("build_parameters", {})
    task_configuration = task.get("configuration", {})
    for field in (
        "configured_parallelism",
        "build_type",
        "release_enabled",
        "assertions_enabled",
        "warnings_as_errors",
        "synthesis_smoke_enabled",
        "cache_mode",
    ):
        if request_parameters.get(field) != task_configuration.get(field):
            errors.append(f"request/task configuration mismatch for {field}")
    if manifest.get("configured_parallelism") != request_parameters.get(
        "configured_parallelism"
    ):
        errors.append("manifest/request configured parallelism mismatch")
    frontend_stage = stages.get("frontend-resolution", {})
    observed_frontend = task_configuration.get("selected_frontend")
    if frontend_stage.get("outcome") == "pass" and not observed_frontend:
        errors.append("passing frontend-resolution requires an observed selected_frontend")
    if frontend_stage.get("outcome") == "skipped" and observed_frontend is not None:
        errors.append("skipped frontend-resolution requires null selected_frontend")

    for stage_id, stage in stages.items():
        for metric_id in stage.get("metric_ids", []):
            if metric_id not in metrics:
                errors.append(f"stage {stage_id!r}: unknown metric reference {metric_id!r}")
        for artifact_id in stage.get("artifact_ids", []):
            if artifact_id not in artifacts:
                errors.append(f"stage {stage_id!r}: unknown artifact reference {artifact_id!r}")
        duration_metric = metrics.get(f"duration.{stage_id}")
        if duration_metric is not None and stage.get("duration_seconds") != duration_metric.get(
            "value"
        ):
            errors.append(f"stage {stage_id!r}: duration does not match its metric")
        outcome = stage.get("outcome")
        if outcome == "skipped":
            if stage.get("execution_state") != "completed":
                errors.append(f"stage {stage_id!r}: skipped stage state must be completed")
            if stage.get("duration_seconds") is not None or stage.get("exit_status") is not None:
                errors.append(f"stage {stage_id!r}: skipped stage must use null duration/status")
        if outcome == "pass" and stage.get("exit_status") != 0:
            errors.append(f"stage {stage_id!r}: passing stage must have exit status 0")
        if outcome == "pass" and stage.get("execution_state") != "completed":
            errors.append(f"stage {stage_id!r}: passing stage state must be completed")
        if outcome == "fail" and stage.get("execution_state") not in {
            "completed",
            "infrastructure_error",
        }:
            errors.append(f"stage {stage_id!r}: failed stage has incompatible state")
        if outcome == "fail" and stage.get("exit_status") == 0:
            errors.append(f"stage {stage_id!r}: failed stage cannot have exit status 0")
        if outcome == "unknown" and stage.get("execution_state") not in {
            "canceled",
            "timed_out",
        }:
            errors.append(f"stage {stage_id!r}: unknown outcome has incompatible state")
        if outcome == "unknown" and stage.get("failure") is None:
            errors.append(f"stage {stage_id!r}: unknown terminal stage requires failure evidence")
        if isinstance(stage.get("failure"), dict) and stage["failure"].get("stage") != stage_id:
            errors.append(f"stage {stage_id!r}: failure points to a different stage")
        _failure_invariants(stage.get("failure"), outcome, f"stage {stage_id!r}", context, errors)

    if task.get("artifacts") != list(ARTIFACT_IDS):
        errors.append("task artifacts must contain every v1 artifact in stable order")
    for artifact_id in task.get("artifacts", []):
        if artifact_id not in artifacts:
            errors.append(f"task: unknown artifact reference {artifact_id!r}")
    for artifact_id, artifact in artifacts.items():
        if not _safe_relative_path(artifact.get("path")):
            errors.append(f"artifact {artifact_id!r}: path is not safe and bundle-relative")
        if artifact.get("producer_task") != "open-build":
            errors.append(f"artifact {artifact_id!r}: producer_task must be open-build")
        if artifact.get("available") is False and (
            artifact.get("size_bytes") is not None or artifact.get("sha256") is not None
        ):
            errors.append(f"artifact {artifact_id!r}: unavailable artifact must have null size/hash")
        associated_stage = artifact.get("associated_stage")
        if associated_stage is not None and associated_stage not in stages:
            errors.append(f"artifact {artifact_id!r}: unknown associated stage")

    for check_id, check in checks.items():
        _failure_invariants(
            check.get("failure"), check.get("outcome"), f"check {check_id!r}", context, errors
        )
    _failure_invariants(
        task.get("failure"), task.get("outcome"), "task open-build", context, errors
    )

    executable_outcomes: list[str] = []
    for check_id in CHECK_IDS[:3]:
        check = checks.get(check_id, {})
        details = check.get("details")
        if not isinstance(details, dict):
            errors.append(f"check {check_id!r}: executable check requires details")
            continue
        exists = details.get("exists")
        starts = details.get("starts")
        expected = (
            "pass"
            if exists is True and starts is True
            else "fail"
            if exists is False or starts is False
            else "skipped"
        )
        if check.get("outcome") != expected:
            errors.append(f"check {check_id!r}: outcome contradicts executable details")
        executable_outcomes.append(check.get("outcome"))

    executable_stage = stages.get("installed-executable-validation", {})
    executable_stage_outcome = executable_stage.get("outcome")
    if executable_stage_outcome == "pass" and not (
        len(executable_outcomes) == 3
        and all(value == "pass" for value in executable_outcomes)
    ):
        errors.append("passing installed executable stage requires every check to pass")
    if executable_stage_outcome == "skipped" and any(
        value != "skipped" for value in executable_outcomes
    ):
        errors.append("skipped installed executable stage requires skipped checks")
    if executable_stage_outcome == "fail" and executable_outcomes and all(
        value == "pass" for value in executable_outcomes
    ):
        errors.append("failed installed executable stage cannot have all checks pass")

    cosimulation_stage = stages.get("xml-verilator-cosimulation", {})
    cosimulation_check = checks.get("xml-verilator-cosimulation", {})
    if cosimulation_check.get("outcome") != cosimulation_stage.get("outcome"):
        errors.append("co-simulation check outcome does not match its stage")
    if cosimulation_check.get("failure") != cosimulation_stage.get("failure"):
        errors.append("co-simulation check failure does not match its stage")
    schema_check = checks.get("result-schema-validation", {})
    if schema_check.get("outcome") != "pass" or schema_check.get("failure") is not None:
        errors.append("result-schema-validation check must confirm this validated bundle")

    task_state = task.get("execution", {}).get("state")
    task_outcome = task.get("outcome")
    if manifest.get("execution_state") != task_state:
        errors.append("manifest execution_state does not match task execution state")
    if task_state == "completed" and task_outcome not in {"pass", "fail"}:
        errors.append("completed open-build task must have pass or fail outcome")
    if task_state in {"canceled", "timed_out", "infrastructure_error"}:
        if task_outcome != "unknown" or task.get("failure") is None:
            errors.append("exceptional open-build state requires unknown outcome and failure")
    if task_state in {"queued", "running"}:
        errors.append("final open-build bundle cannot remain queued or running")
    if task_outcome == "pass" and any(
        stage.get("outcome") != "pass" for stage in stages.values()
    ):
        errors.append("passing open-build task requires every stage to pass")
    if task_outcome == "fail" and not any(
        stage.get("outcome") == "fail" for stage in stages.values()
    ):
        errors.append("failed open-build task requires a failed stage")
    task_failure = task.get("failure")
    if isinstance(task_failure, dict):
        failure_stage = stages.get(task_failure.get("stage"), {})
        if failure_stage.get("outcome") not in {"fail", "unknown"}:
            errors.append("task failure does not point to its failed or interrupted stage")

    for metric_id in ("memory.build.oom-detected", "memory.build.kill-detected"):
        value = metrics.get(metric_id, {}).get("value")
        if value not in (0, 1, None) or isinstance(value, bool):
            errors.append(f"metric {metric_id!r}: expected numeric 0, 1, or null")
    hit_rate = metrics.get("ccache.hit-rate", {}).get("value")
    if isinstance(hit_rate, (int, float)) and not isinstance(hit_rate, bool) and hit_rate > 100:
        errors.append("metric 'ccache.hit-rate': value cannot exceed 100")
    for metric_id, (unit, aggregation, scope) in METRIC_CONTRACTS.items():
        metric = metrics.get(metric_id, {})
        actual = (metric.get("unit"), metric.get("aggregation"), metric.get("scope"))
        if actual != (unit, aggregation, scope):
            errors.append(
                f"metric {metric_id!r}: expected unit/aggregation/scope "
                f"{(unit, aggregation, scope)!r}, got {actual!r}"
            )

    expected_severity = {rule_id: "blocking" for rule_id in RULE_IDS}
    expected_severity["fast-regressions-availability"] = "non-blocking"
    for rule_id, rule in rules.items():
        if rule.get("severity") != expected_severity.get(rule_id):
            errors.append(f"rule {rule_id!r}: incorrect severity")
        for index, reference in enumerate(rule.get("evidence", [])):
            _validate_evidence_reference(
                reference, *context, errors, f"rule {rule_id!r}.evidence[{index}]"
            )

    executable_outcomes = [checks.get(check_id, {}).get("outcome") for check_id in CHECK_IDS[:3]]
    installed_rule = "pass" if all(value == "pass" for value in executable_outcomes) else (
        "fail" if any(value == "fail" for value in executable_outcomes) else "neutral"
    )
    cosim_outcome = checks.get("xml-verilator-cosimulation", {}).get("outcome")
    cosim_rule = "pass" if cosim_outcome == "pass" else (
        "fail" if cosim_outcome == "fail" else "neutral"
    )
    oom = metrics.get("memory.build.oom-detected", {}).get("value")
    killed = metrics.get("memory.build.kill-detected", {}).get("value")
    resource_rule = "fail" if 1 in (oom, killed) else (
        "pass" if oom == 0 and killed == 0 else "neutral"
    )
    task_rule = "pass" if task.get("outcome") == "pass" else "fail"
    expected_rule_outcomes = {
        "ci-result-schema-valid": "pass",
        "open-build-success": task_rule,
        "installed-executable-validation": installed_rule,
        "xml-verilator-cosimulation": cosim_rule,
        "no-oom-or-kill": resource_rule,
        "fast-regressions-availability": "neutral",
    }
    for rule_id, expected in expected_rule_outcomes.items():
        if rules.get(rule_id, {}).get("outcome") != expected:
            errors.append(f"rule {rule_id!r}: outcome does not match raw evidence")

    blocking_outcomes = [
        rule.get("outcome")
        for rule in rule_list
        if rule.get("severity") == "blocking"
    ]
    overall = "fail" if "fail" in blocking_outcomes else (
        "neutral" if any(value != "pass" for value in blocking_outcomes) else "pass"
    )
    recommendation = {
        "pass": "merge",
        "fail": "do-not-merge",
        "neutral": "manual-review",
    }[overall]
    if verdict.get("overall_outcome") != overall:
        errors.append("verdict overall_outcome does not match blocking rules")
    if verdict.get("merge_recommendation") != recommendation:
        errors.append("verdict merge_recommendation does not match overall outcome")
    if request.get("policy_profile") != verdict.get("policy_profile"):
        errors.append("request/verdict policy profile mismatch")

    if errors:
        raise BundleValidationError(errors)
    return documents
