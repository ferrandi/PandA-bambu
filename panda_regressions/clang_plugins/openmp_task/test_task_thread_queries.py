#!/usr/bin/env python3
"""Check task thread-ID diagnostics and ordinary OpenMP thread-ID lowering.

Run with --bambu pointing to a built executable and its runtime environment set.
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bambu', required=True, type=Path)
    parser.add_argument('--ordinary-compiler', default='I386_CLANG13')
    args = parser.parse_args()
    bambu = str(args.bambu.resolve())
    diagnostic = 'OpenMP tasks do not support thread ID queries'
    with tempfile.TemporaryDirectory(prefix='bambu-task-thread-') as directory:
        root = Path(directory)
        for name, helpers, expression, task in [
            ('direct', '', 'omp_get_thread_num()', True),
            ('nested', '__attribute__((noinline)) int leaf() { return omp_get_thread_num(); }\n'
             '__attribute__((noinline)) int helper() { return leaf(); }', 'helper()', True),
            ('query_free', '__attribute__((noinline)) int helper() { return 7; }', 'helper()', True),
            ('ordinary', '', 'omp_get_thread_num()', False),
        ]:
            work = root / name
            work.mkdir()
            source = work / 'kernel.cpp'
            task_pragma = '#pragma omp task firstprivate(i) shared(data) depend(inout: data[i])' if task else ''
            source_text = f'''extern "C" int omp_get_thread_num();
{helpers}
#pragma HLS interface port=data mode=bus
void fn(int *data) {{
#pragma omp parallel num_threads(1) shared(data)
    {{
#pragma omp single
        {{
            for (unsigned i = 0; i < 2; ++i) {{
{task_pragma}
                {{ data[i] = data[i] + {expression}; }}
            }}
        }}
    }}
}}
'''
            if not task:
                source_text = '''extern "C" int omp_get_thread_num();
#pragma HLS interface port=data mode=bus
void fn(int *data) {
#pragma omp parallel num_threads(2) shared(data)
    {
        int tid = omp_get_thread_num();
        data[tid] = tid + 1;
    }
}
'''
            source.write_text(source_text)
            result = subprocess.run(
                [bambu, str(source), '--std=c++17', '-fopenmp',
                 '--compiler=' + ('I386_CLANG19' if task else args.ordinary_compiler),
                 '--top-fname=fn', '--channels-type=MEM_ACC_11',
                 '--memory-allocation-policy=NO_BRAM', '--generate-interface=INFER'],
                cwd=work, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
            reject = task and name != 'query_free'
            if reject:
                if result.returncode == 0 or diagnostic not in result.stdout:
                    raise RuntimeError(f'{name}: missing task query diagnostic\n{result.stdout}')
            elif result.returncode:
                raise RuntimeError(f'{name}: synthesis failed\n{result.stdout}')
            print(f'{name}: passed', flush=True)
    print('Task thread query regressions passed')


if __name__ == '__main__':
    main()
