"""Run the deterministic hosted Bambu fast-regression suite.

The runner deliberately invokes Bambu for synthesis, Verilator preparation,
simulation, and result comparison.  It does not invoke Verilator or compare
test-vector outputs independently of Bambu.
"""

from __future__ import annotations

import os
import copy
import re
import shutil
import signal
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any, Iterable, Mapping

from .constants import (
    MULTI_TASK_SCHEMA_VERSION,
    REGRESSION_ARTIFACT_SUFFIXES,
    REGRESSION_CHECK_IDS,
    REGRESSION_CHECK_TYPES,
    REGRESSION_METRIC_CONTRACTS,
    REGRESSION_METRIC_IDS,
    REGRESSION_STAGE_IDS,
    RUNTIME_LINKAGE_ARTIFACT_SUFFIX,
    RUNTIME_LINKAGE_TASK_ID,
)
from .hashing import sha256_file
from .runtime_linkage import inspect_runtime_linkage
from .serialization import SerializationError, load_json, require_canonical, write_json


@dataclass(frozen=True)
class RegressionSpec:
    """One established, small PandA regression invocation."""

    task_id: str
    category: str
    example_id: str
    source_path: str
    top_function: str
    test_vector_kind: str
    test_vector: str
    optimization: str | None = None
    clock_period: float | None = None
    interface: str | None = None
    experimental_setup: str | None = None
    expose_globals: bool = False
    language_standard: str | None = None
    bambu_parameters: tuple[str, ...] = ()
    extra_arguments: tuple[str, ...] = ()
    rtl_authenticity_instances: tuple[tuple[str, str], ...] = ()


REGRESSION_SPECS = (
    RegressionSpec(
        task_id="regression-scalar",
        category="scalar-arithmetic",
        example_id="bambu-specific-test5/adders",
        source_path="panda_regressions/hls/bambu_specific_test5/adders.c",
        top_function="adders",
        test_vector_kind="inline",
        test_vector="a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8,i=9,j=10,k=11,l=12",
        clock_period=5,
        expose_globals=True,
    ),
    RegressionSpec(
        task_id="regression-control",
        category="switch-and-loop-control",
        example_id="bambu-specific-test2/duff-device",
        source_path="panda_regressions/hls/bambu_specific_test2/duff_device.c",
        top_function="duff",
        test_vector_kind="xml",
        test_vector="panda_regressions/hls/bambu_specific_test2/dd_test1.xml",
        optimization="O2",
        experimental_setup="BAMBU",
        expose_globals=True,
        extra_arguments=("-lm",),
    ),
    RegressionSpec(
        task_id="regression-loop-cxx",
        category="loops-arrays-clang-cxx",
        example_id="bambu-interface-test/ac-fixed2-cxx17",
        source_path="panda_regressions/hls/bambu_interface_test/ac_fixed2_tb.cpp",
        top_function="sum3numbers",
        test_vector_kind="xml",
        test_vector="panda_regressions/hls/bambu_interface_test/ac_fixed2_tb.xml",
        optimization="O2",
        interface="INFER",
        experimental_setup="BAMBU",
        language_standard="c++17",
        extra_arguments=("-lm",),
    ),
    RegressionSpec(
        task_id="regression-memory-interface",
        category="memory-interface-generation",
        example_id="bambu-interface-test/simple4-axi-m",
        source_path="panda_regressions/hls/bambu_interface_test/simple4_axi_m.c",
        top_function="maxNumbers",
        test_vector_kind="xml",
        test_vector="panda_regressions/hls/bambu_interface_test/simple4_axi_tb.xml",
        optimization="O2",
        interface="INFER",
        experimental_setup="BAMBU",
        extra_arguments=("-lm",),
    ),
    RegressionSpec(
        task_id="regression-callgraph",
        category="function-callgraph",
        example_id="bambu-interface-test/nested-axi-m",
        source_path="panda_regressions/hls/bambu_interface_test/nested_axi_m.c",
        top_function="main_module",
        test_vector_kind="xml",
        test_vector="panda_regressions/hls/bambu_interface_test/nested_axi_m.xml",
        optimization="O2",
        interface="INFER",
        experimental_setup="BAMBU",
        bambu_parameters=("function-opt=0",),
        extra_arguments=("-lm",),
    ),
    RegressionSpec(
        task_id="regression-sparta",
        category="openmp-context-switch",
        example_id="openmp-functional/vector-add-context-switch",
        source_path="examples/OpenMP/functional/src/vector_add.cpp",
        top_function="vector_add",
        test_vector_kind="xml",
        test_vector="examples/OpenMP/functional/src/vector.xml",
        extra_arguments=(
            "-lm",
            "-fopenmp",
            "--context_switch=2",
            "--channels-type=MEM_ACC_11",
            "--memory-allocation-policy=GLSS",
        ),
        rtl_authenticity_instances=(
            ("kmp_bambu_cs_manager", "cs_manager"),
            ("kmp_bambu_omp_start_cs", "omp_start_cs"),
            ("kmp_bambu_omp_done_cs", "omp_done_cs"),
        ),
    ),
    RegressionSpec(
        task_id="regression-graphsage",
        category="graph-neural-network-context-switch",
        example_id="graphsage/mean-aggregation-context-switch",
        source_path="examples/GraphSAGE/graphsage_mean.cpp",
        top_function="graphsage_mean",
        test_vector_kind="cxx",
        test_vector="examples/GraphSAGE/graphsage_mean_test.cpp",
        extra_arguments=(
            "-fopenmp",
            "--context_switch=2",
            "--channels-type=MEM_ACC_11",
            "--memory-allocation-policy=GLSS",
        ),
        rtl_authenticity_instances=(
            ("kmp_bambu_cs_manager", "cs_manager"),
            ("kmp_bambu_omp_start_cs", "omp_start_cs"),
            ("kmp_bambu_omp_done_cs", "omp_done_cs"),
        ),
    ),
)

REGRESSION_TASK_IDS = tuple(spec.task_id for spec in REGRESSION_SPECS)

