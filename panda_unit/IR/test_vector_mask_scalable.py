# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Require the installed Bambu frontend to reject scalable vectors explicitly."""

import argparse
import os
import pathlib
import resource
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bambu", required=True)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    scratch = root / "documentation/tmp"
    scratch.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TMPDIR=str(scratch), TMP=str(scratch), TEMP=str(scratch))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with tempfile.TemporaryDirectory(prefix="vector-mask-scalable-", dir=str(scratch)) as work:
        result = subprocess.run(
            [args.bambu, str(root / "panda_unit/IR/vector_mask_scalable.ll"),
             "--compiler=I386_CLANG16", "--top-fname=mask_scalable", "-O0",
             "--output-directory=" + str(pathlib.Path(work) / "output")],
            cwd=work, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            universal_newlines=True, timeout=120)
        if result.returncode == 0 or "Scalable vector types are not supported by Bambu" not in result.stdout:
            print(result.stdout)
            raise SystemExit("FAIL: expected a nonzero exit with the plugin's scalable-vector diagnostic")
    print("PASS: Scalable vector types are not supported by Bambu (Clang 16 frontend)")


if __name__ == "__main__":
    main()
