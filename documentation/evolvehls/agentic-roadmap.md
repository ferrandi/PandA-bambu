# EvolveHLS Portable Agent Framework — Refined Roadmap

**Status:** planning snapshot captured 2026-08-02  
**Current dependency:** PR #23 (`agent/portable-task-supervisor`) at `4bcdb25b86fb9cd7224c29be41f0567746bfdc99`

## Product vision

PAF is the portable, backend-neutral agent framework underlying EvolveHLS. It must support:

1. autonomous multi-agent development of PAF and the broader EvolveHLS software stack;
2. autonomous compiler, HLS, architecture, memory-system, FPGA, ASIC, and cross-layer design-space exploration;
3. governed improvement of PAF itself from auditable development and design evidence.

The normal user experience must be turnkey:

```text
agentctl setup
agentctl doctor
agentctl run <task>
```

Advanced declarative configuration remains available for reproducibility, automation, evaluation, and institutional policy, but must not be required for normal use.

## Architectural invariants

- PAF semantics are independent of Cline, Codex, Claude Code, LangGraph, any provider, and any model.
- Roles, models, agentic environments, endpoints, access bindings, credentials, funding classes, and account scopes are separate concepts.
- Models are discovered and evaluated within a specific access binding and environment.
- Two credentials for the same provider are distinct bindings and may expose different models, quotas, policies, projects, billing, or data boundaries.
- A task describes intent and success, not a provider, client, model, or local command line.
- A workflow describes collaboration, decomposition, joins, retries, remediation, and gates.
- An execution request describes one resolved and authorized attempt.
- Receipts and provenance describe what actually happened.
- Design-space exploration is represented as scientific search over immutable candidates and evidence, not as an ad hoc coding loop.
- Every consumed, produced, transformed, selected, rejected, reviewed, or evaluated object is an artifact or provenance event.
- Audit retention, evaluation eligibility, training eligibility, and redistribution eligibility are independent governance decisions.
- Running workflows cannot silently alter or promote their own trusted controller or policy.
- Merge and other consequential operations remain human-gated unless explicitly authorized by policy.
- Evaluation and ablation runs use pinned routing and reproducible catalog snapshots; development runs may adapt within policy.
- Dynamic fallback never crosses funding, credential, data, network, or authorization boundaries silently.

## Foundation

| Milestone | Objective | Status |
|---|---|---|
| PAF-01 | Provider contract and safe capability probing | Foundation completed |
| PAF-02 | Neutral roles, tasks, results, catalogs, deterministic model selection | Foundation completed |
| PAF-03A | Execution-profile and routing contracts | Foundation completed |
| PAF-03B | Turnkey bootstrap, environment detection, generated adapters | Foundation completed |
| PAF-04 | Durable architecture and inspection knowledge | Foundation completed |
| PAF-05A1 | Execution request, handoff preview, and receipt contracts | Merged in PR #22 |
| PAF-05A2 | Trusted worktree management and process supervision | Draft PR #23; final review pending |

The existing v1 task, result, execution-request, fixture-handoff, and execution-receipt contracts remain compatibility fixtures. They are not yet the complete interfaces for autonomous coding, federated workspaces, adaptive orchestration, or design-space exploration.

## PAF-05A3a — Complete contract, provenance, and learning architecture

**Dependency:** approved and merged PAF-05A2.

Before implementing fixture execution or an ad hoc Cline automation loop, converge the complete canonical contract family.

### PAF-05A3a.1 — Requirements and use-case matrix

Document and test requirements for:

- autonomous EvolveHLS coding;
- dynamic multi-agent planning, implementation, review, remediation, and integration;
- cross-repository work;
- design-space exploration;
- complete provenance;
- dataset and training governance;
- controlled evolution of PAF;
- turnkey setup and recovery;
- dynamic model/environment/access discovery;
- adaptive routing and budget exhaustion.

Assign each semantic responsibility to one authoritative contract and record trust boundaries and cross-contract invariants.

### PAF-05A3a.2 — Shared primitives

Define versioned primitives for:

- canonical identifiers and content digests;
- timestamps and event ordering;
- artifact descriptors and typed representations;
- provenance relations and transformations;
- media types, schemas, units, dimensions, and uncertainty;
- repository and workspace references;
- permissions and requested operations;
- authorization and policy decisions;
- budgets, reserves, estimates, and ledgers;
- workflow, stage, parent, child, attempt, and remediation lineage;
- sensitivity, licensing, retention, evaluation, training, and redistribution governance;
- redaction records preserving lineage and integrity;
- model, runtime, adapter, endpoint, account, project, funding, and access-binding identities.

### PAF-05A3a.3 — Autonomous coding contracts

Create or revise:

- Task v2;
- Plan;
- Workflow;
- Spawn request;
- Result v2;
- Review result and structured findings;
- Remediation task derivation;
- Change set;
- CI and validation evidence;
- Repository publication intent and outcomes;
- Runtime checkpoint;
- Adaptation decision.

Support dynamic planning, implementation, independent review, remediation, parallel read-only investigators, bounded isolated writers, cancellation, retries, resumability, and human-only merge.

Initial local policy may prefer Sol for coding planning, Terra for implementation, Opus for review planning, and Sonnet for review execution. These names are not hard-coded into portable semantics.

### PAF-05A3a.4 — Design-space exploration contracts

Create:

- Exploration study;
- Parameter space;
- Candidate;
- Experiment request;
- Measurement set;
- Exploration strategy;
- Campaign state;
- Pareto/result set;
- Decision report.

Support typed and conditional parameters, feasibility constraints, multi-objective optimization, multiple strategies, deterministic candidate identities, failed/infeasible candidates, repeated trials, noise and uncertainty, mixed fidelity, comparability rules, dynamic parallelism, fidelity escalation, checkpointing, and human gates for expensive experiments.