def _timestamp(epoch: float | None = None) -> str:
    value = time.time() if epoch is None else epoch
    return datetime.fromtimestamp(value, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _seconds(nanoseconds: int | float) -> float | int:
    value = round(float(nanoseconds) / 1_000_000_000, 3)
    return int(value) if value.is_integer() else value


def _normalized_exit_status(returncode: int | None) -> int | None:
    if returncode is None:
        return None
    return 128 + abs(returncode) if returncode < 0 else returncode


def _artifact_ids(task_id: str) -> tuple[str, ...]:
    suffixes = REGRESSION_ARTIFACT_SUFFIXES + (
        (RUNTIME_LINKAGE_ARTIFACT_SUFFIX,) if task_id == RUNTIME_LINKAGE_TASK_ID else ()
    )
    return tuple(sorted(f"{task_id}.{suffix}" for suffix in suffixes))


def _artifact_by_suffix(artifact_ids: tuple[str, ...], suffix: str) -> str:
    return next(item for item in artifact_ids if item.endswith(f".{suffix}"))


def _failure(
    category: str,
    code: str,
    stage: str,
    message: str,
    artifact_id: str,
    *,
    retryable: bool = False,
) -> dict[str, Any]:
    return {
        "category": category,
        "code": code,
        "evidence": [f"artifacts.json#artifact/{artifact_id}"],
        "message": message,
        "retryable": retryable,
        "stage": stage,
    }


def _stage_records(
    durations: dict[str, float | int | None],
    artifact_ids: tuple[str, ...],
    outcome: str,
    failure: dict[str, Any] | None,
    exit_status: int | None,
    exceptional_state: str | None,
) -> list[dict[str, Any]]:
    stage_artifacts = {
        "input-validation": [],
        "hls-synthesis": [_artifact_by_suffix(artifact_ids, "bambu-log")],
        "rtl-generation": [_artifact_by_suffix(artifact_ids, "rtl-output")],
        "simulator-preparation": [_artifact_by_suffix(artifact_ids, "runtime-linkage")]
        if any(item.endswith(".runtime-linkage") for item in artifact_ids) else [],
        "rtl-simulation": [_artifact_by_suffix(artifact_ids, "simulation-log")],
        "result-verification": [_artifact_by_suffix(artifact_ids, "result-report")],
    }
    failure_index = (
        REGRESSION_STAGE_IDS.index(failure["stage"])
        if failure is not None
        else None
    )
    records: list[dict[str, Any]] = []
    for index, stage_id in enumerate(REGRESSION_STAGE_IDS):
        stage_failure = None
        stage_exit: int | None
        if failure_index is None:
            stage_outcome = "pass"
            stage_state = "completed"
            stage_exit = 0
        elif index < failure_index:
            stage_outcome = "pass"
            stage_state = "completed"
            stage_exit = 0
        elif index == failure_index:
            stage_outcome = "unknown" if outcome == "unknown" else "fail"
            stage_state = exceptional_state or "completed"
            stage_exit = exit_status
            stage_failure = failure
        else:
            stage_outcome = "skipped"
            stage_state = "completed"
            stage_exit = None
            durations[stage_id] = None
        records.append(
            {
                "artifact_ids": stage_artifacts[stage_id],
                "duration_seconds": durations[stage_id]
                if stage_outcome != "skipped"
                else None,
                "execution_state": stage_state,
                "exit_status": stage_exit,
                "failure": stage_failure,
                "metric_ids": [f"duration.{stage_id}"],
                "outcome": stage_outcome,
                "stage_id": stage_id,
            }
        )
    return records


def _metrics(
    durations: dict[str, float | int | None], total: float | int | None
) -> list[dict[str, Any]]:
    methods = {
        "input-validation": "Python monotonic clock around repository input and tool checks.",
        "hls-synthesis": (
            "Elapsed from Bambu launch to its first non-version Verilator invocation; "
            "this integrated boundary also includes RTL emission."
        ),
        "rtl-generation": (
            "Bambu exposes no separate wall-clock boundary; generated RTL is validated "
            "after the integrated invocation."
        ),
        "simulator-preparation": (
            "Bambu owns Verilator compilation and exposes no separate wall-clock boundary."
        ),
        "rtl-simulation": (
            "Elapsed from the first non-version Verilator invocation through Bambu exit; "
            "this includes simulator compilation and Bambu comparison."
        ),
        "result-verification": "Python monotonic clock around bambu_results.xml inspection.",
    }
    values = {f"duration.{key}": value for key, value in durations.items()}
    values["duration.regression-total"] = total
    result = []
    for metric_id in REGRESSION_METRIC_IDS:
        unit, aggregation, scope = REGRESSION_METRIC_CONTRACTS[metric_id]
        result.append(
            {
                "aggregation": aggregation,
                "measurement_method": (
                    "End-to-end Python monotonic wall clock for this regression."
                    if metric_id == "duration.regression-total"
                    else methods[scope]
                ),
                "metric_id": metric_id,
                "scope": scope,
                "unit": unit,
                "value": values[metric_id],
            }
        )
    return result


def _checks(stages: list[dict[str, Any]]) -> list[dict[str, Any]]:
    stage_by_id = {stage["stage_id"]: stage for stage in stages}
    stage_for_check = {
        "rtl-artifacts-produced": "rtl-generation",
        "simulation-completed": "rtl-simulation",
        "expected-output-matches": "result-verification",
    }
    return [
        {
            "check_id": check_id,
            "details": None,
            "failure": stage_by_id[stage_for_check[check_id]]["failure"],
            "outcome": stage_by_id[stage_for_check[check_id]]["outcome"],
            "type": REGRESSION_CHECK_TYPES[check_id],
        }
        for check_id in REGRESSION_CHECK_IDS
    ]


def _normalized_arguments(
    spec: RegressionSpec,
    compiler: str,
    parallel_backend: int,
) -> list[str]:
    output = f".ci-regression-work/{spec.task_id}/output"
    arguments: list[str] = []
    if spec.optimization:
        arguments.append(f"-{spec.optimization}")
    arguments.append(spec.source_path)
    arguments.extend(spec.extra_arguments)
    arguments.extend(
        (
            "--simulate",
            "--simulator=VERILATOR",
            f"--generate-tb={spec.test_vector}",
            f"--top-fname={spec.top_function}",
            f"--compiler={compiler}",
            f"--parallel-backend={parallel_backend}",
        )
    )
    if spec.expose_globals:
        arguments.append("--expose-globals")
    if spec.clock_period is not None:
        arguments.append(f"--clock-period={spec.clock_period:g}")
    if spec.interface:
        arguments.append(f"--generate-interface={spec.interface}")
    if spec.experimental_setup:
        arguments.append(f"--experimental-setup={spec.experimental_setup}")
    if spec.language_standard:
        arguments.append(f"--std={spec.language_standard}")
    arguments.extend(f"--bambu-parameter={value}" for value in spec.bambu_parameters)
    arguments.extend((f"--output-directory={output}", "--no-clean"))
    return arguments


def _actual_arguments(
    repository: Path,
    spec: RegressionSpec,
    normalized: Iterable[str],
) -> list[str]:
    result: list[str] = []
    for argument in normalized:
        if argument == spec.source_path:
            result.append(str(repository / argument))
        elif argument == f"--generate-tb={spec.test_vector}" and spec.test_vector_kind in {"xml", "cxx"}:
            result.append(f"--generate-tb={repository / spec.test_vector}")
        elif argument.startswith("--output-directory="):
            relative = argument.split("=", 1)[1]
            result.append(f"--output-directory={repository / relative}")
        else:
            result.append(argument)
    return result


def _write_verilator_wrapper(directory: Path, real_verilator: str) -> Path:
    wrapper = directory / "bin" / "verilator"
    wrapper.parent.mkdir(parents=True, exist_ok=True)
    real_root = Path(real_verilator).resolve().parent.parent
    real_share = real_root / "share" / "verilator"
    shadow_share = directory / "share" / "verilator"
    shadow_share.parent.mkdir(parents=True, exist_ok=True)
    if real_share.is_dir():
        shadow_share.symlink_to(real_share, target_is_directory=True)
    wrapper.write_text(
        "#!/bin/sh\n"
        "case \" $* \" in\n"
        "  *\" --version \"*|*\" -V \"*) exec \"${PANDA_CI_REAL_VERILATOR}\" \"$@\" ;;\n"
        "esac\n"
        "if test ! -e \"${PANDA_CI_VERILATOR_MARKER}\"; then\n"
        "  date +%s%N > \"${PANDA_CI_VERILATOR_MARKER}\"\n"
        "fi\n"
        "exec \"${PANDA_CI_REAL_VERILATOR}\" \"$@\"\n",
        encoding="utf-8",
        newline="\n",
    )
    wrapper.chmod(0o755)
    return wrapper.parent


def _local_name(element: ET.Element) -> str:
    return element.tag.rsplit("}", 1)[-1]


def _inspect_result_report(path: Path | None) -> tuple[bool, int | None, int | None, str | None]:
    if path is None:
        return False, None, None, "Bambu did not produce bambu_results.xml."
    try:
        root = ET.parse(path).getroot()
    except (ET.ParseError, OSError) as error:
        return False, None, None, f"Unable to parse {path.name}: {error}"
    simulation = next((item for item in root.iter() if _local_name(item) == "simulation"), None)
    if simulation is None:
        return False, None, None, "bambu_results.xml contains no simulation result."
    runs = [item for item in simulation if _local_name(item) == "run"]
    total_cycles = 0.0
    try:
        for run in runs:
            total_cycles += float((run.text or "").strip())
    except ValueError:
        return False, len(runs), None, "bambu_results.xml contains a non-numeric cycle count."
    return_value = simulation.get("return_value")
    verified = return_value == "0" and bool(runs)
    message = None
    if return_value != "0":
        message = f"Bambu simulation returned {return_value!r}, not zero."
    elif not runs:
        message = "Bambu produced no verified simulation runs."
    return verified, len(runs), int(round(total_cycles)), message


def _find_outputs(output: Path) -> tuple[list[Path], Path | None, Path | None]:
    rtl_suffixes = {".v", ".sv", ".vhd", ".vhdl"}
    rtl_files = sorted(
        path for path in output.rglob("*") if path.is_file() and path.suffix.lower() in rtl_suffixes
    )
    reports = sorted(output.rglob("bambu_results.xml"))
    simulation_logs = sorted(output.rglob("bambu_time_simulation.txt"))
    return rtl_files, reports[0] if reports else None, simulation_logs[0] if simulation_logs else None


def _missing_rtl_authenticity_instances(
    rtl_files: Iterable[Path], required_instances: Iterable[tuple[str, str]]
) -> tuple[tuple[str, str], ...]:
    """Return required context-switch module instances absent from generated RTL."""

    required = tuple(required_instances)
    pending = set(required)
    for path in rtl_files:
        if not pending:
            break
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        observed = set()
        for module, instance in pending:
            pattern = (
                rf"\b{re.escape(module)}\b(?:\s*#\s*\(.*?\))?"
                rf"\s+\b{re.escape(instance)}\b\s*\("
            )
            if re.search(pattern, text, re.DOTALL):
                observed.add((module, instance))
        pending.difference_update(observed)
    return tuple(item for item in required if item in pending)


def _classify_nonzero(
    log_text: str,
    task_id: str,
    artifact_ids: tuple[str, ...],
    report_message: str | None,
) -> dict[str, Any]:
    lower = log_text.lower()
    if re.search(
        r"verilator.*(?:error|failed)|(?:%error|make(?:\[[0-9]+\])?: \*\*\*)|"
        r"simulator.*(?:build|compil).*(?:error|fail)",
        lower,
    ):
        return _failure(
            "compilation",
            "simulator-build-failed",
            "simulator-preparation",
            f"{task_id}: Verilator preparation failed.",
            _artifact_by_suffix(artifact_ids, "simulation-log"),
        )
    if re.search(r"simulation.*(?:error|failed)|testbench.*(?:error|failed)", lower):
        return _failure(
            "execution",
            "rtl-simulation-failed",
            "rtl-simulation",
            f"{task_id}: RTL simulation failed.",
            _artifact_by_suffix(artifact_ids, "simulation-log"),
        )
    if re.search(r"(?:generate|generation).*(?:rtl|hdl).*(?:error|failed)", lower):
        return _failure(
            "compilation",
            "rtl-generation-failed",
            "rtl-generation",
            f"{task_id}: RTL generation failed.",
            _artifact_by_suffix(artifact_ids, "bambu-log"),
        )
    if re.search(r"clang.*(?:error|failed)|frontend.*(?:error|failed)|unable to load plugin", lower):
        return _failure(
            "configuration",
            "frontend-resolution-failed",
            "hls-synthesis",
            f"{task_id}: the selected Clang frontend or PandA plugin failed.",
            _artifact_by_suffix(artifact_ids, "bambu-log"),
        )
    report_is_mismatch = bool(
        report_message and report_message.startswith("Bambu simulation returned")
    )
    if report_is_mismatch or re.search(
        r"mismatch|expected.+(?:got|actual)|comparison.+fail", lower
    ):
        return _failure(
            "verification",
            "result-mismatch",
            "result-verification",
            report_message
            if report_is_mismatch
            else f"{task_id}: simulated output did not match the established vectors.",
            _artifact_by_suffix(artifact_ids, "result-report"),
        )
    return _failure(
        "compilation",
        "hls-synthesis-failed",
        "hls-synthesis",
        f"{task_id}: Bambu exited unsuccessfully during HLS synthesis.",
        _artifact_by_suffix(artifact_ids, "bambu-log"),
    )


def _task_configuration(
    repository: Path,
    bambu: Path,
    spec: RegressionSpec,
    normalized_arguments: list[str],
    compiler: str,
    parallel_backend: int,
    selected_frontend: str | None,
) -> dict[str, Any]:
    try:
        executable = bambu.relative_to(repository).as_posix()
    except ValueError:
        executable = str(bambu)
    vector_path = spec.test_vector if spec.test_vector_kind in {"xml", "cxx"} else None
    return {
        "category": spec.category,
        "frontend": {"requested": compiler, "selected": selected_frontend},
        "input": {
            "example_id": spec.example_id,
            "source_path": spec.source_path,
            "test_vector_kind": spec.test_vector_kind,
            "test_vector_path": vector_path,
            "test_vector_value": spec.test_vector,
            "top_function": spec.top_function,
        },
        "invocation": {
            "arguments": normalized_arguments,
            "executable": executable,
            "working_directory": f".ci-regression-work/{spec.task_id}",
        },
        "options": {
            "bambu_parameters": list(spec.bambu_parameters),
            "clock_period": spec.clock_period,
            "compiler": compiler,
            "device": None,
            "experimental_setup": spec.experimental_setup,
            "expose_globals": spec.expose_globals,
            "inline_max_cost": None,
            "interface": spec.interface,
            "language_standard": spec.language_standard,
            "optimization": spec.optimization,
            "parallel_backend": parallel_backend,
            "simulate": True,
            "simulator": "VERILATOR",
            "test_vectors": spec.test_vector,
            "top_function": spec.top_function,
        },
    }


def _write_evidence(
    evidence: Path,
    log_text: str,
    output: Path,
    rtl_files: list[Path],
    report: Path | None,
    simulation_log: Path | None,
    report_message: str | None,
    required_authenticity_instances: tuple[tuple[str, str], ...],
    missing_authenticity_instances: tuple[tuple[str, str], ...],
    runtime_linkage_report: str | None = None,
) -> None:
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "bambu.log").write_text(log_text, encoding="utf-8", newline="\n")
    inventory = "".join(
        f"{path.relative_to(output).as_posix()}\t{path.stat().st_size}\n" for path in rtl_files
    )
    if required_authenticity_instances:
        observed = set(required_authenticity_instances) - set(missing_authenticity_instances)
        inventory += "".join(
            f"authenticity\t{module}\t{instance}\t"
            f"{'instantiated' if (module, instance) in observed else 'missing'}\n"
            for module, instance in required_authenticity_instances
        )
    (evidence / "rtl-files.txt").write_text(inventory, encoding="utf-8", newline="\n")
    if runtime_linkage_report is not None:
        (evidence / "runtime-linkage.txt").write_text(
            runtime_linkage_report, encoding="utf-8", newline="\n"
        )
    if report is not None:
        shutil.copy2(report, evidence / "bambu_results.xml")
    if simulation_log is not None:
        shutil.copy2(simulation_log, evidence / "simulation.log")
    else:
        (evidence / "simulation.log").write_text(
            (report_message or "No standalone Bambu simulation log was produced.") + "\n",
            encoding="utf-8",
            newline="\n",
        )


