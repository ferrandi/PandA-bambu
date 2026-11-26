#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0
"""Exercise the native board/discovery/probe interface with complete co-simulation."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import time
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bambu', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1 or not os.environ.get('BAMBU_HLS') or not os.environ.get('RAMULATOR2_DIR'):
        parser.error('source settings.sh, set RAMULATOR2_DIR and use positive --jobs')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    bambu = args.bambu.resolve()
    library = Path(os.environ['BAMBU_HLS']) / 'share/panda/libmdpi'
    sources = list((library / 'configs').rglob('*.yaml')) + list((library / 'calibrations').rglob('*.xml'))
    before = {p: hashlib.sha256(p.read_bytes()).digest() for p in sources}
    discovery = out / 'discovery.yaml'
    discovery.write_bytes((library / 'configs/boards/alveo-u55c.yaml').read_bytes())
    probe = out / 'probe.xml'
    probe.write_text('''<ramulator_configuration schema_version="1" status="experimental" profile="alveo-u55c">
  <delay_ns read_forward="20" write_forward="30" read_return="40" write_return="50"/>
</ramulator_configuration>\n''')
    rounding = out / 'rounding.xml'
    just_above_ten = '10.' + '0' * 100 + '1'
    rounding.write_text(f'''<ramulator_configuration schema_version="1" status="experimental" profile="alveo-u55c">
  <delay_ns read_forward="{just_above_ten}" write_forward="20.000000000001" read_return="30.5" write_return="40.000000000001"/>
</ramulator_configuration>\n''')
    invalid = out / 'invalid.xml'
    invalid.write_text('<ramulator_configuration schema_version="1"><clocks_hz accelerator="200000000"/></ramulator_configuration>\n')
    cases = [
        ('board-default', ['--ramulator-profile=alveo-u55c'], False, 10, (1,1,1,1), None),
        ('board-config-axi', ['--ramulator-profile=alveo-u250', '--ramulator-calibration=default'], True, 10, (1,1,1,1), None),
        ('discovery-only', [f'--ramulator-import={discovery}'], False, 10, (1,1,1,1), None),
        ('probe-only', [f'--ramulator-import={probe}'], True, 10, (2,3,4,5), None),
        ('both', [f'--ramulator-import={discovery}', f'--ramulator-import={probe}'], False, 10, (2,3,4,5), None),
        ('both-reversed-axi', [f'--ramulator-import={probe}', f'--ramulator-import={discovery}'], True, 10, (2,3,4,5), None),
        ('upward-rounding-axi', [f'--ramulator-import={rounding}'], True, 10, (2,3,4,5), None),
        ('default-five-ns', ['--ramulator-profile=alveo-u55c'], False, 5, (1,1,1,1), None),
        ('clock-mismatch', ['--ramulator-profile=alveo-u55c', f'--ramulator-import={invalid}'], False, 10, None, 'calibration clock differs'),
        ('override-conflict', ['--ramulator-profile=alveo-u55c'], True, 10, None, 'RAMULATOR_OVERRIDE_CHANNELS conflicts'),
    ]

    def run(case):
        name, options, axi, clock, cycles, diagnostic = case
        directory = out / name
        directory.mkdir()
        command = [str(bambu), str(HERE / 'kernel.cpp'), f'--generate-tb={HERE / "input.xml"}',
                   '--top-fname=ramulator_kernel', '--compiler=I386_CLANG13', '--simulator=VERILATOR',
                   '--simulate', '--generate-interface=INFER', '--enable-ramulator',
                   '--memory-allocation-policy=NO_BRAM', f'--clock-period={clock}', '-m64', '-lm', '-v5',
                   '--max-sim-cycles=5000', '-DTEST_KERNEL=0', '-DBANKED=0', f'-DUSE_AXI={int(axi)}',
                   '-DDEDICATED_AXI=0', '--channels-type=MEM_ACC_11', '--channels-number=1', *options]
        (directory / 'command.txt').write_text(shlex.join(command) + '\n')
        env = os.environ.copy()
        env.pop('RAMULATOR_OVERRIDE_CHANNELS', None)
        env.pop('LD_LIBRARY_PATH', None)
        # The explicit generated configuration must win over this broken legacy path.
        env['RAMULATOR2_CONFIG'] = str(out / 'must-not-be-selected.yaml')
        if name == 'override-conflict':
            env['RAMULATOR_OVERRIDE_CHANNELS'] = '1'
        error = None
        start = time.monotonic()
        try:
            with (directory / 'run.log').open('w') as log:
                result = subprocess.run(command, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=240)
            text = (directory / 'run.log').read_text(errors='replace')
            if diagnostic:
                if not result.returncode or diagnostic not in text:
                    raise ValueError('expected diagnostic: ' + diagnostic)
            else:
                if result.returncode or 'Ramulator2 successfully initialized' not in text or 'OP=READ' not in text:
                    raise ValueError('co-simulation failed or real Ramulator read evidence missing')
                model = directory / 'HLS_output/memory-model'
                report = ET.parse(model / 'resolved-memory-model.xml').getroot()
                delays = report.findall('resolved/delay')
                actual = tuple(int(node.get('cycles')) for node in delays)
                if actual != cycles:
                    raise ValueError(f'expected transport cycles {cycles}, got {actual}')
                tb = (directory / 'HLS_output/simulation/bambu_testbench.v').read_text()
                for field, value in zip(('READ_FORWARD','WRITE_FORWARD','READ_RETURN','WRITE_RETURN'), cycles):
                    if not re.search(r'\.' + field + r'\s*\(\s*' + str(value) + r'\s*\)', tb):
                        raise ValueError('generated TB is missing resolved ' + field)
                if 'ramulator_phase' in tb:
                    raise ValueError('generated Ramulator TB still has a phase-release signal')
                if not (model / 'ramulator2_config.yaml').is_file():
                    raise ValueError('missing explicit native YAML')
                if clock == 5 and any(node.get('requested_fs') != '5000000' for node in delays):
                    raise ValueError('default delay is not one current accelerator cycle')
        except (OSError, ValueError, subprocess.SubprocessError, ET.ParseError) as exception:
            error = str(exception)
        print('FAIL' if error else 'PASS', name, error or '', flush=True)
        return name, time.monotonic()-start, error, directory

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, cases))
    if before != {p: hashlib.sha256(p.read_bytes()).digest() for p in sources}:
        results.append(('library-immutable', 0, 'installed library changed', out))
    suite = ET.Element('testsuite', name='ramulator-profiles', tests=str(len(results)),
                       failures=str(sum(error is not None for _,_,error,_ in results)))
    for name, elapsed, error, directory in results:
        case = ET.SubElement(suite, 'testcase', name=name, time=f'{elapsed:.3f}')
        if error:
            log = directory / 'run.log'
            ET.SubElement(case, 'failure', message=error).text = log.read_text(errors='replace')[-8000:] if log.exists() else error
    ET.ElementTree(suite).write(out / 'junit.xml', encoding='utf-8', xml_declaration=True)
    return any(error for _,_,error,_ in results)


if __name__ == '__main__':
    raise SystemExit(bool(main()))
