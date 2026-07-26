"""Human-readable views derived only from validated bundle documents."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .bundle import validate_bundle


def _display(value: Any, suffix: str = "") -> str:
    return "n/a" if value is None else f"{value}{suffix}"


def _regression_summary(documents: dict[str, dict[str, Any]]) -> list[str]:
    request = documents["request.json"]
    tasks = [
        documents[f"tasks/{task_id}.json"]
        for task_id in request["requested_task_ids"]
        if task_id != "open-build"
    ]
    if not tasks:
        return []
    lines = [
        "## GitHub-hosted fast regressions",
        "",
        "| Regression | Category | Synthesis | Simulation | Verification | Duration | Result |",
        "| --- | --- | --- | --- | --- | ---: | --- |",
    ]
    for task in tasks:
        stages = {stage["stage_id"]: stage["outcome"] for stage in task["stages"]}
        metrics = {metric["metric_id"]: metric["value"] for metric in task["metrics"]}
        lines.append(
            "| "
            f"`{task['task_id']}` | {task['configuration']['category']} | "
            f"{stages['hls-synthesis']} | {stages['rtl-simulation']} | "
            f"{stages['result-verification']} | "
            f"{_display(metrics['duration.regression-total'], ' s')} | "
            f"**{task['outcome']}** |"
        )
    suite = documents["manifest.json"].get("hosted_regression_suite", {})
    elapsed = suite.get("duration_seconds")
    lines.extend(["", f"Regression suite elapsed: {_display(elapsed, ' s')}", ""])
    blocking = [task for task in tasks if task["outcome"] != "pass"]
    if blocking:
        lines.extend(["Blocking regression results:", ""])
        for task in blocking:
            failure = task["failure"]
            if failure is None:
                detail = "skipped because its prerequisite did not pass"
            else:
                detail = (
                    f"`{failure['code']}` at `{failure['stage']}` — {failure['message']}"
                )
            lines.append(f"- `{task['task_id']}`: {detail}")
        lines.append("")
    if suite.get("outcome") != "pass":
        lines.extend(
            [
                "Hosted regression suite status:",
                "",
                "- "
                f"action=`{suite.get('action_outcome', 'unknown')}`, "
                f"action_exit_status=`{_display(suite.get('action_exit_status'))}`, "
                f"state=`{suite.get('execution_state', 'unknown')}`, "
                f"outcome=`{suite.get('outcome', 'unknown')}`, "
                f"exit_status=`{_display(suite.get('exit_status'))}`, "
                f"failure_stage=`{_display(suite.get('failure_stage'))}`",
            ]
        )
        lines.append("")
    return lines


def render_summary(documents: dict[str, dict[str, Any]]) -> str:
    task = documents["tasks/open-build.json"]
    verdict = documents["verdict.json"]
    task_outcome_label = (
        "Task outcome"
        if task["schema_version"] == "1.0"
        else "Open-build task outcome"
    )
    metrics = {item["metric_id"]: item["value"] for item in task["metrics"]}
    configuration = task["configuration"]
    values = {
        "container_setup_seconds": metrics["duration.container-setup"],
        "configure_seconds": metrics["duration.configure"],
        "build_seconds": metrics["duration.build-total"],
        "install_seconds": metrics["duration.installation"],
        "cosimulation_seconds": metrics["duration.xml-verilator-cosimulation"],
        "total_workflow_seconds": metrics["duration.workflow-total"],
        "ccache_cacheable_calls": metrics["ccache.cacheable-calls"],
        "ccache_hits": metrics["ccache.hits"],
        "ccache_misses": metrics["ccache.misses"],
        "ccache_hit_rate": metrics["ccache.hit-rate"],
        "ccache_size_kibibyte": metrics["ccache.final-size-kib"],
    }
    lines = [
        "## Open build performance",
        "",
        "| Measurement | Value |",
        "| --- | ---: |",
        f"| Container setup | {_display(values['container_setup_seconds'], ' s')} |",
        f"| CMake configure | {_display(values['configure_seconds'], ' s')} |",
        f"| PandA compilation | {_display(values['build_seconds'], ' s')} |",
        f"| Installation | {_display(values['install_seconds'], ' s')} |",
        f"| XML/Verilator co-simulation | {_display(values['cosimulation_seconds'], ' s')} |",
        f"| Total workflow through cache save | {_display(values['total_workflow_seconds'], ' s')} |",
        f"| Cacheable calls | {_display(values['ccache_cacheable_calls'])} |",
        f"| Cache hits | {_display(values['ccache_hits'])} |",
        f"| Cache misses | {_display(values['ccache_misses'])} |",
        f"| Cache hit rate | {_display(values['ccache_hit_rate'], '%')} |",
        f"| Cache size | {_display(values['ccache_size_kibibyte'], ' KiB')} |",
        "",
        "~~~text",
    ]
    lines.extend(f"{key}={_display(value)}" for key, value in values.items())
    lines.extend(
        [
            f"cache_restore={configuration['cache_restore']}",
            f"cache_primary_key={configuration['cache_primary_key'] or ''}",
            f"cache_matched_key={configuration['cache_matched_key'] or ''}",
            f"cache_save_outcome={configuration['cache_save_outcome']}",
            "~~~",
            "",
        ]
    )
    lines.extend(_regression_summary(documents))
    lines.extend(
        [
            "## Machine-readable CI verdict",
            "",
            f"- {task_outcome_label}: `{task['outcome']}`",
            f"- Policy outcome: `{verdict['overall_outcome']}`",
            f"- Merge recommendation: `{verdict['merge_recommendation']}`",
            "- Bundle entry point: `.ci-results/manifest.json`",
            "",
        ]
    )
    return "\n".join(lines)


def render_bundle_summary(bundle_directory: Path) -> str:
    return render_summary(validate_bundle(bundle_directory))