def run_regression(
    repository: Path,
    bambu: Path,
    results_directory: Path,
    evidence_directory: Path,
    spec: RegressionSpec,
    compiler: str,
    parallel_backend: int,
    timeout_seconds: int,
    real_verilator: str | None,
    wrapper_directory: Path,
) -> dict[str, Any]:
    """Run one Bambu-owned XML/inline-vector Verilator co-simulation."""

    task_start_wall = time.time()
    task_start_ns = time.monotonic_ns()
    input_start_ns = task_start_ns
    task_work = repository / ".ci-regression-work" / spec.task_id
    output = task_work / "output"
    evidence = evidence_directory / spec.task_id
    task_work.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    evidence.mkdir(parents=True, exist_ok=True)
    artifact_ids = _artifact_ids(spec.task_id)
    normalized_arguments = _normalized_arguments(spec, compiler, parallel_backend)
    durations: dict[str, float | int | None] = {
        stage_id: None for stage_id in REGRESSION_STAGE_IDS
    }
    missing: list[str] = []
    if not bambu.is_file() or not os.access(bambu, os.X_OK):
        missing.append(f"installed Bambu executable: {bambu}")
    source = repository / spec.source_path
    if not source.is_file():
        missing.append(f"source: {source}")
    if spec.test_vector_kind in {"xml", "cxx"} and not (repository / spec.test_vector).is_file():
        description = "XML test vectors" if spec.test_vector_kind == "xml" else "C++ testbench"
        missing.append(f"{description}: {repository / spec.test_vector}")
    if real_verilator is None:
        missing.append("Verilator executable on PATH")
    durations["input-validation"] = _seconds(time.monotonic_ns() - input_start_ns)

    log_text = ""
    returncode: int | None = None
    timed_out = False
    launch_error: OSError | None = None
    marker = task_work / "verilator-start-nanoseconds.txt"
    marker.unlink(missing_ok=True)
    process_start_ns: int | None = None
    process_end_ns: int | None = None
    if missing:
        log_text = "Input validation failed:\n" + "".join(f"- {item}\n" for item in missing)
    else:
        actual_arguments = _actual_arguments(repository, spec, normalized_arguments)
        environment = os.environ.copy()
        environment.update(
            {
                "PANDA_CI_REAL_VERILATOR": str(real_verilator),
                "PANDA_CI_VERILATOR_MARKER": str(marker),
                "PATH": f"{wrapper_directory}{os.pathsep}{environment.get('PATH', '')}",
            }
        )
        if spec.task_id == "regression-graphsage":
            # Retain the actual installed-run MDPI compile and link commands.
            environment["PANDA_CI_RUNTIME_LINKAGE_EVIDENCE"] = "1"
        process_start_ns = time.time_ns()
        try:
            process = subprocess.Popen(
                [str(bambu), *actual_arguments],
                cwd=task_work,
                env=environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                errors="replace",
                start_new_session=True,
            )
            try:
                log_text, _ = process.communicate(timeout=timeout_seconds)
            except subprocess.TimeoutExpired:
                timed_out = True
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    log_text, _ = process.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    log_text, _ = process.communicate()
            returncode = process.returncode
        except OSError as error:
            launch_error = error
            log_text = f"Unable to launch Bambu: {error}\n"
        process_end_ns = time.time_ns()
    sys.stdout.write(log_text)
    sys.stdout.flush()

    configuration = _task_configuration(
        repository,
        bambu,
        spec,
        normalized_arguments,
        compiler,
        parallel_backend,
        compiler if process_start_ns is not None else None,
    )

    rtl_files, report, simulation_log = _find_outputs(output)
    missing_authenticity_instances = _missing_rtl_authenticity_instances(
        rtl_files, spec.rtl_authenticity_instances
    )
    runtime_linkage_report: str | None = None
    runtime_linkage_errors: list[str] = []
    if spec.task_id == "regression-graphsage" and not missing and launch_error is None:
        runtime_linkage_report, runtime_linkage_errors = inspect_runtime_linkage(
            repository, bambu, output, log_text, compiler, spec.test_vector,
            (instance for instance in spec.rtl_authenticity_instances
             if instance not in missing_authenticity_instances),
        )
    verification_start_ns = time.monotonic_ns()
    verified, execution_count, total_cycles, report_message = _inspect_result_report(report)
    durations["result-verification"] = _seconds(time.monotonic_ns() - verification_start_ns)
    marker_ns: int | None = None
    try:
        marker_ns = int(marker.read_text(encoding="utf-8").strip())
    except (OSError, ValueError):
        pass
    if process_start_ns is not None and process_end_ns is not None:
        if marker_ns is not None and process_start_ns <= marker_ns <= process_end_ns:
            durations["hls-synthesis"] = _seconds(marker_ns - process_start_ns)
            durations["rtl-simulation"] = _seconds(process_end_ns - marker_ns)
        elif returncode not in (0, None):
            durations["hls-synthesis"] = _seconds(process_end_ns - process_start_ns)

    _write_evidence(
        evidence,
        log_text,
        output,
        rtl_files,
        report,
        simulation_log,
        report_message,
        spec.rtl_authenticity_instances,
        missing_authenticity_instances,
        runtime_linkage_report,
    )

    failure: dict[str, Any] | None = None
    task_state = "completed"
    task_outcome = "pass"
    task_exit_status: int | None = 0
    if missing:
        category = "infrastructure" if any("executable" in item or "PATH" in item for item in missing) else "configuration"
        if category == "infrastructure":
            task_state = "infrastructure_error"
            task_outcome = "unknown"
            task_exit_status = None
        else:
            task_outcome = "fail"
            task_exit_status = 2
        code = (
            "regression-infrastructure-failure"
            if category == "infrastructure"
            else "invalid-regression-input"
        )
        failure = _failure(
            category,
            code,
            "input-validation",
            "; ".join(missing),
            _artifact_by_suffix(artifact_ids, "bambu-log"),
        )
    elif launch_error is not None:
        task_state = "infrastructure_error"
        task_outcome = "unknown"
        task_exit_status = None
        failure = _failure(
            "infrastructure",
            "regression-infrastructure-failure",
            "hls-synthesis",
            f"Unable to launch Bambu: {launch_error}",
            _artifact_by_suffix(artifact_ids, "bambu-log"),
            retryable=True,
        )
    elif timed_out:
        task_state = "timed_out"
        task_outcome = "unknown"
        task_exit_status = 124
        failure = _failure(
            "timeout",
            "regression-timeout",
            "rtl-simulation" if marker_ns is not None else "hls-synthesis",
            f"{spec.task_id} exceeded its {timeout_seconds}-second timeout.",
            _artifact_by_suffix(artifact_ids, "simulation-log") if marker_ns is not None else _artifact_by_suffix(artifact_ids, "bambu-log"),
            retryable=True,
        )
    elif returncode != 0:
        task_outcome = "fail"
        task_exit_status = _normalized_exit_status(returncode)
        failure = _classify_nonzero(
            log_text, spec.task_id, artifact_ids, report_message if report is not None else None
        )
    elif marker_ns is None:
        task_state = "infrastructure_error"
        task_outcome = "unknown"
        task_exit_status = None
        failure = _failure(
            "infrastructure",
            "regression-infrastructure-failure",
            "simulator-preparation",
            (
                f"{spec.task_id}: Bambu exited successfully, but the runner did not "
                "observe the required Verilator invocation; synthesis/simulation "
                "timings cannot be reported reliably."
            ),
            _artifact_by_suffix(artifact_ids, "bambu-log"),
            retryable=True,
        )
    elif not rtl_files:
        task_outcome = "fail"
        task_exit_status = 1
        failure = _failure(
            "verification",
            "rtl-generation-failed",
            "rtl-generation",
            f"{spec.task_id}: Bambu exited successfully but produced no RTL files.",
            _artifact_by_suffix(artifact_ids, "rtl-output"),
        )
    elif missing_authenticity_instances:
        task_outcome = "fail"
        task_exit_status = 1
        failure = _failure(
            "verification",
            "rtl-generation-failed",
            "rtl-generation",
            (
                f"{spec.task_id}: generated RTL does not prove the requested "
                "OpenMP context-switch architecture; missing components: "
                + ", ".join(
                    f"{module} as {instance}"
                    for module, instance in missing_authenticity_instances
                )
                + "."
            ),
            _artifact_by_suffix(artifact_ids, "rtl-output"),
        )
    elif runtime_linkage_errors:
        task_outcome = "fail"
        task_exit_status = 1
        failure = _failure(
            "verification", "simulator-build-failed", "simulator-preparation",
            f"{spec.task_id}: runtime linkage evidence failed: "
            + "; ".join(runtime_linkage_errors),
            _artifact_by_suffix(artifact_ids, "runtime-linkage"),
        )
    elif report is None:
        task_outcome = "fail"
        task_exit_status = 1
        failure = _failure(
            "execution",
            "rtl-simulation-failed",
            "rtl-simulation",
            f"{spec.task_id}: Bambu exited successfully but produced no simulation report.",
            _artifact_by_suffix(artifact_ids, "simulation-log"),
        )
    elif not verified:
        task_outcome = "fail"
        task_exit_status = 1
        failure = _failure(
            "verification",
            "result-mismatch",
            "result-verification",
            report_message or f"{spec.task_id}: expected-output verification failed.",
            _artifact_by_suffix(artifact_ids, "result-report"),
        )

    task_end_wall = time.time()
    total = _seconds(time.monotonic_ns() - task_start_ns)
    stages = _stage_records(
        durations,
        artifact_ids,
        task_outcome,
        failure,
        task_exit_status,
        task_state if task_state != "completed" else None,
    )
    stages_by_id = {stage["stage_id"]: stage for stage in stages}
    simulation_passed = stages_by_id["rtl-simulation"]["outcome"] == "pass"
    task = {
        "artifacts": list(artifact_ids),
        "checks": _checks(stages),
        "configuration": configuration,
        "execution": {
            "completed_at": _timestamp(task_end_wall),
            "exit_status": task_exit_status,
            "started_at": _timestamp(task_start_wall),
            "state": task_state,
        },
        "failure": failure,
        "metrics": _metrics(durations, total),
        "outcome": task_outcome,
        "results": {
            "simulation": {
                "completed": simulation_passed,
                "execution_count": execution_count if simulation_passed else None,
                "total_cycles": total_cycles if simulation_passed else None,
                "verified": stages_by_id["result-verification"]["outcome"] == "pass",
            },
            "synthesis": {
                "completed": stages_by_id["rtl-generation"]["outcome"] == "pass",
                "rtl_artifact_count": len(rtl_files),
            },
        },
        "schema": "panda.ci.task-result",
        "schema_version": MULTI_TASK_SCHEMA_VERSION,
        "stages": stages,
        "task_id": spec.task_id,
        "task_type": "regression",
    }
    write_json(results_directory / "tasks" / f"{spec.task_id}.json", task)
    return task


