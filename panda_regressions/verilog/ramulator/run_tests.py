#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Run production MDPI/Ramulator contract tests without invoking Bambu."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import re
from fractions import Fraction
import shutil
import subprocess
import time
import xml.etree.ElementTree as ET
from extract import extract, extract_dependencies

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
    common = ['-D__M64', '-DVERILATOR', '-D__M_OUT_LVL=4', f'-I{ROOT / "etc/libmdpi/include"}',
              f'-I{args.bambu_prefix.resolve() / "include/panda"}', f'-I{dpi}']
    results = []
    start = time.monotonic()
    error = None
    try:
        execute(['g++', '-std=c++14', *common, HERE / 'channel_capacity.cpp',
                 '-o', directory / 'channel_capacity'], directory)
        execute([directory / 'channel_capacity'], directory)
    except (OSError, subprocess.SubprocessError) as exception:
        error = str(exception)
    print('FAIL' if error else 'PASS', 'mdpi-channel-capacity', flush=True)
    results.append(('mdpi-channel-capacity', time.monotonic()-start, error, directory))
    start = time.monotonic()
    error = None
    try:
        execute(['g++', '-std=c++20', *common, '-DHAVE_RAMULATOR2',
                 f'-I{prefix / "include/ramulator"}', f'-I{prefix / "include"}',
                 ROOT / 'etc/libmdpi/mdpi_driver.cpp', HERE / 'host.cpp',
                 f'-L{libdir}', f'-Wl,-rpath,{libdir}', '-lramulator', '-lpthread',
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
    print('FAIL' if error else 'PASS', 'mdpi-acceptance', flush=True)
    results.append(('mdpi-acceptance', time.monotonic()-start, error, directory))
    if not error:
        for concurrent in (False, True):
            name = 'mdpi-simulator-first-shutdown' + ('-concurrent' if concurrent else '')
            shutdown_dir = directory / name
            shutdown_dir.mkdir()
            start = time.monotonic()
            try:
                host = directory / ('host_concurrent' if concurrent else 'host')
                if concurrent:
                    execute(['g++', '-std=c++20', *common, '-DHAVE_RAMULATOR2', '-DBAMBU_CONCURRENT_COSIM',
                             f'-I{prefix / "include/ramulator"}', f'-I{prefix / "include"}',
                             ROOT / 'etc/libmdpi/mdpi_driver.cpp', HERE / 'host.cpp',
                             f'-L{libdir}', f'-Wl,-rpath,{libdir}', '-lramulator', '-lpthread',
                             '-o', host], shutdown_dir)
                shutdown_env = env.copy()
                shutdown_env['MDPI_TEST_WAIT_SIM_EXIT'] = '1'
                execute([host], shutdown_dir, env=shutdown_env, timeout=60)
            except (OSError, subprocess.SubprocessError) as exception:
                error = str(exception)
            print('FAIL' if error else 'PASS', name, flush=True)
            results.append((name, time.monotonic()-start, error, shutdown_dir))
            if error:
                break
    if not error:
        clients = {}
        for name in ('memory_contract', 'clock_contract', 'plain_contract', 'completion_error'):
            clients[name] = directory / name
            execute(['gcc', '-std=c11', '-D_GNU_SOURCE', *common, ROOT / 'etc/libmdpi/mdpi.c',
                     HERE / f'{name}.c', '-o', clients[name]], directory)

        def contract(name, client, updates=None, config=None, expected_ratio=None, plain=False, failure=False,
                     host=None, diagnostic=None):
            case_dir = directory / name
            case_dir.mkdir()
            start = time.monotonic()
            error = None
            case_env = env.copy()
            case_env.pop('RAMULATOR_OVERRIDE_CHANNELS', None)
            case_env['BAMBU_IPC_SIM_CMD'] = str(clients[client])
            if updates:
                case_env.update(updates)
            if config:
                case_env['RAMULATOR2_CONFIG'] = str(config)
            try:
                try:
                    execute([host or directory / 'host'], case_dir, env=case_env, timeout=30)
                except subprocess.CalledProcessError:
                    if not failure:
                        raise
                else:
                    if failure:
                        raise ValueError('Invalid configuration unexpectedly succeeded')
                log = (case_dir / 'run.log').read_text(errors='replace')
                if failure and (diagnostic or 'Ramulator2 initialization failed') not in log:
                    raise ValueError('Missing expected initialization diagnostic')
                if diagnostic and diagnostic not in log:
                    raise ValueError(f'Missing expected diagnostic: {diagnostic}')
                if plain and 'Ramulator2 successfully initialized' in log:
                    raise ValueError('Plain memory unexpectedly initialized Ramulator')
                if expected_ratio is not None:
                    ticks = re.findall(r'CYCLES=(\d+),DRAM_TICKS=(\d+)', log)
                    expected = int(Fraction(str(expected_ratio))*2000)
                    if ticks != [('2000', str(expected)), ('2000', str(expected))]:
                        raise ValueError(f'Incorrect independent clock conversion: {ticks}, expected {expected}')
            except (OSError, subprocess.SubprocessError, ValueError) as exception:
                error = str(exception)
            print('FAIL' if error else 'PASS', name, flush=True)
            results.append((name, time.monotonic()-start, error, case_dir))

        contract('memory-contract', 'memory_contract')
        contract('completion-null-buffer', 'completion_error', updates={'MDPI_TEST_NULL_COMPLETION': '1'},
                 failure=True, diagnostic='Null data buffer for Ramulator completion polling')
        contract('completion-invalid-interface', 'completion_error', failure=True,
                 diagnostic='Ramulator completion polling failed')
        contract('completion-plain-interface', 'completion_error', updates={'MDPI_TEST_PLAIN': '1'},
                 failure=True, diagnostic='Ramulator completion polling failed')
        contract('plain-with-library', 'plain_contract', updates={'MDPI_TEST_PLAIN': '1'},
                 config=directory / 'missing.yaml', plain=True)
        execute(['g++', '-std=c++14', '-fno-exceptions', *common, ROOT / 'etc/libmdpi/mdpi_driver.cpp',
                 HERE / 'host.cpp', '-lpthread', '-ldl', '-o', directory / 'host_plain'], directory)
        contract('plain-without-library', 'plain_contract', updates={'MDPI_TEST_PLAIN': '1'},
                 plain=True, host=directory / 'host_plain')
        contract('ramulator-without-library', 'clock_contract', host=directory / 'host_plain',
                 failure=True, diagnostic='MDPI driver has no Ramulator support')
        default = (ROOT / 'etc/libmdpi/configs/default_ddr4.yaml').read_text()
        # DDR4-1600L has tCK=1.25 ns. Derive all expected ratios from physical
        # periods and compare each read's delivery cycle with a 1:1 reference.
        clock_config = directory / 'clock.yaml'
        clock_config.write_text(default.replace('preset: DDR4_2400R', 'preset: DDR4_1600L'))
        contract('clock-reference', 'clock_contract', config=clock_config, expected_ratio='1',
                 updates={'MDPI_TEST_MEMORIES': '2', 'MDPI_TEST_PERIOD_NS': '1.25'})
        reference = re.search(r'CLOCK_READ_CYCLE=(\d+)', (directory / 'clock-reference/run.log').read_text())
        if not reference:
            raise ValueError('Missing reference read completion cycle')
        reference_ticks = int(reference[1])
        for ratio in ('0.25', '0.3', '1.5', '3.7', '6.5'):
            exact_ratio = Fraction(ratio)
            delivery = (reference_ticks * exact_ratio.denominator + exact_ratio.numerator - 1) // exact_ratio.numerator
            contract(f'clock-{ratio}', 'clock_contract', config=clock_config, expected_ratio=ratio,
                     updates={'MDPI_TEST_MEMORIES': '2',
                              'MDPI_TEST_PERIOD_NS': str(float(Fraction('1.25') * exact_ratio)),
                              'MDPI_TEST_READ_CYCLE': str(delivery)})
        contract('clock-repeating', 'clock_contract', expected_ratio=Fraction(10000, 833),
                 updates={'MDPI_TEST_MEMORIES': '2', 'MDPI_TEST_PERIOD_NS': '10'})
        contract('clock-jumps', 'clock_contract', config=clock_config, expected_ratio='0.3',
                 updates={'MDPI_TEST_MEMORIES': '2', 'MDPI_TEST_PERIOD_NS': '0.375',
                          'MDPI_TEST_CLOCK_JUMPS': '1'})
        # Legacy YAML ratios must neither rescale the model nor reject fractional
        # values: only the accelerator period and the model's tCK drive time.
        for ratio in ('1', '5', '0.3'):
            config = directory / f'legacy-ratio-{ratio}.yaml'
            config.write_text(re.sub(r'  clock_ratio:.*', f'  clock_ratio: {ratio}', clock_config.read_text()))
            contract(f'legacy-ratio-{ratio}', 'clock_contract', config=config, expected_ratio='8',
                     updates={'MDPI_TEST_MEMORIES': '2', 'MDPI_TEST_PERIOD_NS': '10'})
        execute(['g++', '-std=c++20', f'-I{prefix / "include/ramulator"}', f'-I{prefix / "include"}',
                 HERE / 'row_policy.cpp', f'-L{libdir}', f'-Wl,-rpath,{libdir}', '-lramulator',
                 '-o', directory / 'row_policy'], directory)
        for config in sorted((ROOT / 'etc/libmdpi/configs').glob('*.yaml')):
            contract(f'preset-{config.stem}', 'clock_contract', config=config)
            if config.stem == 'default_ddr4' or re.search(r'impl: DDR[35]\b', config.read_text()):
                policies = ['ClosedRowPolicy']
                if config.stem == 'ddr3_1066G_ratio5':
                    policies.append('OpenRowPolicy')
                for policy in policies:
                    name = f'row-policy-{config.stem}-{policy}'
                    case_dir = directory / name
                    case_dir.mkdir()
                    start = time.monotonic()
                    error = None
                    try:
                        execute([directory / 'row_policy', config, case_dir / 'commands.csv', policy],
                                case_dir, timeout=30)
                    except (OSError, subprocess.SubprocessError) as exception:
                        error = str(exception)
                    print('FAIL' if error else 'PASS', name, flush=True)
                    results.append((name, time.monotonic()-start, error, case_dir))
        execute(['g++', '-std=c++20', f'-I{prefix / "include"}', HERE / 'native_profile.cpp',
                 f'-L{libdir}', f'-Wl,-rpath,{libdir}', '-lramulator',
                 '-o', directory / 'native_profile'], directory)
        for board, gib in (('alveo-u250',64), ('alveo-u280',8), ('alveo-u55c',16)):
            profile = ROOT / 'etc/libmdpi/configs/boards' / (board + '.yaml')
            contract('native-' + board, 'clock_contract', config=profile)
            name = 'mapping-' + board
            case_dir = directory / name
            case_dir.mkdir()
            start = time.monotonic()
            error = None
            try:
                execute([directory / 'native_profile', profile, str(gib * 1024**3)], case_dir)
            except (OSError, subprocess.SubprocessError) as exception:
                error = str(exception)
            print('FAIL' if error else 'PASS', name, flush=True)
            results.append((name, time.monotonic()-start, error, case_dir))
        contract('invalid-channel-override', 'clock_contract', config=ROOT / 'etc/libmdpi/configs/default_ddr4.yaml',
                 updates={'RAMULATOR_OVERRIDE_CHANNELS': 'invalid'}, failure=True,
                 diagnostic='RAMULATOR_OVERRIDE_CHANNELS must be 0 or 1')
        include_dir = directory / 'includes'
        include_dir.mkdir()
        (include_dir / 'dram.yaml').write_text('impl: DDR4\norg: {preset: DDR4_8Gb_x8, channel: 1, rank: 1}\n'
                                               'timing: {preset: DDR4_2400R}\n')
        included = re.sub(r'  DRAM:\n.*?  Controller:', '  DRAM: !include dram.yaml\n  Controller:', default, flags=re.S)
        (include_dir / 'root.yaml').write_text(included)
        contract('relative-include', 'clock_contract', config=include_dir / 'root.yaml')
        contract('pre-ready-clock', 'clock_contract', updates={'MDPI_TEST_PREREADY_CLOCK': '1'})
        contract('missing-config', 'clock_contract', config=directory / 'missing.yaml', failure=True)
        contract('missing-config-before-ready', 'clock_contract', config=directory / 'missing.yaml', failure=True,
                 updates={'MDPI_TEST_DELAY_READY': '1'})
        for period in ('0', '-1', 'nan', 'inf', '1e-10', '1e100'):
            contract(f'invalid-period-{period}', 'clock_contract',
                     updates={'MDPI_TEST_PERIOD_NS': period}, failure=True)
        missing_tck = directory / 'missing-tck.yaml'
        missing_tck.write_text('Frontend: {impl: GEM5}\nMemorySystem: {impl: DummyMemorySystem}\n')
        contract('missing-dram-tck', 'clock_contract', config=missing_tck, failure=True,
                 diagnostic='Ramulator must provide a finite positive DRAM tCK')
        (include_dir / 'cycle.yaml').write_text('!include cycle.yaml\n')
        contract('cyclic-include', 'clock_contract', config=include_dir / 'cycle.yaml', failure=True)
    if not error and not args.contract_only:
        models = directory / 'models.sv'
        models.write_text('typedef longint unsigned ptr_t;\n' + extract('TestbenchMEMRamulatorIntegrated') +
                          extract('TestbenchMEMAXIRamulatorIntegrated') + extract('if_utils') + extract('TestbenchMEMAXI') +
                          extract('TestbenchMEMMinimal') +
                          extract_dependencies('Arbiter_connection_interface'))
        execute(['gcc', '-c', '-std=c11', '-D_GNU_SOURCE', *common, ROOT / 'etc/libmdpi/mdpi.c',
                 '-o', directory / 'mdpi.o'], directory)
        for model in ('memory', 'memory_delays', 'classic_memory', 'axi', 'axi_lanes', 'classic_axi_lanes', 'arbiter'):
            if model == 'classic_memory':
                depths = (1, 2, 3, 4, 64)
            elif model == 'memory':
                depths = (1, 2, 4, 64)
            elif model == 'memory_delays' or 'lanes' in model:
                depths = (1, 4)
            else:
                depths = (1, 2, 4)
            for depth in depths:
                name = f'{model}-depth{depth}'
                case_dir = directory / name
                case_dir.mkdir()
                start = time.monotonic()
                error = None
                try:
                    fixture = ('axi_lanes' if 'lanes' in model else
                               'memory' if model in ('memory_delays', 'classic_memory') else model)
                    defines = ['-DCLASSIC_AXI'] if model == 'classic_axi_lanes' else []
                    if model == 'classic_memory':
                        defines += ['-DCLASSIC_MINIMAL']
                    if model == 'memory_delays':
                        defines += ['-GFORWARD=5', '-GRETURN=7']
                    execute([str(verilator), *defines, '--binary', '--timing', '-Wno-fatal', '--top-module', f'tb_{fixture}',
                             f'-GDEPTH={depth}', '--Mdir', case_dir / 'obj', models,
                             HERE / 'ramulator_test_clock.sv', HERE / f'tb_{fixture}.sv', '-LDFLAGS', directory / 'mdpi.o'], case_dir)
                    case_env = env.copy()
                    case_env['BAMBU_IPC_SIM_CMD'] = str(case_dir / f'obj/Vtb_{fixture}')
                    if model in ('classic_axi_lanes', 'classic_memory'):
                        case_env['MDPI_TEST_PLAIN'] = '1'
                    execute([directory / 'host'], case_dir, env=case_env, timeout=60)
                except (OSError, subprocess.SubprocessError) as exception:
                    error = str(exception)
                print('FAIL' if error else 'PASS', name, flush=True)
                results.append((name, time.monotonic()-start, error, case_dir))
    if not args.contract_only and (directory / 'mdpi.o').exists():
        for axi in (0, 1):
            for delays in ((0, 0, 0, 0), (1, 1, 1, 1), (2, 4, 3, 5), (5, 2, 4, 2), (3, 3, 7, 7)):
                name = 'transport-' + str(axi) + '-' + '-'.join(map(str, delays))
                case_dir = directory / name
                case_dir.mkdir()
                start = time.monotonic()
                error = None
                try:
                    params = (['-GRESET_INFLIGHT=1'] if delays == (3, 3, 7, 7) else []) + [f'-G{k}={v}' for k, v in zip(('RF','WF','RR','WR'), delays)]
                    execute([str(verilator), '--binary', '--timing', '-Wno-fatal', '--top-module', 'tb_transport',
                             f'-GAXI={axi}', *params, '--Mdir', case_dir / 'obj', directory / 'models.sv',
                             HERE / 'tb_transport.sv', '-LDFLAGS', directory / 'mdpi.o'], case_dir)
                    case_env = env.copy()
                    case_env['BAMBU_IPC_SIM_CMD'] = str(case_dir / 'obj/Vtb_transport')
                    execute([directory / 'host'], case_dir, env=case_env, timeout=60)
                except (OSError, subprocess.SubprocessError) as exception:
                    error = str(exception)
                print('FAIL' if error else 'PASS', name, flush=True)
                results.append((name, time.monotonic()-start, error, case_dir))
    if not args.contract_only:
        adapters = directory / 'adapters.sv'
        adapters.write_text(extract('MinimalAXI4Adapter', 'NC_AXI_IPs.xml') +
                            extract('MinimalAXI4AdapterSingleBeat', 'NC_AXI_IPs.xml'))
        for tagged in (0, 1):
            for width in (8, 32, 64):
                name = f'axi-adapter-lanes-{tagged}-{width}'
                case_dir = directory / name
                case_dir.mkdir()
                start = time.monotonic()
                error = None
                try:
                    execute([str(verilator), '--binary', '--timing', '-Wno-fatal', '--top-module', 'tb_axi_adapter_lanes',
                             f'-GTAGGED={tagged}', f'-GWIDTH={width}', '--Mdir', case_dir / 'obj', adapters,
                             HERE / 'tb_axi_adapter_lanes.sv'], case_dir)
                    execute([case_dir / 'obj/Vtb_axi_adapter_lanes'], case_dir, timeout=60)
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
