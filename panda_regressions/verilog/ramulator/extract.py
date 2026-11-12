# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Extract the production library cells, preserving provided RTL line breaks."""
import html
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]


def extract(name):
    xml = (ROOT / 'etc/libtech/NC_TESTBENCH_IPs.xml').read_text()
    cell = next(c for c in re.findall(r'<cell>.*?</cell>', xml, re.S) if f'<name>{name}</name>' in c)
    module = ET.fromstring(cell).find('circuit/module_o')
    library = module.find('NP_functionality').get('LIBRARY').split()
    parameters = []
    for p in module.findall('parameter'):
        value = p.text.replace('""', '"')
        parameters.append(f'parameter {p.get("name")} = {value}')
    ports = []
    for port in module.findall('port_o'):
        ident = port.get('id')
        kind = port.find('structural_type_descriptor')
        width = int(kind.get('vector_size', '1'))
        if ident in library:
            parameters.append(f'parameter BITSIZE_{ident} = {width}')
            width = f'BITSIZE_{ident}'
        ports.append(f'{"input" if port.get("dir") == "IN" else "output"} wire [{width}-1:0] {ident}')
    body = html.unescape(re.search(r'VERILOG_PROVIDED="([^"]*)"', cell, re.S)[1])
    body = body.replace('1RESET_VALUE', 'reset == 1\'b1').replace('1RESET_EDGE', '').replace('1INIT_ZERO_VALUE', "= 1'b0")
    return f'module {name} #(\n' + ',\n'.join(parameters) + '\n)(\n' + ',\n'.join(ports) + '\n);\n' + body + '\nendmodule\n'


if __name__ == '__main__':
    import sys
    Path(sys.argv[1]).write_text('typedef longint unsigned ptr_t;\n' + extract('TestbenchMEMRamulatorIntegrated') + extract('TestbenchMEMAXIRamulatorIntegrated'))