def run_regression_suite(
    repository: Path,
    bambu: Path,
    results_directory: Path,
    evidence_directory: Path,
    compiler: str = "I386_CLANG16",
    parallel_backend: int = 2,
    timeout_seconds: int = 300,
) -> dict[str, Any]:
    """Run every selected test, persist every result, then return one suite record."""

    root = repository.resolve()
    results = _managed_output_directory(
        root,
        results_directory,
        "regression results",
        ".ci-regression-results",
    )
    evidence = _managed_output_directory(
        root,
        evidence_directory,
        "regression evidence",
        ".ci-regression-evidence",
    )
    work = _managed_output_directory(
        root,
        root / ".ci-regression-work",
        "regression work",
        ".ci-regression-work",
    )
    targets = (results, evidence, work)
    if len(set(targets)) != len(targets):
        raise ValueError("regression result, evidence, and work directories must be distinct")
    for target in targets:
        if target.exists():
            shutil.rmtree(target)
        target.mkdir(parents=True)
    suite_start_wall = time.time()
    suite_start_ns = time.monotonic_ns()
    real_verilator = shutil.which("verilator")
    wrapper_root = work / ".tools"
    wrapper_directory = wrapper_root / "bin"
    if real_verilator is not None:
        wrapper_directory = _write_verilator_wrapper(wrapper_root, real_verilator)
    tasks: list[dict[str, Any]] = []
    for spec in REGRESSION_SPECS:
        print(f"::group::Fast regression — {spec.task_id}")
        try:
            task = run_regression(
                root,
                bambu.resolve(),
                results,
                evidence,
                spec,
                compiler,
                parallel_backend,
                timeout_seconds,
                real_verilator,
                wrapper_directory,
            )
        except Exception as error:  # Preserve a result for unexpected runner failures.
            failure_timestamp = _timestamp()
            artifact_ids = _artifact_ids(spec.task_id)
            failure = _failure(
                "infrastructure",
                "regression-infrastructure-failure",
                "input-validation",
                f"Unexpected regression runner error: {error}",
                _artifact_by_suffix(artifact_ids, "bambu-log"),
                retryable=True,
            )
            evidence_path = evidence / spec.task_id
            evidence_path.mkdir(parents=True, exist_ok=True)
            (evidence_path / "bambu.log").write_text(
                f"Unexpected regression runner error: {error}\n", encoding="utf-8"
            )
            durations = {stage_id: None for stage_id in REGRESSION_STAGE_IDS}
            stages = _stage_records(
                durations, artifact_ids, "unknown", failure, None, "infrastructure_error"
            )
            normalized = _normalized_arguments(spec, compiler, parallel_backend)
            task = {
                "artifacts": list(artifact_ids),
                "checks": _checks(stages),
                "configuration": _task_configuration(
                    root,
                    bambu,
                    spec,
                    normalized,
                    compiler,
                    parallel_backend,
                    None,
                ),
                "execution": {
                    "completed_at": failure_timestamp,
                    "exit_status": None,
                    "started_at": failure_timestamp,
                    "state": "infrastructure_error",
                },
                "failure": failure,
                "metrics": _metrics(durations, None),
                "outcome": "unknown",
                "results": {
                    "simulation": {
                        "completed": False,
                        "execution_count": None,
                        "total_cycles": None,
                        "verified": False,
                    },
                    "synthesis": {"completed": False, "rtl_artifact_count": 0},
                },
                "schema": "panda.ci.task-result",
                "schema_version": MULTI_TASK_SCHEMA_VERSION,
                "stages": stages,
                "task_id": spec.task_id,
                "task_type": "regression",
            }
            write_json(results / "tasks" / f"{spec.task_id}.json", task)
        tasks.append(task)
        if task["outcome"] != "pass":
            print(f"::error::{spec.task_id} finished with outcome {task['outcome']}")
        print("::endgroup::")
    passed = sum(task["outcome"] == "pass" for task in tasks)
    failed = len(tasks) - passed
    suite = {
        "completed_at": _timestamp(),
        "duration_seconds": _seconds(time.monotonic_ns() - suite_start_ns),
        "exit_status": 0 if failed == 0 else 1,
        "failed_count": failed,
        "outcome": "pass" if failed == 0 else "fail",
        "passed_count": passed,
        "started_at": _timestamp(suite_start_wall),
        "task_count": len(tasks),
        "task_ids": [task["task_id"] for task in tasks],
    }
    write_json(results / "suite.json", suite)
    # The validator may run in a different container user.
    for path in results.rglob("*"):
        if path.is_file():
            path.chmod(path.stat().st_mode | 0o444)
    return suite


