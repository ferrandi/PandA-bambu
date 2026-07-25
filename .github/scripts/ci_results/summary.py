"""Human-readable views derived only from validated bundle documents."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .bundle import validate_bundle


def _display(value: Any, suffix: str = "") -> str:
    return "n/a" if value is None else f"{value}{suffix}"


def render_summary(documents: dict[str, dict[str, Any]]) -> str:
    task = documents["tasks/open-build.json"]
    verdict = documents["verdict.json"]
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
            "## Machine-readable CI verdict",
            "",
            f"- Task outcome: `{task['outcome']}`",
            f"- Policy outcome: `{verdict['overall_outcome']}`",
            f"- Merge recommendation: `{verdict['merge_recommendation']}`",
            "- Bundle entry point: `.ci-results/manifest.json`",
            "",
        ]
    )
    return "\n".join(lines)


def render_bundle_summary(bundle_directory: Path) -> str:
    return render_summary(validate_bundle(bundle_directory))
