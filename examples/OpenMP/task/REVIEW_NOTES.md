# OpenMP task review decisions

The task flow deliberately supports a static subset of OpenMP. Unsupported
constructs need not be implemented as part of this change. Recognizable cases
are diagnosed; the README states the remaining limitations.

The non-banked cache requires one channel because independent channel caches
have no coherence protocol. Keep this validation and the fix that creates one
adapter for a non-banked cache. The banked tag assignment explicitly drives the
whole output to zero when no inner tag exists; keep it as a separate correctness
fix. CSIM inputs here are generated C simulation models and runtime files, so
forcing C compilation also for generated .cpp names is intentional.

The former predication invariant and builtin classification are retained for
ordinary flows; task-private lowering has its own gated behavior. Ordinary
OpenMP no longer identifies task mode by searching user function names.

Priority encoders remain in this branch: task resource managers use them. Their
standalone regression is registered with CTest, which is the current top-level
build/test entry point (there is no top-level autotools Makefile.am).

MNIST code attribution and dataset provenance are now recorded separately in
`src/mnist/PROVENANCE.md` and `src/mnist/dataset/README.md`, with preserved MIT
notices and a checksum-verified download/reconstruction script.

## Current pipeline

Task mode is detected from actual runtime calls in linked LLVM19 IR. No
`enable-task` parameter is needed. `--priority-encoder` and
`--task-resource-duplication` are ordinary CLI options, not parameter-registry
entries. Standard builds no longer rewrite tracked parameter metadata by default.

Pointer widths and dependency layouts follow LLVM DataLayout. XML records the
pointer width, and the binding validates it against the selected -m32/-m64 target.
Generated entry names are matched through shared helpers, including the remaining
datapath and call-graph sites. `TopEntityOmpTaskGraph.cpp` is unchanged.

The task suites are defined in one reusable workflow shared by fast/full CI.
Those workflows already target different PR base branches; consolidating their
definitions avoids drift without deleting either coverage path. Batch arguments
are arrays, runner concurrency is expanded at runtime, and configuration paths
are quoted.

## Validation

Built Bambu and the modified LLVM19 task plugins, plus the ordinary OpenMP and
top-name plugins for Clang4 and Clang13. The focused frontend regression passes
with assertions disabled for cycle detection; it also checks stores, calls,
control-flow joins, indirect calls, taskwait and pointer width errors. The
standalone priority encoder regression passes with Verilator (Yosys was not
available). CLI diagnostics and shell/YAML syntax checks pass.

Twelve selected RTL co-simulations pass: task1Param, taskNParamsSparse,
taskNestedCalls with resource duplication, taskMathCalls, taskNestedCalls banked,
MNIST training, sparse LU banked, ordinary monothread/Clang4, ordinary vector add
with and without context switching/Clang13, a user function named
build_task_graph/Clang13, and ordinary banked caches. These historical task cases used
`enable-task=true` before automatic detection replaced that switch. Generated 32-bit testbenches were executed outside the sandbox,
which otherwise blocks them with SIGSYS.

An additional ordinary OpenMP vector-add probe with Clang19 still crashes in the
legacy OpenMP plugin. Compiling the unmodified HEAD plugin and running it on the
same input reproduces the same crash, so this is a pre-existing limitation.
The full CI matrices and large manual performance configurations were not run.

## Proposed PR title and description

**Add support for OpenMP task graphs**

Extract static OpenMP task dependency graphs with the LLVM19 frontend and generate
hardware resource managers for task execution. Task mode supports invariant
captured base pointers on 32-bit and 64-bit targets; the README describes unsupported OpenMP
constructs and how to run the regression suites.

Keep task-specific lowering separate from ordinary OpenMP, diagnose recognized
unsupported inputs, and run the task CI suites once with Clang19. Include the
priority encoder implementations used by task resource managers and their
standalone Verilator regression. Validation is listed above.

## Additional validation for automatic detection and 64-bit pointers

Eight new/updated co-simulations pass: task1Param at 32/64 bits, task2Params at
64 bits, taskPointerWidths at 32/64 bits and at a base address above 4 GiB,
taskNestedCalls with resource duplication at 64 bits, and taskNParamsSparse at
64 bits. The width fixture includes two pointers, 64-bit data, dependency chains
and non-inlined user functions with task-like names. Graph XML and RTL ports
retain 64-bit addresses. Focused pass tests also check automatic detection from
calls rather than declarations/names, reserved naming patterns and invalid widths.

The local 64-bit support archives predated the current IR format and contained
repeated stale members. Their three affected vector-bearing objects were
regenerated with the current plugin before co-simulation. No library binaries are
part of this source change. The CMake archive rule now removes its generated
output before `ar qc`, preventing stale duplicate members on subsequent rebuilds.

Additional co-simulations pass for taskMultiSource at 32/64 bits and banked
taskNestedCalls at 64 bits (eleven new/updated cases in total). Scalar firstprivate
constants remain 32-bit; wider captured constants are now explicitly rejected
instead of narrowed. This does not restrict 64-bit values in pointed-to memory.

The final CLI selection check also co-simulates `--priority-encoder=gates` at
64 bits (twelve co-simulations). LLVM inputs no longer receive source-only LTO
or OpenMP flags; mismatched LLVM pointer widths are diagnosed before extraction.
The focused pass test rejects task operations without a recognized parallel
region instead of emitting an empty graph.

The feature commit subject was corrected to “Add support for OpenMP task graphs”
through a local reword rebase; a backup ref preserves the previous history.
Updating PR #429's title/body through the GitHub connector returned 403
("Resource not accessible by integration"); the proposed title/description above
remain available for a maintainer to apply. No branch was pushed.
