"""Schema and cross-document validation for a PandA CI result bundle."""

from __future__ import annotations

from datetime import datetime
from pathlib import Path, PurePosixPath
from typing import Any, Iterable

from .constants import (
    ARTIFACT_IDS,
    CHECK_IDS,
    CORE_DOCUMENT_PATHS,
    HOSTED_REGRESSION_RULE_ID,
    LEGACY_SCHEMA_VERSION,
    METRIC_CONTRACTS,
    METRIC_IDS,
    MULTI_TASK_RULE_IDS,
    MULTI_TASK_SCHEMA_VERSION,
    REGRESSION_ARTIFACT_SUFFIXES,
    REGRESSION_CHECK_IDS,
    REGRESSION_CHECK_TYPES,
    REGRESSION_METRIC_CONTRACTS,
    REGRESSION_METRIC_IDS,
    REGRESSION_STAGE_IDS,
    RULE_IDS,
    SCHEMA_FILES,
    STAGE_IDS,
    SUPPORTED_SCHEMA_VERSIONS,
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
    if version not in SUPPORTED_SCHEMA_VERSIONS:
        errors.append(f"{path}.schema_version: unsupported schema version {version!r}")
        return False
    return True


def _validate_evidence_reference(
    reference: Any,
    task_contexts: dict[
        str,
        tuple[
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
        ],
    ],
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
    if document == "manifest.json" and fragment == "hosted-regression-suite":
        return
    task_context = task_contexts.get(document)
    if task_context is not None:
        if fragment == "outcome":
            return
        kind, separator, identifier = fragment.partition("/")
        targets = {
            "stage": task_context[0],
            "metric": task_context[1],
            "check": task_context[2],
        }.get(kind)
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
    stages: dict[str, dict[str, Any]],
    task_contexts: dict[
        str,
        tuple[
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
        ],
    ],
    artifacts: dict[str, dict[str, Any]],
    errors: list[str],
) -> None:
    if outcome == "fail" and value is None:
        errors.append(f"{location}: failed result requires a failure object")
    if outcome in {"pass", "skipped"} and value is not None:
        errors.append(f"{location}: {outcome} result must have null failure")
    if isinstance(value, dict):
        failure_stage = value.get("stage")
        if failure_stage is not None and failure_stage not in stages:
            errors.append(f"{location}: failure references unknown stage {failure_stage!r}")
        for index, reference in enumerate(value.get("evidence", [])):
            _validate_evidence_reference(
                reference,
                task_contexts,
                artifacts,
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
    required_core_paths = ("manifest.json",) + CORE_DOCUMENT_PATHS
    documents: dict[str, dict[str, Any]] = {}

    task_paths = (
        sorted(
            path.relative_to(bundle).as_posix()
            for path in (bundle / "tasks").glob("*.json")
            if path.is_file() and not path.is_symlink()
        )
        if (bundle / "tasks").is_dir()
        else []
    )
    paths_to_load = required_core_paths + tuple(task_paths)
    for relative in paths_to_load:
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

    allowed_files = set(paths_to_load)
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

    if any(relative not in documents for relative in required_core_paths):
        raise BundleValidationError(errors)

    manifest = documents["manifest.json"]
    request = documents["request.json"]
    artifact_document = documents["artifacts.json"]
    verdict = documents["verdict.json"]
    bundle_version = manifest.get("schema_version")
    hosted_regression_suite = manifest.get("hosted_regression_suite")
    if bundle_version == LEGACY_SCHEMA_VERSION and hosted_regression_suite is not None:
        errors.append("schema version 1.0 must not contain hosted_regression_suite")
    if bundle_version == MULTI_TASK_SCHEMA_VERSION and not isinstance(
        hosted_regression_suite, dict
    ):
        errors.append("schema version 1.1 requires hosted_regression_suite telemetry")
    for relative, document in documents.items():
        if document.get("schema_version") != bundle_version:
            errors.append(
                f"{relative}.schema_version: every document must use bundle version "
                f"{bundle_version!r}"
            )

    task_documents: dict[str, dict[str, Any]] = {}
    task_paths_by_id: dict[str, str] = {}
    for relative in task_paths:
        task_document = documents.get(relative)
        if task_document is None:
            continue
        task_id = task_document.get("task_id")
        if not isinstance(task_id, str):
            continue
        if task_id in {"artifacts", "request", "verdict"}:
            errors.append(f"{relative}.task_id: task ID is reserved for a core document")
        expected_path = f"tasks/{task_id}.json"
        if relative != expected_path:
            errors.append(
                f"{relative}.task_id: task document path must be {expected_path!r}"
            )
        if task_id in task_documents:
            errors.append(f"{relative}.task_id: duplicate task ID {task_id!r}")
        task_documents[task_id] = task_document
        task_paths_by_id[task_id] = relative

    if "open-build" not in task_documents:
        errors.append("tasks/open-build.json: required document is missing")
    if bundle_version == LEGACY_SCHEMA_VERSION and set(task_documents) != {"open-build"}:
        errors.append("schema version 1.0 supports only the open-build task")
    if bundle_version == MULTI_TASK_SCHEMA_VERSION and not any(
        task.get("task_type") == "regression" for task in task_documents.values()
    ):
        errors.append("schema version 1.1 requires at least one regression task")
    if "open-build" not in task_documents:
        raise BundleValidationError(errors)

    task = task_documents["open-build"]
    effective_profile = manifest.get("effective_build_profile")
    if isinstance(effective_profile, dict):
        request_parameters = request.get("build_parameters", {})
        task_configuration = task.get("configuration", {})
        duplicate_fields = (
            "assertions_enabled",
            "build_type",
            "configured_parallelism",
            "warnings_as_errors",
        )
        for field in duplicate_fields:
            value = effective_profile.get(field)
            if value != request_parameters.get(field):
                errors.append(
                    f"effective build profile/request mismatch for {field}"
                )
            if value != task_configuration.get(field):
                errors.append(
                    f"effective build profile/open-build mismatch for {field}"
                )
        if effective_profile.get("selected_frontend") != task_configuration.get(
            "selected_frontend"
        ):
            errors.append(
                "effective build profile/open-build mismatch for selected_frontend"
            )
        if effective_profile.get("configured_parallelism") != manifest.get(
            "configured_parallelism"
        ):
            errors.append(
                "effective build profile/manifest mismatch for configured_parallelism"
            )
        container = manifest.get("container", {})
        for profile_field, manifest_field in (
            ("dockerfile_path", "dockerfile_path"),
            ("dockerfile_sha256", "dockerfile_sha256"),
        ):
            if effective_profile.get(profile_field) != container.get(manifest_field):
                errors.append(
                    f"effective build profile/container mismatch for {profile_field}"
                )
        workflow = manifest.get("workflow", {})
        for profile_field, manifest_field in (
            ("workflow_file", "file"),
            ("workflow_file_sha256", "file_sha256"),
        ):
            if effective_profile.get(profile_field) != workflow.get(manifest_field):
                errors.append(
                    f"effective build profile/workflow mismatch for {profile_field}"
                )
        if effective_profile.get("tool_versions") != manifest.get("tools"):
            errors.append("effective build profile/manifest mismatch for tool_versions")

    artifact_list = artifact_document.get("artifacts", [])
    rule_list = verdict.get("rules", [])
    artifacts = _unique_index(
        artifact_list, "artifact_id", "artifacts.json.artifacts", errors
    )
    rules = _unique_index(rule_list, "rule_id", "verdict.json.rules", errors)
    task_contexts: dict[
        str,
        tuple[
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
            dict[str, dict[str, Any]],
        ],
    ] = {}
    for task_id, task_document in task_documents.items():
        relative = task_paths_by_id[task_id]
        stage_list_for_task = task_document.get("stages", [])
        metric_list_for_task = task_document.get("metrics", [])
        check_list_for_task = task_document.get("checks", [])
        task_contexts[relative] = (
            _unique_index(
                stage_list_for_task, "stage_id", f"{relative}.stages", errors
            ),
            _unique_index(
                metric_list_for_task, "metric_id", f"{relative}.metrics", errors
            ),
            _unique_index(
                check_list_for_task, "check_id", f"{relative}.checks", errors
            ),
        )

    stages, metrics, checks = task_contexts["tasks/open-build.json"]
    stage_list = task.get("stages", [])
    metric_list = task.get("metrics", [])
    check_list = task.get("checks", [])
    _exact_ids([item.get("stage_id") for item in stage_list], STAGE_IDS, "stages", errors)
    _exact_ids([item.get("metric_id") for item in metric_list], METRIC_IDS, "metrics", errors)
    _exact_ids([item.get("check_id") for item in check_list], CHECK_IDS, "checks", errors)

    artifact_ids = [item.get("artifact_id") for item in artifact_list]
    if bundle_version == LEGACY_SCHEMA_VERSION:
        _exact_ids(artifact_ids, ARTIFACT_IDS, "artifacts", errors)
        expected_rule_ids = RULE_IDS
    else:
        expected_artifact_ids = list(ARTIFACT_IDS) + sorted(
            artifact_id
            for artifact_id in artifact_ids
            if isinstance(artifact_id, str) and artifact_id not in ARTIFACT_IDS
        )
        if artifact_ids != expected_artifact_ids:
            errors.append(
                "artifacts: expected legacy artifact IDs followed by regression IDs "
                "in lexicographic order"
            )
        expected_rule_ids = MULTI_TASK_RULE_IDS
    _exact_ids(
        [item.get("rule_id") for item in rule_list], expected_rule_ids, "rules", errors
    )

    document_refs = manifest.get("documents", [])
    ref_index = _unique_index(document_refs, "document_id", "manifest.json.documents", errors)
    expected_refs = {
        "artifacts": ("artifacts.json", "panda.ci.artifact-index"),
        "request": ("request.json", "panda.ci.request"),
        "verdict": ("verdict.json", "panda.ci.verdict"),
    }
    expected_refs.update(
        {
            task_id: (task_paths_by_id[task_id], "panda.ci.task-result")
            for task_id in task_documents
        }
    )
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
    expected_task_ids = sorted(task_documents)
    if request.get("requested_task_ids") != expected_task_ids:
        if bundle_version == LEGACY_SCHEMA_VERSION:
            errors.append("request.json.requested_task_ids must contain only open-build")
        else:
            errors.append(
                "request.json.requested_task_ids must match task documents in stable order"
            )
    if request.get("requested_artifact_ids") != artifact_ids:
        if bundle_version == LEGACY_SCHEMA_VERSION:
            errors.append(
                "request.json.requested_artifact_ids must contain every v1 artifact in stable order"
            )
        else:
            errors.append(
                "request.json.requested_artifact_ids must match the artifact index in stable order"
            )
    request_tasks = request.get("tasks", [])
    request_task_index = _unique_index(
        request_tasks, "task_id", "request.json.tasks", errors
    )
    request_task_ids = [
        item.get("task_id") for item in request_tasks if isinstance(item, dict)
    ]
    if request_task_ids != expected_task_ids:
        if bundle_version == LEGACY_SCHEMA_VERSION:
            errors.append("request.json.tasks does not match the open-build task result")
        else:
            errors.append(
                "request.json.tasks must match task documents in stable task ID order"
            )
    for task_id, task_document in task_documents.items():
        requested_task = request_task_index.get(task_id, {})
        if requested_task.get("task_type") != task_document.get("task_type"):
            errors.append(f"request/task type mismatch for {task_id!r}")
        if task_document.get("task_type") == "regression":
            requested_configuration = requested_task.get("configuration")
            observed_configuration = task_document.get("configuration")
            if isinstance(observed_configuration, dict):
                observed_frontend = observed_configuration.get("frontend", {})
                comparable_configuration = {
                    key: value
                    for key, value in observed_configuration.items()
                    if key != "frontend"
                }
                comparable_configuration["frontend"] = {
                    "requested": observed_frontend.get("requested")
                }
                if requested_configuration != comparable_configuration:
                    errors.append(
                        f"request/task regression configuration mismatch for {task_id!r}"
                    )
    if task.get("task_id") != "open-build" or task.get("task_type") != "build":
        errors.append("tasks/open-build.json: task identity mismatch")
    if "results" in task:
        errors.append("tasks/open-build.json: build task must not contain regression results")
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
            elif artifacts[artifact_id].get("producer_task") != "open-build":
                errors.append(
                    f"stage {stage_id!r}: artifact {artifact_id!r} belongs to a different task"
                )
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
        _failure_invariants(
            stage.get("failure"),
            outcome,
            f"stage {stage_id!r}",
            stages,
            task_contexts,
            artifacts,
            errors,
        )

    if task.get("artifacts") != list(ARTIFACT_IDS):
        errors.append("task artifacts must contain every v1 artifact in stable order")
    for task_id, task_document in task_documents.items():
        task_artifact_ids = task_document.get("artifacts", [])
        expected_task_artifacts = [
            artifact_id
            for artifact_id in artifact_ids
            if isinstance(artifact_id, str)
            and artifacts.get(artifact_id, {}).get("producer_task") == task_id
        ]
        if task_artifact_ids != expected_task_artifacts:
            errors.append(
                f"task {task_id!r}: artifacts must match artifacts produced by the task "
                "in stable order"
            )
        for artifact_id in task_artifact_ids:
            if artifact_id not in artifacts:
                errors.append(
                    f"task {task_id!r}: unknown artifact reference {artifact_id!r}"
                )
    for artifact_id, artifact in artifacts.items():
        if not _safe_relative_path(artifact.get("path")):
            errors.append(f"artifact {artifact_id!r}: path is not safe and bundle-relative")
        producer_task = artifact.get("producer_task")
        if producer_task not in task_documents:
            errors.append(f"artifact {artifact_id!r}: producer_task does not resolve")
            continue
        if artifact_id in ARTIFACT_IDS and producer_task != "open-build":
            errors.append(f"artifact {artifact_id!r}: producer_task must be open-build")
        if artifact.get("available") is False and (
            artifact.get("size_bytes") is not None or artifact.get("sha256") is not None
        ):
            errors.append(f"artifact {artifact_id!r}: unavailable artifact must have null size/hash")
        associated_stage = artifact.get("associated_stage")
        producer_path = task_paths_by_id.get(producer_task)
        producer_stages = task_contexts.get(producer_path, ({}, {}, {}))[0]
        if associated_stage is not None and associated_stage not in producer_stages:
            errors.append(f"artifact {artifact_id!r}: unknown associated stage")

    for check_id, check in checks.items():
        _failure_invariants(
            check.get("failure"),
            check.get("outcome"),
            f"check {check_id!r}",
            stages,
            task_contexts,
            artifacts,
            errors,
        )
    _failure_invariants(
        task.get("failure"),
        task.get("outcome"),
        "task open-build",
        stages,
        task_contexts,
        artifacts,
        errors,
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
    if bundle_version == LEGACY_SCHEMA_VERSION:
        if manifest.get("execution_state") != task_state:
            errors.append("manifest execution_state does not match task execution state")
    else:
        task_states = [
            task_document.get("execution", {}).get("state")
            for task_document in task_documents.values()
        ]
        if isinstance(hosted_regression_suite, dict):
            task_states.append(hosted_regression_suite.get("execution_state"))
        if any(state in {"queued", "running"} for state in task_states):
            errors.append("final multi-task bundle cannot contain queued or running tasks")
        if "timed_out" in task_states:
            expected_manifest_state = "timed_out"
        elif "canceled" in task_states:
            expected_manifest_state = "canceled"
        elif "infrastructure_error" in task_states:
            expected_manifest_state = "infrastructure_error"
        else:
            expected_manifest_state = "completed"
        if manifest.get("execution_state") != expected_manifest_state:
            errors.append(
                "manifest execution_state does not match the aggregate task execution state"
            )
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

    regression_tasks = {
        task_id: task_document
        for task_id, task_document in task_documents.items()
        if task_document.get("task_type") == "regression"
    }
    for task_id, task_document in task_documents.items():
        if task_id != "open-build" and task_document.get("task_type") != "regression":
            errors.append(
                f"task {task_id!r}: only open-build may use task type build"
            )

    stage_failure_contracts = {
        "input-validation": {
            ("configuration", "invalid-regression-input"),
            ("configuration", "frontend-resolution-failed"),
        },
        "hls-synthesis": {
            ("configuration", "frontend-resolution-failed"),
            ("compilation", "hls-synthesis-failed"),
        },
        "rtl-generation": {
            ("compilation", "rtl-generation-failed"),
            ("verification", "rtl-generation-failed"),
        },
        "simulator-preparation": {("compilation", "simulator-build-failed")},
        "rtl-simulation": {("execution", "rtl-simulation-failed")},
        "result-verification": {("verification", "result-mismatch")},
    }
    artifact_stage_by_suffix = {
        "bambu-log": "hls-synthesis",
        "result-report": "result-verification",
        "rtl-output": "rtl-generation",
        "simulation-log": "rtl-simulation",
    }

    for task_id in sorted(regression_tasks):
        regression_task = regression_tasks[task_id]
        if "results" not in regression_task:
            errors.append(f"task {task_id!r}: regression results are required")
        relative = task_paths_by_id[task_id]
        regression_stages, regression_metrics, regression_checks = task_contexts[relative]
        regression_stage_list = regression_task.get("stages", [])
        regression_metric_list = regression_task.get("metrics", [])
        regression_check_list = regression_task.get("checks", [])
        _exact_ids(
            [item.get("stage_id") for item in regression_stage_list],
            REGRESSION_STAGE_IDS,
            f"task {task_id!r} stages",
            errors,
        )
        _exact_ids(
            [item.get("metric_id") for item in regression_metric_list],
            REGRESSION_METRIC_IDS,
            f"task {task_id!r} metrics",
            errors,
        )
        _exact_ids(
            [item.get("check_id") for item in regression_check_list],
            REGRESSION_CHECK_IDS,
            f"task {task_id!r} checks",
            errors,
        )

        expected_regression_artifacts = [
            f"{task_id}.{suffix}" for suffix in REGRESSION_ARTIFACT_SUFFIXES
        ]
        if regression_task.get("artifacts") != expected_regression_artifacts:
            errors.append(
                f"task {task_id!r}: expected stable regression artifacts "
                f"{expected_regression_artifacts!r}"
            )
        for suffix, associated_stage in artifact_stage_by_suffix.items():
            artifact_id = f"{task_id}.{suffix}"
            artifact = artifacts.get(artifact_id)
            if artifact is None:
                errors.append(f"task {task_id!r}: missing artifact {artifact_id!r}")
                continue
            if artifact.get("producer_task") != task_id:
                errors.append(
                    f"artifact {artifact_id!r}: producer_task must be {task_id!r}"
                )
            if artifact.get("associated_stage") != associated_stage:
                errors.append(
                    f"artifact {artifact_id!r}: associated_stage must be "
                    f"{associated_stage!r}"
                )
            if artifact.get("available") is True:
                size = artifact.get("size_bytes")
                digest = artifact.get("sha256")
                retention = artifact.get("retention_days")
                github_name = artifact.get("github_artifact_name")
                if not isinstance(size, int) or isinstance(size, bool) or size < 0:
                    errors.append(
                        f"artifact {artifact_id!r}: available evidence requires size_bytes"
                    )
                if not isinstance(digest, str) or len(digest) != 64:
                    errors.append(
                        f"artifact {artifact_id!r}: available evidence requires SHA-256"
                    )
                if not isinstance(retention, int) or isinstance(retention, bool) or retention < 1:
                    errors.append(
                        f"artifact {artifact_id!r}: available evidence requires retention_days"
                    )
                if not isinstance(github_name, str) or not github_name:
                    errors.append(
                        f"artifact {artifact_id!r}: available evidence requires an artifact name"
                    )
            elif any(
                artifact.get(field) is not None
                for field in (
                    "size_bytes",
                    "sha256",
                    "retention_days",
                    "github_artifact_name",
                )
            ):
                errors.append(
                    f"artifact {artifact_id!r}: unavailable evidence metadata must be null"
                )

        terminal_stage_seen = False
        expected_stage_artifacts = {
            "input-validation": [],
            "hls-synthesis": [f"{task_id}.bambu-log"],
            "rtl-generation": [f"{task_id}.rtl-output"],
            "simulator-preparation": [],
            "rtl-simulation": [f"{task_id}.simulation-log"],
            "result-verification": [f"{task_id}.result-report"],
        }
        for stage_id in REGRESSION_STAGE_IDS:
            stage = regression_stages.get(stage_id, {})
            outcome = stage.get("outcome")
            if stage.get("metric_ids") != [f"duration.{stage_id}"]:
                errors.append(
                    f"task {task_id!r} stage {stage_id!r}: expected its duration metric"
                )
            if stage.get("artifact_ids") != expected_stage_artifacts[stage_id]:
                errors.append(
                    f"task {task_id!r} stage {stage_id!r}: expected stable artifact "
                    "references"
                )
            for artifact_id in stage.get("artifact_ids", []):
                artifact = artifacts.get(artifact_id)
                if artifact is None:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: unknown artifact "
                        f"reference {artifact_id!r}"
                    )
                elif artifact.get("producer_task") != task_id:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: artifact "
                        f"{artifact_id!r} belongs to a different task"
                    )
            duration_metric = regression_metrics.get(f"duration.{stage_id}", {})
            if stage.get("duration_seconds") != duration_metric.get("value"):
                errors.append(
                    f"task {task_id!r} stage {stage_id!r}: duration does not match "
                    "its metric"
                )
            if terminal_stage_seen and outcome != "skipped":
                errors.append(
                    f"task {task_id!r} stage {stage_id!r}: downstream stage must be skipped"
                )
            if outcome in {"fail", "unknown", "skipped"}:
                terminal_stage_seen = True
            if outcome == "skipped":
                if stage.get("execution_state") != "completed":
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: skipped stage state "
                        "must be completed"
                    )
                if stage.get("duration_seconds") is not None or stage.get("exit_status") is not None:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: skipped stage must use "
                        "null duration/status"
                    )
            elif outcome == "pass":
                if stage.get("execution_state") != "completed" or stage.get("exit_status") != 0:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: passing stage must be "
                        "completed with exit status 0"
                    )
            elif outcome == "fail":
                if stage.get("execution_state") not in {"completed", "infrastructure_error"}:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: failed stage has "
                        "incompatible state"
                    )
                if stage.get("exit_status") == 0:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: failed stage cannot "
                        "have exit status 0"
                    )
            elif outcome == "unknown":
                if stage.get("execution_state") not in {
                    "canceled",
                    "timed_out",
                    "infrastructure_error",
                }:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: unknown outcome has "
                        "incompatible state"
                    )
                if stage.get("failure") is None:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: unknown outcome "
                        "requires failure evidence"
                    )
            else:
                errors.append(
                    f"task {task_id!r} stage {stage_id!r}: unsupported regression outcome"
                )

            failure = stage.get("failure")
            if isinstance(failure, dict):
                if failure.get("stage") != stage_id:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: failure points to a "
                        "different stage"
                    )
                failure_pair = (failure.get("category"), failure.get("code"))
                exceptional_pairs = {
                    ("timeout", "regression-timeout"),
                    ("infrastructure", "regression-infrastructure-failure"),
                    ("canceled", "workflow-canceled"),
                }
                if failure_pair not in stage_failure_contracts[stage_id] | exceptional_pairs:
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: unsupported failure "
                        "classification"
                    )
                exceptional_states = {
                    ("timeout", "regression-timeout"): "timed_out",
                    (
                        "infrastructure",
                        "regression-infrastructure-failure",
                    ): "infrastructure_error",
                    ("canceled", "workflow-canceled"): "canceled",
                }
                expected_exceptional_state = exceptional_states.get(failure_pair)
                if expected_exceptional_state is not None and (
                    outcome != "unknown"
                    or stage.get("execution_state") != expected_exceptional_state
                ):
                    errors.append(
                        f"task {task_id!r} stage {stage_id!r}: exceptional failure "
                        "requires unknown outcome and matching execution state"
                    )
            _failure_invariants(
                failure,
                outcome,
                f"task {task_id!r} stage {stage_id!r}",
                regression_stages,
                task_contexts,
                artifacts,
                errors,
            )

        for metric_id, (unit, aggregation, scope) in REGRESSION_METRIC_CONTRACTS.items():
            metric = regression_metrics.get(metric_id, {})
            actual = (metric.get("unit"), metric.get("aggregation"), metric.get("scope"))
            if actual != (unit, aggregation, scope):
                errors.append(
                    f"task {task_id!r} metric {metric_id!r}: expected "
                    f"unit/aggregation/scope {(unit, aggregation, scope)!r}, got "
                    f"{actual!r}"
                )

        check_stage_ids = dict(
            zip(
                REGRESSION_CHECK_IDS,
                ("rtl-generation", "rtl-simulation", "result-verification"),
            )
        )
        for check_id in REGRESSION_CHECK_IDS:
            check = regression_checks.get(check_id, {})
            stage = regression_stages.get(check_stage_ids[check_id], {})
            if check.get("type") != REGRESSION_CHECK_TYPES[check_id]:
                errors.append(f"task {task_id!r} check {check_id!r}: incorrect type")
            if check.get("details") is not None:
                errors.append(f"task {task_id!r} check {check_id!r}: details must be null")
            if check.get("outcome") != stage.get("outcome"):
                errors.append(
                    f"task {task_id!r} check {check_id!r}: outcome does not match its stage"
                )
            if check.get("failure") != stage.get("failure"):
                errors.append(
                    f"task {task_id!r} check {check_id!r}: failure does not match its stage"
                )
            _failure_invariants(
                check.get("failure"),
                check.get("outcome"),
                f"task {task_id!r} check {check_id!r}",
                regression_stages,
                task_contexts,
                artifacts,
                errors,
            )

        configuration = regression_task.get("configuration", {})
        input_configuration = configuration.get("input", {})
        frontend_configuration = configuration.get("frontend", {})
        invocation = configuration.get("invocation", {})
        options = configuration.get("options", {})
        for path_field, path_value in (
            ("source_path", input_configuration.get("source_path")),
            ("working_directory", invocation.get("working_directory")),
            ("executable", invocation.get("executable")),
        ):
            if not _safe_relative_path(path_value):
                errors.append(
                    f"task {task_id!r} configuration {path_field}: expected a safe "
                    "repository-relative path"
                )
        test_vector_kind = input_configuration.get("test_vector_kind")
        test_vector_path = input_configuration.get("test_vector_path")
        test_vector_value = input_configuration.get("test_vector_value")
        if test_vector_kind in {"xml", "cxx"}:
            if not _safe_relative_path(test_vector_path):
                errors.append(
                    f"task {task_id!r}: file test vectors require a safe test_vector_path"
                )
            if test_vector_path != test_vector_value:
                errors.append(
                    f"task {task_id!r}: file test_vector_path/value must match"
                )
        elif test_vector_kind == "inline" and test_vector_path is not None:
            errors.append(f"task {task_id!r}: inline test vectors require null path")
        if options.get("compiler") != frontend_configuration.get("requested"):
            errors.append(f"task {task_id!r}: compiler option/requested frontend mismatch")
        if options.get("top_function") != input_configuration.get("top_function"):
            errors.append(f"task {task_id!r}: top-function option/input mismatch")
        if options.get("test_vectors") != test_vector_value:
            errors.append(f"task {task_id!r}: test-vector option/input mismatch")
        if options.get("simulate") is not True:
            errors.append(f"task {task_id!r}: hosted regression must enable simulation")
        arguments = invocation.get("arguments", [])
        compiler_option = options.get("compiler")
        simulator_option = options.get("simulator")
        top_function = input_configuration.get("top_function")
        required_arguments = [
            input_configuration.get("source_path"),
            f"--compiler={compiler_option}",
            f"--simulator={simulator_option}",
            f"--generate-tb={test_vector_value}",
        ]
        if options.get("simulate") is True:
            required_arguments.append("--simulate")
        if top_function is not None:
            required_arguments.append(f"--top-fname={top_function}")
        optimization = options.get("optimization")
        if optimization is not None:
            required_arguments.append(f"-{optimization}")
        device = options.get("device")
        if device is not None:
            required_arguments.append(f"--device-name={device}")
        clock_period = options.get("clock_period")
        if isinstance(clock_period, (int, float)) and not isinstance(clock_period, bool):
            required_arguments.append(f"--clock-period={clock_period:g}")
        interface = options.get("interface")
        if interface is not None:
            required_arguments.append(f"--generate-interface={interface}")
        language_standard = options.get("language_standard")
        if language_standard is not None:
            required_arguments.append(f"--std={language_standard}")
        parallel_backend = options.get("parallel_backend")
        required_arguments.append(f"--parallel-backend={parallel_backend}")
        experimental_setup = options.get("experimental_setup")
        if experimental_setup is not None:
            required_arguments.append(f"--experimental-setup={experimental_setup}")
        if options.get("expose_globals") is True:
            required_arguments.append("--expose-globals")
        inline_max_cost = options.get("inline_max_cost")
        if inline_max_cost is not None:
            required_arguments.append(f"--inline-max-cost={inline_max_cost}")
        required_arguments.extend(
            f"--bambu-parameter={value}" for value in options.get("bambu_parameters", [])
        )
        for argument in required_arguments:
            if argument not in arguments:
                errors.append(
                    f"task {task_id!r}: normalized invocation is missing {argument!r}"
                )

        selected_frontend = frontend_configuration.get("selected")
        if regression_stages.get("hls-synthesis", {}).get("outcome") == "pass":
            if selected_frontend != frontend_configuration.get("requested"):
                errors.append(
                    f"task {task_id!r}: passing HLS requires the selected frontend to "
                    "match the request"
                )
        elif regression_stages.get("hls-synthesis", {}).get("outcome") == "skipped":
            if selected_frontend is not None:
                errors.append(
                    f"task {task_id!r}: skipped HLS requires null selected frontend"
                )

        results = regression_task.get("results", {})
        synthesis_result = results.get("synthesis", {})
        simulation_result = results.get("simulation", {})
        rtl_passed = regression_stages.get("rtl-generation", {}).get("outcome") == "pass"
        simulation_passed = regression_stages.get("rtl-simulation", {}).get("outcome") == "pass"
        verification_passed = (
            regression_stages.get("result-verification", {}).get("outcome") == "pass"
        )
        if synthesis_result.get("completed") is not rtl_passed:
            errors.append(
                f"task {task_id!r}: synthesis result contradicts RTL-generation stage"
            )
        rtl_artifact_count = synthesis_result.get("rtl_artifact_count")
        if rtl_passed and (
            not isinstance(rtl_artifact_count, int)
            or isinstance(rtl_artifact_count, bool)
            or rtl_artifact_count < 1
        ):
            errors.append(
                f"task {task_id!r}: passing RTL generation requires an RTL artifact"
            )
        for stage_id, artifact_suffix in (
            ("hls-synthesis", "bambu-log"),
            ("rtl-generation", "rtl-output"),
            ("rtl-simulation", "simulation-log"),
            ("result-verification", "result-report"),
        ):
            if (
                regression_stages.get(stage_id, {}).get("outcome") == "pass"
                and artifacts.get(f"{task_id}.{artifact_suffix}", {}).get("available")
                is not True
            ):
                errors.append(
                    f"task {task_id!r}: passing {stage_id!r} requires its artifact"
                )
        if simulation_result.get("completed") is not simulation_passed:
            errors.append(f"task {task_id!r}: simulation result contradicts simulation stage")
        if simulation_result.get("verified") is not verification_passed:
            errors.append(
                f"task {task_id!r}: verification result contradicts verification stage"
            )
        if not simulation_passed and (
            simulation_result.get("execution_count") is not None
            or simulation_result.get("total_cycles") is not None
        ):
            errors.append(
                f"task {task_id!r}: unavailable simulation observations must be null"
            )
        if simulation_passed:
            execution_count = simulation_result.get("execution_count")
            total_cycles = simulation_result.get("total_cycles")
            if (
                not isinstance(execution_count, int)
                or isinstance(execution_count, bool)
                or execution_count < 1
            ):
                errors.append(
                    f"task {task_id!r}: passing simulation requires an execution count"
                )
            if (
                not isinstance(total_cycles, int)
                or isinstance(total_cycles, bool)
                or total_cycles < 0
            ):
                errors.append(
                    f"task {task_id!r}: passing simulation requires a cycle count"
                )

        regression_state = regression_task.get("execution", {}).get("state")
        regression_outcome = regression_task.get("outcome")
        if regression_state == "completed" and regression_outcome not in {
            "pass",
            "fail",
            "skipped",
        }:
            errors.append(
                f"task {task_id!r}: completed regression must pass, fail, or be skipped"
            )
        if regression_state in {"canceled", "timed_out", "infrastructure_error"}:
            if regression_outcome != "unknown" or regression_task.get("failure") is None:
                errors.append(
                    f"task {task_id!r}: exceptional state requires unknown outcome and failure"
                )
        if regression_state in {"queued", "running"}:
            errors.append(f"task {task_id!r}: final task cannot remain queued or running")
        stage_outcomes = [
            regression_stages.get(stage_id, {}).get("outcome")
            for stage_id in REGRESSION_STAGE_IDS
        ]
        if regression_outcome == "pass" and any(
            outcome != "pass" for outcome in stage_outcomes
        ):
            errors.append(
                f"task {task_id!r}: passing regression requires every stage to pass"
            )
        if regression_outcome == "pass":
            for metric_id in (
                "duration.hls-synthesis",
                "duration.rtl-simulation",
                "duration.regression-total",
            ):
                if regression_metrics.get(metric_id, {}).get("value") is None:
                    errors.append(
                        f"task {task_id!r}: passing regression requires {metric_id!r}"
                    )
        if regression_outcome == "fail" and "fail" not in stage_outcomes:
            errors.append(f"task {task_id!r}: failed regression requires a failed stage")
        if regression_outcome == "skipped" and any(
            outcome != "skipped" for outcome in stage_outcomes
        ):
            errors.append(
                f"task {task_id!r}: skipped regression requires every stage to be skipped"
            )
        _failure_invariants(
            regression_task.get("failure"),
            regression_outcome,
            f"task {task_id!r}",
            regression_stages,
            task_contexts,
            artifacts,
            errors,
        )
        regression_failure = regression_task.get("failure")
        if isinstance(regression_failure, dict):
            failure_stage = regression_stages.get(regression_failure.get("stage"), {})
            if failure_stage.get("outcome") not in {"fail", "unknown"}:
                errors.append(
                    f"task {task_id!r}: failure does not point to its failed or "
                    "interrupted stage"
                )
            if failure_stage.get("failure") != regression_failure:
                errors.append(
                    f"task {task_id!r}: task failure must match the failing stage"
                )

    if bundle_version == MULTI_TASK_SCHEMA_VERSION and isinstance(
        hosted_regression_suite, dict
    ):
        suite_task_count = hosted_regression_suite.get("task_count")
        suite_passed_count = hosted_regression_suite.get("passed_count")
        suite_failed_count = hosted_regression_suite.get("failed_count")
        expected_passed_count = sum(
            task.get("outcome") == "pass" for task in regression_tasks.values()
        )
        if suite_task_count != len(regression_tasks):
            errors.append("hosted_regression_suite.task_count does not match task documents")
        if suite_passed_count != expected_passed_count:
            errors.append("hosted_regression_suite.passed_count does not match task outcomes")
        if suite_failed_count != len(regression_tasks) - expected_passed_count:
            errors.append("hosted_regression_suite.failed_count does not match task outcomes")

        suite_state = hosted_regression_suite.get("execution_state")
        suite_outcome = hosted_regression_suite.get("outcome")
        suite_action_outcome = hosted_regression_suite.get("action_outcome")
        suite_action_exit_status = hosted_regression_suite.get("action_exit_status")
        suite_exit_status = hosted_regression_suite.get("exit_status")
        regression_outcomes = [
            task.get("outcome") for task in regression_tasks.values()
        ]
        if suite_state == "completed" and suite_outcome == "pass":
            if (
                suite_action_outcome != "success"
                or suite_action_exit_status != 0
                or suite_exit_status != 0
                or not regression_outcomes
                or any(outcome != "pass" for outcome in regression_outcomes)
            ):
                errors.append(
                    "passing hosted_regression_suite contradicts action or task outcomes"
                )
        elif suite_state == "completed" and suite_outcome == "fail":
            if (
                suite_action_outcome != "failure"
                or not isinstance(suite_action_exit_status, int)
                or isinstance(suite_action_exit_status, bool)
                or suite_action_exit_status == 0
                or not isinstance(suite_exit_status, int)
                or isinstance(suite_exit_status, bool)
                or suite_exit_status == 0
                or all(outcome == "pass" for outcome in regression_outcomes)
            ):
                errors.append(
                    "failed hosted_regression_suite contradicts action or task outcomes"
                )
        elif suite_state == "completed" and suite_outcome == "skipped":
            if (
                suite_action_outcome != "skipped"
                or suite_action_exit_status is not None
                or suite_exit_status is not None
                or any(outcome != "skipped" for outcome in regression_outcomes)
            ):
                errors.append(
                    "skipped hosted_regression_suite contradicts action or task outcomes"
                )
        elif suite_state in {"canceled", "timed_out", "infrastructure_error"}:
            if suite_outcome != "unknown" or suite_exit_status == 0:
                errors.append(
                    "exceptional hosted_regression_suite requires unknown outcome and "
                    "a null or nonzero exit status"
                )
            if not hosted_regression_suite.get("failure_stage"):
                errors.append(
                    "exceptional hosted_regression_suite requires a failure_stage"
                )
            if suite_state == "timed_out" and (
                suite_action_outcome not in {"failure", "cancelled"}
                or suite_action_exit_status == 0
            ):
                errors.append(
                    "timed-out hosted_regression_suite contradicts action telemetry"
                )
        else:
            errors.append("hosted_regression_suite has incompatible state and outcome")

        duration = hosted_regression_suite.get("duration_seconds")
        duration_method = hosted_regression_suite.get("duration_measurement_method")
        if duration_method == "unavailable" and duration is not None:
            errors.append("unavailable hosted regression duration must be null")
        if duration_method in {"suite-monotonic-clock", "action-wall-clock"} and (
            duration is None
            or hosted_regression_suite.get("started_at") is None
            or hosted_regression_suite.get("completed_at") is None
        ):
            errors.append(
                "measured hosted regression duration requires value and timestamps"
            )
        if duration_method == "action-wall-clock" and duration is not None:
            try:
                start = datetime.fromisoformat(
                    hosted_regression_suite["started_at"].replace("Z", "+00:00")
                )
                completion = datetime.fromisoformat(
                    hosted_regression_suite["completed_at"].replace("Z", "+00:00")
                )
                expected_duration = (completion - start).total_seconds()
            except (AttributeError, TypeError, ValueError):
                expected_duration = None
            if expected_duration is None or expected_duration != duration:
                errors.append(
                    "action-wall-clock duration does not match suite timestamps"
                )
        if duration_method in {"runner-wall-clock", "unavailable"} and (
            hosted_regression_suite.get("started_at") is not None
            or hosted_regression_suite.get("completed_at") is not None
        ):
            errors.append(
                f"{duration_method} hosted regression duration must not use timestamps"
            )
        if duration_method == "runner-wall-clock" and duration is None:
            errors.append("runner-wall-clock hosted regression duration requires a value")
        if suite_outcome in {"pass", "skipped"} and hosted_regression_suite.get(
            "failure_stage"
        ) is not None:
            errors.append(
                "passing or skipped hosted_regression_suite must not name a failure stage"
            )

    expected_severity = {rule_id: "blocking" for rule_id in expected_rule_ids}
    expected_severity["fast-regressions-availability"] = "non-blocking"
    for rule_id, rule in rules.items():
        if rule.get("severity") != expected_severity.get(rule_id):
            errors.append(f"rule {rule_id!r}: incorrect severity")
        for index, reference in enumerate(rule.get("evidence", [])):
            _validate_evidence_reference(
                reference,
                task_contexts,
                artifacts,
                errors,
                f"rule {rule_id!r}.evidence[{index}]",
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
    if bundle_version == MULTI_TASK_SCHEMA_VERSION:
        regression_outcomes = [
            regression_tasks[task_id].get("outcome")
            for task_id in sorted(regression_tasks)
        ]
        suite_passed = bool(
            isinstance(hosted_regression_suite, dict)
            and hosted_regression_suite.get("execution_state") == "completed"
            and hosted_regression_suite.get("outcome") == "pass"
            and hosted_regression_suite.get("action_outcome") == "success"
            and hosted_regression_suite.get("action_exit_status") == 0
            and hosted_regression_suite.get("exit_status") == 0
        )
        hosted_regression_outcome = (
            "fail"
            if "fail" in regression_outcomes
            else "pass"
            if regression_outcomes
            and all(outcome == "pass" for outcome in regression_outcomes)
            and suite_passed
            else "neutral"
        )
        expected_rule_outcomes[HOSTED_REGRESSION_RULE_ID] = hosted_regression_outcome
        expected_hosted_evidence = [
            f"tasks/{task_id}.json#outcome" for task_id in sorted(regression_tasks)
        ] + ["manifest.json#hosted-regression-suite"]
        if rules.get(HOSTED_REGRESSION_RULE_ID, {}).get("evidence") != expected_hosted_evidence:
            errors.append(
                f"rule {HOSTED_REGRESSION_RULE_ID!r}: evidence must reference every "
                "regression outcome in stable order"
            )
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
