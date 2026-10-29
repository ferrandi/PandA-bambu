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

The optional MNIST archives are retained to keep existing manual runs usable.
Checksums and extraction instructions were added; the unrecorded download origin
still needs provenance before they can be replaced by a reliable download script.
No existing commit was amended and no PR metadata was published.

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
build_task_graph/Clang13, and ordinary banked caches. The task cases also exercise
`enable-task=true`. Generated 32-bit testbenches were executed outside the sandbox,
which otherwise blocks them with SIGSYS.

An additional ordinary OpenMP vector-add probe with Clang19 still crashes in the
legacy OpenMP plugin. Compiling the unmodified HEAD plugin and running it on the
same input reproduces the same crash, so this is a pre-existing limitation.
The full CI matrices and large manual performance configurations were not run.

## Proposed PR title and description

**Add support for OpenMP task graphs**

Extract static OpenMP task dependency graphs with the LLVM19 frontend and generate
hardware resource managers for task execution. Task mode supports invariant
captured base pointers on 32-bit targets; the README describes unsupported OpenMP
constructs and how to run the regression suites.

Keep task-specific lowering separate from ordinary OpenMP, diagnose recognized
unsupported inputs, and run the task CI suites once with Clang19. Include the
priority encoder implementations used by task resource managers and their
standalone Verilator regression. Validation is listed above.
