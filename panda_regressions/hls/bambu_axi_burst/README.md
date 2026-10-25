# Classic AXI burst regression

Run the complete suite through `../bambu_axi_burst.sh`. The script runs the
six Mantis cycle-gated sum cases, the boundary and negative compiler cases,
offline gate/metadata/export/logging checks, focused compiler profile and fallback
suites, ModelSim read- and write-engine suites, and a generated O=1/D=256/B=16
kernel plus wrapper-RID simulation. The
read-engine suite is split into four independent output trees and dispatched
concurrently; scenarios within each tree use one private ModelSim work library
and run serially. The write-engine gate also runs as an independent helper job.
The wrapper-RID simulation reuses the RTL generated for the kernel test.

`bambu_axi_burst.sh -j N` bounds the outer Mantis/helper pool. The four read
gates, writer gate, generated-kernel gate, and `vlog` portability gate use
ModelSim/Questa; the generated-kernel and wrapper-RID checks share one serial
job. Optional duplex/development runners outside this classic regression may
still use Verilator. The writer gate dispatches independent profiles,
parameter rejections, and mutation checks through its own bounded pool;
the main regression defaults `BURST_WRITER_GROUP_JOBS` to 8 and limits it to
the outer `-j N` value. Direct invocation of
`rtl_tests/run_write_burst_engine.sh` still requires
`BURST_WRITER_SIM_OUTSIDE_SANDBOX=1`; the main regression supplies that guard
when dispatching the gate. The standalone writer runner defaults its group
limit to 12; set `BURST_WRITER_GROUP_JOBS` to tune either mode.

The logging checks exercise large binary streams, producer and log-writer errors,
and inherited simulator logging without relying on the system `tee` implementation.

The recurring regression wrapper also runs the offline unit checks for the
write-artifact checker and write-engine extractor. The broader write compiler
matrix and generated-kernel G2 simulations remain opt-in development suites and
are not added to CI. Run them together with a fresh output directory and the
exact installed Bambu executable:

```sh
bash panda_regressions/hls/bambu_axi_burst/run_write_development_tests.sh all \
  /path/to/install/bin/bambu out_axi_write_development_run
```

Use mode `offline`, `compiler`, or `g2` to run one part. Compiler mode runs the
write-profile compiler matrix and frontend compatibility checks. G2 mode
compiles and simulates the three supported generated tops; run it outside the
sandbox with `BURST_G2_SIM_OUTSIDE_SANDBOX=1`. The aggregator refuses an existing
output path and retains per-suite output under it. Historical F1b G2 pass
markers do not qualify the current runner/script revision; G2 remains unqualified
until the current aggregator is run successfully.

The entry point intentionally does not run the broader `all` or `burst-type`
compiler-runner modes: they duplicate Mantis coverage.
By default all durable output is placed in `./out_bambu_axi_burst`, relative
to the caller's current directory. The script refuses to run if that path (or
an explicitly selected `-o/--output` path) already exists, including a
dangling symlink. Use a fresh path for each run. Mantis writes cases/report
under `<out_root>/mantis`; temporary files use `<out_root>/tmp`. Internal mock
checks and auxiliary Python suites write to per-suite `out_<suite>` children
under `<out_root>`. No run artifacts are created outside `<out_root>`.

Standalone shell runners default to a fresh `./out_<test>` directory in the
launch directory; an explicit relative output path is resolved from that same
directory. Python offline suites create their own `out_<suite>` directory under
`BURST_TEST_TMPDIR` when set, or under the launch directory otherwise. The main
regression sets that parent to its selected output root, keeping helper outputs
and temporary files beneath the same run.

Like sibling classic regressions, the default behavior cleans generated
intermediates after all gates pass, while preserving the `out_` root, report,
and canonical Mantis logs/results (`execution.log`, `timeout.log`,
`failure.log`, `return_value`, `bambu_results.xml`). On failure diagnostics
are retained. Pass `--no-clean` to retain successful-run intermediates too.
Mantis writes its configured cases under
`out_bambu_axi_burst/mantis/AXI-BURST/`; its suite report is in that Mantis
output directory.

The cycle expectation table contains measured read and writer signatures. The
three writer cases were simulated externally with ModelSim, checked against the
write-specific RTL profile, and passed the golden-data/canary testbench. Their
recorded cycle counts are regression signatures for the documented model
configuration, not a physical timing or general performance claim.
Auxiliary job logs are in `<out_root>/jobs/` when outputs are retained with
`--no-clean` or after a failure; successful default-clean runs remove them.

Pass `-j N` or `--parallel N` (also `-jN`, `-j=N`, and `--parallel=N`) to set
the maximum parallelism; the default is the `J` environment value or 1. Mantis
`-o DIR` or `--output DIR` (including `--output=DIR`) selects another output
root; repeated output options use the last value.
Repeated parallelism options use their minimum value, matching Mantis. The
Mantis regressions run first using that limit internally. Afterwards, the
independent compiler suites, ModelSim engine suites, and
generated-kernel/wrapper chain share a bounded pool of at most N jobs. The
generated-kernel test and its wrapper-RID check remain one serial job because
the latter consumes the former's RTL. On failure, no more jobs are launched;
all already-started jobs are awaited and their logs are retained under
`<out_root>/jobs/`.

`--restart` resumes Mantis in the existing output root and runs the cycle gate
against its retained results. After that gate passes, helper outputs go to a
new `<out_root>/out_helpers_restart.*` directory so evidence from earlier
attempts is preserved. Restart therefore requires a failed run whose outputs
were retained, or a successful run made with `--no-clean`. A successful
default-clean run removes generated RTL and needs a fresh output root; the
wrapper does not cache or reconstruct those artifacts.
