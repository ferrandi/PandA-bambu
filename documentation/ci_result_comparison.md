# CI result comparison protocol

PandA CI compares an untrusted candidate result bundle with an explicitly supplied baseline. The command validates both inputs as complete protocol 1.1 bundles before reading their results:

```bash
python3 -m ci_results compare \
  --baseline baseline-bundle \
  --candidate candidate-bundle \
  --output comparison.json
python3 -m ci_results render-comparison --input comparison.json
```

The first command emits canonical, deterministic JSON with schema marker `panda.ci.comparison` and schema version `1.0`. Its identity is the SHA-256 of the two recorded bundle identities. The timestamp is inherited from the candidate bundle rather than the wall clock, so identical inputs produce identical bytes. The renderer validates and reads only `comparison.json`; it never reinterprets the source bundles. The compare command exits `0` for `accept`, `1` for `reject`, and `2` for `manual-review`, allowing automation to distinguish both non-accepting outcomes.

## Comparability

Only regression tasks with the same task ID, task type, complete `effective_build_profile`, input, frontend, normalized invocation, compiler, simulator, device, clock, interface, language standard, optimization, experimental setup, Bambu parameters, global exposure, inlining, and parallel backend are comparable. A changed configuration is reported as `not-comparable`, a candidate-only task as `missing-in-baseline`, and a baseline task absent from the candidate as `missing-in-candidate`.

The effective profile is semantic evidence: effective `PANDA_OPTIMIZED_FLAGS` (so `-march=native` and `-march=x86-64` differ), CMake build type, selected frontend, assertions, warnings-as-errors, parallelism, CPU target profile, Dockerfile path and SHA-256, workflow path and SHA-256, and compiler/tool versions. Missing, `null`, empty-string, empty-list, or incomplete required tool-version evidence is never silently accepted; it makes every paired regression task `not-comparable` and requires manual review.

For comparable tasks, the document records synthesis, simulation, verification, overall-outcome, execution-state, and failure-category transitions. It also records baseline value, candidate value, absolute delta, and percentage delta for simulation cycles and the HLS-synthesis, RTL-simulation, and total-regression durations. Missing observations remain JSON `null`; they are never converted to zero. A zero baseline has an absolute delta but a `null` percentage delta.

## Policy

The deterministic policy has three decisions:

- `reject`: a previously passing build regresses; a required candidate task is missing; previously passing synthesis or simulation fails; or candidate result verification fails.
- `manual-review`: the baseline build or regression was not passing, a candidate adds a task, configurations differ, or cycle information is unavailable.
- `accept`: neither rejection nor manual-review evidence exists.

Rejection takes precedence over manual review, including when configuration differences prevent QoR comparison. Correctness transitions remain policy evidence for paired tasks even when their metric deltas are suppressed. Performance deltas are measured but are not rejection thresholds in version 1.0.

Examples: `pass → fail` synthesis rejects; a changed FPGA device requires manual review; `42 → 50` cycles is reported as `+19.047619%` without changing an otherwise accepting decision; and a missing candidate regression rejects.

## Trust boundary and exclusions

Both bundle manifests, hashes, schemas, evidence references, and cross-document invariants are validated first. Duplicate profile provenance must agree with the manifest workflow, container, tools and parallelism records, the request build parameters, and the observed open-build configuration. Errors explicitly name the baseline or candidate input.

The comparison document separates primitive observations from derived fields. Primitive observations include task presence, paired correctness outcomes, execution state, failure category, canonical comparability checks, and metric values. Standalone validation cannot independently authenticate those observations without the original bundles. Derived fields include task classification, comparability reasons, transitions, failure flags, metric provenance and deltas, policy reasons, policy decision, summary, and overall outcome.

Validation reconstructs derived fields from the serialized primitive observations instead of trusting the stored derived fields, so a canonically reserialized policy edit is still rejected. Standalone validation therefore proves internal consistency of the comparison document; it does not prove authenticity of the original bundles or correctness of the primitive observations if all primitive observations are changed together consistently. The comparison does not fetch artifacts, choose a baseline, publish a trusted baseline, run synthesis, or infer results from logs. It deliberately excludes volatile execution metadata from profile identity: workflow run ID, run attempt, timestamps, cache restore/save outcomes, pull-request number, runner worker and region identity, commit SHA, and artifact names. Baseline and candidate commits are expected to differ. Trusted baseline discovery and publication are a follow-on milestone; callers must provide the intended immutable baseline bundle.

The schema is `.github/schemas/ci/v1/comparison.schema.json`. The mounted development workspace reports temporary files as mode `0600` even after the protocol writer calls `fchmod(0644)`; the identical permission assertion on untouched `origin/dev/panda` fails on that mount and passes from a normal `/tmp` filesystem. GitHub-hosted execution remains the authoritative cross-UID check because the production writer still explicitly sets `0644` before atomic replacement. Focused tests use the real protocol 1.1 five-regression shape and cover invalid inputs, configuration mismatches, missing tasks, correctness changes, null and zero metrics, schema/semantic rejection, rendering, and byte-for-byte determinism.
