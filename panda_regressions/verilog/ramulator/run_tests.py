#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Run production MDPI/Ramulator contract tests without invoking Bambu."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def execute(command, directory, env=None, timeout=180):
    with (directory / 'run.log').open('a') as log:
        log.write(shlex.join(map(str, command)) + '\n')
        log.flush()
        subprocess.run(list(map(str, command)), cwd=directory, env=env, stdout=log,
                       stderr=subprocess.STDOUT, check=True, timeout=timeout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--junit', required=True, type=Path)
    parser.add_argument('--ramulator', type=Path, default=os.environ.get('RAMULATOR2_DIR'))
    parser.add_argument('--bambu-prefix', type=Path, default=ROOT / 'install')
    args = parser.parse_args()
    if not args.ramulator:
        parser.error('export RAMULATOR2_DIR or supply --ramulator')
    prefix = args.ramulator.resolve()
    libdir = next((p for p in (prefix / 'lib', prefix / 'lib64') if (p / 'libramulator.so').is_file()), None)
    if not libdir:
        parser.error('Ramulator installation has no libramulator.so')
    verilator = Path(shutil.which('verilator') or '/usr/bin/verilator').resolve()
    dpi = Path(subprocess.check_output([str(verilator), '-getenv', 'VERILATOR_ROOT'], text=True).strip()) / 'include/vltstd'
    directory = args.out.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    common = ['-D__M64', '-DBAMBU_CSIM', '-D__M_OUT_LVL=2', f'-I{ROOT / "etc/libmdpi/include"}',
              f'-I{args.bambu_prefix.resolve() / "include/panda"}', f'-I{dpi}']
    start = time.monotonic()
    error = None
    try:
        execute(['g++', '-std=c++20', *common, '-DHAVE_RAMULATOR2',
                 f'-I{prefix / "include/ramulator"}', f'-I{prefix / "include"}',
                 ROOT / 'etc/libmdpi/mdpi_driver.cpp', HERE / 'host.cpp',
                 f'-L{libdir}', f'-Wl,-rpath,{libdir}', '-lramulator', '-lyaml-cpp', '-lpthread',
                 '-o', directory / 'host'], directory)
        execute(['gcc', '-std=c11', '-D_GNU_SOURCE', *common, ROOT / 'etc/libmdpi/mdpi.c',
                 HERE / 'acceptance.c', '-o', directory / 'acceptance'], directory)
        env = os.environ.copy()
        env['RAMULATOR2_CONFIG'] = str(ROOT / 'etc/libmdpi/configs/default_ddr4.yaml')
        env['BAMBU_IPC_SIM_CMD'] = str(directory / 'acceptance')
        env.pop('BAMBU_IPC_FILENAME', None)
        execute([directory / 'host'], directory, env=env, timeout=60)
    except (OSError, subprocess.SubprocessError) as exception:
        error = str(exception)
    suite = ET.Element('testsuite', name='ramulator-rtl', tests='1', failures=str(int(error is not None)))
    case = ET.SubElement(suite, 'testcase', name='mdpi-acceptance', time=f'{time.monotonic()-start:.3f}')
    if error:
        ET.SubElement(case, 'failure', message=error).text = (directory / 'run.log').read_text(errors='replace')[-8000:]
    args.junit.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(suite).write(args.junit, encoding='utf-8', xml_declaration=True)
    print('FAIL' if error else 'PASS', 'mdpi-acceptance', flush=True)
    return bool(error)


if __name__ == '__main__':
    raise SystemExit(main())
