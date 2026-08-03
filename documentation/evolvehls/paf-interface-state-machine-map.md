# PAF Interface and State-Machine Map

Status: skeletal implementation index; update in every slice  
Version: 2026-08-03 revision 3

## 1. Dependency spine

```text
Identity and Canonical Form
  ↓
Contract / Error / Event Envelope
  ↓
Security Classification / Redaction / Context Release
  ↓
Charter / Authority / Policy ─── Artifact / Context / Tool / Resource
  ↓                                      ↓
Objective / Work Item / Step ─── Resolved Execution Route
  ↓                                      ↓
Attempt Contract ── Runtime ── Normalized Events
  ↓                         ├── Health / Progress / Stall
Result / Receipts / Validation / Review
  ↓
Evidence Service / Manifest
  ↓
Completion Assessment
  ↓
Retry / Route Transition or Planning Trigger
  ↓
Proposal / Critique / Policy / Accepted Mutation
  ↓
Work Graph Snapshot / Scheduling / Operations Projections
```

## 2. Interface index

| Interface | Owner | Operations | Key states/errors | Evidence/authority | Milestone |
|---|---|---|---|---|---|
| Logical ID, Revision ID, Typed Digest | identity registry | create, validate, compare, resolve | invalid form, unknown type/version | canonical vectors | R1 |
| Contract Envelope | contract registry | validate, register, negotiate, migrate | unsupported/unknown version | producer/provenance | R2 |
| Event and Error Envelope | event service | append, replay, verify, segment | ordering, causality, corruption, truncation | cursor/digest | R2 |
| Classification and Redaction | security/event gateway | classify, redact, release, deny | secret detected, release denied, policy mismatch | security policy | R2-S |
| Context Release Decision | security/context service | filter, transform, authorize route release | denied/partial/stale | Charter/data policy | R2-S/R8 |
| Campaign Charter and grants | authority service | accept, amend, revoke, evaluate scope | expired/revoked/insufficient | accountable authority | R2+ |
| Objective / Requirement | objective service | create, revise, assess | active/satisfied/blocked | Charter | R3/R9 |
| Bootstrap Task Contract | bootstrap controller | generate, validate, authorize, stale, supersede | generated/authorized/stale | backlog digest/base SHA/operator action | R0/R3 |
| Work Item / Plan / Step | Work Graph/planning | propose, authorize, revise, execute | ready/running/blocked/complete | accepted mutation | R9 |
| Execution Route | route resolver | enumerate, filter, rank, freeze | ineligible/stale/opaque | policy/capability snapshot | R6 |
| Model Deployment / Resource | runtime/scheduler | probe, reserve, start, health, release | cold/OOM/unavailable/contended | resource receipt | R6 |
| Runtime / Review Policy | controller/policy | set budgets, retries, tools, resume rules | exhausted/violated | Charter/policy | R3/R5 |
| Permission Profile | policy/runtime | resolve, translate, attest | partial/unenforceable | grant/policy decision | R5 |
| Framework Adapter | runtime | probe, prepare, start, events, cancel, recover, resume | unsupported/native failure | adapter conformance | R5/R10 |
| Attempt | controller/runtime | prepare, dispatch, supervise, finalize, recover | created→running→terminal | Attempt Contract | R3-R5 |
| Stage | runtime/controller | start, budget, stop, resume | ready/running/blocked/stalled/terminal | exact checkpoint/context | R3/R5 |
| Tool/MCP Invocation | runtime/tool service | authorize, invoke, cancel, reconcile | denied/timed-out/ambiguous | grant and receipt | R8 |
| Health Assessment | monitoring/runtime | observe, classify, recommend | healthy/waiting/degraded/stalled | event/process evidence | R3-O/R4-O |
| Progress Assessment | execution decision service | compare criteria/evidence/history | progress/no-progress/oscillation | attempt history | R7 |
| Stall Assessment | monitoring/runtime | detect, classify, alert | provider/tool/loop/resource stall | process/tool evidence | R3-O/R4-O |
| Provider/Budget Circuit Breaker | runtime/controller | classify, open, reset | auth/quota/budget/policy block | binding/policy change | R3/R7 |
| Result / Receipt | producer/runtime | record claim/effect | success/failure/ambiguous | exact route/operation | R3+ |
| Validation Record | validation service | execute, record, compare | pass/fail/incomplete | command/tool digest | R3+ |
| Review Result / Independence | review service | request, execute, assess | approved/blocked/invalid | exact subject/policy | R3+ |
| Evidence Component / Manifest | runtime + evidence service | collect, assemble, finalize, verify | finalized/incomplete/unverifiable | child refs/digests | R3 |
| Completion Assessment | completion service | evaluate criteria | complete/incomplete/blocked/unknown | evidence refs | R7 |
| Retry Policy / Route Transition | controller | classify, select, authorize | retry/switch/replan/stop | failure/progress/policy | R7 |
| Retrieval Record / Context Update | retrieval/context | query, filter, transform, attach | stale/denied/incomplete | source trust/lineage | R8 |
| Work Proposal / Graph Mutation | planner/controller | propose, critique, authorize, apply | accepted/rejected/conflict | policy/review | R9 |
| Resource Reservation / Lease | scheduler | reserve, renew, preempt, release | queued/leased/expired | budget/resource snapshot | R6+ |
| Campaign/Attempt Projection | operations plane | query, stream, reconstruct | fresh/stale/corrupt | event cursor/digest | R4-O |
| Alert / Operator Action | operations/policy | raise, acknowledge, execute | open/ack/resolved/denied | principal/policy | R4-O/R5 |
| Release / Migration Manifest | release authority | stage, verify, promote, rollback | candidate/promoted/revoked | independent review/human authority | R12+ |

## 3. Runtime state machine

```text
CREATED
→ PREPARING
→ READY
→ DISPATCH_INTENT_RECORDED
→ STARTING
→ RUNNING
→ FINALIZING
→ process-complete | process-failed | refused | cancelled |
  timed-out | runtime-unavailable | credential-or-budget-block |
  evidence-incomplete | effect-ambiguous
```

Observable substates: waiting-for-resource, waiting-for-provider, reasoning, requesting-tool, tool-running, validating, checkpointing, reviewing, recovering.

## 4. Tool state machine

```text
requested
→ authorized | denied
→ started
→ result | failed | timed-out | cancelled | effect-ambiguous
```

## 5. Work Item completion state machine

```text
authorized
→ planned
→ ready
→ executing
→ assessing
→ complete
   | incomplete→retry/replan
   | blocked
   | unverifiable
   | effect-ambiguous
   | superseded
```

Agent status markers are claims inside Result records, not state-transition authority.

## 6. Bootstrap task state machine

```text
generated
→ authorized
→ running
→ approved→merged
   | blocked→retry or review-resume
   | failed→recover/retry
   | stale→supersede/regenerate
   | superseded
```

## 7. Planning state machine

```text
material trigger
→ evidence/gap assessment
→ proposal set
→ deterministic validation
→ critic/review as required
→ policy/authority decision
→ accepted or rejected mutation
→ new Work Graph revision
```

## 8. Recovery state machine

```text
controller restart
→ acquire lease/fencing token
→ replay journal
→ verify workspace/process/effects/stage
→ Recovery Decision
→ reattach | resume-stage | finalize | retry | reconcile | block
```

## 9. Monitoring state machine

```text
events/resources/process observations
→ health and progress projection
→ healthy | waiting | degraded | stalled | policy-budget-exhausted
→ alert
→ operator/controller action
→ resolved or escalated
```