def _unexecuted_task(
    repository: Path,
    bambu: Path,
    spec: RegressionSpec,
    compiler: str,
    parallel_backend: int,
    *,
    infrastructure_failure: bool,
) -> dict[str, Any]:
    artifact_ids = _artifact_ids(spec.task_id)
    durations = {stage_id: None for stage_id in REGRESSION_STAGE_IDS}
    normalized = _normalized_arguments(spec, compiler, parallel_backend)
    configuration = _task_configuration(
        repository,
        bambu,
        spec,
        normalized,
        compiler,
        parallel_backend,
        None,
    )
    failure = None
    stages: list[dict[str, Any]]
    if infrastructure_failure:
        failure = _failure(
            "infrastructure",
            "regression-infrastructure-failure",
            "input-validation",
            f"{spec.task_id}: no result was produced after the open build passed.",
            _artifact_by_suffix(artifact_ids, "bambu-log"),
            retryable=True,
        )
        stages = _stage_records(
            durations,
            artifact_ids,
            "unknown",
            failure,
            None,
            "infrastructure_error",
        )
        execution_state = "infrastructure_error"
        outcome = "unknown"
    else:
        stage_artifacts = {
            "input-validation": [],
            "hls-synthesis": [_artifact_by_suffix(artifact_ids, "bambu-log")],
            "rtl-generation": [_artifact_by_suffix(artifact_ids, "rtl-output")],
            "simulator-preparation": (
                [_artifact_by_suffix(artifact_ids, "runtime-linkage")]
                if spec.task_id == RUNTIME_LINKAGE_TASK_ID else []
            ),
            "rtl-simulation": [_artifact_by_suffix(artifact_ids, "simulation-log")],
            "result-verification": [_artifact_by_suffix(artifact_ids, "result-report")],
        }
        stages = [
            {
                "artifact_ids": stage_artifacts[stage_id],
                "duration_seconds": None,
                "execution_state": "completed",
                "exit_status": None,
                "failure": None,
                "metric_ids": [f"duration.{stage_id}"],
                "outcome": "skipped",
                "stage_id": stage_id,
            }
            for stage_id in REGRESSION_STAGE_IDS
        ]
        execution_state = "completed"
        outcome = "skipped"
    return {
        "artifacts": list(artifact_ids),
        "checks": _checks(stages),
        "configuration": configuration,
        "execution": {
            "completed_at": None,
            "exit_status": None,
            "started_at": None,
            "state": execution_state,
        },
        "failure": failure,
        "metrics": _metrics(durations, None),
        "outcome": outcome,
        "results": {
            "simulation": {
                "completed": False,
                "execution_count": None,
                "total_cycles": None,
                "verified": False,
            },
            "synthesis": {"completed": False, "rtl_artifact_count": 0},
        },
        "schema": "panda.ci.task-result",
        "schema_version": MULTI_TASK_SCHEMA_VERSION,
        "stages": stages,
        "task_id": spec.task_id,
        "task_type": "regression",
    }


