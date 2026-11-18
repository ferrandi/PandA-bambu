#!/usr/bin/env python3
"""Check that bambu emits exactly one, language-adjusted --std option per source.

The frontend parameters are built by CompilerWrapper::GetFrontendCompilerParameters:
the global --std option is normalized per translation unit language (a C source
gets a C standard, a C++ source a C++ one), while paths without a source file
(GetCompilerParameters, e.g. the cflags recorded in bambu_results.xml) keep the
global standard.  The value of the applied standard is verified portably through
__STDC_VERSION__/__cplusplus guards compiled by the front-end itself; when
strace is available the test additionally inspects the clang/opt argv to check
that each command carries a single --std (the expected one), that -fopenmp is
appended as usual, and that the LTO/opt commands receive no --std at all.

Run it against the installation produced by "cmake --install" of the same
build (the settings.sh next to that installation is sourced automatically);
the test reports SKIP when the given bambu does not exist yet.

Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

KERNEL = """#include <omp.h>
void {name}(int* a, int n)
{{
   {guard}
   int i;
#pragma omp parallel for num_threads(2)
   for(i = 0; i < n; ++i) a[i] = i * 2;
}}
"""

# (global --std, file extension, expected --std for the source, language guard)
CASES = [
    ("c99", ".c", "--std=c99", "#if __STDC_VERSION__ != 199901L\n#error wrong C standard\n#endif"),
    ("c99", ".cpp", "--std=c++14", "#if __cplusplus < 201402L || __cplusplus >= 201703L\n#error wrong C++ standard\n#endif"),
    ("c++17", ".cpp", "--std=c++17", "#if __cplusplus < 201703L\n#error wrong C++ standard\n#endif"),
    ("c++17", ".c", "--std=c11", "#if __STDC_VERSION__ < 201112L\n#error wrong C standard\n#endif"),
]


def bambu_environment(bambu):
    """Return the environment to run bambu with.

    The settings.sh next to the given installation is always sourced, so that
    the run uses the very resources of the installation under test even when
    the inherited environment was initialized from a different one; the
    inherited environment is a fallback for layouts without settings.sh.
    """
    env = dict(os.environ)
    binary = Path(bambu)
    for candidate in (binary.parent.parent, binary.parent):
        settings = candidate / "settings.sh"
        if not settings.is_file():
            continue
        dump = subprocess.run(["bash", "-c", f"source '{settings}' >/dev/null 2>&1 && env -0"],
                              capture_output=True, text=True, check=True).stdout
        for entry in dump.split("\0"):
            key, sep, value = entry.partition("=")
            if sep and key:
                env[key] = value
        if env.get("BAMBU_HLS") and env.get("BAMBU_HLS_BACKEND_PATH"):
            return env
    if env.get("BAMBU_HLS") and env.get("BAMBU_HLS_BACKEND_PATH"):
        return env
    raise SystemExit(f"cannot initialize the bambu environment for {bambu}: no usable settings.sh found")


def run_bambu(bambu, workdir, source, extra=(), env=None):
    cmd = [bambu, str(source), "--top-fname=kernel", *extra]
    proc = subprocess.run(cmd, cwd=workdir, capture_output=True, text=True, env=env)
    return proc


def clang_commands(strace_log):
    """Yield (executable, argv) of every clang execve captured by strace."""
    text = Path(strace_log).read_text(errors="ignore")
    for match in re.finditer(r'execve\("([^"]*clang[^"]*)", \[(.*?)\]', text, re.S):
        argv = re.findall(r'"((?:[^"\\]|\\.)*)"', match.group(2))
        yield match.group(1), argv


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bambu", required=True, help="bambu executable to test, usually the installed one")
    parser.add_argument("--keep", action="store_true", help="keep the temporary directory")
    args = parser.parse_args()
    bambu = shutil.which(args.bambu)
    if bambu is None and not Path(args.bambu).is_file():
        print(f"SKIP {args.bambu} not found: install this build (cmake --install) before running the test")
        return 0
    bambu = bambu or str(Path(args.bambu).resolve())

    strace = shutil.which("strace")
    failures = []
    env = bambu_environment(bambu)
    tmpdir = Path(tempfile.mkdtemp(prefix="frontend_std_"))
    try:
        for std, extension, expected, guard in CASES:
            workdir = tmpdir / f"case_{std.replace('+', 'x')}_{extension.strip('.')}"
            workdir.mkdir()
            source = workdir / f"kernel{extension}"
            source.write_text(KERNEL.format(name="kernel", guard=guard))
            proc = run_bambu(bambu, workdir, source, ["--std=" + std, "-fopenmp",
                                                      "--memory-allocation-policy=NO_BRAM"], env)
            if proc.returncode != 0:
                tail = (proc.stdout + proc.stderr).strip().splitlines()[-3:]
                failures.append(f"{std} + {extension}: bambu exit {proc.returncode}: {' | '.join(tail)}")
                continue
            print(f"OK  value of --std for {extension} with global --std={std} is {expected}")
            if std == "c++17" and extension == ".c":
                cflags = re.search(r'cflags="([^"]*)"', (workdir / "bambu_results.xml").read_text())
                if not cflags:
                    failures.append("bambu_results.xml without cflags attribute")
                else:
                    emitted = re.findall(r"--std=\S+", cflags.group(1))
                    if emitted != [f"--std={std}"]:
                        failures.append(f"cflags carry {emitted} instead of one --std={std}")
                    elif "-D__NO_INLINE__" not in cflags.group(1):
                        failures.append("cflags lost the options built together with --std")
                    else:
                        print(f"OK  cflags of bambu_results.xml keep one global --std={std} plus the other options")

        if not strace:
            print("SKIP argv inspection: strace not available")
        else:
            for std, extension, expected, guard in CASES:
                workdir = tmpdir / f"strace_{std.replace('+', 'x')}_{extension.strip('.')}"
                workdir.mkdir()
                source = workdir / f"kernel{extension}"
                source.write_text(KERNEL.format(name="kernel", guard=guard))
                log = workdir / "strace.log"
                with log.open("w") as sink:
                    proc = subprocess.run([strace, "-f", "-qq", "-s", "6000", "-e", "trace=execve",
                                           bambu, str(source), "--top-fname=kernel", "--std=" + std,
                                           "-fopenmp", "--memory-allocation-policy=NO_BRAM"],
                                          cwd=workdir, stdout=sink, stderr=subprocess.STDOUT, env=env)
                if proc.returncode != 0:
                    failures.append(f"strace run {std} + {extension}: bambu exit {proc.returncode}")
                    continue
                checked = 0
                for exe, argv in clang_commands(log):
                    # the OpenMP runtime unit is compiled as C++ independently of
                    # the user source language
                    if any("kmp_single_file" in arg for arg in argv):
                        continue
                    if not any(arg.endswith(extension) for arg in argv):
                        # LTO/opt steps must never receive a frontend standard
                        if any(arg.startswith("--std=") for arg in argv):
                            failures.append(f"{exe} received a --std option: {argv}")
                        continue
                    emitted = [arg for arg in argv if arg.startswith("--std=")]
                    if emitted != [expected]:
                        failures.append(f"{exe} for {source.name}: {emitted} instead of one {expected}")
                    if "-fopenmp" not in argv:
                        failures.append(f"{exe} for {source.name}: -fopenmp option lost")
                    checked += 1
                if checked == 0:
                    failures.append(f"strace run {std} + {extension}: no clang command inspected")
                else:
                    print(f"OK  argv of {checked} clang command(s) for {extension} with global --std={std}: "
                          f"one {expected}, -fopenmp kept, no --std elsewhere")
    finally:
        if args.keep:
            print(f"temporary files kept in {tmpdir}")
        else:
            shutil.rmtree(tmpdir, ignore_errors=True)

    if failures:
        print("\n".join(f"FAIL {failure}" for failure in failures))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
