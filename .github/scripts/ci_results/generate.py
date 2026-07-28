"""Generate the PandA open-build structured result bundle."""

from __future__ import annotations

import os
import shutil
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

from .bundle import BundleValidationError, validate_bundle
from .constants import ARTIFACT_IDS, CHECK_IDS, METRIC_IDS, RULE_IDS, STAGE_IDS
from .hashing import (
    docker_base_image,
    path_size,
    regular_file_metadata,
    sha256_file,
    submodule_commits,
)
from .regressions import extend_bundle_with_regressions
from .schema import SchemaValidationError
from .serialization import SerializationError, write_json


CMAKE_ARGUMENTS = (
    "-DPANDA_ENABLE_RELEASE=ON",
    "-DPANDA_ENABLE_ASSERTS=OFF",
    "-DPANDA_ENABLE_WERROR=ON",
    "-DPANDA_LIBBAMBU_COMPILER=I386_CLANG16",
)


def _raw(environment: Mapping[str, str], name: str) -> str:
    return environment.get(f"PANDA_CI_{name}", "").strip()


def _nullable_string(value: str) -> str | None:
    return value if value and value.lower() not in {"unknown", "not-run", "running"} else None


def _nullable_int(value: str) -> int | None:
    if not value or value.lower() in {"unknown", "not-run", "running", "null"}:
        return None
    try:
        parsed = int(value)
    except ValueError:
        return None
    return parsed if parsed >= 0 else None


def _nullable_float(value: str) -> float | int | None:
    if not value or value.lower() in {"unknown", "not-run", "running", "null"}:
        return None
    try:
        parsed = float(value)
    except ValueError:
        return None
    if parsed < 0:
        return None
    return int(parsed) if parsed.is_integer() else parsed


def _nullable_bool(value: str) -> bool | None:
    normalized = value.lower()
    if normalized in {"true", "yes", "1", "on"}:
        return True
    if normalized in {"false", "no", "0", "off"}:
        return False
    return None


def _detection(value: str) -> int | None:
    parsed = _nullable_bool(value)
    return None if parsed is None else int(parsed)


def _normalized_build_outcome(environment: Mapping[str, str]) -> str:
    outcome = _raw(environment, "BUILD_OUTCOME").lower()
    if _nullable_bool(_raw(environment, "JOB_CANCELLED")) is True and outcome in {
        "",
        "cancelled",
        "skipped",
    }:
        return "cancelled"
    timeout_seconds = _nullable_int(_raw(environment, "BUILD_TIMEOUT_SECONDS"))
    action_start = _nullable_int(_raw(environment, "ACTION_START_EPOCH"))
    completion = _nullable_int(_raw(environment, "COMPLETION_EPOCH"))
    if (
        outcome in {"failure", "cancelled"}
        and timeout_seconds is not None
        and action_start is not None
        and completion is not None
        and completion - action_start >= timeout_seconds
    ):
        return "timed_out"
    return outcome


def _required_int(environment: Mapping[str, str], name: str, default: int) -> int:
    return _nullable_int(_raw(environment, name)) or default