def _environment_value(environment: Mapping[str, str], name: str) -> str:
    return environment.get(f"PANDA_CI_{name}", "").strip()


def _environment_int(environment: Mapping[str, str], name: str) -> int | None:
    value = _environment_value(environment, name)
    try:
        parsed = int(value)
    except ValueError:
        return None
    return parsed if parsed >= 0 else None


def _environment_number(
    environment: Mapping[str, str], name: str
) -> float | int | None:
    value = _environment_value(environment, name)
    try:
        parsed = float(value)
    except ValueError:
        return None
    if parsed < 0 or parsed != parsed or parsed in {float("inf"), float("-inf")}:
        return None
    return int(parsed) if parsed.is_integer() else parsed


def _environment_bool(environment: Mapping[str, str], name: str) -> bool | None:
    value = _environment_value(environment, name).lower()
    if value in {"true", "yes", "1"}:
        return True
    if value in {"false", "no", "0"}:
        return False
    return None


def _epoch_timestamp(epoch: int | None) -> str | None:
    return _timestamp(epoch) if epoch is not None else None


def _action_outcome(environment: Mapping[str, str]) -> str:
    value = _environment_value(environment, "REGRESSION_ACTION_OUTCOME").lower()
    if value == "canceled":
        value = "cancelled"
    return value if value in {"success", "failure", "cancelled", "skipped"} else "unknown"


def _regression_action_state(
    environment: Mapping[str, str], build_passed: bool
) -> str:
    if not build_passed:
        return "skipped"
    outcome = _action_outcome(environment)
    if _environment_bool(environment, "JOB_CANCELLED") is True:
        return "canceled"
    start = _environment_int(environment, "REGRESSION_ACTION_START_EPOCH")
    completion = _environment_int(
        environment, "REGRESSION_ACTION_COMPLETION_EPOCH"
    ) or _environment_int(environment, "COMPLETION_EPOCH")
    timeout = _environment_int(environment, "REGRESSION_TIMEOUT_SECONDS")
    if (
        outcome in {"failure", "cancelled"}
        and start is not None
        and completion is not None
        and timeout is not None
        and completion - start >= timeout
    ):
        return "timed_out"
    if outcome == "cancelled":
        return "canceled"
    if outcome == "skipped":
        return "skipped"
    return "pending"


def _raw_suite(raw_results: Path) -> dict[str, Any] | None:
    path = raw_results / "suite.json"
    if not path.is_file() or path.is_symlink():
        return None
    try:
        value = load_json(path)
        require_canonical(path, value)
    except (OSError, SerializationError):
        return None
    return value if isinstance(value, dict) else None


