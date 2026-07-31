"""Deterministic bundled OpenMP and MDPI linkage evidence."""

from __future__ import annotations

import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path
from typing import Iterable


OPENMP_SOURCE = "libopenmp/kmp_single_file.cpp"
OPENMP_INCLUDED_SOURCES = (
    "kmp_barrier.cpp",
    "kmp_csupport.cpp",
    "kmp_runtime.cpp",
    "kmp_sched.cpp",
    "omp.cpp",
)
MDPI_SOURCES = ("mdpi_driver.cpp", "mdpi.c", "mdpi_wrapper.cpp")
MDPI_ARTIFACTS = ("libmdpi_driver.so", "libmdpi.so", "testbench")
PROHIBITED_LIBRARIES = ("libgomp.so", "libomp.so", "libiomp5.so")
RUNTIME_SYMBOL = re.compile(r"^(?:GOMP_|__kmpc_|omp_)")


def _normalize(value: str, repository: Path, output: Path) -> str:
    normalized = value.replace("\\", "/")
    replacements = (
        (str(output.resolve()).replace("\\", "/"), "<output>"),
        (str(repository.resolve()).replace("\\", "/"), "<repo>"),
    )
    for prefix, marker in replacements:
        normalized = normalized.replace(prefix, marker)
    normalized = re.sub(
        r"(?:/[^\s'\"]+)+/share/panda/(lib(?:openmp|mdpi)/)",
        r"<panda-share>/\1",
        normalized,
    )
    return normalized


def _command_inventory(
    log_text: str, repository: Path, output: Path, additional_terms: Iterable[str] = ()
) -> list[str]:
    """Return normalized compiler/linker commands relevant to MDPI."""

    selected: set[str] = set()
    needles = (*MDPI_SOURCES, *MDPI_ARTIFACTS, *additional_terms)
    for raw in log_text.splitlines():
        line = raw.strip()
        if not line or not any(needle in line for needle in needles):
            continue
        try:
            tokens = shlex.split(line)
        except ValueError as error:
            raise ValueError(f"malformed MDPI command inventory: {error}") from error
        if len(tokens) < 2 or not any(
            token == "-c" or token == "-shared" or token.startswith("-o") for token in tokens
        ):
            continue
        selected.add(_normalize(line, repository, output))
    return sorted(selected)


def _parse_readelf_dynamic(text: str) -> tuple[list[str], list[str]]:
    if not text.strip():
        raise ValueError("malformed readelf dynamic-section output")
    if "There is no dynamic section" in text:
        return [], []
    if "Dynamic section" not in text:
        raise ValueError("malformed readelf dynamic-section output")
    needed = sorted(set(re.findall(r"\(NEEDED\).*?\[([^]]+)\]", text)))
    return needed, [name for name in needed if name.startswith(PROHIBITED_LIBRARIES)]


def _parse_readelf_symbols(text: str) -> tuple[list[str], list[str]]:
    if not text.strip() or "Symbol table" not in text:
        raise ValueError("malformed readelf symbol-table output")
    defined: set[str] = set()
    unresolved: set[str] = set()
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 8 or not fields[0].rstrip(":").isdigit():
            continue
        name = fields[7].split("@", 1)[0]
        if not RUNTIME_SYMBOL.match(name):
            continue
        (unresolved if fields[6] == "UND" else defined).add(name)
    return sorted(defined), sorted(unresolved)


def _parse_nm_dynamic(text: str) -> tuple[list[str], list[str]]:
    defined: set[str] = set()
    unresolved: set[str] = set()
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        symbol_type, name = fields[-2], fields[-1].split("@", 1)[0]
        if not RUNTIME_SYMBOL.match(name):
            continue
        (unresolved if symbol_type.upper() == "U" else defined).add(name)
    return sorted(defined), sorted(unresolved)


def _run_tool(command: list[str]) -> str:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise ValueError(f"{' '.join(command[:2])} failed: {result.stderr.strip()}")
    return result.stdout


def _elf_artifacts(output: Path) -> list[Path]:
    artifacts: list[Path] = []
    for path in output.rglob("*"):
        if (
            not path.is_file()
            or path.is_symlink()
            or not (path.suffix == ".so" or os.access(path, os.X_OK))
        ):
            continue
        try:
            with path.open("rb") as stream:
                if stream.read(4) == b"\x7fELF":
                    artifacts.append(path)
        except OSError:
            continue
    return sorted(artifacts)


def _source_observations(output: Path, log_text: str) -> set[str]:
    observed = {source for source in (OPENMP_SOURCE, *MDPI_SOURCES) if source in log_text}
    textual_suffixes = {".d", ".ll", ".log", ".mk", ".txt", ".xml"}
    for path in output.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in textual_suffixes:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        observed.update(
            source for source in (OPENMP_SOURCE, *MDPI_SOURCES) if source in text
        )
    return observed


