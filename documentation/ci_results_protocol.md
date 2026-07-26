# PandA-bambu machine-readable CI protocol

## Status and scope

Version 1.0 defines the machine-readable interface for the GitHub-hosted open
build smoke test. Version 1.1 is an additive multi-task profile for that open
build plus a small GitHub-hosted regression suite. The structured result bundle
is authoritative. The GitHub job summary is a human-readable view rendered
from the validated bundle, not a separate source of CI facts.

Version 1.0 describes exactly one `open-build` task. Version 1.1 adds regression
task results without changing the meaning of the v1.0 open-build document. The
protocol does not yet define quality thresholds, baseline comparison, repeated
sampling, or remote orchestration.

## Bundle architecture

Every open-build run attempts to upload this directory as one artifact:

```text
.ci-results/
├── manifest.json
├── request.json
├── tasks/
│   └── open-build.json
├── artifacts.json
└── verdict.json
```

A v1.1 multi-regression bundle keeps those entry-point documents and adds one
task document per stable regression ID:

```text
.ci-results/
├── manifest.json
├── request.json
├── tasks/
│   ├── open-build.json
│   ├── regression-scalar.json
│   ├── regression-control.json
│   ├── regression-loop-cxx.json
│   ├── regression-memory-interface.json
│   └── regression-callgraph.json
├── artifacts.json
└── verdict.json
```

Data flows in one direction:

```text
requested intent → raw task evidence → artifact discovery → policy verdict
                                    ↘ manifest entry point ↙
```

`manifest.json` is the entry point. Document and artifact paths are safe,
bundle- or workspace-relative POSIX paths. Document references include the
target schema, version, and SHA-256 so consumers can detect missing or changed
evidence.

## Common JSON contract

Every document has a `panda.ci.*` schema marker. An open-build-only bundle uses
`schema_version: "1.0"` throughout; an extended multi-regression bundle uses
`schema_version: "1.1"` throughout, including manifest references. A bundle
must not mix protocol versions. Documents use:

- UTF-8;
- two-space indentation;
- lexicographically sorted object keys;
- a trailing newline;
- JSON numbers for measurements;
- JSON `null` for unavailable values;
- second-resolution UTC timestamps in canonical `YYYY-MM-DDTHH:MM:SSZ` form;
- canonical stage order and stable metric, check, artifact, rule, and document
  ordering.

Numeric strings, non-finite numbers, negative durations, fabricated zeroes for
unavailable observations, and sentinel strings such as `"unknown"` are not
valid metric values.

## Document responsibilities

### `request.json`

Records immutable requested intent: repository and target commit identity, event and
pull request context, requested task and artifact IDs, CMake arguments,
parallelism, build type, assertions, warnings-as-errors, selected frontend,
synthesis smoke, cache mode, and policy profile. It never changes to reflect a
later build failure.

For pull requests, the target commit is the pull-request head. It may differ
from the merge commit that GitHub checks out and that `manifest.json` records as
the executed commit.

### `manifest.json`

Records run identity and provenance: workflow run and attempt, lifecycle
timestamps, repository/ref/base identity, the commit actually checked out and
executed, submodule commits, workflow and
Dockerfile hashes, base image identity, image digest when available, runner and
tool versions, parallelism, and cryptographic references to every task and
bundle document. A v1.0 manifest references the four non-manifest documents;
a v1.1 manifest additionally enumerates every regression task. Consumers compare the named `effective_build_profile` before treating two runs as equivalent implementations or environments. The profile records the effective optimization flags and CPU target profile, CMake build type, selected frontend, assertions and warnings-as-errors settings, configured parallelism, Dockerfile path and SHA-256, workflow path and SHA-256, and compiler/tool versions. Values come from the configured build action and CMake cache, not later log parsing. Missing profile values remain `null`; comparison treats incomplete profile evidence as requiring manual review.

A mutable Docker tag is not an image digest. When the Docker runtime does not
expose a digest, `image_digest` is `null` while the Dockerfile SHA-256 and base
image text remain available.

#### `manifest.json.hosted_regression_suite`

