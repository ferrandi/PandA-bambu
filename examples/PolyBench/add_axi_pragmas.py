#!/usr/bin/env python3
# Copyright (C) 2026 Jesun Firoz
# SPDX-License-Identifier: Apache-2.0
# Author: Jesun Firoz <jesun.firoz@pnnl.gov>

"""Add #pragma HLS interface m_axi pragmas to PolyBench kernel functions."""

import re
import os
import glob

POLYBENCH_AXI_DIR = os.path.join(os.path.dirname(__file__), "PolyBenchC_AXI")

def find_kernel_files():
    """Find all kernel .c files (not tb_ or utilities)."""
    files = []
    for root, dirs, fnames in os.walk(POLYBENCH_AXI_DIR):
        if "utilities" in root:
            continue
        for f in fnames:
            if f.endswith(".c") and not f.startswith("tb_") and f != "Nussinov.orig.c":
                files.append(os.path.join(root, f))
    return sorted(files)

def extract_array_params(filepath):
    """Extract array parameter names from kernel function signature."""
    with open(filepath, "r") as f:
        content = f.read()

    # Find POLYBENCH_ND(name, ...) patterns in kernel function
    params = re.findall(r'POLYBENCH_[1-5]D\s*\(\s*(\w+)\s*,', content)
    return params

def add_pragmas(filepath, params):
    """Add m_axi pragmas before the kernel function definition."""
    with open(filepath, "r") as f:
        lines = f.readlines()

    # Find the line with "void kernel_"
    kernel_line = None
    for i, line in enumerate(lines):
        if re.match(r'\s*void\s+kernel_', line):
            kernel_line = i
            break

    if kernel_line is None:
        print(f"  WARNING: No kernel function found in {filepath}")
        return False

    # Build pragma lines
    pragmas = []
    for p in params:
        pragmas.append(f"#pragma HLS interface port={p} mode=m_axi offset=direct\n")

    # Check if pragmas already exist
    if any("m_axi" in lines[max(0, kernel_line - len(params) - 1):kernel_line + 1]
           for _ in [0]):
        # More robust check
        preceding = "".join(lines[max(0, kernel_line - 10):kernel_line])
        if "m_axi" in preceding:
            print(f"  SKIP: {filepath} (pragmas already present)")
            return False

    # Insert pragmas before kernel function
    for j, pragma in enumerate(pragmas):
        lines.insert(kernel_line + j, pragma)

    with open(filepath, "w") as f:
        f.writelines(lines)

    return True

def main():
    files = find_kernel_files()
    print(f"Found {len(files)} kernel files")

    for filepath in files:
        relpath = os.path.relpath(filepath, POLYBENCH_AXI_DIR)
        params = extract_array_params(filepath)
        if not params:
            print(f"  SKIP: {relpath} (no array params)")
            continue

        if add_pragmas(filepath, params):
            print(f"  OK: {relpath} -> {', '.join(params)}")

    print("\nDone!")

if __name__ == "__main__":
    main()
