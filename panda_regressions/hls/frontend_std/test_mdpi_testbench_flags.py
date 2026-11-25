#!/usr/bin/env python3
"""Check that native testbench objects receive flags for their source language."""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run_make(make, makefile, compiler, sim_dir, beh_dir, tb_sources, cflags, tb_cflags, env, cpp_cflags=None):
    cmd = [
        make,
        "-f",
        str(makefile),
        "V=1",
        f"SIM_DIR={sim_dir}",
        f"BEH_DIR={beh_dir}",
        "TOP_FNAME=top",
        "MTOP_FNAME=__m_top",
        f"CC={compiler}",
        f"CFLAGS={cflags}",
        f"TB_CFLAGS={tb_cflags}",
        f"TB_SRCS={' '.join(str(source) for source in tb_sources)}",
    ]
    if cpp_cflags is not None:
        cmd.append(f"CPP_CFLAGS={cpp_cflags}")
    cmd.extend(str(sim_dir / "build" / "tb" / f"{source.name}.o") for source in tb_sources)
    proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if proc.returncode:
        print(proc.stdout, end="")
        print(proc.stderr, end="", file=sys.stderr)
        return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--makefile", required=True)
    parser.add_argument("--cc", required=True)
    args = parser.parse_args()

    make = shutil.which("make")
    if not make:
        print("FAIL make is not available")
        return 1

    root = Path(__file__).resolve().parents[3]
    scratch = root / "documentation" / "tmp"
    scratch.mkdir(parents=True, exist_ok=True)
    workdir = Path(tempfile.mkdtemp(prefix="mdpi_testbench_flags_", dir=scratch))
    env = dict(os.environ, TMPDIR=str(workdir), TMP=str(workdir), TEMP=str(workdir))
    fixtures = workdir / "fixtures"
    fixtures.mkdir()
    c_source = fixtures / "tb.c"
    cpp_source = fixtures / "tb.cpp"
    override_source = fixtures / "override.cpp"
    c_source.write_text(
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L\n"
        '#error "C testbench did not receive C11"\n#endif\n'
        "#ifndef TB_ONLY\n#error testbench-only flags were lost\n#endif\n"
        "void top(void) {}\n"
    )
    cpp_source.write_text(
        "#if !defined(__cplusplus) || __cplusplus < 201402L || __cplusplus >= 201703L\n"
        '#error "C++ testbench did not receive C++14"\n#endif\n'
        "#ifndef TB_ONLY\n#error testbench-only flags were lost\n#endif\n"
        "#ifndef __EXCEPTIONS\n#error inherited -fno-exceptions was not removed\n#endif\n"
        'extern "C" void top(void) {}\n'
    )
    override_source.write_text(
        "#if !defined(__cplusplus) || __cplusplus < 201703L\n"
        '#error "testbench C++ standard override lost precedence"\n#endif\n'
        "#ifndef TB_ONLY\n#error testbench-only flags were lost\n#endif\n"
        'extern "C" void top(void) {}\n'
    )

    try:
        split_dir = workdir / "split"
        if not run_make(make, args.makefile, args.cc, split_dir, workdir / "beh", (c_source, cpp_source),
                        "-std=c11 -fno-exceptions", "-DTB_ONLY=1",
                        env, "-std=c++14 -fno-exceptions"):
            print("FAIL C and C++ testbench objects must use language-specific flags")
            return 1
        print("OK C11 and C++14 testbench objects retain testbench extras and exception support")

        override_dir = workdir / "override"
        if not run_make(make, args.makefile, args.cc, override_dir, workdir / "beh", (override_source,),
                        "-std=c11 -fno-exceptions", "-DTB_ONLY=1 -std=c++17",
                        env, "-std=c++14 -fno-exceptions"):
            print("FAIL testbench C++ standard override must follow CPP_CFLAGS")
            return 1
        print("OK testbench C++ standard override has precedence")

        legacy_dir = workdir / "legacy"
        if not run_make(make, args.makefile, args.cc, legacy_dir, workdir / "beh", (cpp_source,),
                        "-std=c++14 -fno-exceptions", "-DTB_ONLY=1", env):
            print("FAIL legacy Makefile invocation without CPP_CFLAGS")
            return 1
        print("OK legacy C++ testbench flags fall back to CFLAGS")
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
