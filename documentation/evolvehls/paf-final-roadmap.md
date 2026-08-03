# PAF Final Roadmap

Status: implementation baseline
Version: 1.0
Date: 2026-08-03

## Decision on the bootstrap

Continue with the corrected Cline bootstrap now.

Do not replace it with OpenHands, LangGraph, Temporal, CrewAI, AutoGen/Microsoft Agent Framework or another framework before PAF identity, contracts, durable state, runtime SPI and evidence boundaries exist.

External frameworks enter through explicit conformance milestones.

## Milestones

### R0 — Architecture closure
Final specification, ecosystem decisions, Runtime Specification, security plan, four-graph architecture, observability and bootstrap invariants.

### R1 — Wire identity
Logical/revision identities, typed digests, canonical encoding, namespaces, versioning, migration/extensions and cross-language vectors.

### R2 — Core contracts/events
Common envelopes, events/errors/provenance, authority, artifacts, budgets/resources, model/deployment/routes and graph references.

### R2-R — Refinement Graph
Intent, objectives, requirements, assumptions, constraints, strategies, revisions, consistency and traceability.

### R2-I — Interoperability profiles
MCP, A2A, OpenTelemetry, OPA, in-toto/SLSA and durable-backend mappings.

### R2-S — Security/context release
Classification, origin labels, credentials, context release, redaction, retention, supply-chain admission and commit authorization.

### R3 — Shadow runtime/evidence
Normalize Sol/Terra/Opus/Sonnet, process, tool, validation and review events; build Evidence Packages and OTel export prototype.

### R3-E — Replay/evaluation/EDA evidence
Replay Package, Evaluation Record, Toolchain Epoch, simulation/formal/synthesis/PPA evidence and fidelity-aware results.

### R3-O — Bootstrap monitoring/conformance
Status/watch, process health, no-progress detection, bounded review, provider circuit breakers and event-schema tests.

### R4 — Durable state/backend SPI
SQLite event journal, blobs, snapshots, leases, checkpoints, effects, idempotency, replay, reconciliation and DurableExecutionBackend.

### R4-B — Backend evaluation
Compare local SQLite, Temporal, Dapr, LangGraph and Argo.

### R4-O — Durable operations
Campaign/Work/Attempt projections, timelines, health/stalls, budgets/resources, alerts and read-only CLI/TUI/API.

### R5 — Secure generic runtime/Cline
Supervisor, outer sandbox, scoped credentials, permissions, cancellation, recovery and Cline conformance.

### R5-X1 — OpenHands spike
Run one common Attempt Contract through Cline and OpenHands.

### R5-X2 — Agent-loop spike
Use OpenAI Agents SDK or Microsoft Agent Framework for planner/critic/specialist loops while PAF remains authoritative.

### R6 — Routes/local and specialized models
Model Artifact/Deployment, EDA capability profile, probes, resource scheduling, offline execution and authorized fallback.

### R7 — Completion/retry/health/commit
Criterion-level and multi-fidelity completion, progress/failure classification, health tripwires, retries, route transitions, compensation and commit-time authorization.

### R8 — Governed MCP/A2A
MCP admission/per-tool grants/retrieval provenance first; A2A remote-agent mapping second.

### R9 — Hierarchical planning
Intent → requirements → strategies → Work Graph → plans → steps → attempts, with bounded generated work, critics, policy and atomic mutations.

### R9-E — FIZZ DSE
Study, Experiment, Candidate, Factor, Metric, Fidelity, ParetoSet and SelectionDecision.

### R10 — Heterogeneous adapters
Claude Code, selected OpenHands adapter and optional Agents SDK/Microsoft Agent Framework adapters.

### R11 — EvolveHLS self-hosted campaign
High-level objective to validated Bambu/SODA-OPT change, EDA-specialized route, synthesis/PPA, injected failure, restart recovery, exact-head review and human-protected merge.

### R12 — FIZZ end-to-end campaign
Intent to architecture alternatives, candidates, HLS/RTL, synthesis/PPA, Pareto frontier and human selection.

### R13 — Bootstrap retirement
Remove duplicate authority, retain known-good recovery, stabilize packaging, release and rollback.

### R14 — Federation/governed evolution
Signed catalogs, cross-site evidence, approved learning datasets, fine-tuning/model promotion and protected PAF evolution.

## Initial backlog

```text
BS-000  Merge final architecture/specification
BS-010  Wire identity and canonical form
BS-020  Core contract and event envelope
BS-025  Refinement Graph contracts
BS-030  Shadow runtime and Evidence Package
BS-035  Replay and EDA evaluation records
BS-040  Durable state and recovery
BS-043  DurableExecutionBackend SPI
BS-045  Durable observability projections
BS-050  Secure generic runtime
BS-051  Cline adapter
BS-055  OpenHands conformance spike
BS-060  Routes, specialized models and EDA profile
BS-070  Completion, retry, health and commit authorization
BS-080  MCP gateway
BS-085  A2A gateway
BS-090  Hierarchical planning and graph mutation
BS-095  FIZZ Study/Experiment/DSE
BS-100  Claude Code and agent-loop adapters
BS-110  EvolveHLS self-hosted campaign
BS-120  FIZZ end-to-end campaign
BS-130  Bootstrap retirement
```
