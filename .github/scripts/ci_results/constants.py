"""Stable identifiers used by the PandA CI protocol version 1.0."""

from __future__ import annotations


STAGE_IDS = (
    "container-setup",
    "configure",
    "frontend-resolution",
    "plugin-build",
    "project-build",
    "installation",
    "installed-executable-validation",
    "xml-verilator-cosimulation",
)

DURATION_METRIC_IDS = tuple(f"duration.{stage_id}" for stage_id in STAGE_IDS)

METRIC_IDS = DURATION_METRIC_IDS + (
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

METRIC_CONTRACTS = {
    **{
        f"duration.{stage_id}": ("seconds", "elapsed", stage_id)
        for stage_id in STAGE_IDS
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

CHECK_IDS = (
    "installed-bambu-exists-and-starts",
    "installed-bambu-cc-exists-and-starts",
    "installed-eucalyptus-exists-and-starts",
    "xml-verilator-cosimulation",
    "result-schema-validation",
)

ARTIFACT_IDS = (
    "structured-result-bundle",
    "compilation-database",
    "installed-distribution",
    "build-stderr",
    "cmake-diagnostics",
    "synthesis-smoke-diagnostics",
    "memory-samples",
)

RULE_IDS = (
    "ci-result-schema-valid",
    "open-build-success",
    "installed-executable-validation",
    "xml-verilator-cosimulation",
    "no-oom-or-kill",
    "fast-regressions-availability",
)

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
