#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Run production MDPI/Ramulator contract tests without invoking Bambu."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import shutil
import subprocess
import time
import xml.etree.ElementTree as ET
from extract import extract

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def execute(command, directory, env=None, timeout=180):
    with (directory / 'run.log').open('a') as log:
        log.write(shlex.join(map(str, command)) + '\n')
        log.flush()
        with subprocess.Popen(list(map(str, command)), cwd=directory, env=env, stdout=log,
                              stderr=subprocess.STDOUT, start_new_session=True) as process:
            try:
                result = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait()
                raise
            if result:
                raise subprocess.CalledProcessError(result, command)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--junit', required=True, type=Path)
    parser.add_argument('--ramulator', type=Path, default=os.environ.get('RAMULATOR2_DIR'))
    parser.add_argument('--contract-only', action='store_true')
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
    results = []
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
        env.pop('LD_LIBRARY_PATH', None)
        execute([directory / 'host'], directory, env=env, timeout=60)
    except (OSError, subprocess.SubprocessError) as exception:
        error = str(exception)
    results.append(('mdpi-acceptance', time.monotonic()-start, error, directory))
    if not error and not args.contract_only:
        models = directory / 'models.sv'
        models.write_text('typedef longint unsigned ptr_t;\n' + extract('TestbenchMEMRamulatorIntegrated') +
                          extract('TestbenchMEMAXIRamulatorIntegrated'))
        execute(['gcc', '-c', '-std=c11', '-D_GNU_SOURCE', *common, ROOT / 'etc/libmdpi/mdpi.c',
                 '-o', directory / 'mdpi.o'], directory)
        for model in ('memory', 'axi'):
            for depth in ((1, 2, 4, 64) if model == 'memory' else (1, 2, 4)):
                name = f'{model}-depth{depth}'
                case_dir = directory / name
                case_dir.mkdir()
                start = time.monotonic()
                error = None
                try:
                    execute([str(verilator), '--binary', '--timing', '-Wno-fatal', '--top-module', f'tb_{model}',
                             f'-GDEPTH={depth}', '--Mdir', case_dir / 'obj', models,
                             HERE / f'tb_{model}.sv', '-LDFLAGS', directory / 'mdpi.o'], case_dir)
                    case_env = env.copy()
                    case_env['BAMBU_IPC_SIM_CMD'] = str(case_dir / f'obj/Vtb_{model}')
                    execute([directory / 'host'], case_dir, env=case_env, timeout=60)
                except (OSError, subprocess.SubprocessError) as exception:
                    error = str(exception)
                print('FAIL' if error else 'PASS', name, flush=True)
                results.append((name, time.monotonic()-start, error, case_dir))
    suite = ET.Element('testsuite', name='ramulator-rtl', tests=str(len(results)),
                       failures=str(sum(error is not None for _, _, error, _ in results)))
    for name, elapsed, error, case_dir in results:
        case = ET.SubElement(suite, 'testcase', name=name, time=f'{elapsed:.3f}')
        if error:
            ET.SubElement(case, 'failure', message=error).text = (case_dir / 'run.log').read_text(errors='replace')[-8000:]
    args.junit.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(suite).write(args.junit, encoding='utf-8', xml_declaration=True)
    return any(error is not None for _, _, error, _ in results)


if __name__ == '__main__':
    raise SystemExit(main())
