# PAF Bootstrap-to-Self-Hosting Plan

Status: temporary realization plan  
Version: 2026-08-03 revision 3

## 1. Purpose

The hardened Cline bootstrap builds PAF until PAF can run its own governed campaigns. It remains separate, versioned, recoverable, and temporary. It must not evolve into a second permanent PAF runtime.

The bootstrap scripts are the first concrete runtime/supervision implementation. PAF replaces their responsibilities one at a time after explicit conformance, monitoring, security, recovery, and rollback gates.

## 2. Current bootstrap flow

```text
authorized backlog item
→ Sol generates a bounded task contract
→ operator authorizes the generated contract
→ Sol plans the task
→ Terra implements in the working tree
→ controller validates allowed paths
→ controller runs long deterministic validation
→ controller creates exact checkpoint
→ Opus produces bounded review plan
→ Sonnet performs exact-head verification
→ bounded remediation or stage-only review resume
→ draft PR
→ human review and merge
```

The bootstrap never merges automatically.

## 3. Corrected role boundary

Terra owns implementation and implementation-stage checks. It must not block because:

- no commit exists;
- no `dev/panda...HEAD` digest includes unstaged changes;
- no independent review has occurred;
- no PR exists.

The controller validates task scope, runs deterministic validation, creates the checkpoint, coordinates exact-head review, and performs configured publication effects after Terra returns `COMPLETE`.

Review agents receive controller-owned validation evidence. Review limits are enforced by the supervisor, not merely requested in prompts.

## 4. Immediate bootstrap hardening requirements

These are required before autonomous backlog execution:

- generated tasks are bound to exact base SHA and backlog digest;
- every changed path is checked against authorized metadata before staging;
- generated tasks require explicit operator authorization;
- missing-key, authentication, quota, and exhausted-budget failures do not retry blindly;
- Cline daemon credential mismatch is detected before invocation;
- review tool, iteration, duplicate-command, and inactivity budgets are enforced;
- Opus/Sonnet review can resume against an existing exact checkpoint without rerunning Sol/Terra;
- campaign state updates are locked and recoverable;
- installer backups remain outside the repository;
- all strategic documents, templates, schemas, monitor, and tests are installed and validated.

## 5. Implementation ladder

### R0 — Architecture closure and bootstrap/runtime decision freeze

Deliver:

- requirements closure and traceability;
- permanent product roadmap;
- PAF Runtime Specification;
- Security, Data Governance, and Trust-Boundary Plan;
- Observability and Operations Plan;
- Bootstrap Operational Invariants;
- this bootstrap plan;
- responsibility-transfer matrix;
- interface/state-machine map;
- decisions blocking wire identity.

Gate: all revision-3 bootstrap smoke tests pass and no unresolved decision blocks PAF-05A3a.2a.

### R1 — Wire identity and canonical form

Implement logical IDs, revision IDs, typed digests, canonical encoding, namespaces, negotiation, migration envelope, extension rules, and cross-language valid/adversarial vectors.

Gate: independent implementations or verifiers produce identical bytes/digests and deterministic rejection.

### R2 — Minimal contract, error, and event envelope

Implement common object and normalized event envelopes, typed errors, causal ordering, registry lookup, provenance references, and unknown-version behavior.

Gate: every later bootstrap record uses the envelope.

### R2-S — Security and event-governance substrate

Implement classification, redaction, exact-secret suppression, Context Release Decisions, trust/source labels, event size/backpressure rules, retention metadata, and tamper-evident segment digests.

Gate: seeded secrets and prompt-injection content do not expand authority or appear in normalized events/evidence.

### R3 — Shadow runtime and evidence

Instrument real Sol, Terra, Opus, Sonnet, process, tool, validation, and review activity as PAF objects while the shell remains authoritative.

Gate: every terminal condition produces attempt-total Evidence Packages and a normalized timeline.

### R3-O — Bootstrap observability and conformance harness

Implement the first adapter conformance fixtures and read-only live status/event projections. Improve the bootstrap monitor to follow stage transitions, unique progress, policy-budget exhaustion, and scoped process health.

Gate: a mock campaign exposes plan, implementation, validation, review, provider failure, loop, and stall states without raw recursive JSON extraction.

### R4 — Durable state and recovery

Implement SQLite WAL journal, content-addressed blobs, stable identities, snapshots, leases/fencing, checkpoints, idempotent commands, effect intents/receipts, replay, reconciliation, backup/restore, and failure injection.

Gate: restart at every stage/effect boundary without duplicate effects, lost evidence, destructive cleanup, or silent ambiguity.

Authority transfer: PAF owns campaign state and deterministic recovery decisions.

