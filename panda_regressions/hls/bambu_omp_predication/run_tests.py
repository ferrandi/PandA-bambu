#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0
"""OpenMP native-predication RTL and end-to-end regressions for minimal CI."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import time
import xml.etree.ElementTree as ET
from rtl_tests import run_rtl

HERE = Path(__file__).resolve().parent


def check_predication(directory, required):
    """Check dynamic predicate cones and raw starts on the actual FU instances.

    s_predicate is emitted only for a dynamic IR call predicate. Shared FUs OR
    the start-qualified predicates of their call sites; follow that OR cone as
    well, so both a conditional and an unconditional call can share the FU.
    """
    rtl = '\n'.join(p.read_text() for p in directory.glob('*.v'))
    drivers = {}
    for kind, ports in re.findall(r'\b(and|or)\s+\w+\s*\(([^;]+)\);', rtl):
        signals = [p.strip() for p in ports.split(',')]
        drivers[signals[0]] = (kind, signals[1:])

    def dynamic(signal, visited=None):
        visited = set() if visited is None else visited
        if signal in visited:
            return False
        visited.add(signal)
        gate, inputs = drivers.get(signal, ('', []))
        if gate == 'and' and any(p.startswith('s_predicate_') for p in inputs):
            return True
        return gate == 'or' and any(dynamic(p, visited) for p in inputs)

    verified = set()
    instances = re.findall(r'KMP_BAMBU_(CRITICAL|END_CRITICAL|BARRIER_REACHED|(?:CS_)?WAIT_ALL_THREADS)_FU\s+'
                           r'(?:#\s*\(.*?\)\s*)?\w+\s*\((.*?)\);', rtl, re.S)
    for name, instance in instances:
        predicate = re.search(r'\.predicate\(([^)]+)\)', instance)
        start = re.search(r'\.start_port\(([^)]+)\)', instance)
        if predicate and start and dynamic(predicate[1]) and not dynamic(start[1]):
            verified.add(name)
    missing = set(required) - verified
    if missing:
        raise ValueError('Missing native dynamic predication with raw start: ' + ', '.join(sorted(missing)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bambu', default=shutil.which('bambu'))
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--junit', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=1)
    parser.add_argument('--case', action='append', default=[], help='Run selected case names')
    parser.add_argument('--skip-rtl', action='store_true')
    args = parser.parse_args()
    if not args.bambu or args.jobs < 1:
        parser.error('bambu must be installed and --jobs must be positive')
    bambu = str(Path(args.bambu).resolve())
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    results = []
    if not args.skip_rtl:
        start = time.monotonic()
        error = None
        try:
            run_rtl(out)
        except (OSError, subprocess.SubprocessError, ValueError) as exception:
            error = str(exception)
        results.append(('rtl-reset-profiles', time.monotonic() - start, error, out))
    tasks = [(source, threads, context) for source in ('critical', 'barrier', 'phases')
             for threads, context in ((1, 0), (4, 0), (4, 2))]
    names = {f'{source}-{threads}t' + ('-cs2' if context else '') for source, threads, context in tasks}
    if set(args.case) - names:
        parser.error('Unknown case: ' + ', '.join(set(args.case) - names))

    def run(task):
        source, threads, context = task
        name = f'{source}-{threads}t' + ('-cs2' if context else '')
        directory = out / name
        directory.mkdir(exist_ok=False)
        command = [bambu, str(HERE / f'{source}.cpp'), f'--generate-tb={HERE / "input.xml"}',
                   '--top-fname=omp_predication', '--compiler=I386_CLANG13', '--simulator=VERILATOR',
                   '--simulate', '-fopenmp', '-m64', '-lm', f'-DTHREADS={threads}',
                   f'--channels-type={"MEM_ACC_11" if threads == 1 else "MEM_ACC_NN"}',
                   f'--channels-number={min(threads, 2)}',
                   '--memory-allocation-policy=NO_BRAM', '--max-sim-cycles=15000']
        if context:
            command.append(f'--context_switch={context}')
        (directory / 'command.txt').write_text(shlex.join(command) + '\n')
        start = time.monotonic()
        error = None
        try:
            with (directory / 'run.log').open('w') as log:
                subprocess.run(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT,
                               check=True, timeout=240)
            if threads == 4 and source in ('critical', 'barrier'):
                required = ['CRITICAL'] if source == 'critical' else [
                    'BARRIER_REACHED', 'CS_WAIT_ALL_THREADS' if context else 'WAIT_ALL_THREADS']
                check_predication(directory, required)
        except (OSError, subprocess.SubprocessError, ValueError) as exception:
            error = str(exception)
        print(f'{"FAIL" if error else "PASS"} {name}' + (f': {error}' if error else ''), flush=True)
        return name, time.monotonic() - start, error, directory

    selected = [task for task in tasks if not args.case or
                f'{task[0]}-{task[1]}t' + ('-cs2' if task[2] else '') in args.case]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results.extend(pool.map(run, selected))
    failures = sum(error is not None for _, _, error, _ in results)
    suite = ET.Element('testsuite', name='bambu-omp-predication', tests=str(len(results)), failures=str(failures))
    for name, elapsed, error, directory in results:
        case = ET.SubElement(suite, 'testcase', name=name, classname='bambu.omp_predication', time=f'{elapsed:.3f}')
        if error:
            log = directory / 'run.log'
            ET.SubElement(case, 'failure', message=error).text = log.read_text(errors='replace')[-6000:] if log.exists() else error
    args.junit.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(suite).write(args.junit, encoding='utf-8', xml_declaration=True)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
