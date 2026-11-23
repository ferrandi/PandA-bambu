#!/usr/bin/env python3
"""Check natural OpenMP reduction handling for typed and opaque pointer IR."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


SOURCE = '''int kernel(int *a, int n) {
  int sum = 0;
#pragma omp parallel for reduction(+:sum) num_threads(2)
  for (int i = 0; i < n; ++i)
    sum += a[i];
  return sum;
}
'''


def run(command, **kwargs):
    return subprocess.run(command, text=True, capture_output=True, check=True, **kwargs)


def check_profile(args, pointer_mode):
    root = Path(__file__).resolve().parents[3]
    scratch = root / 'documentation' / 'tmp'
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f'openmp-reduction-{pointer_mode}-', dir=scratch) as tmp:
        work = Path(tmp)
        env = dict(os.environ, TMPDIR=tmp, TMP=tmp, TEMP=tmp)
        source = work / 'reduction.c'
        reduction_bc = work / 'reduction.bc'
        runtime_bc = work / 'runtime.bc'
        linked_bc = work / 'linked.bc'
        optimized_ir = work / 'optimized.ll'
        source.write_text(SOURCE)
        typed_flags = ['-Xclang', '-no-opaque-pointers'] if pointer_mode == 'typed' else []
        opaque_flags = ['-opaque-pointers=0'] if pointer_mode == 'typed' else []
        run([args.clang, '-fopenmp', '-O1', *typed_flags, '-emit-llvm', '-c', str(source), '-o', str(reduction_bc)],
            cwd=work, env=env)
        run([args.cxx, '-std=c++14', '-O1', *typed_flags, '-emit-llvm', '-c',
             str(root / 'etc' / 'libopenmp' / 'kmp_single_file.cpp'), '-o', str(runtime_bc)], cwd=work, env=env)
        run([args.llvm_link, *opaque_flags, str(reduction_bc), str(runtime_bc), '-o', str(linked_bc)],
            cwd=work, env=env)
        run([args.opt, *opaque_flags, '-passes=internalize,default<O2>', '--internalize-public-api-list=kernel',
             '-vectorize-loops=false', '-vectorize-slp=false', '-S', str(linked_bc), '-o', str(optimized_ir)],
            cwd=work, env=env)
        if '@__kmp_bambu_set_th_local_reduce_data' not in optimized_ir.read_text():
            raise RuntimeError(f'{pointer_mode}: optimized natural IR does not reach the reduction data hook')
        result = subprocess.run(
            [args.opt, *opaque_flags, '-load-pass-plugin=' + str(Path(args.plugin).resolve()),
             '-passes=default<O2>,openmpBambu', '-vectorize-loops=false', '-vectorize-slp=false',
             '-verify-each', str(optimized_ir), '-o', str(work / 'out.bc')],
            cwd=work, env=env, text=True, capture_output=True)
        if pointer_mode == 'typed':
            if result.returncode != 0:
                raise RuntimeError(f'typed: expected the Bambu typed-pointer reduction pipeline to pass, '
                                   f'exit={result.returncode}\n{result.stderr}')
            return
        expected = 'cannot promote local reduce data: expected a pointer value'
        if result.returncode <= 0 or expected not in result.stderr:
            raise RuntimeError(f'{pointer_mode}: expected diagnostic {expected!r}, '
                               f'exit={result.returncode}\n{result.stderr}')
        if any(marker in result.stderr for marker in ('PLEASE submit a bug report', 'Stack dump:')):
            raise RuntimeError(f'{pointer_mode}: unsupported input reported as an LLVM crash\n{result.stderr}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--llvm-link', required=True)
    parser.add_argument('--opt', required=True)
    parser.add_argument('--plugin', required=True)
    parser.add_argument('--pointer-mode', choices=('typed', 'opaque'), required=True)
    args = parser.parse_args()
    check_profile(args, args.pointer_mode)
    print(f'OpenMP reduction {args.pointer_mode}-pointer regression passed')


if __name__ == '__main__':
    main()
