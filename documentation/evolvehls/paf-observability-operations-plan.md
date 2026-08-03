# PAF Observability and Operations Plan

Status: permanent cross-cutting plan  
Version: 2026-08-03 revision 3

## 1. Principle

Monitoring is part of correctness. An autonomous campaign is not operationally acceptable when an operator must inspect raw JSONL, process tables, and detached worktrees to determine whether an agent is reasoning, waiting, looping, blocked by quota, or stalled.

All operator interfaces consume durable PAF events and projections. No CLI, TUI, GUI, dashboard, or alerting service becomes a source of truth.

## 2. Two-stage operations strategy

Read-only observability begins with the shadow runtime and is made durable after the event journal exists. Authority-bearing pause, cancel, retry, and approval controls become active only after the secure generic runtime and policy interfaces can enforce them.

```text
R3/R3-O: live read-only bootstrap events and monitor
R4/R4-O: durable projections, health, alerts, query API
R5+: authenticated and policy-governed operator actions
```

## 3. Operations plane

```text
Durable redacted event journal and evidence
          ↓
projection and query service
  ├── paf status
  ├── paf watch
  ├── terminal TUI
  ├── web/API dashboard
  ├── alerts
  └── evidence/timeline export
```

## 4. Required projections

- Campaign Status;
- hierarchical Work Graph;
- Work Item and Step status;
- Attempt and Stage status;
- current framework/model/route/deployment;
- tool and MCP activity;
- validation and review checklist;
- health, progress, and stall assessments;
- retries and route transitions;
- budgets, cost, tokens, and resources;
- pending authority and operator decisions;
- external effects and evidence finalization;
- security/redaction and conformance status.

## 5. Operator-visible attempt states

```text
queued
preparing
waiting-for-resource
waiting-for-provider
reasoning
requesting-tool
tool-running
validating
finalizing-evidence
checkpointing
reviewing
retrying
recovering
blocked
stalled
complete
```

## 6. Progress and stall detection

Signals include:

- event heartbeat and source cursor;
- last unique tool action;
- repeated or semantically equivalent command batches;
- criteria resolved;
- new evidence and artifacts;
- worktree changes;
- provider latency;
- descendant-process health and zombie children;
- budget use without material progress;
- repeated failure signatures;
- no-progress duration;
- enforced runtime-policy budget exhaustion.

Stall classification and recommended actions are durable records, not ephemeral UI heuristics.

## 7. Event quality and scale

The operations plane must distinguish:

- raw native events;
- normalized events;
- material-progress events;
- derived projections;
- alerts.

It suppresses repeated accumulated prompt/output snapshots, externalizes large payloads, supports cursors and backpressure, limits previews, and exposes explicit truncation or dropped-event records.

## 8. Security and privacy

Monitoring is a trust boundary.

Required controls:

- redaction before durable persistence;
- field-level classification;
- exact-secret suppression;
- role-based access and query authorization;
- retention and export policy;
- tamper-evident event segments;
- audit receipts for operator actions and exports;
- no hidden chain-of-thought collection.

The operator sees structured decisions, evidence, assumptions, alternatives, uncertainty, and policy basis.

## 9. Interfaces

Initial read-only CLI:

```text
paf status CAMPAIGN
paf watch CAMPAIGN
paf work list CAMPAIGN
paf attempt show ATTEMPT
paf logs ATTEMPT
paf health CAMPAIGN
paf budget CAMPAIGN
paf evidence show ATTEMPT
paf decisions CAMPAIGN
```

After the secure-runtime gate:

```text
paf pause CAMPAIGN
paf resume CAMPAIGN
paf cancel ATTEMPT
paf retry ATTEMPT
```

The development bootstrap provides analogous `paf-cline-monitor` and stage-resume commands until the permanent CLI exists.

## 10. GUI strategy

Build interfaces in this order:

1. stable read-only query/event API;
2. CLI status and watch;
3. terminal TUI;
4. web GUI;
5. institutional alert and workflow integrations.

The web GUI should include:

- campaign overview and hierarchical graph;
- live timeline/Gantt;
- attempt detail and current operation;
- route/model/framework topology;
- criterion-level completion;
- validation and review findings;
- cost, token, and resource charts;
- local-model health and GPU placement;
- MCP/retrieval provenance and trust;
- approval inbox;
- retry and route-transition history;
- security/conformance warnings.

## 11. Authority

Observation and operation are separate.

```text
Viewer: inspect authorized projections
Operator: pause, resume, cancel, request permitted retry
Reviewer: submit findings and verdict
Authority holder: approve protected transitions
Administrator: configure runtime, resource, retention, and security policy
```

Every state-changing operator action is authenticated, journaled, policy-evaluated, and linked to an accountable principal.

## 12. Alerts

Required alert classes:

- provider authentication, quota, or budget block;
- inactivity or tool-supervisor stall;
- repeated-action/no-progress loop;
- local-model OOM or resource starvation;
- evidence-finalization failure;
- ambiguous external effect;
- expired lease;
- pending human decision;
- review disagreement;
- generated-work growth limit;
- deadline or budget threshold;
- redaction/security-policy failure;
- campaign completion.

## 13. Bootstrap acceptance scenario

1. Start a Cline attempt.
2. Display exact stage, route, model, current operation, elapsed and idle time.
3. Inject a hanging tool batch and zombie children.
4. Detect `tool-supervisor-stall` without relying solely on file mtime.
5. Alert the operator.
6. Resume only the affected review stage at the exact checkpoint.
7. Restart the controller.
8. Reconstruct the same status from events.
9. Verify secrets and accumulated prompt echoes are absent from normalized output.
10. Export the complete timeline into the Evidence Package.