### R4-O — Durable observability projections

Implement Campaign/Work/Attempt/Stage projections, normalized event query, health/progress/stall assessments, budgets/resources, logs/evidence queries, alert records, and a stable read-only API suitable for TUI and GUI.

Gate: restart and reconstruct the same timeline and health state from durable events.

### R5 — Secure generic runtime and Cline adapter

Implement the Runtime Specification, generic executor, Cline adapter, process/tool supervisor, isolated worktree, ephemeral home, filesystem/network/resource boundaries, scoped credentials, cancellation, timeout, cleanup evidence, stage resume, and recovery.

Gate: the current cycle runs through the generic runtime with equivalent behavior, enforced review budgets, better monitoring, and no Cline-native logic in the bootstrap.

Operator pause/resume/cancel becomes authoritative only after this runtime gate; R4-O remains read-only until then.

### R6 — Routes, capabilities, resources, scheduler, and local models

Implement descriptors for model, deployment, provider, framework, binding, funding, workspace, tools, permissions, resources, capabilities, reservations, and resolved routes.

Support:

- one remote Cline route;
- one local OpenAI-compatible Cline route;
- deployment/resource health;
- OOM, context, endpoint, and contention classification;
- one offline execution;
- bounded queueing and lease expiration.

Gate: the same task runs locally and remotely with exact deployment/capability evidence.

### R7 — Completion, deterministic execution decisions, retry, and switching

Implement criterion-level Completion Assessment, failure/progress classification, failure signatures, campaign health, Retry Policy, checkpoint selection, circuit breakers, and Route Transition Decision.

Gate: seeded infrastructure, budget, policy, and semantic failures trigger the correct retry/switch/stop behavior independently from agent status claims.

Authority transfer: PAF owns semantic completion, retry, and route transition. Bootstrap marker and profile rules are removed.

### R8 — Retrieval and MCP

Implement tool/MCP/retrieval contracts, grants, source trust, lineage, receipts, Context Release Decisions, Context Package updates, one sandboxed read-only local server, and monitoring.

Gate: retrieve a required fact with provenance while unauthorized data, files, tools, credentials, and network remain inaccessible.

### R9 — Work Graph and hierarchical planning

Implement Objectives, Work Items, Work Item Plans, Steps, triggers, assessments, proposals, critics, policy decisions, atomic mutations, and scheduling.

Initial limits:

```text
maximum planning depth: 3
maximum generated items per mutation: 2
maximum total generated items: 6
maximum replans per item: 1
maximum graph mutations: 2
```

Gate: one parent generates exactly two reviewed child tasks; unsupported work is rejected; only the controller mutates the graph.

Authority transfer: PAF owns role resolution and hierarchical planning; the fixed role loop leaves normal operation.

### R10 — Claude Code as second runtime adapter

Implement Claude Code through the same Runtime Specification, permissions, Context, events, monitoring, evidence, completion, recovery, and stage-resume semantics.

Gate: one child executes through Cline and one through Claude Code, with independent review and portable context.

### R11 — First monitored self-hosted campaign

PAF owns state, runtime, monitoring, planning, routing, local/remote execution, retry, retrieval, generated work, review coordination, evidence, and resume.

The bootstrap owns launch, watchdog, emergency stop, state preservation, known-good recovery, and the human publication boundary.

Acceptance includes:

1. hierarchical planning and critique;
2. local execution;
3. runtime-failure recovery;
4. semantic route switch;
5. exactly two child tasks;
6. Cline and Claude Code execution;
7. governed MCP retrieval;
8. live status/health/stall visibility;
9. controller interruption and stage resume;
10. exact-head independent review;
11. complete redacted evidence;
12. stop before human-protected publication.

### R12 — Bootstrap retirement and standalone extraction

Normal operation becomes `paf campaign run`. The Cline bootstrap retains inspect, recover, launch-known-good, review-resume, and repair capabilities for at least one additional release.

Publication transfers only after effect-intent and reconciliation tests. Merge remains human-authorized.

## 6. Cross-cutting gates

Every slice updates:

- interfaces and ownership;
- state transitions and typed failures;
- authority, security, and policy requirements;
- runtime and monitoring obligations;
- valid/adversarial fixtures and conformance tests;
- deterministic and semantic tests;
- interruption, stall, budget, and residue tests;
- migration, backup, restore, and rollback behavior;
- evidence and redaction obligations;
- exact-head review proportional to risk.

## 7. One-authority rule

Shadow implementations may observe and compare. After a cutover gate passes, PAF becomes authoritative and duplicate bootstrap logic is removed. There is never more than one authoritative implementation for state, completion, retry, routing, planning, review coordination, security decisions, or effects.
