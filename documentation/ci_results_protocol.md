# PandA-bambu machine-readable CI protocol

## Status and scope

Version 1.0 defines the machine-readable interface for the GitHub-hosted open
build smoke test. The structured result bundle is authoritative. The GitHub
job summary is a human-readable view rendered from the validated bundle, not a
separate source of CI facts.

This first version describes one `open-build` task. It does not define quality
of results thresholds, baseline comparison, repeated sampling, or remote
orchestration.

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

Every document has a `panda.ci.*` schema marker and `schema_version: "1.0"`.
Documents use:

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
tool versions, parallelism, and cryptographic references to the other four
documents. Consumers compare these fields before treating two runs as
equivalent implementations or environments.

A mutable Docker tag is not an image digest. When the Docker runtime does not
expose a digest, `image_digest` is `null` while the Dockerfile SHA-256 and base
image text remain available.

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
proof of upload success or an exact archive-member layout. Every v1 entry names
`open-build` as its producer task. An unavailable artifact has
`available: false` and `null` size and SHA-256. Regular files are hashed when
available. Directory hashes are not fabricated.
The bundle entry has `null` size and hash because a document cannot
cryptographically describe an archive containing that same document without a
self-reference. Generation uses a fresh temporary directory and replaces the
previous bundle only after validation, so stale files are not uploaded.

### `verdict.json`

Evaluates the `pull-request-default` policy without changing raw task evidence.
The schema, open-build, executable-validation, co-simulation, and resource
rules are blocking. Fast Regressions availability is non-blocking. A queued or
unavailable laboratory runner is therefore `neutral`; it is never reported as
an open-build pass or failure.

## Failure and partial-execution semantics

Bundle generation, validation, summary rendering, and upload use `always()`
when the GitHub runner remains alive. They do not use `continue-on-error` to
mask schema errors, and they do not replace the build or executable step
conclusion. An underlying compile, install, executable, or co-simulation
failure still fails the workflow.

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

Producers currently emit exactly version 1.0. The repository validator accepts
exactly 1.0; later compatible minor versions require a validator update before
they are accepted. Consumers must reject unsupported major versions.

A major version is required when a change removes or renames a field, changes a
type, unit, meaning, reference format, required enum, or compatibility rule. A
minor version may add optional fields or new optional IDs without changing
existing meaning. Editorial clarification that changes no validation behavior
does not require a schema version change.

The `schema` member identifies the protocol document. JSON Schema files use
their own `$schema` keyword and live under `.github/schemas/ci/v1/`.

## Successful complete example

A complete successful run has this evidence relationship:

| Evidence | Value |
| --- | --- |
| Task execution state / outcome | `completed` / `pass` |
| All eight stages | `completed` / `pass`, exit status `0` |
| `bambu`, `bambu-cc`, `eucalyptus` checks | `pass` |
| XML/Verilator check | `pass` |
| OOM / kill metrics | `0` / `0` |
| Blocking policy rules | all `pass` |
| Fast Regressions availability | `neutral`, non-blocking |
| Verdict | `pass`, recommendation `merge` |

Every numeric value is a JSON number. The manifest hashes all four referenced
result documents, and the summary renderer reads these same validated values.

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
the verdict recommends `do-not-merge`; Fast Regressions remains independently
neutral and non-blocking.

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