def _hosted_regression_suite(
    raw_results: Path,
    environment: Mapping[str, str],
    build_passed: bool,
    regression_tasks: list[dict[str, Any]],
) -> dict[str, Any]:
    """Normalize raw runner and GitHub action telemetry without hiding contradictions."""

    action_outcome = _action_outcome(environment)
    if not build_passed and action_outcome == "unknown":
        action_outcome = "skipped"
    action_exit_status = _environment_int(
        environment, "REGRESSION_ACTION_EXIT_STATUS"
    )
    action_state = _regression_action_state(environment, build_passed)
    task_ids = [task["task_id"] for task in regression_tasks]
    outcomes = [task["outcome"] for task in regression_tasks]
    task_count = len(regression_tasks)
    passed_count = sum(outcome == "pass" for outcome in outcomes)
    failed_count = task_count - passed_count
    expected_runner_outcome = "pass" if failed_count == 0 else "fail"
    expected_runner_exit = 0 if failed_count == 0 else 1
    raw_task_documents_complete = all(
        (raw_results / "tasks" / f"{task_id}.json").is_file()
        and not (raw_results / "tasks" / f"{task_id}.json").is_symlink()
        for task_id in task_ids
    )

    raw = _raw_suite(raw_results)
    raw_internal = bool(
        raw is not None
        and raw_task_documents_complete
        and raw.get("task_ids") == task_ids
        and raw.get("task_count") == task_count
        and raw.get("passed_count") == passed_count
        and raw.get("failed_count") == failed_count
        and raw.get("outcome") == expected_runner_outcome
        and raw.get("exit_status") == expected_runner_exit
        and isinstance(raw.get("duration_seconds"), (int, float))
        and not isinstance(raw.get("duration_seconds"), bool)
        and raw.get("duration_seconds") >= 0
        and isinstance(raw.get("started_at"), str)
        and bool(raw.get("started_at"))
        and isinstance(raw.get("completed_at"), str)
        and bool(raw.get("completed_at"))
    )

    exported_duration = _environment_number(environment, "REGRESSION_SECONDS")
    exported_counts = (
        _environment_int(environment, "REGRESSION_TASK_COUNT"),
        _environment_int(environment, "REGRESSION_PASSED_COUNT"),
        _environment_int(environment, "REGRESSION_FAILED_COUNT"),
    )
    exported_outcome = _environment_value(environment, "REGRESSION_OUTCOME").lower()
    output_consistent = bool(
        raw_internal
        and exported_outcome == raw["outcome"]
        and exported_counts == (task_count, passed_count, failed_count)
        and exported_duration is not None
        and abs(float(exported_duration) - float(raw["duration_seconds"])) < 0.001
    )
    action_consistent = bool(
        (
            (
                expected_runner_outcome == "pass"
                and action_outcome == "success"
                and action_exit_status == 0
            )
            or (
                expected_runner_outcome == "fail"
                and action_outcome == "failure"
                and action_exit_status is not None
                and action_exit_status != 0
            )
        )
    )

    if action_state == "skipped":
        execution_state = "completed"
        suite_outcome = "skipped"
    elif action_state in {"canceled", "timed_out"}:
        execution_state = action_state
        suite_outcome = "unknown"
    elif raw_internal and output_consistent and action_consistent:
        execution_state = "completed"
        suite_outcome = expected_runner_outcome
    else:
        execution_state = "infrastructure_error"
        suite_outcome = "unknown"

    start_epoch = _environment_int(environment, "REGRESSION_ACTION_START_EPOCH")
    completion_epoch = _environment_int(
        environment, "REGRESSION_ACTION_COMPLETION_EPOCH"
    ) or _environment_int(environment, "COMPLETION_EPOCH")
    if raw_internal:
        started_at = raw["started_at"]
        completed_at = raw["completed_at"]
        duration_seconds = raw["duration_seconds"]
        duration_method = "suite-monotonic-clock"
    else:
        if start_epoch is not None and completion_epoch is not None:
            started_at = _epoch_timestamp(start_epoch)
            completed_at = _epoch_timestamp(completion_epoch)
            duration_seconds = max(0, completion_epoch - start_epoch)
            duration_method = "action-wall-clock"
        elif exported_duration is not None:
            started_at = None
            completed_at = None
            duration_seconds = exported_duration
            duration_method = "runner-wall-clock"
        else:
            started_at = None
            completed_at = None
            duration_seconds = None
            duration_method = "unavailable"

    failure_stage = _environment_value(environment, "REGRESSION_FAILURE_STAGE")
    if failure_stage.lower() in {"", "none", "not-run", "running"}:
        failure_stage = None
    if execution_state in {"canceled", "timed_out", "infrastructure_error"} and failure_stage is None:
        failure_stage = "fast-regressions"

    exit_status = action_exit_status
    if execution_state == "infrastructure_error" and exit_status == 0:
        exit_status = None
    if exit_status is None and raw_internal and execution_state == "completed":
        exit_status = raw["exit_status"]
    if suite_outcome == "skipped":
        exit_status = None

    return {
        "action_exit_status": action_exit_status,
        "action_outcome": action_outcome,
        "completed_at": completed_at,
        "container_setup_seconds": _environment_number(
            environment, "REGRESSION_CONTAINER_SETUP_SECONDS"
        ),
        "duration_measurement_method": duration_method,
        "duration_seconds": duration_seconds,
        "execution_state": execution_state,
        "exit_status": exit_status,
        "failed_count": failed_count,
        "failure_stage": failure_stage,
        "outcome": suite_outcome,
        "passed_count": passed_count,
        "started_at": started_at,
        "task_count": task_count,
    }


def _repository_relative_directory(
    repository: Path, value: str, default: str
) -> tuple[Path, str]:
    supplied = Path(value or default)
    absolute = supplied if supplied.is_absolute() else repository / supplied
    absolute = absolute.resolve()
    try:
        relative = absolute.relative_to(repository).as_posix()
    except ValueError as error:
        raise ValueError(f"CI evidence directory must be inside the repository: {absolute}") from error
    pure = PurePosixPath(relative)
    if not relative or ".." in pure.parts:
        raise ValueError(f"unsafe CI evidence directory: {relative!r}")
    return absolute, relative


def _managed_output_directory(
    repository: Path,
    path: Path,
    label: str,
    expected_name: str,
) -> Path:
    absolute = path if path.is_absolute() else repository / path
    if absolute.is_symlink():
        raise ValueError(f"refusing symlinked {label} directory: {absolute}")
    absolute = absolute.resolve()
    try:
        relative = absolute.relative_to(repository)
    except ValueError as error:
        raise ValueError(f"{label} directory must be inside the repository: {absolute}") from error
    if not relative.parts:
        raise ValueError(f"refusing repository root as {label} directory")
    if len(relative.parts) != 1 or relative.name != expected_name:
        raise ValueError(
            f"{label} directory must be the dedicated repository path "
            f"{expected_name!r}, got {relative.as_posix()!r}"
        )
    if absolute.exists() and not absolute.is_dir():
        raise ValueError(f"{label} output exists and is not a directory: {absolute}")
    return absolute


def _regression_artifacts(
    repository: Path,
    evidence_directory: Path,
    evidence_relative: str,
    environment: Mapping[str, str],
) -> list[dict[str, Any]]:
    artifact_name = _environment_value(environment, "REGRESSION_ARTIFACT_NAME") or (
        f"fast-regression-evidence-{_environment_value(environment, 'WORKFLOW_RUN_ID')}-"
        f"attempt-{_environment_value(environment, 'RUN_ATTEMPT')}"
    )
    details = {
        "bambu-log": ("bambu.log", "execution-log", "text/plain", "hls-synthesis"),
        "result-report": (
            "bambu_results.xml",
            "co-simulation-result",
            "application/xml",
            "result-verification",
        ),
        "rtl-output": ("rtl-files.txt", "rtl-inventory", "text/plain", "rtl-generation"),
        "simulation-log": (
            "simulation.log",
            "simulation-log",
            "text/plain",
            "rtl-simulation",
        ),
        "runtime-linkage": (
            "runtime-linkage.txt",
            "runtime-linkage-report",
            "text/plain",
            "simulator-preparation",
        ),
    }
    artifacts: list[dict[str, Any]] = []
    for spec in REGRESSION_SPECS:
        suffixes = REGRESSION_ARTIFACT_SUFFIXES + (
            (RUNTIME_LINKAGE_ARTIFACT_SUFFIX,)
            if spec.task_id == RUNTIME_LINKAGE_TASK_ID else ()
        )
        for suffix in suffixes:
            filename, role, media_type, stage_id = details[suffix]
            path = evidence_directory / spec.task_id / filename
            available = path.is_file() and not path.is_symlink()
            artifacts.append(
                {
                    "artifact_id": f"{spec.task_id}.{suffix}",
                    "associated_stage": stage_id,
                    "available": available,
                    "github_artifact_name": artifact_name if available else None,
                    "media_type": media_type,
                    "path": f"{evidence_relative}/{spec.task_id}/{filename}",
                    "producer_task": spec.task_id,
                    "retention_days": 7 if available else None,
                    "role": role,
                    "sha256": sha256_file(path) if available else None,
                    "size_bytes": path.stat().st_size if available else None,
                }
            )
    return sorted(artifacts, key=lambda item: item["artifact_id"])