def inspect_runtime_linkage(
    repository: Path,
    bambu: Path,
    output: Path,
    log_text: str,
    selected_frontend: str,
    testbench_source: str,
    rtl_instances: Iterable[tuple[str, str]],
) -> tuple[str, list[str]]:
    """Build the normalized report and return deterministic verification errors."""

    errors: list[str] = []
    tool_paths = {tool: shutil.which(tool) for tool in ("readelf", "nm")}
    for tool, path in tool_paths.items():
        if path is None:
            errors.append(f"required inspection tool unavailable: {tool}")

    observed_sources = _source_observations(output, log_text)
    bundled_openmp = bambu.resolve().parent.parent / "share/panda" / OPENMP_SOURCE
    included_openmp_sources: set[str] = set()
    if not bundled_openmp.is_file():
        errors.append(f"missing installed bundled OpenMP source: {OPENMP_SOURCE}")
    else:
        observed_sources.add(OPENMP_SOURCE)
        bundled_text = bundled_openmp.read_text(encoding="utf-8", errors="replace")
        included_openmp_sources = {
            source for source in OPENMP_INCLUDED_SOURCES
            if f"#include \"{source}\"" in bundled_text
        }
        for source in OPENMP_INCLUDED_SOURCES:
            if source not in included_openmp_sources:
                errors.append(f"missing bundled OpenMP included source: {source}")
    for source in MDPI_SOURCES:
        expected = output.rglob(source)
        if source not in observed_sources and not next(expected, None):
            errors.append(f"missing bundled/generated MDPI source: {source}")

    artifacts_by_name: dict[str, list[Path]] = {
        name: sorted(output.rglob(name)) for name in MDPI_ARTIFACTS
    }
    for name, paths in artifacts_by_name.items():
        if not any(path.is_file() for path in paths):
            errors.append(f"missing MDPI artifact: {name}")

    try:
        commands = _command_inventory(log_text, repository, output, (testbench_source,))
    except ValueError as error:
        commands = []
        errors.append(str(error))
    required_command_terms = (*MDPI_SOURCES, "libmdpi_driver.so", "libmdpi.so", testbench_source)
    for term in required_command_terms:
        if not any(term in command for command in commands):
            errors.append(f"missing normalized MDPI compile/link command for: {term}")

    dependencies: list[str] = []
    defined: set[str] = set()
    unresolved: set[str] = set()
    prohibited: set[str] = set()
    inspected: list[str] = []
    if all(tool_paths.values()):
        for artifact in _elf_artifacts(output):
            relative = _normalize(str(artifact), repository, output)
            try:
                dynamic = _run_tool([tool_paths["readelf"] or "readelf", "-d", str(artifact)])
                symbols = _run_tool([tool_paths["readelf"] or "readelf", "-Ws", str(artifact)])
                nm_symbols = _run_tool([tool_paths["nm"] or "nm", "-D", str(artifact)])
                needed, bad = _parse_readelf_dynamic(dynamic)
                readelf_defined, readelf_unresolved = _parse_readelf_symbols(symbols)
                nm_defined, nm_unresolved = _parse_nm_dynamic(nm_symbols)
            except ValueError as error:
                errors.append(f"{relative}: {error}")
                continue
            inspected.append(relative)
            dependencies.extend(
                f"{relative}\t{_normalize(name, repository, output)}" for name in needed
            )
            prohibited.update(bad)
            defined.update(readelf_defined)
            defined.update(nm_defined)
            unresolved.update(readelf_unresolved)
            unresolved.update(nm_unresolved)
    if not inspected:
        errors.append("no ELF host artifacts were inspected")
    if prohibited:
        errors.append("prohibited OpenMP dynamic dependencies: " + ", ".join(sorted(prohibited)))
    if unresolved:
        errors.append("unresolved OpenMP runtime symbols: " + ", ".join(sorted(unresolved)))

    lines = [
        "runtime-linkage-report-v1",
        f"selected-frontend\t{selected_frontend}",
        "tool\treadelf\t" + ("available" if tool_paths["readelf"] else "missing"),
        "tool\tnm\t" + ("available" if tool_paths["nm"] else "missing"),
    ]
    lines.extend(f"openmp-source\t{source}" for source in sorted(observed_sources) if "openmp" in source)
    lines.extend(
        f"openmp-included-source\t{source}" for source in sorted(included_openmp_sources)
    )
    lines.extend(f"mdpi-source\t{source}" for source in MDPI_SOURCES if source in observed_sources)
    lines.append(f"testbench-source\t{testbench_source}")
    lines.extend(
        f"mdpi-artifact\t{_normalize(str(path), repository, output)}"
        for name in MDPI_ARTIFACTS
        for path in artifacts_by_name[name]
        if path.is_file()
    )
    lines.extend(f"command\t{command}" for command in commands)
    lines.extend(f"elf\t{path}" for path in inspected)
    lines.extend(f"needed\t{item}" for item in sorted(dependencies))
    lines.extend(f"defined-openmp\t{name}" for name in sorted(defined))
    lines.extend(f"unresolved-openmp\t{name}" for name in sorted(unresolved))
    lines.extend(f"prohibited-runtime\t{name}" for name in sorted(prohibited))
    lines.extend(f"sparta-instance\t{module}\t{instance}" for module, instance in rtl_instances)
    lines.extend(f"verification-error\t{error}" for error in errors)
    lines.append(f"verification\t{'pass' if not errors else 'fail'}")
    return "\n".join(lines) + "\n", errors
