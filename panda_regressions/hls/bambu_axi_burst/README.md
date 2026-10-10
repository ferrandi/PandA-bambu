# Classic AXI burst regression

Run the complete suite through `../bambu_axi_burst.sh`. The script runs the
six Mantis cycle-gated sum cases, the boundary and negative compiler cases,
offline gate/metadata/export checks, focused compiler profile and fallback
suites, the configure-to-read virtual-SSA check, the standalone Verilator
burst-engine suite, and a generated O=1/D=256/B=16 kernel plus wrapper-RID
simulation. The wrapper-RID simulation reuses the RTL generated for the
kernel test.

The entry point intentionally does not run the broader `all` or `burst-type`
compiler-runner modes: they duplicate Mantis coverage.
By default all durable output is placed in `./out_bambu_axi_burst`, relative
to the caller's current directory. The script refuses to run if that path (or
an explicitly selected `-o/--output` path) already exists, including a
dangling symlink. Use a fresh path for each run. Mantis writes cases/report
under `<out_root>/mantis`; Mantis, the cycle gate, internal mock checks, and
auxiliary runners use `<out_root>/tmp` for temporary files. No run artifacts
are created outside `<out_root>`.

Standalone helper/test scripts do not choose a scratch location: pass a fresh
output directory to RTL runners and set `BURST_TEST_TMPDIR` to a directory
under the intended output root for mock tests.

Like sibling classic regressions, the default behavior cleans generated
intermediates after all gates pass, while preserving the `out_` root, report,
and canonical Mantis logs/results (`execution.log`, `timeout.log`,
`failure.log`, `return_value`, `bambu_results.xml`). On failure diagnostics
are retained. Pass `--no-clean` to retain successful-run intermediates too.
Mantis writes its configured cases under
`out_bambu_axi_burst/mantis/AXI-BURST/`; its suite report is in that Mantis
output directory.
Auxiliary job logs are in `<out_root>/jobs/` when outputs are retained with
`--no-clean` or after a failure; successful default-clean runs remove them.

Pass `-j N` or `--parallel N` (also `-jN`, `-j=N`, and `--parallel=N`) to set
the maximum parallelism; the default is the `J` environment value or 1. Mantis
`-o DIR` or `--output DIR` (including `--output=DIR`) selects another output
root; repeated output options use the last value.
Repeated parallelism options use their minimum value, matching Mantis. The
Mantis regressions run first using that limit internally. Afterwards, the
independent compiler suites, virtual-SSA check, Verilator engine suite, and
generated-kernel/wrapper chain share a bounded pool of at most N jobs. The
generated-kernel test and its wrapper-RID check remain one serial job because
the latter consumes the former's RTL. On failure, no more jobs are launched;
all already-started jobs are awaited and their logs are retained under
`<out_root>/jobs/`.
