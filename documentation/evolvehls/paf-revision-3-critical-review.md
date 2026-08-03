# PAF Bootstrap Revision 3 — Critical Review

Status: implementation-readiness assessment  
Date: 2026-08-03

## 1. Executive verdict

The revision-2 architectural direction was strong, but the revision-2 bundle was not operationally reliable enough to start an unattended bootstrap campaign.

The most serious problems were not conceptual. They were concrete bootstrap failures:

- the installer omitted strategic documents and the monitor;
- the next-task generator depended on a prompt template that was not packaged;
- retries could not safely adopt preserved dirty work;
- a stalled exact-head reviewer required manual orchestration;
- review tool/iteration limits were advisory rather than enforced;
- generated tasks were not bound tightly enough to an exact base and operator authorization;
- monitoring could misclassify global zombie processes and repeatedly parse entire growing JSONL files;
- security, redaction, event-volume control, and context-release policy were underweighted.

Revision 3 corrects these issues sufficiently for **bounded, operator-supervised bootstrap use**. It is not a production PAF runtime and must not be described as one.

## 2. What was already strong

The following additions should be retained:

1. A permanent PAF product roadmap separated from the temporary bootstrap plan.
2. A stable Runtime Specification independent of Cline, Claude Code, or any model.
3. Durable state and recovery before autonomous graph mutation.
4. Observability treated as correctness infrastructure rather than a late GUI feature.
5. Distinct runtime, model, provider, framework, access, funding, workspace, tool, permission, and resource dimensions.
6. Local models treated as exact deployments rather than model names.
7. Hierarchical planning with bounded mutations and risk-proportional critique.
8. Completion Assessment separated from an agent's `COMPLETE` claim.
9. Human authority retained for protected effects, publication, merge, and PAF promotion.
10. Incremental responsibility transfer with no duplicate authoritative logic.

These decisions survived actual bootstrap failures and remain the correct foundation.

## 3. Critical corrections made in revision 3

### 3.1 Packaged and installable bootstrap

Revision 3:

- packages every required document, schema, template, test, and executable;
- installs `paf-cline-monitor`, preflight, and review-resume tools;
- stores backups outside the repository;
- runs offline smoke tests during installation;
- validates the campaign JSON against its schema when `jsonschema` is available.

### 3.2 Generated-task integrity

A generated task now carries:

- exact base SHA;
- typed backlog-item digest;
- typed task-contract digest;
- allowed paths constrained to authorized backlog path families;
- bounded runtime and review budgets;
- safe offline validation commands;
- explicit human gates.

Generated tasks enter a `generated` state and require an audited operator authorization before normal execution. A task becomes stale when its exact base changes.

### 3.3 Correct role lifecycle

The bootstrap operational invariants now state:

```text
Sol: plan only
Terra: edit and perform implementation-stage checks
Controller: validate, enforce scope, checkpoint, publish draft, and coordinate review
Opus: bounded review plan
Sonnet: exact-head verification
Human: review and merge
```

Terra cannot block because a commit, exact-head digest, PR, or independent review does not yet exist.

### 3.4 Enforced review bounds

Review budgets are now enforced by the supervisor rather than only requested in prompts:

- maximum tool calls;
- maximum iterations;
- duplicate-command limit;
- persistent descendant-zombie detection;
- total and inactivity timeouts;
- one review-stage attempt by default;
- nonretryable provider budget/authentication circuit breakers.

A stalled reviewer can be resumed at Opus or Sonnet against the existing exact checkpoint without rerunning Sol and Terra.

### 3.5 Monitoring corrections

The temporary monitor now:

- follows the newest stage as the run progresses;
- reads bounded tails rather than the full JSONL on every refresh;
- displays stage, iteration, retained tool-call count, idle time, terminal summary, and recent controller transitions;
- reports only descendants of active bootstrap controller processes;
- detects repeated commands and persistent zombie descendants;
- avoids recursively printing accumulated prompt content.

### 3.6 Security and data governance

Revision 3 adds an explicit security/data-governance plan covering:

- trust boundaries;
- data classification and context release;
- prompt-injection boundaries;
- credential scoping;
- event/log redaction;
- supply-chain provenance;
- outer sandbox requirements;
- external-effect authorization;
- secure observability and retention.

