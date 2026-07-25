"""Stable identifiers used by the PandA CI protocol major version 1."""

from __future__ import annotations


LEGACY_SCHEMA_VERSION = "1.0"
MULTI_TASK_SCHEMA_VERSION = "1.1"
SUPPORTED_SCHEMA_VERSIONS = (
    LEGACY_SCHEMA_VERSION,
    MULTI_TASK_SCHEMA_VERSION,
)

OPEN_BUILD_STAGE_IDS = (
    "container-setup",
    "configure",
    "frontend-resolution",
    "plugin-build",
    "project-build",
    "installation",
    "installed-executable-validation",
    "xml-verilator-cosimulation",
)

OPEN_BUILD_DURATION_METRIC_IDS = tuple(
    f"duration.{stage_id}" for stage_id in OPEN_BUILD_STAGE_IDS
)

OPEN_BUILD_METRIC_IDS = OPEN_BUILD_DURATION_METRIC_IDS + (
    "duration.build-total",
    "duration.workflow-total",
    "memory.build.peak-cgroup-kib",
    "memory.build.peak-aggregate-rss-kib",
    "memory.build.available-before-kib",
    "memory.build.available-after-kib",
    "memory.build.oom-detected",
    "memory.build.kill-detected",
    "ccache.cacheable-calls",
    "ccache.hits",
    "ccache.misses",
    "ccache.hit-rate",
    "ccache.final-size-kib",
)

OPEN_BUILD_METRIC_CONTRACTS = {
    **{
        f"duration.{stage_id}": ("seconds", "elapsed", stage_id)
        for stage_id in OPEN_BUILD_STAGE_IDS
    },
    "duration.build-total": ("seconds", "elapsed", "build"),
    "duration.workflow-total": ("seconds", "elapsed", "workflow"),
    "memory.build.peak-cgroup-kib": ("kibibytes", "maximum", "build"),
    "memory.build.peak-aggregate-rss-kib": ("kibibytes", "maximum", "build"),
    "memory.build.available-before-kib": ("kibibytes", "snapshot", "build"),
    "memory.build.available-after-kib": ("kibibytes", "snapshot", "build"),
    "memory.build.oom-detected": ("boolean", "detected", "build"),
    "memory.build.kill-detected": ("boolean", "detected", "build"),
    "ccache.cacheable-calls": ("count", "sum", "ccache"),
    "ccache.hits": ("count", "sum", "ccache"),
    "ccache.misses": ("count", "sum", "ccache"),
    "ccache.hit-rate": ("percent", "ratio", "ccache"),
    "ccache.final-size-kib": ("kibibytes", "final", "ccache"),
}

OPEN_BUILD_CHECK_IDS = (
    "installed-bambu-exists-and-starts",
    "installed-bambu-cc-exists-and-starts",
    "installed-eucalyptus-exists-and-starts",
    "xml-verilator-cosimulation",
    "result-schema-validation",
)

OPEN_BUILD_ARTIFACT_IDS = (
    "structured-result-bundle",
    "compilation-database",
    "installed-distribution",
    "build-stderr",
    "cmake-diagnostics",
    "synthesis-smoke-diagnostics",
    "memory-samples",
)

LEGACY_RULE_IDS = (
    "ci-result-schema-valid",
    "open-build-success",
    "installed-executable-validation",
    "xml-verilator-cosimulation",
    "no-oom-or-kill",
    "fast-regressions-availability",
)

REGRESSION_STAGE_IDS = (
    "input-validation",
    "hls-synthesis",
    "rtl-generation",
    "simulator-preparation",
    "rtl-simulation",
    "result-verification",
)

REGRESSION_METRIC_IDS = tuple(
    f"duration.{stage_id}" for stage_id in REGRESSION_STAGE_IDS
) + ("duration.regression-total",)

REGRESSION_METRIC_CONTRACTS = {
    **{
        f"duration.{stage_id}": ("seconds", "elapsed", stage_id)
        for stage_id in REGRESSION_STAGE_IDS
    },
    "duration.regression-total": ("seconds", "elapsed", "regression"),
}

REGRESSION_CHECK_IDS = (
    "rtl-artifacts-produced",
    "simulation-completed",
    "expected-output-matches",
)

REGRESSION_CHECK_TYPES = {
    "rtl-artifacts-produced": "artifact-validation",
    "simulation-completed": "simulation",
    "expected-output-matches": "result-verification",
}

REGRESSION_ARTIFACT_SUFFIXES = (
    "bambu-log",
    "result-report",
    "rtl-output",
    "simulation-log",
)

HOSTED_REGRESSION_RULE_ID = "hosted-fast-regressions-success"
MULTI_TASK_RULE_IDS = LEGACY_RULE_IDS + (HOSTED_REGRESSION_RULE_ID,)

CORE_DOCUMENT_PATHS = (
    "request.json",
    "artifacts.json",
    "verdict.json",
)

# Backward-compatible aliases used by the 1.0 producer and its tests.
STAGE_IDS = OPEN_BUILD_STAGE_IDS
DURATION_METRIC_IDS = OPEN_BUILD_DURATION_METRIC_IDS
METRIC_IDS = OPEN_BUILD_METRIC_IDS
METRIC_CONTRACTS = OPEN_BUILD_METRIC_CONTRACTS
CHECK_IDS = OPEN_BUILD_CHECK_IDS
ARTIFACT_IDS = OPEN_BUILD_ARTIFACT_IDS
RULE_IDS = LEGACY_RULE_IDS

DOCUMENT_PATHS = (
    "request.json",
    "tasks/open-build.json",
    "artifacts.json",
    "verdict.json",
)

SCHEMA_FILES = {
    "panda.ci.request": "request.schema.json",
    "panda.ci.manifest": "manifest.schema.json",
    "panda.ci.task-result": "task-result.schema.json",
    "panda.ci.artifact-index": "artifacts.schema.json",
    "panda.ci.verdict": "verdict.schema.json",
}

EXECUTION_STATES = {
    "queued",
    "running",
    "completed",
    "canceled",
    "timed_out",
    "infrastructure_error",
}

OUTCOMES = {"pass", "fail", "skipped", "unknown", "neutral"}