### PAF-05A3a.5 — Execution and environment integration contracts

Generalize:

- Handoff;
- Execution request;
- Execution receipt;
- Policy decision;
- Artifact visibility record;
- Interaction trace;
- Provenance event;
- Decision record;
- Evaluation record;
- Feedback record;
- Access binding;
- Credential reference;
- Endpoint;
- Account/project scope;
- Catalog snapshot;
- Discovered model;
- Capability evidence;
- Binding status;
- Binding budget;
- Binding transition policy;
- Routing decision;
- Budget policy and ledger;
- Runtime checkpoint.

Models are discovered per access binding and environment. Multiple keys for the same provider remain separate bindings with separate catalogs, policies, budgets, and provenance.

### PAF-05A3a.6 — Learning and governed PAF evolution contracts

Create:

- Dataset manifest;
- Learning run;
- Policy candidate;
- Policy evaluation;
- Policy release;
- Promotion decision;
- Rollback record.

Require immutable source evidence, explicit dataset-selection policies, separation of audit retention from training eligibility, licensing/consent/security restrictions, held-out and adversarial evaluation, independent review, human promotion, pinned releases, and rollback.

### PAF-05A3a.7 — Conformance suite

Prove autonomous coding, DSE, turnkey setup, multiple bindings, environment portability, adaptive budgets, checkpointing, governance, and controlled PAF evolution with valid, invalid, and adversarial fixtures.

**Acceptance criterion:** the contract family represents all required scenarios without provider-, model-, client-, repository-, or DSE-tool-specific schema extensions.

## PAF-05A3b — End-to-end fixture execution

Implement a trusted fixture executor consuming the converged contracts:

- explicit preview and authorization;
- isolated worktree;
- general handoff persistence;
- trusted process launch;
- result collection;
- evidence capture;
- receipts and provenance events;
- budget attribution;
- checkpoint creation;
- deterministic cleanup and retention.

**Acceptance criterion:** an approved fixture task runs end-to-end and emits a validated, content-addressed evidence graph.

## PAF-05B — Turnkey EvolveHLS autonomous coding reference cycle

Implement the first real autonomous coding cycle as a consumer of PAF contracts:

```text
plan → implement → independent review plan → review execution
     → structured remediation when blocked → human merge gate
```

The cycle must operate on a real EvolveHLS task, use isolated worktrees, support Cline first without embedding Cline semantics, reserve budget for review and final validation, stop or checkpoint on exhaustion, emit structured evidence, and require human merge authorization.

## PAF-05C — Second environment and access-mode portability proof

Execute the same canonical task/workflow through a materially different runtime family such as Codex, Claude Code, direct API, or a local direct executor.

## PAF-06 — Standalone PAF extraction

Move generic PAF infrastructure from PandA-Bambu into a standalone package and CLI. Bambu remains the first consumer and SODA-OPT becomes the second.

## PAF-07 — Federated workspace and coordinated change sets

Support multiple repositories, upstreams, forks, mirrors, pinned dependencies, exact revisions, per-agent access, coordinated branches/commits/PRs, cross-repository artifact edges, and restricted-source classifications.

## PAF-08 — Guardrails, authorization, funding, and governance

Implement operation authorization, credential/network policy, data boundaries, personal/project/institutional/subscription/API/local funding classes, binding-transition authorization, budget reserves, ledgers, human gates, and artifact/training governance.

## PAF-09 — Dynamic orchestration kernel

Implement backend-neutral workflow DAGs, dynamic fan-out/join, bounded retries and remediation, cancellation, failure isolation, lineage, adaptive routing, checkpoints, restart, and deterministic controller decisions.

## PAF-10 — Durable orchestration adapter

Add a LangGraph or equivalent durable backend for persistent checkpoints, resumption, interrupts, approvals, subgraphs, and long-running campaigns. PAF retains ownership of contracts, policies, evidence, and semantics.

## PAF-11 — Model, environment, capability, and modality intelligence

Build evidence-backed catalogs from provider/gateway discovery, environment-native discovery, local runtimes, declared metadata, bounded probes, historical PAF outcomes, and independent benchmarks. Support source code, images, PDFs, MLIR, RTL, waveforms, reports, LEF/DEF, GDSII, and derived representations with provenance.

## PAF-12 — Adaptive routing and budget optimization

Dynamically select and adapt model, provider/runtime, environment, access binding, account/funding source, concurrency, delegation, context, experiment batch size, fidelity, and retry/stop behavior. Every decision is recorded; no transition silently crosses a boundary.

## PAF-13 — Governed evidence-driven evolution of PAF

Use approved evidence to improve routing, decomposition, context selection, reviewer assignment, test selection, candidate proposal, fidelity escalation, and stopping policies through offline learning, held-out/adversarial evaluation, independent review, human promotion, pinned releases, and rollback.

## PAF-14 — EvolveHLS autonomous codesign and design-space exploration

Apply PAF to compiler transformation exploration, MLIR lowering and dialect evolution, HLS configuration, dataflow/accelerator architecture, dynamic scheduling, memory architecture and OpenRAM/FPGA macro mapping, FPGA/ASIC implementation, and cross-layer optimization.

## Target user experience

A user should be able to state:

```text
Implement this paper's algorithm in EvolveHLS, validate it, review it,
and explore the relevant compiler and architecture choices within this budget.
```

PAF should discover authorized environments, bindings, and models; ask minimal policy questions; normalize the objective; select and adapt the team; manage workspaces, budgets, evidence, and checkpoints; stop safely when needed; present only human-authority decisions; preserve complete provenance; and retain only explicitly eligible evidence for evaluation or learning.