Security is a gate before runtime authority transfers from the bootstrap to PAF.

## 4. Important areas that were undervalued

### Event volume, deduplication, and backpressure

The repeated `event.accumulated` payloads demonstrated that normalized events need:

- bounded payloads;
- content-addressed large bodies;
- deduplication;
- sampling rules for high-rate telemetry;
- consumer cursors;
- backpressure;
- retention and compaction policy.

These are now included in the observability and runtime plans.

### Operator action safety

Read-only status and watch interfaces can arrive early. State-changing GUI/TUI operations must wait until:

- durable state;
- policy evaluation;
- accountable principal identity;
- idempotent operation handling;
- effect receipts;
- recovery semantics.

A button is an authority-bearing interface, not merely presentation.

### Review is a specialized runtime policy

Review should not be an unrestricted coding-agent session. It requires:

- an immutable subject;
- controller-supplied validation evidence;
- changed-path and criterion linkage;
- stricter permissions;
- smaller tool and iteration budgets;
- duplicate/no-progress detection;
- explicit independence requirements.

### Execution-decision service, not a privileged intelligence agent

“Execution intelligence” is narrowed to deterministic and policy-governed decisions over durable facts:

- classify failure and progress;
- select among authorized retry transitions;
- detect no progress or oscillation;
- request planning or human escalation.

A model may propose or explain a decision, but it does not become a permanent privileged controller.

### Release, migration, and rollback

Every contract/runtime slice must eventually include:

- schema migration;
- conformance vectors;
- release manifest;
- backup/restore;
- known-good rollback;
- compatibility policy;
- revocation of bad releases.

Self-hosting without rollback would be unsafe.

## 5. Residual limitations

Revision 3 deliberately does not claim to solve the following:

1. **No kernel-enforced outer sandbox yet.** Cline command permissions are not a complete security boundary.
2. **No deny-by-default network namespace yet.**
3. **No complete native-output secret redaction yet.** Sensitive provider/framework output may still reach raw evidence.
4. **No durable event-sourced PAF campaign store yet.** The temporary campaign ledger uses locked atomic JSON.
5. **No distributed lease or multi-controller operation.**
6. **No provider-native budget telemetry.** Circuit breakers currently classify returned errors; they do not query authoritative remaining allowance.
7. **Review enforcement depends on known Cline event shapes.** Adapter conformance tests must detect incompatible event-schema changes.
8. **No production local-model scheduler or deployment manager yet.**
9. **No stable web-GUI API yet.** The temporary monitor is diagnostic bootstrap tooling.
10. **No cryptographic signing or organizational attestation yet.**
11. **Stage resume remains recovery tooling**, not the final durable campaign-recovery implementation.
12. **No automatic merge or self-promotion**, intentionally.

These are backlog targets, not reasons to postpone wire identity.

## 6. Readiness decision

Revision 3 is strong enough to:

- merge the revised plans and temporary bootstrap tooling;
- initialize the authorized bootstrap backlog;
- generate BS-010 from a clean exact `dev/panda`;
- require operator review and authorization of the generated task;
- run BS-010 through the bounded controller;
- monitor it and resume exact-head review without rerunning implementation.

It is not strong enough to:

- run unsupervised for days;
- handle sensitive credentials or classified data under a production assurance claim;
- allow arbitrary MCP servers or network access;
- transfer runtime authority from the bootstrap to PAF;
- perform protected publication or promotion without a human.

## 7. Required gate before each generated task

Before `paf-cline-campaign run`:

1. repository is clean or the exact preserved task branch is intentionally adopted;
2. generated base SHA equals current `origin/dev/panda`;
3. task contract and allowed paths were inspected;
4. validation commands are appropriate and non-destructive;
5. API keys are present and the Cline daemon has matching credentials;
6. provider budget is sufficient;
7. human/operator authorization is recorded;
8. monitoring is available;
9. publication remains draft-only and merge remains human-controlled.

## 8. Recommendation

Proceed with revision 3. Do not reopen the architecture broadly.

The next architectural implementation task remains BS-010, wire identity and canonical form. Security/event governance, shadow runtime evidence, bootstrap observability, durable state, and the secure generic runtime follow as explicit prerequisites before dynamic autonomous campaigns.
