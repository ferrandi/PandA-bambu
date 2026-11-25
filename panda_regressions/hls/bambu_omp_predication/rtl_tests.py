#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0
"""Exercise the actual OpenMP library RTL with Verilator in all reset profiles."""
from pathlib import Path
import html
import re
import subprocess
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def run_rtl(out):
    xml_path = ROOT / 'etc/libtech/C_OPENMP_IPs.xml'
    modules = ET.parse(xml_path).findall('.//module_o')
    raw_cells = re.findall(r'<cell>.*?</cell>', xml_path.read_text(), re.S)
    names = ('CRITICAL', 'END_CRITICAL', 'BARRIER_REACHED', 'WAIT_ALL_THREADS', 'CS_WAIT_ALL_THREADS',
             'SET_TH_LOCAL_REDUCE_DATA', 'GET_TH_LOCAL_REDUCE_DATA')
    for asynchronous in (False, True):
        for active_low in (False, True):
            directory = out / f'rtl-{int(asynchronous)}-{int(active_low)}'
            directory.mkdir(parents=True, exist_ok=False)
            source = []
            for name in names:
                name = f'KMP_BAMBU_{name}_FU'
                module = next(m for m in modules if m.get('id') == name)
                ports = []
                for port in module.findall('port_o'):
                    desc = port.find('structural_type_descriptor')
                    width = int(desc.get('vector_size', '1'))
                    if 'TH_LOCAL_REDUCE_DATA' in name and port.get('id') in (
                            'in1', 'in2', 'out1', 'kmp_bambu_set_th_local_reduce_data_val'):
                        width = 32
                    ports.append(f"{'input' if port.get('dir') == 'IN' else 'output'} wire [{width-1}:0] {port.get('id')}")
                # Preserve line breaks: XML attribute normalization would swallow // comments.
                cell = next(c for c in raw_cells if f'<name>{name}</name>' in c)
                body = html.unescape(re.search(r'VERILOG_PROVIDED="([^"]*)"', cell, re.S)[1])
                edge = 'or negedge reset' if active_low else 'or posedge reset'
                body = body.replace('1RESET_EDGE', edge if asynchronous else '')
                body = body.replace('1RESET_VALUE', f"reset == 1'b{int(not active_low)}")
                body = body.replace('1INIT_ZERO_VALUE', "= 1'b0")
                body = body.replace('BITSIZE_selector_register_file', '1')
                body = body.replace('BITSIZE_in2', '32')
                body = body.replace('CONTEXT_BIT_START', '0').replace('CONTEXT_BIT_END', '0')
                assert not re.search(r'1[A-Z_]+', body)
                source.append(f'module {name}({", ".join(ports)});\n{body}\nendmodule\n')
            (directory / 'units.sv').write_text('\n'.join(source))
            with (directory / 'run.log').open('w') as log:
                subprocess.run(['verilator', '--cc', '--exe', '-Wno-fatal', '--top-module', 'rtl_test',
                                '-CFLAGS', f'-DACTIVE_LOW={int(active_low)}',
                                '--Mdir', str(directory / 'obj'), str(HERE / 'rtl_test.sv'),
                                str(directory / 'units.sv'), str(HERE / 'rtl_driver.cpp')],
                               stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
                subprocess.run(['make', '-C', str(directory / 'obj'), '-f', 'Vrtl_test.mk', '-j2'],
                               stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
                subprocess.run([str(directory / 'obj/Vrtl_test')], stdout=log, stderr=subprocess.STDOUT,
                               check=True, timeout=15)
            print(f'PASS {directory.name}', flush=True)
