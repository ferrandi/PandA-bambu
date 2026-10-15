#!/usr/bin/env python3
"""Check all three priority encoder families against the lowest-set-bit specification.

Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""
import argparse
import html
import json
import re
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET


FAMILY_PREFIXES = {'hierarchical': 'hierarchical_', 'tree': 'tree_', 'gates': ''}
WIDTHS = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]
ENCODER_NAME = r'(?:(?:hierarchical|tree)_)?multiple_priority_encoder_\d+_\d+'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verilator', default='verilator')
    parser.add_argument('--yosys', help='Also prove equivalence and the priority specification using Yosys SAT')
    parser.add_argument('--synthesis-report', type=Path,
                        help='Write a JSON comparison after 6-input LUT mapping (requires --yosys and ABC)')
    args = parser.parse_args()
    if args.synthesis_report and not args.yosys:
        parser.error('--synthesis-report requires --yosys')
    synthesis_results = []
    root = Path(__file__).resolve().parents[2]
    library_path = root / 'etc/libtech/NC_OPENMP_IPs.xml'
    library_text = library_path.read_text()
    library = ET.fromstring(library_text)
    # PandA's XML reader preserves literal newlines in RTL attributes. Standard
    # XML parsers normalize them, which can change // comments and very long lines.
    raw_rtl = {}
    for match in re.finditer(r'<cell>\s*<name>([^<]+)</name>(.*?)</cell>', library_text, re.S):
        body = re.search(r'VERILOG_PROVIDED="(.*?)"', match[2], re.S)
        if body:
            raw_rtl[match[1]] = html.unescape(body[1])
    modules = {}
    for cell in library.findall('.//cell'):
        name = cell.findtext('name')
        if 'multiple_priority_encoder_' not in name:
            continue
        if name in modules:
            raise RuntimeError(f'Duplicate encoder definition: {name}')
        width, bits = map(int, name.split('_')[-2:])
        rtl = raw_rtl[name]
        modules[name] = (width, bits, rtl)
    expected_modules = {prefix + f'multiple_priority_encoder_{width}_{max(1, (width-1).bit_length())}'
                        for prefix in FAMILY_PREFIXES.values() for width in WIDTHS}
    if set(modules) != expected_modules:
        raise RuntimeError(f'Incorrect encoder families: missing {expected_modules - set(modules)}, '
                           f'unexpected {set(modules) - expected_modules}')
    for name, (_, _, body) in modules.items():
        missing = set(re.findall(ENCODER_NAME, body)) - set(modules)
        if missing:
            raise RuntimeError(f'{name}: missing encoder dependencies {missing}')
    with tempfile.TemporaryDirectory(prefix='panda-priority-encoder-') as tmp:
        work = Path(tmp)
        rtl = ''
        checks = ''
        for name, (width, bits, body) in modules.items():
            rtl += (f'module {name}(input [{width-1}:0] i, output [{bits-1}:0] o, output empty);\n'
                    f'{body}\nendmodule\n')
            checks += (f'wire [{bits-1}:0] {name}_o; wire {name}_empty;\n'
                       f'{name} u_{name}(i[{width-1}:0], {name}_o, {name}_empty);\n')
        rtl += 'module testbench;\nreg [511:0] i;\n' + checks
        rtl += '''integer t, k, expected;
initial begin
  i = 0;
  for(t = 0; t < 8192; t = t + 1) begin
    if(t == 0) i = 0;
    else if(t <= 512) i = 512'b1 << (t-1);
    else if(t <= 1024) i = (512'b1 << (t-513)) | (512'b1 << ((t-513+17)%512));
    else begin
      for(k = 0; k < 16; k = k + 1) i[k*32+:32] = $urandom;
      // Exercise sparse inputs and priority across group boundaries too.
      i = i << (t % 512);
    end
    #1;
'''
        for name, (width, bits, _) in modules.items():
            rtl += (f'    expected = 0;\n'
                    f'    for(k = {width-1}; k >= 0; k = k - 1) if(i[k]) expected = k;\n'
                    f'    if({name}_o != {bits}\'(expected) || {name}_empty != (~|i[{width-1}:0]))\n'
                    f'      $fatal(1, "{name}: incorrect result for input %h", i[{width-1}:0]);\n')
        rtl += '  end\n  $display("All three encoder families passed all widths (1..512)");\n  $finish;\nend\nendmodule\n'
        (work / 'encoders.sv').write_text(rtl)
        command = [args.verilator, '--binary', '--timing', '-Wno-fatal', '--top-module', 'testbench',
                   '--Mdir', 'obj', '-j', '2', 'encoders.sv']
        result = subprocess.run(command, cwd=work, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            print(result.stdout)
            return result.returncode
        subprocess.run([str(work / 'obj/Vtestbench')], cwd=work, check=True)
        if args.yosys:
            for width in WIDTHS:
                bits = max(1, (width-1).bit_length())
                base = f'multiple_priority_encoder_{width}_{bits}'
                needed = {prefix + base for prefix in FAMILY_PREFIXES.values()}
                while True:
                    previous = len(needed)
                    for name in list(needed):
                        needed.update(re.findall(ENCODER_NAME, modules[name][2]))
                    if len(needed) == previous:
                        break
                formal_rtl = ''
                for name in sorted(needed):
                    n, b, body = modules[name]
                    formal_rtl += (f'module {name}(input [{n-1}:0] i, output [{b-1}:0] o, output empty);\n'
                                   f'{body}\nendmodule\n')
                (work / 'formal.v').write_text(formal_rtl + f'''
module miter(input [{width-1}:0] i, output ok);
wire [{bits-1}:0] hierarchical_index, tree_index, gates_index;
wire hierarchical_empty, tree_empty, gates_empty;
hierarchical_{base} hierarchical(i, hierarchical_index, hierarchical_empty);
tree_{base} tree(i, tree_index, tree_empty);
{base} gates(i, gates_index, gates_empty);
// An independent specification catches bugs shared by multiple implementations.
function [{bits-1}:0] reference_index;
input [{width-1}:0] value;
integer index;
begin
  reference_index = 0;
  for(index = {width-1}; index >= 0; index = index - 1)
    if(value[index]) reference_index = index;
end
endfunction
assign ok = (hierarchical_index == reference_index(i)) && (hierarchical_empty == (~|i)) &&
            (tree_index == reference_index(i)) && (tree_empty == (~|i)) &&
            (gates_index == reference_index(i)) && (gates_empty == (~|i));
endmodule
''')
                subprocess.run([args.yosys, '-Q', '-T', '-p',
                                'read_verilog -defer formal.v; hierarchy -top miter; proc; flatten; opt; '
                                'sat -verify -prove ok 1 -show-inputs -show-outputs'], cwd=work, check=True)
                if args.synthesis_report:
                    for family, prefix in FAMILY_PREFIXES.items():
                        module = prefix + base
                        command = (f'read_verilog -defer formal.v; synth -top {module} -flatten -lut 6; '
                                   'stat; ltp -noff')
                        result = subprocess.run([args.yosys, '-Q', '-T', '-p', command], cwd=work,
                                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                                check=True)
                        counts = re.findall(r'\$lut\s+(\d+)|(\d+)\s+\$lut', result.stdout)
                        depths = re.findall(r'length=(\d+)', result.stdout)
                        if not counts or not depths:
                            raise RuntimeError(f'Incomplete synthesis result for {module}:\n{result.stdout}')
                        count = next(value for value in counts[-1] if value)
                        synthesis_results.append(dict(width=width, family=family, luts=int(count),
                                                      lut_depth=int(depths[-1])))
            if args.synthesis_report:
                version = subprocess.check_output([args.yosys, '-V'], text=True).strip()
                args.synthesis_report.write_text(json.dumps(dict(yosys=version, lut_size=6,
                                                                 results=synthesis_results), indent=2) + '\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
