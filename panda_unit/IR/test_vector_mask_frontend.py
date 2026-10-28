# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Mandatory installed-frontend gate for the protected vector-mask fixtures."""

import argparse
import importlib.util
import os
import pathlib
import subprocess
import tempfile


def run(command, cwd, env):
    result = subprocess.run(command, cwd=str(cwd), env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, universal_newlines=True, timeout=180)
    if result.returncode:
        raise ValueError("Command failed: {}\n{}".format(command, result.stdout))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bambu", required=True)
    parser.add_argument("--compiler", default="I386_CLANG16")
    parser.add_argument("--llvm-dis", required=True)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    fixtures = root / "panda_regressions/hls/bambu_vector_test"
    spec = importlib.util.spec_from_file_location("checker", fixtures / "check_vector_mask_ir.py")
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    scratch = root / "documentation/tmp"
    scratch.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TMPDIR=str(scratch), TMP=str(scratch), TEMP=str(scratch),
               OMP_NUM_THREADS="1", CMAKE_BUILD_PARALLEL_LEVEL="1")
    checked = 0
    try:
        with tempfile.TemporaryDirectory(prefix="vector-mask-frontend-", dir=str(scratch)) as temporary:
            work = pathlib.Path(temporary)
            for filename in ("vector_mask_ir.ll", "vector_mask_qor.ll"):
                source = fixtures / filename
                reference = checker.parse_ir(source)
                for level in (0, 3):
                    for top in sorted(reference):
                        output = work / (source.stem + "-" + top + "-O" + str(level))
                        run([args.bambu, str(source), "--compiler=" + args.compiler,
                             "--top-fname=" + top, "-O" + str(level), "--generate-interface=INFER",
                             "--no-clean", "--output-directory=" + str(output)], work, env)
                        objects = list((output / "panda-temp").glob(filename + ".*.o"))
                        if len(objects) != 1:
                            raise ValueError("{}: expected one frontend bitcode object, got {}".format(top, objects))
                        frontend = output / "frontend.ll"
                        run([args.llvm_dis, str(objects[0]), "-o", str(frontend)], work, env)
                        actual = checker.parse_ir(frontend)
                        if reference[top] != actual.get(top):
                            raise ValueError("{} -O{} {}: frontend instructions changed\nexpected: {}\nactual: {}".format(
                                args.compiler, level, top, reference[top], actual.get(top)))
                        checked += 1
                        print("PASS FRONTEND_IR {} -O{} {}".format(args.compiler, level, top), flush=True)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        raise SystemExit("FAIL: {}".format(error))
    print("PASS: {} frontend IR checks".format(checked))


if __name__ == "__main__":
    main()