A v1.1 manifest requires `hosted_regression_suite`; a v1.0 manifest must omit
it. The object preserves the runner/action boundary separately from the five
raw task documents:

- `action_outcome` is the GitHub action conclusion normalized to `success`,
  `failure`, `cancelled`, `skipped`, or `unknown`;
- `action_exit_status` is the exit status reported by the Docker action, or
  `null` when the action could not report one;
- `execution_state`, `outcome`, and `exit_status` are the normalized suite
  result used by protocol consumers;
- `started_at`, `completed_at`, `duration_seconds`, and
  `duration_measurement_method` describe the measured suite interval;
- `container_setup_seconds` records the separate action/container startup
  interval when available;
- `failure_stage` identifies the action stage for canceled, timed-out, or
  infrastructure-error execution and is otherwise `null` for a pass or skip;
- `task_count`, `passed_count`, and `failed_count` are derived from the emitted
  regression task documents and cross-checked against runner and action
  outputs. `failed_count` is the runner's non-passing count
  (`task_count - passed_count`), so it also includes skipped or unknown tasks
  in an interrupted or prerequisite-skipped suite.

When a canonical, internally consistent `suite.json` is available,
`duration_seconds` is the exact Python monotonic-clock interval measured by the
suite runner from suite start through completion. Its method is
`suite-monotonic-clock`; it is not reconstructed from second-resolution UTC
timestamps and is not the sum of task durations. If that record is unavailable,
the producer may use the exported action duration or action start/completion
epochs with method `action-wall-clock`. With neither source, the duration and
timestamps are `null` as applicable and the method is `unavailable`. If only
the runner's exported duration survives, it is labeled `runner-wall-clock` and
has no absolute timestamps; it is never combined with the wider action
start/completion interval.

`action_exit_status` remains the direct action observation. The normalized
`exit_status` represents the suite result after task counts, raw suite status,
action conclusion, and action outputs have been reconciled. A normal technical
regression failure is `completed` / `fail` with nonzero action and normalized
statuses; it is not reclassified as infrastructure failure merely because the
GitHub step conclusion is `failure`. Conversely, a passing raw suite followed
by a nonzero action failure, missing or contradictory telemetry, or an
inconsistent task/count/result record is `infrastructure_error` / `unknown`.
Exceptional states never use exit status `0`.

Cancellation is `canceled` / `unknown`. A failed or canceled action whose
measured elapsed time reaches its configured deadline is `timed_out` /
`unknown`. When the open build prerequisite did not pass, the hosted suite is
`completed` / `skipped`. Missing failure-stage telemetry for canceled,
timed-out, and infrastructure-error results is normalized to
`fast-regressions`. The aggregate manifest execution state includes this suite
state, so a suite interruption cannot be hidden by already completed tasks.

### `tasks/open-build.json`

Contains raw task evidence. Execution state is distinct from technical
outcome. A task may, for example, have state `infrastructure_error` and outcome
`unknown`, or state `completed` and outcome `fail`.

The canonical stages are:

1. `container-setup`;
2. `configure`;
3. `frontend-resolution`;
4. `plugin-build`;
5. `project-build`;
6. `installation`;
7. `installed-executable-validation`;
8. `xml-verilator-cosimulation`.

A stage that was not reached has execution state `completed`, outcome
`skipped`, and `null` duration, exit status, and failure. It is not rewritten as
a pass. A failed stage contains a category, stable code, stage, message,
retryability, and evidence references.

Measurements are typed metric records. OOM and kill detection use numeric
`0`, `1`, or `null` with unit `boolean`. Peak cgroup memory is the preferred
resource measurement. Aggregate RSS is also reported, but summing process
`VmRSS` may count shared pages more than once.

Checks independently record whether each installed executable exists and
starts, whether XML/Verilator co-simulation passes, and whether result schema
validation passes.

The schema-validation check is the producer's assertion, and the generator and
the following independent workflow step both validate it. A candidate that
does not validate fails the workflow and is not an authoritative v1 bundle,
even if the unconditional upload step preserves it for diagnosis.

