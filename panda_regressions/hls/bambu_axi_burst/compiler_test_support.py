"""Small shared helpers for offline compiler regression runners."""

# Author: Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import signal
import shlex
import subprocess
import threading
from typing import Sequence


class CompilerTestError(RuntimeError):
    """A compiler regression could not satisfy its execution contract."""


def validate_install(bambu: Path, bambu_hls: Path, plugin: Path) -> None:
    """Require the plugin to belong to the install containing the selected Bambu."""
    executable = bambu.resolve(strict=True)
    prefix = executable.parent.parent
    selected = bambu_hls.resolve(strict=True)
    expected_plugin = selected / "lib/panda/clang-16/ASTAnalyzer.so"
    if selected != prefix.resolve(strict=True):
        raise CompilerTestError(
            f"settings selected BAMBU_HLS={selected}, but Bambu belongs to {prefix}")
    if plugin.resolve(strict=True) != expected_plugin.resolve(strict=True) or not expected_plugin.is_file():
        raise CompilerTestError(f"ASTAnalyzer plugin does not match selected install: {expected_plugin}")


def create_output_dir(path: Path) -> Path:
    """Create a fresh output directory, refusing existing paths and symlinks."""
    if path.exists() or path.is_symlink():
        raise CompilerTestError(f"output directory must not already exist: {path}")
    result = path.resolve()
    result.mkdir(parents=True)
    return result


def format_argv(argv: Sequence[str]) -> str:
    """Return a shell-display representation; callers must execute argv directly."""
    return shlex.join([str(arg) for arg in argv])


@dataclass(frozen=True)
class CompilerResult:
    argv: tuple[str, ...]
    returncode: int | None
    stdout: str
    stderr: str
    timed_out: bool = False


def run_compiler(argv: Sequence[str], *, cwd: Path, timeout: float,
                 expected_failure: str | None = None,
                 sandbox_markers: Sequence[str] = (),
                 diagnostic_output_dir: Path | None = None,
                 allow_success: bool = False) -> CompilerResult:
    """Run one compiler command and enforce normal/expected-failure semantics."""
    if allow_success and expected_failure is None:
        raise CompilerTestError("allow_success requires expected_failure")
    command = tuple(str(arg) for arg in argv)
    process = None
    received_signal = None
    received_frame = None
    previous_handlers = {}

    def handle_signal(signum, frame):
        nonlocal received_signal, received_frame
        if received_signal is None:
            received_signal = signum
            received_frame = frame
        if process is not None:
            _terminate_process_group(process)

    if threading.current_thread() is threading.main_thread():
        for signum in (signal.SIGINT, signal.SIGTERM):
            previous = signal.getsignal(signum)
            if previous != signal.SIG_IGN:
                previous_handlers[signum] = previous
                signal.signal(signum, handle_signal)
    try:
        process = subprocess.Popen(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, start_new_session=True)
        if received_signal is not None:
            _terminate_process_group(process)
        stdout, stderr = process.communicate(timeout=timeout)
        result = CompilerResult(command, process.returncode, stdout, stderr)
    except subprocess.TimeoutExpired as exc:
        if process is not None:
            _terminate_process_group(process)
            stdout, stderr = process.communicate()
            _persist_result(diagnostic_output_dir, stdout or "", stderr or "", process.returncode)
        else:
            stdout = _as_text(exc.stdout)
            stderr = _as_text(exc.stderr)
        if received_signal is not None:
            _restore_and_forward_signal(received_signal, received_frame, previous_handlers)
        raise CompilerTestError(
            f"compiler timed out after {timeout}s; stdout={stdout!r}; stderr={stderr!r}") from exc
    except OSError as exc:
        raise CompilerTestError(f"could not execute compiler: {exc}") from exc
    finally:
        for signum, previous in previous_handlers.items():
            signal.signal(signum, previous)

    _persist_result(diagnostic_output_dir, result.stdout, result.stderr, result.returncode)
    if received_signal is not None:
        _restore_and_forward_signal(received_signal, received_frame, previous_handlers)
    diagnostic_texts = [result.stdout, result.stderr]
    if diagnostic_output_dir is not None:
        for path in (diagnostic_output_dir / "stdout.log",
                     diagnostic_output_dir / "stderr.log",
                     *sorted(diagnostic_output_dir.glob("panda-temp/__cc_output*"))):
            if path.is_file():
                diagnostic_texts.append(path.read_text(errors="replace"))
    combined = "\n".join(diagnostic_texts)
    if any(marker in combined for marker in sandbox_markers):
        raise CompilerTestError("compiler output contains a sandbox failure marker")
    if result.returncode < 0:
        raise CompilerTestError(f"compiler crashed with signal {-result.returncode}")
    if expected_failure is None:
        if result.returncode != 0:
            raise CompilerTestError(f"compiler exited with status {result.returncode}")
    else:
        if result.returncode == 0 and allow_success:
            return result
        if result.returncode == 0:
            raise CompilerTestError("compiler succeeded; expected a diagnostic failure")
        if expected_failure not in combined:
            raise CompilerTestError(f"expected diagnostic not found: {expected_failure!r}")
    return result


def check_compiler_diagnostics(output_dir: Path, expected: str) -> tuple[Path, ...]:
    """Find expected text in compiler logs or generated __cc_output files."""
    candidates = [output_dir / "stdout.log", output_dir / "stderr.log"]
    candidates.extend(sorted(output_dir.glob("panda-temp/__cc_output*")))
    readable = tuple(path for path in candidates if path.is_file())
    if not any(expected in path.read_text(errors="replace") for path in readable):
        raise CompilerTestError(f"expected compiler diagnostic not found: {expected!r}")
    return readable


def _as_text(value: str | bytes | None) -> str:
    if value is None:
        return ""
    return value.decode(errors="replace") if isinstance(value, bytes) else value


def _persist_result(output_dir: Path | None, stdout: str, stderr: str, returncode: int | None) -> None:
    if output_dir is None:
        return
    (output_dir / "stdout.log").write_text(stdout, encoding="utf-8")
    (output_dir / "stderr.log").write_text(stderr, encoding="utf-8")
    (output_dir / "exit_code.txt").write_text(f"{returncode}\n", encoding="ascii")


def _terminate_process_group(process: subprocess.Popen[str]) -> None:
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def _restore_and_forward_signal(signum: int, frame, previous_handlers) -> None:
    previous = previous_handlers.get(signum, signal.SIG_DFL)
    signal.signal(signum, previous)
    if previous == signal.SIG_DFL:
        os.kill(os.getpid(), signum)
    elif previous != signal.SIG_IGN:
        previous(signum, frame)
