# OpenMP task integration validation

The task change is kept as one commit, `Add support for OpenPM task`, on top of
`dev/panda` at `a48e5ae347c397bc30cf5fba90954dff7d4a096d`. The original private
branch tip is preserved by `backup/openmp-task-original` at
`c7b9343cc6b83da4774b159983e4bc293aa0b2f9`.

## Implementation checks

- The distributed task binding follows the neighboring OpenMP binding's naming,
  const placement, containers and formatting. Generic port-forwarding templates,
  variadic callbacks, local type aliases, scoped enumerations, local RTL-generation
  lambdas and unused scaffolding were
  replaced by ordinary methods and concrete port descriptions.
- Repeated vector specialization reuses an already sized port and checks its
  shape. Forwarding preserves compatible parent ports; tag ports are found and
  created under the same owner. Intentionally unused tag and manager outputs
  have explicit connections. A one-resource controller keeps a nonzero tag width.
- Task entries establish independent OpenMP call-graph contexts. Callees inherit
  single-channel memory allocation and are cloned transitively, including library
  functions. Cloned call edges use the cloned statement IDs. Existing predicates
  are preserved and single-context tasks do not request context-switch memory
  pages. There is no `expf` exception in `function_behavior.cpp`.
- CMake builds and installs both compiler and optimizer variants of the LLVM 19
  task passes. Task extraction uses the selected LLVM optimizer to emit textual
  IR, without relying on an unrelated `llvm-dis` in PATH. Loop flattening stays
  enabled in the ordinary flow and is disabled for extracted task graphs, where
  it could introduce overflow intrinsics unsupported by the IR exporter.
- Project source headers use the exact Apache-2.0 WITH LLVM-exception phrase and
  SPDX identifier used by the current Bambu sources. Author attributions are
  preserved. The BOTS benchmark's original third-party GPL notice is retained;
  technology RTL uses the MIT tag required by the existing IP library.

## Executed verification

| Check | Result |
| --- | --- |
| Bambu C++23 build with GCC 13 and assertions/warnings-as-errors enabled | Passed |
| LLVM 19 task compiler and optimizer plugin variants | Passed |
| OpenMP plugin compatibility builds for LLVM 4 and 13 | Passed |
| Self-contained task RTL configurations | 35 passed |
| Ordinary OpenMP RTL regressions, including context switching and LLVM 4/13 | 4 passed |
| Native task, sparse LU, MNIST training and inference testbenches | Passed |
| Priority encoder RTL specification checks, both families, widths 1 through 512 | Passed |
| Priority encoder SAT equivalence, all ten supported widths | Passed |
| Yosys/ABC 6-input LUT comparison, both families, widths 16 through 512 | Completed |
| Workflow YAML parsing, task shell syntax and Git whitespace checks | Passed |

The 35 task configurations include all seven existing small task kernels;
non-inlined nested calls with local arrays; `expf` and `sqrtf`; one memory channel;
one task-entry type; normal and pipelined buses; NO_BRAM and GLSS policies; two
and four copies of task resources; both encoder families; banked memory with
private nested callees; sparse LU with ordinary, banked and AXI interfaces; and
MNIST training/inference with ordinary and banked memory. MNIST inference checks
every final tensor output against the host, rather than relying only on equal
aggregate accuracy; training rejects nonfinite losses. These tests include
native testbench checks, rather than only successful HDL generation.

The task suites are included in both minimal and reusable OpenMP CI matrices.
The full-clang build now includes LLVM 19. The new integer, floating-point and
single-entry regressions are in the arbiter suite; the nested banked cases are
also in the banked suite. The standalone encoder regression is registered in
CTest and run by minimal CI. The GitHub-hosted workflows themselves were not
triggered from this session.

All local runs used the existing checked-out dependency revisions and toolchains,
an isolated build and installation, and Verilator for co-simulation. Testbench
executables are 32-bit and were run outside the sandbox's syscall filter. No
remote synthesis service or vendor place-and-route tool was used. Real-dataset
performance runs remain manual; their required paths are documented in the task
README. Synthesis measurements are logic-mapping results, not measured MHz.

See [the encoder comparison](openmp_task_priority_encoders.md) and
[the machine-readable synthesis results](openmp_task_priority_encoders_results.json)
for commands and measurements.

The exact local RTL configurations and outcomes are recorded in
[`openmp_task_validation_results.json`](openmp_task_validation_results.json).
Build and validation artifacts are retained under `.task-work/` in the original
workspace; unrelated local files are preserved.

## Subsequent priority encoder optimization

The encoder implementations were subsequently optimized and formally proved
against their original versions and an independent priority specification for
both families at all ten widths (20 checks). The permanent regression now proves
the specification as well as equivalence. Four new RTL co-simulations passed,
covering banked duplicated task resources, mathematical callees and critical
managers with both families. The 35 task and four ordinary OpenMP runs above are
the original integration runs; they were not all repeated for this equivalent
RTL rewrite. See `openmp_task_priority_encoders_optimization.json` for the new runs.
The temporary rebase worktree was removed after the original integration; the
active checkout now contains the final branch.

## Three complete encoder families

The library subsequently split the two explicit implementations into `tree` and
`gates`, each complete from 1 to 512 inputs, alongside the default `hierarchical`
family. All 30 modules passed the RTL reference regression and SAT specification
checks. A shared C++ prefix selector drives library consumers and generated task
managers. Twelve new RTL co-simulations passed for all three families, including
both critical managers and both arbiters; emitted prefixes were verified. The
previous 35-task/four-OpenMP and two-family optimization results above remain
historical runs. Current results are in `openmp_task_priority_encoders_results.json`
and the `priority_encoder_families` field of `openmp_task_validation_results.json`.