### `tasks/regression-*.json`

Version 1.1 represents every regression as an independent
`panda.ci.task-result` document. Its configuration records:

- the regression category and stable example ID;
- source path, top function, and XML or inline test-vector identity;
- requested and selected frontend;
- executable, working directory, and the ordered argument vector;
- normalized compiler, language-standard, simulator, simulation, optimization,
  device, clock, interface, backend-parallelism, experimental-setup,
  global-exposure, inlining, and Bambu-parameter options.

The invocation array is the exact executed interface; normalized options make
equivalence checks possible without reparsing shell text. A regression result
also records whether synthesis completed and how many RTL artifacts were
produced, plus whether simulation completed and verified, its execution count,
and total cycles when available.

The canonical regression stages are:

1. `input-validation`;
2. `hls-synthesis`;
3. `rtl-generation`;
4. `simulator-preparation`;
5. `rtl-simulation`;
6. `result-verification`.

The generic execution-state and outcome rules used by `open-build` also apply
to each regression. Once a stage fails, later unattempted stages are
`completed` / `skipped` with `null` duration and exit status. A simulator
process exiting successfully is not sufficient for a pass:
`result-verification` must confirm Bambu's native-reference comparison.

Regression failures use the existing failure-category enum and these stable
codes:

- `invalid-regression-input`;
- `frontend-resolution-failed`;
- `hls-synthesis-failed`;
- `rtl-generation-failed`;
- `simulator-build-failed`;
- `rtl-simulation-failed`;
- `result-mismatch`;
- `regression-timeout`;
- `regression-infrastructure-failure`.

The existing `workflow-canceled` code remains available with the `canceled`
category for workflow cancellation; it is not a regression technical-failure
classification.

Timeout and infrastructure failures name the stage active when evidence was
lost or the deadline was reached. A mismatch is a verification failure, not a
successful simulation.

## Hosted fast-regression selection

The v1.1 profile deliberately uses five small existing regression inputs. Each
uses `I386_CLANG16` and Bambu's built-in test-vector co-simulation with
`--simulate --simulator=VERILATOR`; Bambu invokes Verilator and performs the
native-reference comparison. Four use checked-in XML vectors, while the scalar
case preserves the inline vector from its existing regression list.

| Task ID | Existing source and vectors | Representative coverage |
| --- | --- | --- |
| `regression-scalar` | `panda_regressions/hls/bambu_specific_test5/adders.c` and the established `a=1,...,l=12` inline vector | Straight-line integer arithmetic, a pipelined addition chain, and scalar argument/return handling. |
| `regression-control` | `panda_regressions/hls/bambu_specific_test2/duff_device.c` and `dd_test1.xml` | Switch fall-through, a `do`/`while` loop, and pointer/array traffic. |
| `regression-loop-cxx` | `panda_regressions/hls/bambu_interface_test/ac_fixed2_tb.cpp` and `ac_fixed2_tb.xml` | C++17 fixed-point templates, an array loop, and the Clang 16 C++ PandA plugin path. |
| `regression-memory-interface` | `panda_regressions/hls/bambu_interface_test/simple4_axi_m.c` and `simple4_axi_tb.xml` | Three explicit `m_axi` ports, zero-length and nonzero loop paths, and memory writeback verification. |
| `regression-callgraph` | `panda_regressions/hls/bambu_interface_test/nested_axi_m.c` and `nested_axi_m.xml` | A preserved no-inline call graph, nested calls, loops, aliasing, and `m_axi` interfaces. |

These cases complement rather than repeat the open-build matrix-multiplication
smoke test. They add control-flow, frontend, interface, and call-graph signal
without introducing new test sources, proprietary tools, or an external
Verilator comparator.

### `artifacts.json`

Indexes every stable artifact ID even when an optional artifact is unavailable:

- `structured-result-bundle`;
- `compilation-database`;
- `installed-distribution`;
- `build-stderr`;
- `cmake-diagnostics`;
- `synthesis-smoke-diagnostics`;
- `memory-samples`.