def _timestamp_from_epoch(value: str) -> str | None:
    epoch = _nullable_int(value)
    if epoch is None:
        return None
    return datetime.fromtimestamp(epoch, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _completion_timestamp(environment: Mapping[str, str]) -> str:
    supplied = _raw(environment, "COMPLETED_AT")
    if supplied:
        return supplied
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _failure(
    category: str,
    code: str,
    stage: str | None,
    message: str,
    evidence: list[str],
    retryable: bool = False,
) -> dict[str, Any]:
    return {
        "category": category,
        "code": code,
        "evidence": evidence,
        "message": message,
        "retryable": retryable,
        "stage": stage,
    }


def _metric(
    metric_id: str,
    value: float | int | None,
    unit: str,
    aggregation: str,
    scope: str,
    measurement_method: str | None,
) -> dict[str, Any]:
    return {
        "aggregation": aggregation,
        "measurement_method": measurement_method,
        "metric_id": metric_id,
        "scope": scope,
        "unit": unit,
        "value": value,
    }


def _artifact(
    repository: Path,
    artifact_id: str,
    role: str,
    relative_path: str,
    media_type: str,
    associated_stage: str | None,
    github_artifact_name: str | None,
    retention_days: int | None,
    producer_task: str | None = "open-build",
    force_available: bool | None = None,
    hash_regular_file: bool = True,
    measure_size: bool = True,
) -> dict[str, Any]:
    path = repository / relative_path
    available = path.exists() if force_available is None else force_available
    size: int | None = None
    digest: str | None = None
    if available:
        if hash_regular_file:
            size, digest = regular_file_metadata(path)
        if size is None and measure_size:
            size = path_size(path)
    if not available:
        size = None
        digest = None
    return {
        "artifact_id": artifact_id,
        "associated_stage": associated_stage,
        "available": available,
        "github_artifact_name": github_artifact_name,
        "media_type": media_type,
        "path": relative_path,
        "producer_task": producer_task,
        "retention_days": retention_days,
        "role": role,
        "sha256": digest,
        "size_bytes": size,
    }


def _cache_evidence(environment: Mapping[str, str]) -> dict[str, Any]:
    hit = _raw(environment, "CACHE_HIT").lower()
    matched = _nullable_string(_raw(environment, "CACHE_MATCHED_KEY"))
    restore_outcome = _raw(environment, "CACHE_RESTORE_OUTCOME").lower()
    if hit == "true":
        restore = "exact"
    elif matched:
        restore = "prefix"
    elif restore_outcome == "failure":
        restore = "error"
    elif restore_outcome in {"skipped", "cancelled"}:
        restore = "skipped"
    else:
        restore = "miss"
    save_outcome = _raw(environment, "CACHE_SAVE_OUTCOME").lower()
    if save_outcome not in {"success", "failure", "cancelled", "skipped"}:
        save_outcome = "unknown"
    return {
        "cache_matched_key": matched,
        "cache_primary_key": _nullable_string(_raw(environment, "CACHE_PRIMARY_KEY")),
        "cache_restore": restore,
        "cache_save_outcome": save_outcome,
    }


def _build_artifacts(
    repository: Path,
    environment: Mapping[str, str],
    diagnostics_expected: bool,
) -> dict[str, Any]:
    jobs = _required_int(environment, "PARALLELISM", 2)
    result_artifact_name = _raw(environment, "RESULT_ARTIFACT_NAME") or (
        f"panda-ci-results-{_raw(environment, 'WORKFLOW_RUN_ID')}-attempt-"
        f"{_raw(environment, 'RUN_ATTEMPT')}"
    )
    diagnostic_artifact_name = None
    if diagnostics_expected:
        diagnostic_artifact_name = _raw(
            environment, "DIAGNOSTICS_ARTIFACT_NAME"
        ) or f"open-build-diagnostics-{_raw(environment, 'WORKFLOW_RUN_ID')}"
    compilation_database = "compilation_db/build/compile_commands.json"
    distribution = _nullable_string(_raw(environment, "DIST_DIR")) or "panda_dist"
    distribution_available = bool(_nullable_string(_raw(environment, "DIST_DIR"))) and (
        repository / distribution
    ).is_dir()
    build_stderr = f".ci-telemetry/jobs-{jobs}/build-stderr.log"
    memory_samples = f".ci-telemetry/jobs-{jobs}/memory-samples.tsv"
    artifacts = [
        _artifact(
            repository,
            "structured-result-bundle",
            "structured-result-bundle",
            ".ci-results",
            "application/vnd.panda.ci-results+directory",
            None,
            result_artifact_name,
            14,
            force_available=True,
            hash_regular_file=False,
            measure_size=False,
        ),
        _artifact(
            repository,
            "compilation-database",
            "compilation-database",
            compilation_database,
            "application/json",
            "project-build",
            diagnostic_artifact_name
            if diagnostics_expected and (repository / compilation_database).is_file()
            else None,
            3 if diagnostics_expected else None,
        ),
        _artifact(
            repository,
            "installed-distribution",
            "installed-distribution",
            distribution,
            "application/vnd.panda.distribution+directory",
            "installation",
            None,
            None,
            force_available=distribution_available,
            hash_regular_file=False,
        ),
        _artifact(
            repository,
            "build-stderr",
            "build-stderr",
            build_stderr,
            "text/plain",
            "project-build",
            diagnostic_artifact_name
            if diagnostics_expected and (repository / build_stderr).is_file()
            else None,
            3 if diagnostics_expected else None,
        ),
        _artifact(
            repository,
            "cmake-diagnostics",
            "cmake-diagnostics",
            "build/CMakeFiles",
            "application/vnd.cmake.diagnostics+directory",
            "configure",
            diagnostic_artifact_name
            if diagnostics_expected and (repository / "build/CMakeFiles").is_dir()
            else None,
            3 if diagnostics_expected else None,
            hash_regular_file=False,
        ),
        _artifact(
            repository,
            "synthesis-smoke-diagnostics",
            "synthesis-smoke-diagnostics",
            "synthesis-smoke",
            "application/vnd.panda.synthesis-diagnostics+directory",
            "xml-verilator-cosimulation",
            diagnostic_artifact_name
            if diagnostics_expected and (repository / "synthesis-smoke").is_dir()
            else None,
            3 if diagnostics_expected else None,
            hash_regular_file=False,
        ),
        _artifact(
            repository,
            "memory-samples",
            "memory-samples",
            memory_samples,
            "text/tab-separated-values",
            "project-build",
            diagnostic_artifact_name
            if diagnostics_expected and (repository / memory_samples).is_file()
            else None,
            3 if diagnostics_expected else None,
        ),
    ]
    return {
        "artifacts": sorted(artifacts, key=lambda item: ARTIFACT_IDS.index(item["artifact_id"])),
        "schema": "panda.ci.artifact-index",
        "schema_version": "1.0",
    }


def _stage_failure(
    stage_id: str,
    action_status: int | None,
    oom: int | None,
    killed: int | None,
) -> dict[str, Any]:
    if oom == 1:
        return _failure(
            "resource",
            "oom-detected",
            stage_id,
            "The build stage failed and out-of-memory evidence was detected.",
            ["artifacts.json#artifact/build-stderr", "artifacts.json#artifact/memory-samples"],
            retryable=True,
        )
    if killed == 1:
        return _failure(
            "resource",
            "process-killed",
            stage_id,
            "The build stage failed after a process kill was detected.",
            ["artifacts.json#artifact/build-stderr"],
            retryable=True,
        )
    mapping = {
        "container-setup": (
            "infrastructure",
            "docker-action-failed",
            "The build container did not reach a usable execution environment.",
            [],
        ),
        "configure": (
            "configuration",
            "cmake-configure-exit-nonzero",
            "CMake configuration exited with a nonzero status.",
            ["artifacts.json#artifact/cmake-diagnostics"],
        ),
        "frontend-resolution": (
            "configuration",
            "frontend-resolution-failed",
            "The selected Clang frontend or plugin directory could not be resolved.",
            ["artifacts.json#artifact/cmake-diagnostics"],
        ),
        "plugin-build": (
            "compilation",
            "compiler-exit-nonzero",
            "PandA Clang plugin compilation or validation failed.",
            ["artifacts.json#artifact/build-stderr"],
        ),
        "project-build": (
            "compilation",
            "compiler-exit-nonzero",
            "PandA project compilation exited with a nonzero status.",
            ["artifacts.json#artifact/build-stderr"],
        ),
        "installation": (
            "installation",
            "install-exit-nonzero",
            "PandA installation exited with a nonzero status.",
            ["artifacts.json#artifact/cmake-diagnostics"],
        ),
        "xml-verilator-cosimulation": (
            "verification",
            "cosimulation-exit-nonzero",
            "The XML-driven Bambu/Verilator co-simulation failed.",
            ["artifacts.json#artifact/synthesis-smoke-diagnostics"],
        ),
    }
    category, code, message, evidence = mapping[stage_id]
    if action_status is not None:
        message = f"{message} Exit status: {action_status}."
    return _failure(category, code, stage_id, message, evidence, retryable=category == "infrastructure")


def _stage_record(
    stage_id: str,
    outcome: str,
    duration: int | float | None,
    exit_status: int | None,
    failure: dict[str, Any] | None,
    execution_state: str = "completed",
) -> dict[str, Any]:
    if outcome == "skipped":
        duration = None
        exit_status = None
        failure = None
    return {
        "artifact_ids": {
            "configure": ["cmake-diagnostics"],
            "plugin-build": ["build-stderr", "memory-samples"],
            "project-build": ["build-stderr", "memory-samples", "compilation-database"],
            "installation": ["installed-distribution"],
            "xml-verilator-cosimulation": ["synthesis-smoke-diagnostics"],
        }.get(stage_id, []),
        "duration_seconds": duration,
        "execution_state": execution_state,
        "exit_status": exit_status,
        "failure": failure,
        "metric_ids": [f"duration.{stage_id}"],
        "outcome": outcome,
        "stage_id": stage_id,
    }


def _build_task(
    environment: Mapping[str, str],
    started_at: str | None,
    completed_at: str,
) -> dict[str, Any]:
    build_outcome = _normalized_build_outcome(environment) or "failure"
    verify_outcome = _raw(environment, "VERIFY_OUTCOME").lower() or "skipped"
    verification_canceled = (
        build_outcome == "success"
        and _nullable_bool(_raw(environment, "JOB_CANCELLED")) is True
        and verify_outcome not in {"success", "failure"}
    )
    action_status = _nullable_int(_raw(environment, "ACTION_EXIT_STATUS"))
    failure_stage_raw = _raw(environment, "FAILURE_STAGE").lower()
    stage_aliases = {
        "initialization": "container-setup",
        "frontend-env": "container-setup",
        "build": "project-build",
        "install": "installation",
        "cosimulation": "xml-verilator-cosimulation",
        "complete": "",
        "none": "",
    }
    failed_stage = stage_aliases.get(failure_stage_raw, failure_stage_raw)
    if failed_stage not in STAGE_IDS:
        failed_stage = ""
    oom = _detection(_raw(environment, "OOM_DETECTED"))
    killed = _detection(_raw(environment, "KILL_DETECTED"))
    status_names = {
        "configure": "CONFIGURE_EXIT_STATUS",
        "frontend-resolution": "FRONTEND_RESOLUTION_EXIT_STATUS",
        "plugin-build": "PLUGIN_BUILD_EXIT_STATUS",
        "project-build": "PROJECT_BUILD_EXIT_STATUS",
        "installation": "INSTALLATION_EXIT_STATUS",
        "xml-verilator-cosimulation": "COSIMULATION_EXIT_STATUS",
    }
    duration_names = {
        "container-setup": "CONTAINER_SETUP_SECONDS",
        "configure": "CONFIGURE_SECONDS",
        "frontend-resolution": "FRONTEND_RESOLUTION_SECONDS",
        "plugin-build": "PLUGIN_BUILD_SECONDS",
        "project-build": "PROJECT_BUILD_SECONDS",
        "installation": "INSTALL_SECONDS",
        "installed-executable-validation": "VERIFY_SECONDS",
        "xml-verilator-cosimulation": "COSIMULATION_SECONDS",
    }
    internal_order = (
        "container-setup",
        "configure",
        "frontend-resolution",
        "plugin-build",
        "project-build",
        "installation",
        "xml-verilator-cosimulation",
    )
    entrypoint_started = bool(
        failure_stage_raw
        or _raw(environment, "ACTION_EXIT_STATUS")
        or _raw(environment, "CONTAINER_SETUP_SECONDS")
    )
    internal_records: dict[str, dict[str, Any]] = {}
    for stage_id in internal_order:
        duration = _nullable_float(_raw(environment, duration_names[stage_id]))
        stage_status = _nullable_int(_raw(environment, status_names.get(stage_id, "")))
        if build_outcome == "success":
            internal_records[stage_id] = _stage_record(stage_id, "pass", duration, 0, None)
            continue
        if build_outcome in {"cancelled", "canceled", "timed_out"}:
            interrupted_stage = failed_stage if failed_stage in internal_order else "container-setup"
            interrupted_index = internal_order.index(interrupted_stage)
            stage_index = internal_order.index(stage_id)
            if stage_index < interrupted_index:
                internal_records[stage_id] = _stage_record(stage_id, "pass", duration, 0, None)
            elif stage_index == interrupted_index:
                timed_out = build_outcome == "timed_out"
                interrupted_status = (
                    stage_status if stage_status not in (None, 0) else action_status
                )
                if interrupted_status == 0:
                    interrupted_status = None
                failure = _failure(
                    "timeout" if timed_out else "canceled",
                    "stage-timeout" if timed_out else "workflow-canceled",
                    stage_id,
                    "The stage timed out before completion."
                    if timed_out
                    else "The stage was canceled before completion.",
                    [],
                    retryable=True,
                )
                internal_records[stage_id] = _stage_record(
                    stage_id,
                    "unknown",
                    duration,
                    interrupted_status,
                    failure,
                    execution_state="timed_out" if timed_out else "canceled",
                )
            else:
                internal_records[stage_id] = _stage_record(
                    stage_id, "skipped", None, None, None
                )
            continue
        if not entrypoint_started or not failed_stage:
            if stage_id == "container-setup":
                failure = _stage_failure(stage_id, action_status, oom, killed)
                internal_records[stage_id] = _stage_record(
                    stage_id,
                    "fail",
                    duration,
                    action_status,
                    failure,
                    execution_state="infrastructure_error",
                )
            else:
                internal_records[stage_id] = _stage_record(
                    stage_id, "skipped", None, None, None
                )
            continue
        failed_index = internal_order.index(failed_stage) if failed_stage in internal_order else 0
        stage_index = internal_order.index(stage_id)
        if stage_index < failed_index:
            internal_records[stage_id] = _stage_record(stage_id, "pass", duration, 0, None)
        elif stage_index == failed_index:
            failed_status = stage_status if stage_status not in (None, 0) else action_status
            if failed_status == 0:
                failed_status = None
            failure = _stage_failure(stage_id, failed_status, oom, killed)
            internal_records[stage_id] = _stage_record(
                stage_id,
                "fail",
                duration,
                failed_status,
                failure,
                execution_state="infrastructure_error"
                if stage_id == "container-setup"
                else "completed",
            )
        else:
            internal_records[stage_id] = _stage_record(stage_id, "skipped", None, None, None)

    verify_duration = _nullable_float(_raw(environment, "VERIFY_SECONDS"))
    verify_status = _nullable_int(_raw(environment, "VERIFY_EXIT_STATUS"))
    if build_outcome != "success":
        verify_record = _stage_record(
            "installed-executable-validation", "skipped", None, None, None
        )
    elif verify_outcome == "success":
        verify_record = _stage_record(
            "installed-executable-validation", "pass", verify_duration, 0, None
        )
    elif verify_outcome == "failure":
        verify_failure = _failure(
            "verification",
            "installed-executable-validation-failed",
            "installed-executable-validation",
            "One or more installed executables were missing or could not start.",
            [],
        )
        verify_record = _stage_record(
            "installed-executable-validation",
            "fail",
            verify_duration,
            verify_status,
            verify_failure,
        )
    elif verification_canceled:
        verify_failure = _failure(
            "canceled",
            "workflow-canceled",
            "installed-executable-validation",
            "Installed-executable validation was canceled before completion.",
            [],
            retryable=True,
        )
        verify_record = _stage_record(
            "installed-executable-validation",
            "unknown",
            verify_duration,
            verify_status if verify_status not in (None, 0) else None,
            verify_failure,
            execution_state="canceled",
        )
    else:
        verify_record = _stage_record(
            "installed-executable-validation", "skipped", None, None, None
        )

    records = dict(internal_records)
    records["installed-executable-validation"] = verify_record
    stages = [records[stage_id] for stage_id in STAGE_IDS]

    if build_outcome == "success" and verify_outcome == "success":
        task_state = "completed"
        task_outcome = "pass"
        task_exit_status = 0
        task_failure = None
    elif verification_canceled:
        task_state = "canceled"
        task_outcome = "unknown"
        task_exit_status = verify_record["exit_status"]
        task_failure = verify_record["failure"]
    elif build_outcome in {"cancelled", "canceled"}:
        task_state = "canceled"
        task_outcome = "unknown"
        task_exit_status = action_status
        task_failure = _failure(
            "canceled",
            "workflow-canceled",
            failed_stage or "container-setup",
            "The task was canceled before completion.",
            [],
            retryable=True,
        )
    elif build_outcome == "timed_out":
        task_state = "timed_out"
        task_outcome = "unknown"
        task_exit_status = action_status
        task_failure = _failure(
            "timeout",
            "task-timeout",
            failed_stage or "container-setup",
            "The task timed out before completion.",
            [],
            retryable=True,
        )
    elif build_outcome == "success":
        task_state = "completed"
        task_outcome = "fail"
        task_exit_status = verify_status
        task_failure = verify_record["failure"]
    elif failed_stage == "container-setup":
        task_state = "infrastructure_error"
        task_outcome = "unknown"
        task_exit_status = action_status
        task_failure = internal_records["container-setup"]["failure"]
    elif not entrypoint_started or not failed_stage:
        task_state = "infrastructure_error"
        task_outcome = "unknown"
        task_exit_status = action_status
        task_failure = internal_records["container-setup"]["failure"]
    elif build_outcome != "success":
        task_state = "completed"
        task_outcome = "fail"
        task_exit_status = records[failed_stage]["exit_status"]
        task_failure = records[failed_stage]["failure"]

    executable_checks: list[dict[str, Any]] = []
    executable_inputs = (
        ("bambu", "BAMBU"),
        ("bambu-cc", "BAMBU_CC"),
        ("eucalyptus", "EUCALYPTUS"),
    )
    for executable, prefix in executable_inputs:
        exists = _nullable_bool(_raw(environment, f"{prefix}_EXISTS"))
        starts = _nullable_bool(_raw(environment, f"{prefix}_STARTS"))
        if verify_outcome not in {"success", "failure"}:
            exists = None
            starts = None
        if exists is True and starts is True:
            outcome = "pass"
            check_failure = None
        elif exists is False or starts is False:
            outcome = "fail"
            code = "executable-missing" if exists is False else "executable-start-failed"
            check_failure = _failure(
                "verification",
                code,
                "installed-executable-validation",
                f"Installed executable {executable} did not pass existence and startup checks.",
                [],
            )
        else:
            outcome = "skipped"
            check_failure = None
        executable_checks.append(
            {
                "check_id": f"installed-{executable}-exists-and-starts",
                "details": {"exists": exists, "starts": starts},
                "failure": check_failure,
                "outcome": outcome,
                "type": "executable-validation",
            }
        )

    cosim_record = records["xml-verilator-cosimulation"]
    cosim_check = {
        "check_id": "xml-verilator-cosimulation",
        "details": None,
        "failure": cosim_record["failure"],
        "outcome": cosim_record["outcome"],
        "type": "co-simulation",
    }
    schema_check = {
        "check_id": "result-schema-validation",
        "details": None,
        "failure": None,
        "outcome": "pass",
        "type": "schema-validation",
    }
    checks = executable_checks + [cosim_check, schema_check]

    metrics: list[dict[str, Any]] = []
    for stage in stages:
        metrics.append(
            _metric(
                f"duration.{stage['stage_id']}",
                stage["duration_seconds"],
                "seconds",
                "elapsed",
                stage["stage_id"],
                "Wall-clock timestamps captured immediately around the stage.",
            )
        )
    workflow_start_epoch = _nullable_int(_raw(environment, "WORKFLOW_START_EPOCH"))
    completion_epoch = _nullable_int(
        _raw(environment, "OPEN_BUILD_COMPLETION_EPOCH")
        or _raw(environment, "COMPLETION_EPOCH")
    )
    workflow_total_method = (
        "Workflow epoch delta through installed verification and cache save."
        if _raw(environment, "OPEN_BUILD_COMPLETION_EPOCH")
        else "Workflow epoch delta through cache save and result generation."
    )
    workflow_total = (
        completion_epoch - workflow_start_epoch
        if workflow_start_epoch is not None
        and completion_epoch is not None
        and completion_epoch >= workflow_start_epoch
        else None
    )
    metrics.extend(
        [
            _metric(
                "duration.build-total",
                _nullable_float(_raw(environment, "BUILD_SECONDS")),
                "seconds",
                "elapsed",
                "build",
                "Wall-clock interval spanning plugin and project compilation.",
            ),
            _metric(
                "duration.workflow-total",
                workflow_total,
                "seconds",
                "elapsed",
                "workflow",
                workflow_total_method,
            ),
            _metric(
                "memory.build.peak-cgroup-kib",
                _nullable_int(_raw(environment, "PEAK_BUILD_CGROUP_KIB")),
                "kibibytes",
                "maximum",
                "build",
                "Maximum sampled cgroup memory.current; preferred resource measurement.",
            ),
            _metric(
                "memory.build.peak-aggregate-rss-kib",
                _nullable_int(_raw(environment, "PEAK_BUILD_RSS_KIB")),
                "kibibytes",
                "maximum",
                "build",
                "Sum of process VmRSS sampled every 250 ms; shared pages may be counted more than once.",
            ),
            _metric(
                "memory.build.available-before-kib",
                _nullable_int(_raw(environment, "MEMORY_AVAILABLE_BEFORE_KIB")),
                "kibibytes",
                "snapshot",
                "build",
                "Linux /proc/meminfo MemAvailable immediately before compilation.",
            ),
            _metric(
                "memory.build.available-after-kib",
                _nullable_int(_raw(environment, "MEMORY_AVAILABLE_AFTER_KIB")),
                "kibibytes",
                "snapshot",
                "build",
                "Linux /proc/meminfo MemAvailable immediately after compilation.",
            ),
            _metric(
                "memory.build.oom-detected",
                oom,
                "boolean",
                "detected",
                "build",
                "Numeric 0/1 from cgroup OOM counter deltas and compiler stderr evidence.",
            ),
            _metric(
                "memory.build.kill-detected",
                killed,
                "boolean",
                "detected",
                "build",
                "Numeric 0/1 from signal exit status and compiler stderr evidence.",
            ),
            _metric(
                "ccache.cacheable-calls",
                _nullable_int(_raw(environment, "CCACHE_CACHEABLE_CALLS")),
                "count",
                "sum",
                "ccache",
                "ccache --print-stats after the build.",
            ),
            _metric(
                "ccache.hits",
                _nullable_int(_raw(environment, "CCACHE_HITS")),
                "count",
                "sum",
                "ccache",
                "Direct plus preprocessed ccache hits.",
            ),
            _metric(
                "ccache.misses",
                _nullable_int(_raw(environment, "CCACHE_MISSES")),
                "count",
                "sum",
                "ccache",
                "ccache cache misses.",
            ),
            _metric(
                "ccache.hit-rate",
                _nullable_float(_raw(environment, "CCACHE_HIT_RATE")),
                "percent",
                "ratio",
                "ccache",
                "100 multiplied by hits divided by cacheable calls.",
            ),
            _metric(
                "ccache.final-size-kib",
                _nullable_int(_raw(environment, "CCACHE_SIZE_KIB")),
                "kibibytes",
                "final",
                "ccache",
                "ccache cache_size_kibibyte after the build.",
            ),
        ]
    )
    metric_index = {item["metric_id"]: item for item in metrics}
    metrics = [metric_index[metric_id] for metric_id in METRIC_IDS]

    cache = _cache_evidence(environment)
    configuration = {
        "assertions_enabled": False,
        "build_type": "Release",
        "cache_matched_key": cache["cache_matched_key"],
        "cache_mode": "persistent-read-write",
        "cache_primary_key": cache["cache_primary_key"],
        "cache_restore": cache["cache_restore"],
        "cache_save_outcome": cache["cache_save_outcome"],
        "configured_parallelism": _required_int(environment, "PARALLELISM", 2),
        "release_enabled": True,
        "selected_frontend": _nullable_string(_raw(environment, "SELECTED_FRONTEND")),
        "synthesis_smoke_enabled": True,
        "warnings_as_errors": True,
    }
    return {
        "artifacts": list(ARTIFACT_IDS),
        "checks": checks,
        "configuration": configuration,
        "execution": {
            "completed_at": completed_at,
            "exit_status": task_exit_status,
            "started_at": started_at,
            "state": task_state,
        },
        "failure": task_failure,
        "metrics": metrics,
        "outcome": task_outcome,
        "schema": "panda.ci.task-result",
        "schema_version": "1.0",
        "stages": stages,
        "task_id": "open-build",
        "task_type": "build",
    }


def _build_verdict(task: dict[str, Any]) -> dict[str, Any]:
    checks = {item["check_id"]: item for item in task["checks"]}
    metrics = {item["metric_id"]: item for item in task["metrics"]}
    executable_outcomes = [checks[check_id]["outcome"] for check_id in CHECK_IDS[:3]]
    executable_rule = "pass" if all(value == "pass" for value in executable_outcomes) else (
        "fail" if any(value == "fail" for value in executable_outcomes) else "neutral"
    )
    cosim = checks["xml-verilator-cosimulation"]["outcome"]
    cosim_rule = "pass" if cosim == "pass" else "fail" if cosim == "fail" else "neutral"
    oom = metrics["memory.build.oom-detected"]["value"]
    killed = metrics["memory.build.kill-detected"]["value"]
    resource_rule = "fail" if 1 in (oom, killed) else (
        "pass" if oom == 0 and killed == 0 else "neutral"
    )
    outcomes = {
        "ci-result-schema-valid": "pass",
        "open-build-success": "pass" if task["outcome"] == "pass" else "fail",
        "installed-executable-validation": executable_rule,
        "xml-verilator-cosimulation": cosim_rule,
        "no-oom-or-kill": resource_rule,
        "fast-regressions-availability": "neutral",
    }
    reasons = {
        "ci-result-schema-valid": "The generated bundle passed schema and cross-document validation.",
        "open-build-success": "The open-build task outcome is evaluated as a blocking rule.",
        "installed-executable-validation": "All three installed executable checks must pass.",
        "xml-verilator-cosimulation": "The XML-driven Bambu/Verilator co-simulation must pass.",
        "no-oom-or-kill": "Detected OOM or kill evidence blocks merging; unavailable evidence is neutral.",
        "fast-regressions-availability": "Laboratory Fast Regressions availability is not observed by this workflow and is non-blocking.",
    }
    evidence = {
        "ci-result-schema-valid": ["tasks/open-build.json#check/result-schema-validation"],
        "open-build-success": ["tasks/open-build.json#outcome"],
        "installed-executable-validation": [
            f"tasks/open-build.json#check/{check_id}" for check_id in CHECK_IDS[:3]
        ],
        "xml-verilator-cosimulation": [
            "tasks/open-build.json#check/xml-verilator-cosimulation"
        ],
        "no-oom-or-kill": [
            "tasks/open-build.json#metric/memory.build.oom-detected",
            "tasks/open-build.json#metric/memory.build.kill-detected",
        ],
        "fast-regressions-availability": ["request.json#policy-profile"],
    }
    rules = [
        {
            "evidence": evidence[rule_id],
            "outcome": outcomes[rule_id],
            "reason": reasons[rule_id],
            "rule_id": rule_id,
            "severity": "non-blocking"
            if rule_id == "fast-regressions-availability"
            else "blocking",
        }
        for rule_id in RULE_IDS
    ]
    blocking = [item["outcome"] for item in rules if item["severity"] == "blocking"]
    overall = "fail" if "fail" in blocking else (
        "neutral" if any(value != "pass" for value in blocking) else "pass"
    )
    return {
        "merge_recommendation": {
            "pass": "merge",
            "fail": "do-not-merge",
            "neutral": "manual-review",
        }[overall],
        "overall_outcome": overall,
        "policy_profile": "pull-request-default",
        "rules": rules,
        "schema": "panda.ci.verdict",
        "schema_version": "1.0",
    }


def _generate_bundle_contents(
    output_directory: Path,
    env: Mapping[str, str],
    root: Path,
) -> dict[str, dict[str, Any]]:
    """Generate and validate one bundle in an empty working directory."""

    output = output_directory.resolve()
    started_at = _timestamp_from_epoch(_raw(env, "WORKFLOW_START_EPOCH"))
    completed_at = _completion_timestamp(env)
    open_build_completed_at = _timestamp_from_epoch(
        _raw(env, "OPEN_BUILD_COMPLETION_EPOCH")
    ) or completed_at
    build_outcome = _normalized_build_outcome(env)
    verify_outcome = _raw(env, "VERIFY_OUTCOME").lower()
    diagnostics_expected = build_outcome in {"failure", "timed_out"} or (
        verify_outcome == "failure"
    )

    request_id = _raw(env, "REQUEST_ID") or (
        f"panda-request-{_raw(env, 'WORKFLOW_RUN_ID')}-{_raw(env, 'RUN_ATTEMPT')}"
    )
    repository_name = _raw(env, "REPOSITORY")
    commit_sha = _raw(env, "COMMIT_SHA")
    request_commit_sha = _raw(env, "REQUEST_COMMIT_SHA") or commit_sha
    base_sha = _nullable_string(_raw(env, "BASE_SHA"))
    pull_request_number = _nullable_int(_raw(env, "PULL_REQUEST_NUMBER"))
    parallelism = _required_int(env, "PARALLELISM", 2)
    frontend = _raw(env, "REQUESTED_FRONTEND") or "I386_CLANG16"

    request = {
        "base_sha": base_sha,
        "build_parameters": {
            "assertions_enabled": False,
            "build_type": "Release",
            "cache_mode": "persistent-read-write",
            "cmake_arguments": list(CMAKE_ARGUMENTS),
            "configured_parallelism": parallelism,
            "release_enabled": True,
            "selected_frontend": frontend,
            "synthesis_smoke_enabled": True,
            "warnings_as_errors": True,
        },
        "commit_sha": request_commit_sha,
        "event_type": _raw(env, "EVENT_TYPE"),
        "policy_profile": "pull-request-default",
        "pull_request_number": pull_request_number,
        "ref": _raw(env, "REF"),
        "repository": repository_name,
        "request_id": request_id,
        "requested_artifact_ids": list(ARTIFACT_IDS),
        "requested_task_ids": ["open-build"],
        "schema": "panda.ci.request",
        "schema_version": "1.0",
        "tasks": [{"task_id": "open-build", "task_type": "build"}],
    }
    task = _build_task(env, started_at, open_build_completed_at)
    artifacts = _build_artifacts(root, env, diagnostics_expected)
    verdict = _build_verdict(task)

    values = {
        "request.json": request,
        "tasks/open-build.json": task,
        "artifacts.json": artifacts,
        "verdict.json": verdict,
    }
    for relative_path, value in values.items():
        write_json(output / relative_path, value)

    workflow_path = _raw(env, "WORKFLOW_FILE") or ".github/workflows/open-build-smoke.yml"
    dockerfile_path = ".github/actions/build-panda/Dockerfile"
    schema_by_path = {
        "request.json": ("request", "panda.ci.request"),
        "tasks/open-build.json": ("open-build", "panda.ci.task-result"),
        "artifacts.json": ("artifacts", "panda.ci.artifact-index"),
        "verdict.json": ("verdict", "panda.ci.verdict"),
    }
    document_references = [
        {
            "document_id": document_id,
            "path": relative_path,
            "schema": marker,
            "schema_version": "1.0",
            "sha256": sha256_file(output / relative_path),
        }
        for relative_path, (document_id, marker) in schema_by_path.items()
    ]
    document_references.sort(key=lambda item: item["document_id"])
    tool_names = (
        ("ccache", "CCACHE_VERSION"),
        ("clang", "CLANG_VERSION"),
        ("clangxx", "CLANGXX_VERSION"),
        ("cmake", "CMAKE_VERSION"),
        ("gcc", "GCC_VERSION"),
        ("gxx", "GXX_VERSION"),
        ("llvm", "LLVM_VERSION"),
        ("verilator", "VERILATOR_VERSION"),
    )
    tools = [
        {"tool_id": tool_id, "version": _nullable_string(_raw(env, variable))}
        for tool_id, variable in tool_names
    ]
    effective_build_profile = {
        "assertions_enabled": _nullable_bool(_raw(env, "EFFECTIVE_ASSERTIONS_ENABLED")),
        "build_type": _nullable_string(_raw(env, "EFFECTIVE_BUILD_TYPE")),
        "configured_parallelism": parallelism,
        "cpu_target_profile": _nullable_string(_raw(env, "CPU_TARGET_PROFILE")),
        "dockerfile_path": dockerfile_path,
        "dockerfile_sha256": sha256_file(root / dockerfile_path),
        "optimized_flags": _nullable_string(_raw(env, "EFFECTIVE_OPTIMIZED_FLAGS")),
        "selected_frontend": _nullable_string(_raw(env, "SELECTED_FRONTEND")),
        "tool_versions": tools,
        "warnings_as_errors": _nullable_bool(_raw(env, "EFFECTIVE_WARNINGS_AS_ERRORS")),
        "workflow_file": workflow_path,
        "workflow_file_sha256": sha256_file(root / workflow_path),
    }
    manifest = {
        "base_sha": base_sha,
        "commit_sha": commit_sha,
        "completed_at": completed_at,
        "configured_parallelism": parallelism,
        "container": {
            "base_image": docker_base_image(root / dockerfile_path),
            "dockerfile_path": dockerfile_path,
            "dockerfile_sha256": sha256_file(root / dockerfile_path),
            "image_digest": _nullable_string(_raw(env, "CONTAINER_IMAGE_DIGEST")),
        },
        "documents": document_references,
        "event_type": _raw(env, "EVENT_TYPE"),
        "effective_build_profile": effective_build_profile,
        "execution_state": task["execution"]["state"]
        if task["execution"]["state"] in {"canceled", "timed_out", "infrastructure_error"}
        else "completed",
        "pull_request_number": pull_request_number,
        "ref": _raw(env, "REF"),
        "repository": repository_name,
        "request_id": request_id,
        "run_attempt": _required_int(env, "RUN_ATTEMPT", 1),
        "run_id": _raw(env, "RUN_ID")
        or f"panda-run-{_raw(env, 'WORKFLOW_RUN_ID')}-{_raw(env, 'RUN_ATTEMPT')}",
        "runner": {
            "architecture": _nullable_string(_raw(env, "RUNNER_ARCH")),
            "image": _nullable_string(_raw(env, "RUNNER_IMAGE")),
            "os": _nullable_string(_raw(env, "RUNNER_OS")),
        },
        "schema": "panda.ci.manifest",
        "schema_version": "1.0",
        "started_at": started_at,
        "submodules": submodule_commits(root),
        "tools": tools,
        "workflow": {
            "file": workflow_path,
            "file_sha256": sha256_file(root / workflow_path),
            "implementation_commit": _raw(env, "WORKFLOW_IMPLEMENTATION_COMMIT"),
        },
        "workflow_run_id": _required_int(env, "WORKFLOW_RUN_ID", 1),
    }
    write_json(output / "manifest.json", manifest)
    return validate_bundle(output)


def generate_bundle(
    output_directory: Path,
    environment: Mapping[str, str] | None = None,
    repository: Path | None = None,
    regression_results: Path | None = None,
) -> dict[str, dict[str, Any]]:
    """Replace the destination with one freshly generated, validated bundle."""

    env = os.environ if environment is None else environment
    root = (repository or Path.cwd()).resolve()
    candidate_dir_path = _raw(env, "CANDIDATE_BUNDLE_DIR")
    candidate_path = Path(candidate_dir_path) if candidate_dir_path else None
    candidate_directory = None
    if candidate_path is not None:
        candidate_directory = (
            candidate_path if candidate_path.is_absolute() else root / candidate_path
        )
    output = output_directory.absolute()
    if output.is_symlink():
        raise ValueError(f"refusing to replace symlinked bundle directory: {output}")
    output = output.resolve()
    if output == root or output == Path(output.anchor):
        raise ValueError(f"refusing unsafe bundle output directory: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(
        tempfile.mkdtemp(prefix=f".{output.name}.generate-", dir=output.parent)
    )
    try:
        _generate_bundle_contents(temporary, env, root)
        if regression_results is not None:
            raw_results = (
                regression_results
                if regression_results.is_absolute()
                else root / regression_results
            )
            extend_bundle_with_regressions(temporary, raw_results, env, root)
            validate_bundle(temporary)
        if output.exists():
            if output.is_dir():
                shutil.rmtree(output)
            else:
                output.unlink()
        os.replace(temporary, output)
        return validate_bundle(output)
    except (
        BundleValidationError,
        SchemaValidationError,
        SerializationError,
        OSError,
        ValueError,
    ):
        if candidate_directory is not None:
            try:
                if candidate_directory.exists():
                    if candidate_directory.is_dir():
                        shutil.rmtree(candidate_directory)
                    else:
                        candidate_directory.unlink()
                candidate_directory.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(temporary, candidate_directory)
            except OSError:
                pass
        raise
    finally:
        shutil.rmtree(temporary, ignore_errors=True)
