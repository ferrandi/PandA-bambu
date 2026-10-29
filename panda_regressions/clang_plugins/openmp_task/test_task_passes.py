#!/usr/bin/env python3
"""Exercise task pass memory semantics and unsupported-input diagnostics.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
Copyright (C) 2026 Politecnico di Milano
"""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--opt', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--plugin-dir', required=True, type=Path)
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    args.plugin_dir = args.plugin_dir.resolve()
    metadata = '\n!llvm.ident = !{!0}\n!0 = !{!"clang version 19.1.7"}\n'
    with tempfile.TemporaryDirectory(prefix='bambu-task-passes-') as directory:
        work = Path(directory)

        def transform(name, source, pipeline, expected_error=None):
            source_file = work / (name + '.ll')
            output = work / (name + '.out.ll')
            source_file.write_text(source)
            plugin = {'deduplicateLoadInst': 'opt_deduplicateLoadInst.so',
                      'openmpTask': 'opt_openmpTask.so'}[pipeline]
            result = subprocess.run(
                [args.opt, '-load-pass-plugin=' + str(args.plugin_dir / plugin),
                 '-passes=' + pipeline, '-verify-each', '-S', str(source_file), '-o', str(output)],
                cwd=work, text=True, capture_output=True)
            if expected_error:
                if result.returncode == 0 or expected_error not in result.stderr:
                    raise RuntimeError(f'{name}: missing diagnostic {expected_error!r}\n{result.stderr}')
            elif result.returncode:
                raise RuntimeError(f'{name}: {result.stderr}')
            return output

        preamble = '''target datalayout = "e-p:32:32"
            define void @__kmpc_omp_task() memory(none) { ret void }
            define void @clobber(ptr %p) { store i32 7, ptr %p ret void }
        '''
        # Observe results after stores, calls, and at control-flow joins, rather
        # than asserting a particular implementation or number of removed loads.
        for name, middle, expected in [
            ('same_memory', '', 2),
            ('store_clobber', 'store i32 7, ptr %p', 8),
            ('call_clobber', 'call void @clobber(ptr %p)', 8),
        ]:
            source = preamble + f'''
                define i32 @kernel.omp_outlined(ptr %p) {{
                    call void @__kmpc_omp_task()
                    %a = load i32, ptr %p
                    {middle}
                    %b = load i32, ptr %p
                    %sum = add i32 %a, %b
                    ret i32 %sum
                }}
                define i32 @main() {{
                    %p = alloca i32
                    store i32 1, ptr %p
                    %result = call i32 @kernel.omp_outlined(ptr %p)
                    %ok = icmp eq i32 %result, {expected}
                    %exit = select i1 %ok, i32 0, i32 1
                    ret i32 %exit
                }}
            '''
            output = transform(name, source, 'deduplicateLoadInst')
            exe = work / name
            # Execute natively even on hosts that cannot run 32-bit processes.
            # This fixture uses only i32 values and opaque pointer arguments.
            native_ir = work / (name + '.native.ll')
            native_ir.write_text(output.read_text().replace('target datalayout = "e-p:32:32"', ''))
            subprocess.run([args.clang, str(native_ir), '-o', str(exe)],
                           check=True, capture_output=True)
            subprocess.run([str(exe)], check=True)

        transform('dominance', preamble + '''
            define i32 @kernel.omp_outlined(ptr %p, i1 %condition) {
                br i1 %condition, label %left, label %right
            left:
                call void @__kmpc_omp_task()
                %a = load i32, ptr %p
                br label %join
            right:
                br label %join
            join:
                call void @__kmpc_omp_task()
                %b = load i32, ptr %p
                ret i32 %b
            }
        ''', 'deduplicateLoadInst')
        for name, source, message in [
            ('pointer_width', 'target datalayout = "e-p:64:64"\n', 'requires 32-bit pointers'),
            ('taskwait', '''target datalayout = "e-p:32:32"
                declare void @__kmpc_omp_taskwait()
                define void @f() { call void @__kmpc_omp_taskwait() ret void }
            ''', 'does not support __kmpc_omp_taskwait'),
            ('indirect_call', '''target datalayout = "e-p:32:32"
                define void @kernel.omp_outlined(ptr %callback) {
                    call void %callback()
                    ret void
                }
            ''', 'does not support indirect calls'),
        ]:
            transform(name, source + metadata, 'openmpTask', message)
        root = Path(__file__).resolve().parents[3]
        config = Path(args.opt).with_name('llvm-config-19')
        flags = shlex.split(subprocess.check_output(
            [str(config), '--cxxflags', '--ldflags', '--libs', 'support', '--system-libs'], text=True))
        topology = work / 'topology'
        subprocess.run([args.cxx, '-DPANDA_CLANG_MAJOR=19', '-DNDEBUG',
                        '-I' + str(root / 'etc/clang_plugin'), '-I' + str(root / 'ext/pugixml/src'),
                        str(Path(__file__).with_name('topology.cpp')), '-o', str(topology), *flags], check=True)
        subprocess.run([str(topology)], check=True)
        cyclic = subprocess.run([str(topology), 'cycle'], capture_output=True, text=True)
        if cyclic.returncode == 0 or 'contains a cycle' not in cyclic.stderr:
            raise RuntimeError('Cyclic task graph was not rejected with assertions disabled')
    print('Task pass regressions passed (memory, dominance, cycles, unsupported inputs)')


if __name__ == '__main__':
    main()
