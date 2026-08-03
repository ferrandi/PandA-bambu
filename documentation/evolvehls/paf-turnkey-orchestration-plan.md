# PAF Turnkey Orchestration Plan

**Status:** final post-PR #26 operational realization; specification only.
**Authority:** public operational behavior, controller loops, routing, scheduling, recovery and administration. The v1 CLI implementation remains compatibility evidence and is not renamed.

SODA-SPRITZ is the user intent/steering/approval layer above the domain-independent PAF API and CLI. The normative public surface is:

```text
paf setup        paf doctor       paf plan        paf run         paf status
paf explain      paf decisions    paf approve     paf pause       paf resume
paf evidence
```

`setup` resolves layered configuration and policy; `doctor` reports adapter/catalog/security health; `plan` creates proposals only; `run` starts authorized work; `status`, `explain`, `decisions`, and `evidence` expose projections and immutable facts; `approve` records an exact-subject decision; pause/resume use compatible checkpoints. Existing v1 CLI code is unchanged compatibility evidence, not this public contract.

## Campaign startup and controller loop

Startup accepts a Charter, resolves configuration/policy into immutable snapshots, discovers capabilities/catalogs, performs initial planning, authorizes the initial graph, and schedules work. The deterministic controller validates Charter/grants/policy, schemas/revisions/idempotency, budgets/leases, protected boundaries, graph mutations and evidence. It stops/escalates when authority is absent. Reasoning planners and critics only assess evidence, propose/rank work, and challenge drift/feasibility/instability.

Material planning triggers and the full proposal/mutation/stability contract are owned by the provenance plan. The operational loop performs them only at a recorded trigger, applies policy/human gates, schedules accepted items, and finalizes attempt evidence even for failure, cancellation, timeout, refusal, ambiguity, and interrupted cleanup. A checkpoint records the deterministic next action, exact graph/work/route/policy state, artifacts, findings, budget/leases and compatible portable context.

## Routes, adapters, scheduling, and continuity

Route eligibility is the intersection of task requirements, model, environment, adapter, workspace/security, access-binding permissions, data/licensing/network policy, and budget/resources. The immutable route has independent model, provider/runtime, environment, adapter, binding, workspace/executor, tools, context, policy and resource axes; unknown/opaque observations reduce claims. Adapters expose detect, inspect, doctor, discover models/capabilities, prepare, session, invoke, events, input, cancel, checkpoint/resume, context export, result/receipt collection and cleanup, with honest L0–L5 conformance. Context Package plus Transfer Decision/Receipt provides portable continuity, never hidden reasoning.

Cycle Policy may pin, fall back, specialize roles, retry/switch, escalate/downgrade, round-robin, ensemble, compete, shadow/ablate, adapt portfolios, prefer local/restricted routes, or escalate fidelity. It always bounds eligible routes, attempts/switches, budgets/reserves, transitions, independence, oscillation/no-progress, stopping and human gates.

Scheduling records priority/deadline, fairness/starvation, WIP/concurrency, resource fit, placement/anti-affinity, reservations/leases, aging, preemption, coordinated teams, budget reconciliation and replayable explanation. Stores and policy engines are replaceable; configuration follows documented precedence; Git/issue/board/document adapters use references, cursors, requests/receipts, mappings, drift/conflict/reconciliation and cannot replace the Work Graph.

## Protected operation

Before 05B, security preflight requires isolation, filesystem/process/time limits, deny-by-default network with approved egress, scoped ephemeral secrets, untrusted-input classification, controller/target separation, supply-chain evidence, destructive authorization and forensic cleanup. Incident flow stops affected work, preserves evidence, revokes credentials/extensions, verifies audit, recovers under human gates, and creates corrective governed work. Air-gapped operation exports/imports signed portable campaign/evidence bundles with allowed classes, transformations/redaction, partial export, disconnected execution, reconciliation, duplicate/conflict provenance, residency, and delayed policy/catalog handling.

Acceptance covers every command, protected-boundary denial/gating, route degradation, scheduling explanation, checkpoint recovery, package finalization and external-sync drift; transport success never proves semantic or external-effect success.
