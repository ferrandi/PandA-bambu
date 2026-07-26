# CI result comparison protocol

PandA CI compares an untrusted candidate result bundle with an explicitly supplied baseline. The command validates both inputs as complete protocol 1.1 bundles before reading their results:

```bash
python3 -m ci_results compare \
  --baseline baseline-bundle \
  --candidate candidate-bundle \
  --output comparison.json
python3 -m ci_results render-comparison --input comparison.json
```

The first command emits canonical, deterministic JSON with schema marker `panda.ci.comparison` and schema version `1.0`. Its identity is the SHA-256 of the two recorded bundle identities. The timestamp is inherited from the candidate bundle rather than the wall clock, so identical inputs produce identical bytes. The renderer validates and reads only `comparison.json`; it never reinterprets the source bundles.

## Comparability

Only regression tasks with the same task ID, task type, build profile, input, frontend, normalized invocation, compiler, simulator, device, clock, interface, language standard, optimization, experimental setup, Bambu parameters, global exposure, inlining, and parallel backend are comparable. A changed configuration is reported as `not-comparable`, a candidate-only task as `missing-in-baseline`, and a baseline task absent from the candidate as `missing-in-candidate`.

For comparable tasks, the document records synthesis, simulation, verification, overall-outcome, execution-state, and failure-category transitions. It also records baseline value, candidate value, absolute delta, and percentage delta for simulation cycles and the HLS-synthesis, RTL-simulation, and total-regression durations. Missing observations remain JSON `null`; they are never converted to zero. A zero baseline has an absolute delta but a `null` percentage delta.

## Policy

The deterministic policy has three decisions:

- `reject`: a previously passing build regresses; a required candidate task is missing; previously passing synthesis or simulation fails; or candidate result verification fails.
- `manual-review`: the baseline build or regression was not passing, a candidate adds a task, configurations differ, or cycle information is unavailable.
- `accept`: neither rejection nor manual-review evidence exists.

Rejection takes precedence over manual review. Performance deltas are measured but are not rejection thresholds in version 1.0.

Examples: `pass → fail` synthesis rejects; a changed FPGA device requires manual review; `42 → 50` cycles is reported as `+19.047619%` without changing an otherwise accepting decision; and a missing candidate regression rejects.

## Trust boundary and exclusions

Both bundle manifests, hashes, schemas, evidence references, and cross-document invariants are validated first. Errors explicitly name the baseline or candidate input. The comparison does not fetch artifacts, choose a baseline, publish a trusted baseline, run synthesis, or infer results from logs. Trusted baseline discovery and publication are a follow-on milestone; callers must provide the intended immutable baseline bundle.

The schema is `.github/schemas/ci/v1/comparison.schema.json`. Focused tests use the real protocol 1.1 five-regression shape and cover invalid inputs, configuration mismatches, missing tasks, correctness changes, null and zero metrics, schema/semantic rejection, rendering, and byte-for-byte determinism.