`available` means that the path existed in the runner workspace when the index
was generated; it does not by itself mean an upload completed.
`github_artifact_name` names the GitHub artifact expected to carry some or all
of that indexed content, or is `null` when no upload is expected. It is not
proof of upload success or an exact archive-member layout. Every v1.0 entry
names `open-build` as its producer task. A v1.1 entry names the task that
produced it. An unavailable artifact has
`available: false` and `null` size and SHA-256. Regular files are hashed when
available. Directory hashes are not fabricated.
The bundle entry has `null` size and hash because a document cannot
cryptographically describe an archive containing that same document without a
self-reference. Generation uses a fresh temporary directory and replaces the
previous bundle only after validation, so stale files are not uploaded.

### `verdict.json`

Evaluates the `pull-request-default` policy without changing raw task evidence.
The schema, open-build, executable-validation, co-simulation, and resource
rules are blocking. In v1.1, `hosted-fast-regressions-success` is also
blocking and evaluates both the five hosted regression tasks and
`manifest.json#hosted-regression-suite`. It passes only when every regression
task passes, the normalized suite is `completed` / `pass`, the action
conclusion is `success`, both action and normalized exit statuses are `0`, and
the task counts are complete and consistent. A conclusive technical task
failure makes the rule fail. Cancellation, timeout, or infrastructure error
with no technical task failure makes this blocking rule neutral, which
prevents an automatic merge; if a task also failed, the rule remains failed.

The separate
`fast-regressions-availability` rule continues to describe the laboratory
workflow and remains non-blocking. A queued or unavailable laboratory runner
is therefore `neutral`; it is never reported as an open-build or hosted
regression pass or failure.

## Failure and partial-execution semantics

Bundle generation, validation, summary rendering, and upload use `always()`
when the GitHub runner remains alive. They do not use `continue-on-error` to
mask schema errors, and they do not replace the build or executable step
conclusion. An underlying compile, install, executable, co-simulation, or
blocking hosted regression failure still fails the applicable workflow.

Detectable setup failures produce a partial bundle with execution state
`infrastructure_error`. A dedicated 110-minute build-step timeout leaves ten
minutes inside the 120-minute job timeout for post-processing. When the failed
build step reaches that measured deadline, it is represented as `timed_out`;
manual cancellation is captured separately when GitHub allows the `always()`
steps to run. A hard runner loss or job-level timeout may prevent bundle
creation entirely; consumers must interpret a missing result artifact as
infrastructure uncertainty, never as an implicit pass or failure.

Allowed execution states are `queued`, `running`, `completed`, `canceled`,
`timed_out`, and `infrastructure_error`. Allowed outcomes are `pass`, `fail`,
`skipped`, `unknown`, and `neutral`. Failure categories are `configuration`,
`compilation`, `linkage`, `installation`, `execution`, `verification`,
`resource`, `timeout`, `infrastructure`, `canceled`, `schema`, and `policy`.

## Regression output-directory safety

The runner performs recursive pre-run cleanup only for these exact direct
children of the repository root:

- `.ci-regression-results`;
- `.ci-regression-evidence`;
- `.ci-regression-work`.

These dedicated names are the destructive-cleanup boundary. The runner rejects
the repository root, paths outside it, nested or differently named paths,
symbolic links, and existing non-directory targets before calling recursive
removal. Callers cannot redirect cleanup at source, Git metadata, or another
arbitrary workspace directory.

## Artifact naming

The result artifact name is:

```text
panda-ci-results-<workflow-run-id>-attempt-<run-attempt>
```

It retains the whole `.ci-results` directory for 14 days. Because the directory
name starts with a dot, the workflow explicitly enables hidden-file upload.
Existing failure diagnostics keep their three-day retention and additionally
include build stderr and memory samples.

## Schema evolution

Open-build-only producers continue to emit the exact v1.0 profile. A producer
emits v1.1 only for the additive multi-task profile. Consumers must validate
the declared version and reject unsupported minor as well as major versions; a
legacy v1.0 consumer must not interpret a v1.1 bundle as an open-build-only
bundle.
In particular, a v1.0 manifest omits `hosted_regression_suite`, while a v1.1
manifest requires it. Adding that object does not change the meaning of any
v1.0 field or permit a v1.0 producer to emit it.