def _load_regression_tasks(
    bundle: Path,
    raw_results: Path,
    repository: Path,
    compiler: str,
    parallel_backend: int,
    missing_as_infrastructure: bool,
) -> list[dict[str, Any]]:
    tasks: list[dict[str, Any]] = []
    for spec in REGRESSION_SPECS:
        raw_path = raw_results / "tasks" / f"{spec.task_id}.json"
        if raw_path.is_file():
            task = load_json(raw_path)
            require_canonical(raw_path, task)
            if not isinstance(task, dict):
                raise ValueError(f"{raw_path}: regression result must be an object")
            expected = (
                task.get("schema") == "panda.ci.task-result"
                and task.get("schema_version") == MULTI_TASK_SCHEMA_VERSION
                and task.get("task_id") == spec.task_id
                and task.get("task_type") == "regression"
            )
            if not expected:
                raise ValueError(f"{raw_path}: regression result identity is inconsistent")
        else:
            task = _unexecuted_task(
                repository,
                repository / "panda_dist" / "bin" / "bambu",
                spec,
                compiler,
                parallel_backend,
                infrastructure_failure=missing_as_infrastructure,
            )
        write_json(bundle / "tasks" / f"{spec.task_id}.json", task)
        tasks.append(task)
    return tasks


def _extend_verdict(
    verdict: dict[str, Any],
    regression_tasks: list[dict[str, Any]],
    suite: dict[str, Any],
) -> None:
    outcomes = [task["outcome"] for task in regression_tasks]
    suite_passed = (
        suite["execution_state"] == "completed"
        and suite["outcome"] == "pass"
        and suite["action_outcome"] == "success"
        and suite["exit_status"] == 0
        and suite["task_count"] == len(regression_tasks)
        and suite["passed_count"] == len(regression_tasks)
        and suite["failed_count"] == 0
    )
    if outcomes and all(outcome == "pass" for outcome in outcomes) and suite_passed:
        hosted_outcome = "pass"
        reason = "Every requested GitHub-hosted fast regression passed."
    elif "fail" in outcomes:
        hosted_outcome = "fail"
        reason = "At least one requested GitHub-hosted fast regression failed."
    else:
        hosted_outcome = "neutral"
        reason = "At least one fast regression was skipped or ended without a conclusive result."
    verdict["rules"].append(
        {
            "evidence": [
                *(
                    f"tasks/{task['task_id']}.json#outcome"
                    for task in sorted(regression_tasks, key=lambda item: item["task_id"])
                ),
                "manifest.json#hosted-regression-suite",
            ],
            "outcome": hosted_outcome,
            "reason": reason,
            "rule_id": "hosted-fast-regressions-success",
            "severity": "blocking",
        }
    )
    blocking = [
        rule["outcome"] for rule in verdict["rules"] if rule["severity"] == "blocking"
    ]
    overall = (
        "fail"
        if "fail" in blocking
        else "neutral"
        if any(outcome != "pass" for outcome in blocking)
        else "pass"
    )
    verdict["overall_outcome"] = overall
    verdict["merge_recommendation"] = {
        "pass": "merge",
        "fail": "do-not-merge",
        "neutral": "manual-review",
    }[overall]
    verdict["schema_version"] = MULTI_TASK_SCHEMA_VERSION


def extend_bundle_with_regressions(
    bundle_directory: Path,
    raw_results_directory: Path,
    environment: Mapping[str, str],
    repository: Path,
) -> None:
    """Upgrade one valid 1.0 build bundle into the additive 1.1 task profile."""

    bundle = bundle_directory.resolve()
    root = repository.resolve()
    raw_results = raw_results_directory.resolve()
    request = load_json(bundle / "request.json")
    open_build = load_json(bundle / "tasks" / "open-build.json")
    artifact_document = load_json(bundle / "artifacts.json")
    verdict = load_json(bundle / "verdict.json")
    manifest = load_json(bundle / "manifest.json")
    compiler = _environment_value(environment, "REQUESTED_FRONTEND") or "I386_CLANG16"
    try:
        parallel_backend = max(1, int(_environment_value(environment, "PARALLELISM") or "2"))
    except ValueError:
        parallel_backend = 2
    evidence_directory, evidence_relative = _repository_relative_directory(
        root,
        _environment_value(environment, "REGRESSION_EVIDENCE_DIR"),
        ".ci-regression-evidence",
    )
    build_passed = open_build.get("outcome") == "pass"
    action_state = _regression_action_state(environment, build_passed)
    regression_tasks = _load_regression_tasks(
        bundle,
        raw_results,
        root,
        compiler,
        parallel_backend,
        missing_as_infrastructure=build_passed
        and action_state not in {"skipped", "canceled", "timed_out"},
    )
    suite = _hosted_regression_suite(
        raw_results, environment, build_passed, regression_tasks
    )

    open_build["schema_version"] = MULTI_TASK_SCHEMA_VERSION
    request["schema_version"] = MULTI_TASK_SCHEMA_VERSION
    regression_request_tasks = []
    for task in regression_tasks:
        configuration = copy.deepcopy(task["configuration"])
        configuration["frontend"] = {
            "requested": configuration["frontend"]["requested"]
        }
        regression_request_tasks.append(
            {
                "configuration": configuration,
                "task_id": task["task_id"],
                "task_type": "regression",
            }
        )
    request["tasks"] = sorted(
        [
            {"task_id": "open-build", "task_type": "build"},
            *regression_request_tasks,
        ],
        key=lambda item: item["task_id"],
    )
    request["requested_task_ids"] = [item["task_id"] for item in request["tasks"]]
    regression_artifacts = _regression_artifacts(
        root, evidence_directory, evidence_relative, environment
    )
    artifact_document["schema_version"] = MULTI_TASK_SCHEMA_VERSION
    artifact_document["artifacts"] = [
        *artifact_document["artifacts"],
        *regression_artifacts,
    ]
    request["requested_artifact_ids"] = [
        *(artifact["artifact_id"] for artifact in artifact_document["artifacts"]),
    ]
    _extend_verdict(verdict, regression_tasks, suite)

    write_json(bundle / "request.json", request)
    write_json(bundle / "tasks" / "open-build.json", open_build)
    write_json(bundle / "artifacts.json", artifact_document)
    write_json(bundle / "verdict.json", verdict)

    schema_by_path = {
        "request.json": ("request", "panda.ci.request"),
        "tasks/open-build.json": ("open-build", "panda.ci.task-result"),
        "artifacts.json": ("artifacts", "panda.ci.artifact-index"),
        "verdict.json": ("verdict", "panda.ci.verdict"),
        **{
            f"tasks/{task_id}.json": (task_id, "panda.ci.task-result")
            for task_id in REGRESSION_TASK_IDS
        },
    }
    manifest["documents"] = sorted(
        (
            {
                "document_id": document_id,
                "path": relative_path,
                "schema": marker,
                "schema_version": MULTI_TASK_SCHEMA_VERSION,
                "sha256": sha256_file(bundle / relative_path),
            }
            for relative_path, (document_id, marker) in schema_by_path.items()
        ),
        key=lambda item: item["document_id"],
    )
    manifest["hosted_regression_suite"] = suite
    states = [
        manifest.get("execution_state"),
        suite["execution_state"],
        *(task["execution"]["state"] for task in regression_tasks),
    ]
    if "timed_out" in states:
        manifest["execution_state"] = "timed_out"
    elif "canceled" in states:
        manifest["execution_state"] = "canceled"
    elif "infrastructure_error" in states:
        manifest["execution_state"] = "infrastructure_error"
    else:
        manifest["execution_state"] = "completed"
    manifest["schema_version"] = MULTI_TASK_SCHEMA_VERSION
    write_json(bundle / "manifest.json", manifest)
