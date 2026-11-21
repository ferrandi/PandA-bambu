#!/usr/bin/env python3
"""Check the OpenMP parallel-loop pass with opaque-pointer LLVM IR."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run(command, **kwargs):
    return subprocess.run(command, text=True, capture_output=True, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--opt', required=True)
    parser.add_argument('--plugin', required=True, type=Path)
    args = parser.parse_args()
    args.plugin = args.plugin.resolve()
    root = Path(__file__).resolve().parents[3]
    scratch = root / 'documentation' / 'tmp'
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='openmp-parallel-', dir=scratch) as tmp:
        work = Path(tmp)
        env = dict(os.environ, TMPDIR=tmp, TMP=tmp, TEMP=tmp)

        def compile_and_run(name, body, debug=False, reduction=False):
            source = work / f'{name}.c'
            raw_ir = work / f'{name}.raw.ll'
            out_ir = work / f'{name}.ll'
            source.write_text(body)
            cmd = [args.clang, '-fopenmp', '-O1', '-S', '-emit-llvm', str(source), '-o', str(raw_ir)]
            if debug:
                cmd.insert(1, '-g')
            run(cmd, cwd=work, env=env)
            if reduction:
                input_ir = raw_ir.read_text()
                array = re.search(r'^(\s*)(%[\w.]+) = alloca \[1 x ptr\]', input_ir, re.M)
                if not array:
                    raise RuntimeError(f'{name}: pointer-array alloca missing from IR')
                slot = re.compile(r'^(\s*)store ptr (%[\w.]+), ptr ' + re.escape(array.group(2)) + r'(,.*)$', re.M)
                input_ir, count = slot.subn(
                    rf'\1%bambu_reduce_slot = getelementptr [1 x ptr], ptr {array.group(2)}, i64 0, i64 0\n'
                    rf'\1store ptr \2, ptr %bambu_reduce_slot\3', input_ir, count=1)
                if count != 1:
                    raise RuntimeError(f'{name}: pointer-array store missing from IR')
                raw_ir.write_text(input_ir)
            result = subprocess.run(
                [args.opt, '-load-pass-plugin=' + str(args.plugin), '-passes=openmpBambu',
                 '-verify-each', '-S', str(raw_ir), '-o', str(out_ir)],
                cwd=work, env=env, text=True, capture_output=True)
            if reduction:
                input_ir = raw_ir.read_text()
                if not re.search(r'call\b[^\n]*@__kmp_bambu_set_th_local_reduce_data', input_ir):
                    raise RuntimeError(f'{name}: IR does not reach the reduce-data runtime hook')
                expected = 'cannot promote the local reduce data with opaque pointers'
                if result.returncode <= 0 or expected not in result.stderr:
                    raise RuntimeError(f'{name}: expected diagnostic {expected!r}, exit={result.returncode}\n'
                                       f'{result.stderr}\nInput IR:\n{input_ir}')
                if any(marker in result.stderr for marker in ('PLEASE submit a bug report', 'Stack dump:')):
                    raise RuntimeError(f'{name}: unsupported input reported as an LLVM crash\n{result.stderr}')
                return input_ir, []
            if result.returncode:
                raise RuntimeError(f'{name}: opt failed\n{result.stderr}')
            ir = out_ir.read_text()
            outlined = re.search(r'^(define\b[^@]*@[^\s(]*\.omp_outlined[^\n]*\{\n)(.*?)^}', ir, re.M | re.S)
            if not outlined or not outlined.group(2).strip():
                raise RuntimeError(f'{name}: outlined function body missing or empty')
            calls = re.findall(r'^\s*(?:%[^=]+ = )?call\b[^\n]*?@([^\s(]+)\(', ir, re.M)
            if '__kmpc_fork_call' in calls or '__kmp_bambu_fork_call' not in calls:
                raise RuntimeError(f'{name}: fork call was not rewritten: {calls}')
            if '__kmp_bambu_t_nproc' in calls:
                raise RuntimeError(f'{name}: thread-count intrinsic call remains')
            return ir, calls

        source = '''extern void __kmp_bambu_th_set_nproc(int, int);
extern int __kmp_bambu_t_nproc(void);
void kernel(int *a, int n) {
  int i;
  __kmp_bambu_th_set_nproc(0, 2);
#pragma omp parallel for num_threads(2)
  for (i = 0; i < n; ++i) a[i] = __kmp_bambu_t_nproc();
}
'''
        ir, calls = compile_and_run('parallel', source)
        outlined = re.search(r'^(define\b[^@]*@[^\s(]*\.omp_outlined[^\n]*\{\n)(.*?)^}', ir, re.M | re.S)
        if not outlined or re.search(r'call\b[^\n]*@__kmp_bambu_t_nproc', outlined.group(2)):
            raise RuntimeError('parallel: source nproc query was not replaced by the requested thread count')
        if not re.search(r'store i32 2, ptr', outlined.group(2)):
            raise RuntimeError('parallel: expected propagated thread count 2 is not observable')
        compile_and_run('parallel_debug', '''void kernel(int *a, int n) {
  int i;
#pragma omp parallel for num_threads(2)
  for (i = 0; i < n; ++i) a[i] = i;
}
        ''', debug=True)
        reduction_source = '''extern void __kmp_bambu_set_th_local_reduce_data(int, void *);
void kernel(int *a, int n) {
#pragma omp parallel num_threads(2)
  {
    int value = a[0] + a[n - 1];
    int *local[1];
    local[0] = &value;
    __kmp_bambu_set_th_local_reduce_data(0, local);
  }
}
'''
        compile_and_run('reduction', reduction_source, reduction=True)
    print('OpenMP parallel pass regressions passed')


if __name__ == '__main__':
    main()