A major version is required when a change removes or renames a field, changes a
type, unit, meaning, reference format, required enum, or compatibility rule. A
minor version may add optional fields or new optional IDs without changing
existing meaning. Editorial clarification that changes no validation behavior
does not require a schema version change.

The `schema` member identifies the protocol document. JSON Schema files use
their own `$schema` keyword and live under `.github/schemas/ci/v1/`.

## Successful complete example

A complete successful v1.0 run has this evidence relationship:

| Evidence | Value |
| --- | --- |
| Task execution state / outcome | `completed` / `pass` |
| All eight stages | `completed` / `pass`, exit status `0` |
| `bambu`, `bambu-cc`, `eucalyptus` checks | `pass` |
| XML/Verilator check | `pass` |
| OOM / kill metrics | `0` / `0` |
| Blocking policy rules | all `pass` |
| Laboratory Fast Regressions availability | `neutral`, non-blocking |
| Verdict | `pass`, recommendation `merge` |

Every numeric value is a JSON number. The manifest hashes all four referenced
result documents, and the summary renderer reads these same validated values.

A complete successful v1.1 run additionally has all five regression task
executions completed with outcome `pass`. All six stages of each task pass,
synthesis completes with at least one RTL artifact, and simulation completes
with native-reference verification. Its hosted suite record is
`completed` / `pass`, both action and normalized exit statuses are `0`, its
counts are `5` tasks / `5` passed / `0` failed, and its suite duration uses
the `suite-monotonic-clock` measurement. The blocking
`hosted-fast-regressions-success` rule passes. The separate laboratory
availability rule remains neutral and non-blocking when it is unobserved.

## Partial compilation-failure example

For a project compiler exit with status 2, the task records:

| Stage | State / outcome | Exit | Duration | Failure |
| --- | --- | ---: | ---: | --- |
| Container through plugin build | `completed` / `pass` | `0` | measured or `null` | `null` |
| Project build | `completed` / `fail` | `2` | measured or `null` | `compilation` / `compiler-exit-nonzero` |
| Installation | `completed` / `skipped` | `null` | `null` | `null` |
| Executable validation | `completed` / `skipped` | `null` | `null` | `null` |
| XML/Verilator co-simulation | `completed` / `skipped` | `null` | `null` | `null` |

`build-stderr` is indexed when produced. The open-build policy rule fails and
the verdict recommends `do-not-merge`; laboratory Fast Regressions remains
independently neutral and non-blocking.

## Download and independent validation

An agent with the repository checkout and GitHub CLI can validate a downloaded
bundle without installing Python packages:

```bash
repository=Antonyt80/PandA-bambu
run_id=<workflow-run-id>
attempt="$(gh api "repos/${repository}/actions/runs/${run_id}" --jq .run_attempt)"
artifact="panda-ci-results-${run_id}-attempt-${attempt}"
download_dir="$(mktemp -d)"

gh run download "${run_id}" \
  --repo "${repository}" \
  --name "${artifact}" \
  --dir "${download_dir}"

bundle_dir="$(dirname "$(find "${download_dir}" -name manifest.json -print -quit)")"
implementation_commit="$(python3 -c \
  'import json, sys; print(json.load(open(sys.argv[1], encoding="utf-8"))["workflow"]["implementation_commit"])' \
  "${bundle_dir}/manifest.json")"
test "$(git rev-parse HEAD)" = "${implementation_commit}"
git diff --quiet
git diff --cached --quiet
PYTHONPATH=.github/scripts python3 -m ci_results validate "${bundle_dir}"
```

Use a trusted repository checkout at the workflow implementation commit; do
not execute validation code supplied by the downloaded artifact. A zero exit
status validates canonical serialization, every document schema,
manifest hashes, internal identities, stage/metric/check/artifact links, evidence
references, and policy consistency. Diagnostics identify the offending
document and logical path on failure.
