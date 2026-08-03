# PAF Product Roadmap

Status: permanent product plan  
Version: 2026-08-03 revision 3

## 1. Product boundary

PAF is a domain-independent, auditable autonomous-engineering control plane. SODA-EVOLVE is a domain extension and proving ground, not the owner of portable PAF semantics.

```text
SODA-SPRITZ
  user intent, steering, explanation, approvals
        ↓
PAF
  authority, durable state, security, runtime, monitoring,
  planning, execution, evidence, review, routing, recovery
        ↓
SODA-EVOLVE
  compiler/HLS/hardware/system semantics
        ↓
SODA-FIZZ + EvolveHLS + SODA-OPT + PandA-Bambu
```

## 2. Permanent architectural planes

### Contract and identity plane

Canonical identities, versions, typed digests, contracts, errors, events, provenance, migration, extension, and conformance.

### Authority and policy plane

Campaign Charter, grants, policy decisions, obligations, human boundaries, funding, data, security, and protected effects.

### Security and data-governance plane

Trust boundaries, data classification, Context Release Decisions, redaction, secret handling, prompt-injection defense, supply-chain identity, retention, access control, and effect classification.

### Durable state and evidence plane

Event journal, Work Graph mutations, snapshots, checkpoints, attempts, receipts, validation, review, completion, recovery, and evidence packages.

### Runtime and resource plane

Generic executor, adapters, process/tool supervision, sandboxing, scoped credentials, MCP, local/remote model deployments, scheduling, reservations, and cancellation.

### Observability and operations plane

Status, timelines, health, progress, stalls, budgets, alerts, CLI/TUI/GUI/API, and audited operator actions.

### Planning and execution-decision plane

Hierarchical planning, critics, Work Proposals, graph mutation, Completion Assessment, deterministic retry policy, route switching, no-progress detection, and budget-aware campaign health.

This plane is not a privileged permanent “intelligence agent.” Initially its decisions are deterministic and policy-driven; model advice is evidence, not authority.

### Domain-extension plane

Generic Study/Experiment semantics in PAF; compiler, HLS, hardware, Toolchain Epoch, fidelity, fabrication, and silicon semantics in SODA-EVOLVE.

## 3. Core invariants

- autonomy does not imply authority;
- planners propose and controllers mutate;
- credentials and technical ability are not grants;
- events and accepted mutations are authoritative; views are derived;
- runtime process success and agent claims do not establish Work Item completion;
- every attempt ends with complete evidence or an explicit evidence/ambiguity state;
- runtime, model, provider, framework, deployment, binding, funding, workspace, tools, permissions, and resources remain distinct route dimensions;
- unknown capabilities remain unknown;
- no planner, reviewer, runtime, GUI, framework, or model becomes an alternative authority;
- no hidden chain-of-thought is required or persisted; structured decisions and evidence are required;
- PAF cannot approve or promote its own replacement;
- the bootstrap and PAF never contain duplicate authoritative logic after cutover;
- security and redaction apply before event persistence, not only before external publication.

## 4. Capability progression

```text
P0 contracts, identity, and trust boundaries
P1 durable single-attempt execution and evidence
P2 monitored, recoverable bounded workflow
P3 dynamic retry and route switching
P4 hierarchical adaptive campaign
P5 heterogeneous frameworks and local/remote models
P6 federated organizational operation
P7 governed learning and self-evolution
```

## 5. Runtime strategy

The PAF Runtime Specification is the stable contract for Cline, Claude Code, local agents, API-only routes, MCP-backed tools, and the future self-hosted runtime.

Runtime responsibilities include:

- bounded invocation;
- normalized events and evidence components;
- process/tool supervision;
- permissions and sandbox;
- model/deployment identity;
- health and progress observations;
- cancellation and recovery.

The Evidence service, not the framework adapter, owns the final immutable Evidence Package Manifest. Runtime components do not decide objectives, authority, Work Graph mutations, semantic completion, or publication.

## 6. Dynamic switching and retries

Switching is driven by Completion Assessment, failure classification, progress, resource state, capability evidence, budget state, and Retry Policy.

Supported transitions include:

```text
same route retry
model change in same framework
framework change
local deployment change
local-to-remote escalation
fresh or resumed session
continue or clean-checkpoint remediation
replan Work Item
propose prerequisite
stop and escalate
```

Authority, funding, data jurisdiction, security, and reviewer independence cannot change silently. Missing-key, authentication, exhausted-budget, and policy-denial failures are circuit-breaker events, not blind retry triggers.

## 7. Hierarchical campaigns

```text
Campaign Objective
→ Work Graph
→ Work Item
→ Work Item Plan
→ Steps
→ Attempts
→ Evidence and Completion Assessment
→ local replan, route transition, or reviewed graph mutation
```

Generated work is bounded by depth, count, mutation, replan, budget, and authority limits. Each material planning change is deterministically validated and receives risk-proportional critique or review.

## 8. Local models

Local support is first-class through Model Artifact and Model Deployment identities, resource placement, health, capability probes, offline execution, and local-to-remote authorized fallback.

Deployment identity includes weights, tokenizer, quantization, template, runtime, serving configuration, hardware, and limits. Capability evidence is deployment- and route-specific.

## 9. Retrieval and MCP

Retrieval is governed execution:

```text
authorized query
→ approved tool/MCP server
→ bounded retrieval
→ source, trust, and transformation lineage
→ Context Release Decision
→ Context Package update
```

MCP is not a permission bypass. Servers and tools have explicit identities, effects, scopes, credentials, receipts, sandbox boundaries, and monitoring.

## 10. Observability

PAF exposes a normalized durable event stream and read-only operations API before building a GUI. CLI, TUI, web GUI, alerts, and evidence exports are projections.

Operators must be able to distinguish reasoning, provider wait, tool execution, validation, review, retry, recovery, blockage, and stall without raw-log archaeology. Event volume, duplicate accumulated output, backpressure, redaction, retention, and role-based access are part of the design.

## 11. Conformance and release discipline

Every stable interface has:

- versioned schemas;
- valid and adversarial vectors;
- reference or mechanically independent verifiers;
- failure-injection tests;
- migration and rollback rules;
- conformance levels and capability claims;
- a known-good release manifest.

Database and contract migrations are reversible or backed by a verified restore path. A new PAF runtime cannot promote itself.

## 12. Long-term milestones

- contract/identity substrate;
- security and event-governance substrate;
- durable monitored runtime;
- secure Cline adapter;
- explicit route/capability/local-model support;
- completion/retry/switching;
- retrieval/MCP;
- hierarchical planning;
- Claude Code and additional adapters;
- first self-hosted campaign;
- standalone extraction;
- federation and organizational authority;
- long-horizon adaptive campaigns;
- capability intelligence and portfolio optimization;
- governed self-evolution.
