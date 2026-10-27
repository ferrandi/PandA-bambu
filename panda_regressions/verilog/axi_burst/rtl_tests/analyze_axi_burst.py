#!/usr/bin/env python3
"""Compare burst-core LUT area/depth with ABC, optionally prove equivalence.

The baseline is an explicit XML snapshot or Git revision. LUT depth is a
technology-independent estimate, not a routed FPGA clock frequency. Each
profile has identical parameters and synthesis commands on both sides.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

from extract_write_engine import extract

ROOT = Path(__file__).resolve().parents[4]
XML = Path('etc/libtech/NC_AXI_IPs.xml')
TARGETS = ('MinimalAXI4MasterPipelined', 'MinimalAXI4MasterPipelinedWrite')


def run_tool(yosys, directory, name, script, timeout):
    (directory / f'{name}.ys').write_text(script)
    with (directory / f'{name}.log').open('w') as log:
        try:
            result = subprocess.run([yosys, '-Q', '-T', '-s', f'{name}.ys'],
                                    cwd=directory, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=timeout, check=False)
        except subprocess.TimeoutExpired:
            return 'timeout'
    return 'pass' if result.returncode == 0 else 'fail'


def abstract_payload_fifo(rtl, target):
    """Expose a FIFO's read address and clocked write command for composition.

    Equal initial memories, read addresses and write commands imply equal
    memories and read data at every cycle. Proving the surrounding control
    with a shared arbitrary read word therefore covers every payload value.
    Descriptor and planner arrays are still checked without abstraction.
    """
    data_width = 'BITSIZE_m_axi_wdata' if target.endswith('Write') else 'BITSIZE_m_axi_rdata'
    ptr_width = '((FIFO_DEPTH <= 2) ? 1 : $clog2(FIFO_DEPTH))'
    ports = (f",\n    input wire [{data_width}-1:0] fifo_model_read_data,"
             f"\n    output wire [{ptr_width}-1:0] fifo_model_read_address,"
             f"\n    output reg [{data_width}-1:0] fifo_model_write_data,"
             f"\n    output reg [{ptr_width}-1:0] fifo_model_write_address,"
             "\n    output reg fifo_model_write_enable\n);")
    rtl = rtl.replace('\n);', ports, 1)
    rtl, count = re.subn(r'\breg\s+\[(?:DATA_BITS|BITSIZE_in3)-1:0\]\s+fifo\s*\[0:FIFO_DEPTH-1\];', '', rtl)
    if count != 1:
        raise ValueError('expected one payload FIFO declaration')
    rtl = re.sub(r'\bfifo\s*\[fifo_rd_ptr\]', 'fifo_model_read_data', rtl)
    # Every source write uses the same current write pointer. Capture the
    # final command with the same procedural priority as the original writes.
    writes = list(re.finditer(r'\bfifo\s*\[fifo_wr_ptr\]\s*<=\s*([^;]+);', rtl))
    if not writes:
        raise ValueError('payload FIFO has no writes')
    process_starts = set()
    for match in writes:
        start = rtl.rfind('always @', 0, match.start())
        process_starts.add(rtl.index('begin', start) + len('begin'))
    edits = [(m.start(), m.end(), 'begin fifo_model_write_enable <= 1\'b1; '
              'fifo_model_write_address <= fifo_wr_ptr; fifo_model_write_data <= '
              + m[1] + '; end') for m in writes]
    for start in process_starts:
        edits.append((start, start, "\n    fifo_model_write_enable <= 1'b0;\n"
                      "    if (!reset) begin fifo_model_write_address <= 0; "
                      "fifo_model_write_data <= 0; end\n"))
    for start, end, replacement in sorted(edits, reverse=True):
        rtl = rtl[:start] + replacement + rtl[end:]
    rtl = rtl.replace('\nendmodule', '\nassign fifo_model_read_address = fifo_rd_ptr;\nendmodule')
    if re.search(r'\bfifo\s*\[', rtl):
        raise ValueError('unhandled payload FIFO access')
    return rtl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    baseline = parser.add_mutually_exclusive_group(required=True)
    baseline.add_argument('--baseline-ref', help='Git revision containing baseline XML')
    baseline.add_argument('--baseline-xml', type=Path)
    parser.add_argument('--candidate-xml', type=Path, default=ROOT / XML)
    parser.add_argument('--yosys', default='yosys')
    parser.add_argument('--out', type=Path, required=True, help='fresh artifact directory')
    parser.add_argument('--profiles', default='16,1,32;16,3,64;16,8,128;256,16,512',
                        help='semicolon-separated B_MAX,MAX_OUTSTANDING,FIFO_DEPTH triples')
    parser.add_argument('--abstract-fifo', action='store_true',
                        help='prove FIFO read/write commands with a shared arbitrary read word')
    parser.add_argument('--formal', action='store_true', help='also prove cycle equivalence')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--timeout', type=int, default=900, help='seconds per tool invocation')
    args = parser.parse_args()
    yosys = shutil.which(args.yosys)
    if not yosys:
        parser.error(f'Yosys not found: {args.yosys}')
    yosys = str(Path(yosys).resolve())
    if args.jobs < 1 or args.timeout < 1:
        parser.error('jobs and timeout must be positive')
    try:
        profiles = [tuple(map(int, part.split(','))) for part in args.profiles.split(';')]
        if any(len(profile) != 3 for profile in profiles):
            raise ValueError
    except ValueError:
        parser.error('profiles must be B_MAX,MAX_OUTSTANDING,FIFO_DEPTH triples')
    output = args.out.resolve()
    output.mkdir(parents=True, exist_ok=False)
    if args.baseline_ref:
        content = subprocess.check_output(['git', 'show', f'{args.baseline_ref}:{XML}'], cwd=ROOT)
    else:
        content = args.baseline_xml.read_bytes()
    (output / 'baseline.xml').write_bytes(content)
    (output / 'candidate.xml').write_bytes(args.candidate_xml.read_bytes())
    metadata = {
        'baseline_ref': args.baseline_ref,
        'baseline_sha256': hashlib.sha256(content).hexdigest(),
        'candidate_sha256': hashlib.sha256((output / 'candidate.xml').read_bytes()).hexdigest(),
        'yosys': subprocess.check_output([yosys, '-V'], text=True).strip(),
        'lut_inputs': 6, 'profiles': profiles,
        'formal': args.formal, 'abstract_payload_fifo': args.abstract_fifo, 'formal_peer_fault_enable': 1,
        'reset': 'synchronous active-low',
        'metric_limit': 'LUT depth excludes routing, fanout and device-specific carry/RAM delays',
    }
    (output / 'provenance.json').write_text(json.dumps(metadata, indent=2) + '\n')
    tasks = [(target, profile) for target in TARGETS for profile in profiles]

    def analyze(task):
        target, (burst, outstanding, depth) = task
        directory = output / f'{target}-B{burst}-O{outstanding}-D{depth}'
        directory.mkdir()
        row = {'target': target, 'burst': burst, 'outstanding': outstanding, 'fifo_depth': depth}
        rtl = {}
        for version in ('baseline', 'candidate'):
            rtl[version] = extract(output / f'{version}.xml', target)
            (directory / f'{version}.v').write_text(rtl[version])
            setup = (f'chparam -set B_MAX {burst} -set MAX_OUTSTANDING {outstanding} '
                     f'-set FIFO_DEPTH {depth} {target}; ')
            script = (f'read_verilog -sv {version}.v; {setup}'
                      f'synth -top {target} -flatten -lut 6; stat; ltp -noff; '
                      f'write_json {version}-mapped.json')
            status = run_tool(yosys, directory, version, script, args.timeout)
            row[version] = {'status': status}
            if status == 'pass':
                data = json.loads((directory / f'{version}-mapped.json').read_text())
                counts = {}
                for cell in data['modules'][target]['cells'].values():
                    counts[cell['type']] = counts.get(cell['type'], 0) + 1
                log = (directory / f'{version}.log').read_text()
                row[version].update(luts=counts.get('$lut', 0), cells=counts,
                                    depth=int(re.findall(r'length=(\d+)', log)[-1]))
        if args.formal:
            # Align renamed storage only for state correspondence. Ports,
            # expressions and sequential logic stay exactly as extracted.
            baseline_regs = set(re.findall(r'\breg\s+(?:\[[^]]+\]\s*)?(\w+)', rtl['baseline']))
            rename = {name + '_reg': name for name in baseline_regs if not name.endswith('_reg')}
            proof_rtl = re.sub(r'\b[A-Za-z_]\w*\b', lambda m: rename.get(m[0], m[0]), rtl['candidate'])
            if args.abstract_fifo:
                proof_rtl = abstract_payload_fifo(proof_rtl, target)
                (directory / 'proof-baseline.v').write_text(abstract_payload_fifo(rtl['baseline'], target))
            else:
                (directory / 'proof-baseline.v').write_text(rtl['baseline'])
            (directory / 'proof-candidate.v').write_text(proof_rtl)
            row['proofs'] = {}
            for fast in ((0, 1) if target.endswith('Write') else (1,)):
                script = ''
                for version, source in (('gold', 'proof-baseline.v'), ('gate', 'proof-candidate.v')):
                    script += f'read_verilog -sv {source}; '
                    script += (f'chparam -set B_MAX {burst} -set MAX_OUTSTANDING {outstanding} '
                               f'-set FIFO_DEPTH {depth} -set PEER_FAULT_ENABLE 1 ')
                    if target.endswith('Write'):
                        script += f'-set FAST_STORE_ACK {fast} '
                    script += f'{target}; proc; memory_map; opt; rename {target} {version}; '
                    if version == 'gold':
                        script += 'design -stash baseline; '
                script += ('design -copy-from baseline -as gold gold; equiv_make gold gate equiv; '
                           'hierarchy -top equiv; equiv_simple -short -seq 1; '
                           'equiv_induct -seq 4; equiv_status -assert')
                row['proofs'][f'fast{fast}'] = run_tool(yosys, directory, f'proof-fast{fast}', script,
                                                       args.timeout)
        print(json.dumps(row), flush=True)
        return row

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        rows = list(pool.map(analyze, tasks))
    (output / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
    failures = any(row[version]['status'] != 'pass' for row in rows
                   for version in ('baseline', 'candidate'))
    failures |= any(status != 'pass' for row in rows for status in row.get('proofs', {}).values())
    return int(failures)


if __name__ == '__main__':
    sys.exit(main())
