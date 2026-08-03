# EvolveHLS Portable Agent Framework Roadmap

**Status:** post-PR #25 synchronization and architecture refinement, 2026-08-03
**Authority:** this roadmap assigns milestones; the [requirements matrix](paf-contract-requirements-matrix.md) remains the normative PAF-05A3a.1 completeness audit. See also the [turnkey plan](paf-turnkey-orchestration-plan.md), [contract and provenance plan](paf-contract-provenance-plan.md), and [soundness analysis](paf-architecture-soundness-analysis.md).

PAF-05A2 merged in PR #23 at `be25381df5f8f50363a98d9ff5557364d8656e1f`. PAF-05A3a.1 merged in PR #25 at `0f3accc999f4402d211fdfca4cb0555117f855a1`. The latter remains accepted matrix work; this document is a post-merge refinement, not a rewrite of that history. **PAF-05A3a.2 is the next implementation phase, and PAF-05A3a.2a is its immediate next coding task.**

## Product and dependency boundary

PAF is a **domain-independent autonomous engineering control plane**. SODA-EVOLVE is the flagship hardware/software codesign application and depends on PAF, never the reverse.

```text
PAF
  generic autonomous engineering infrastructure
        ↓
SODA-EVOLVE domain extension
  compiler/HLS/codesign semantics
        ↓
SODA-FIZZ + EvolveHLS
        ↓
SODA-OPT, PandA-Bambu, and implementation tools
```

Portable PAF semantics and v2 vocabulary must not depend on Bambu, SODA-OPT, SODA-FIZZ, SODA-EVOLVE domain types, MLIR, HLS or compiler-specific types, named models, providers, clients, repository paths, private endpoints, or private configuration. Bambu and SODA-OPT are consumer adapters or workspace identities, never core vocabulary. Domain semantics enter only through adapters and namespaced extension profiles.

## Invariants

- Autonomy is independent of authority, risk, budget, data sensitivity, duration, publication impact, infrastructure impact, and reproducibility mode.
- A request or proposal does not grant authority; a generated Work Proposal becomes a Work Item only through applicable policy or human authorization.
- One semantic fact has one authoritative owner. Boards, backlogs, sprint views, epic trees, Kanban views, roles, and ceremonies are projections or configuration, not competing authorities.
- Logical identity, immutable revision/version identity, and content digest are distinct. A digest includes its algorithm, canonicalization profile, domain, schema, and version.
- Results own semantic outcomes; receipts attest attempted effects and are claims until issuer, executor, exact subject, and attestation validate. Neither a receipt nor a result alone establishes the other fact.
- Pinned evaluation and reproducibility runs do not silently change route, fidelity, catalog, or protected boundary. Missing evidence remains missing, unknown, or `null`; failure never becomes a successful zero.
- No orchestration framework owns PAF contracts, authority, Work Graphs, checkpoints, or evidence.

## Incubation and standalone extraction

PAF remains temporarily incubated in this fork only through:

```text
PAF-05A3 contract convergence
→ PAF-05A3b trusted executor
→ PAF-05B supported A2 coding cycle
→ PAF-05C second-runtime proof
→ PAF-06 standalone extraction
```

PAF-06 is a firm gate before the large A3/A4 adaptive, federated, and distributed runtime. Every new v2 implementation is extraction-ready from its first change: a portable PAF-owned package and contract namespace; no Bambu or SODA-EVOLVE imports; independent and dependency-boundary tests; v1 `evolvehls.agentic.*` compatibility adapters; relocation-stable portable identities and digests; standalone installation/import/CLI smoke tests; and licensing plus contributor-provenance review. Repository-specific build, CI, and workspace behavior remains in consumer adapters. Extraction proves portability of the established boundary, not stability of a future A3–A5 API.

## Autonomy model

| Level | Meaning |
|---|---|
| A0 — Assistive | Advice or generated material; a human directly controls execution. |
| A1 — Delegated stage | One bounded stage is delegated under explicit inputs and authority. |
| A2 — Bounded workflow | A controller runs a finite workflow with gates, limits, evidence, and recovery. |
| A3 — Adaptive development campaign | The system revises Work Graphs and routes within explicit policy. |
| A4 — Federated autonomous engineering organization | Durable cross-team, cross-repository, cross-site operation. |
| A5 — Governed self-evolution | Evidence-backed changes to PAF policies or capabilities under promotion controls. |

PR #25 is A2 bootstrap evidence, not proof of a supported PAF runtime. The shell/bootstrap controller is not that supported runtime.

## PAF-05A3a — contract convergence

### PAF-05A3a.1 — requirements matrix — complete

Merged in PR #25. The [matrix](paf-contract-requirements-matrix.md) supplies the accepted completeness audit, 69 use cases, compatibility inventory, and v1 migration rules.

### PAF-05A3a.2 — small semantic kernel and shared primitives — next phase

**PAF-05A3a.2a — next coding task** defines the language-neutral kernel: Contract Envelope; Contract/Object Reference; Version; Canonicalization Profile; Typed Digest; Principal Reference; Artifact; Representation; Attestation; Provenance Event; Governance Decision; and Error Record. It resolves logical-object versus revision versus content identity, a PAF-owned v2 namespace, schema/version registries, digest domain separation, portable principals and attestation subjects, language-neutral conformance vectors, and v1 treatment. It selects and publishes the exact canonical JSON profile rather than prematurely naming one here.

Later 05A3a.2 slices may add reusable governance, budget, unit, lineage, and resource primitives, but must reuse this kernel and must not add envelopes, references, or UI/methodology schemas with competing authority.

### PAF-05A3a.3 — Work and Work Graph contracts

Owns Objective, Work Proposal, Work Item, Plan, Workflow, Work Graph Snapshot, Work Graph Mutation, Result, Review Result, Validation Record, ChangeSet, publication requests, and work-lifecycle checkpoints. Split, merge, supersession, reprioritization, deferral, cancellation, dependency change, prerequisite insertion, assignment, and regeneration are mutation kinds, not separate authoritative families. Each mutation records prior revision/digest, ordered operations, preconditions, idempotency key, cited policy or human decision, resulting revision/digest, and conflict outcome. Evidence from completed work survives replanning, cancellation, and partial failure.

### PAF-05A3a.4 — DSE contracts

Owns immutable candidates, experiment fidelity, toolchain epochs, semantic-equivalence obligations, progressive-fidelity evaluation, and leases for scarce experimental resources.

### PAF-05A3a.5 — execution, environment, access, and switching

Owns generalized Execution Request, Handoff, Receipt, and Interaction Trace; environment, adapter, provider/runtime, endpoint, model, catalog, access, credential-reference, account/project/organization, funding, data-boundary, capability, and resource-lease identities; Route Candidate Sets, Routing Decisions, Transition Decisions, and transition receipts. It must not create a generic Decision Record or Adaptation Decision that duplicates policy, human, routing, transition, workflow-adaptation, or replanning authority.

### PAF-05A3a.6 — governed learning

Owns offline learning, evaluation, promotion, rollback, retention, eligibility, and redaction governance.

### PAF-05A3a.7 — conformance

Owns cross-language, cross-contract, migration, portability, negative, adversarial, and second-environment conformance.

## Generated work, methodologies, and parallelism

PAF supports high-level Objectives, generated Work Proposals, authoritative Work Items, and dependency-aware Work Graphs. Evidence may generate remediation and integration proposals. Explicit graph revisions support splitting, merging, supersession, reprioritization, deferral, cancellation, prerequisite insertion, reassignment, and regeneration. Objectives, success criteria, authority, prohibited scope, data boundaries, budgets, review requirements, publication rules, and fabrication policy may never change silently.

Scrum-like, Kanban-like, research-development, and hybrid research/hardening/release/maintenance modes are configurable methodology profiles. They change projections, not core semantics or the preceding protected facts.

PAF supports parallel read-only investigators, competing planners, isolated implementers, alternative implementations, test/oracle developers, independent and complementary reviewer ensembles, integration agents, DSE workers, and cross-repository/site-local teams. Independence is a verifiable policy predicate over session, model family, environment, access binding, mutable workspace, visible context, prior conflicting roles, and route selection. Fan-out/join, quorum, dissent, conflict handling, adjudication, duplicate detection, cancellation, budget reconciliation, and retained evidence are explicit.

## Later PAF milestones

| Milestone | Allocation |
|---|---|
| PAF-05A3b | Trusted executor after contract convergence. |
| PAF-05B | Supported A2 coding cycle with human merge gate. |
| PAF-05C | Second materially different runtime proof. |
| PAF-06 | Mandatory standalone-extraction gate. |
| PAF-07/08 | Federated ChangeSets; authorization, funding, and governance. |
| PAF-09 | A3 adaptive development campaigns and governed Work Graph revision. |
| PAF-10 | A4 durable/distributed operation; an adapter such as LangGraph may be used but is not authoritative. |
| PAF-11 | Capability and modality intelligence. |
| PAF-12 | Dynamic switching across all protected dimensions. |
| PAF-13 | A5 governed self-evolution. |

## SODA-EVOLVE domain realization

These are extension semantics, never PAF core contracts.

| Phase | Scope |
|---|---|
| X-A — Domain contracts and adapters | Application/algorithm intent, compiler/IR lineage, hardware-candidate identity, semantic equivalence, fidelity, cross-layer objectives, and capability gaps. |
| X-B — EvolveHLS autonomous toolchain evolution | Capability gap → generated compiler/HLS work → implementation → validation → independent review → integration → versioned toolchain capability. |
| X-C — SODA-FIZZ design evolution | Immutable candidates, feasibility, fidelity, experiments, measurements, Pareto updates, and recommendations. |
| X-D — Nested co-evolution | Design campaign → capability gap → EvolveHLS campaign → validated toolchain release → explicit campaign transition → resumed exploration. |
| X-E — Progressive-fidelity ladder | Semantic/model evaluation → compiler analysis → HLS estimation → RTL → FPGA/ASIC synthesis → place-and-route → package/system → prototype/silicon. |
| X-F — Federated campaigns | PAF, SODA-EVOLVE, SODA-OPT, Bambu, architecture libraries, memory generators, OpenFPGA, OpenROAD, benchmarks, and models. |
| X-G — Reference campaigns and baselines | Paper-to-implementation, graph/sparse ingestion, memory semantics, SPARTA, SVELTO, dataflow, OpenRAM, OpenFPGA, physically aware HLS, and emerging computing; compare one-agent, fixed-pipeline, manual-task, no-switch, no-toolchain-evolution, and single-fidelity baselines. |
| X-H — Governed domain learning | Improve decomposition, transformations, architecture proposals, fidelity escalation, test selection, review assignment, and stopping rules under PAF policy-promotion controls. |

A toolchain epoch binds exact compiler, HLS tool, library, model, adapter, and flow revisions. Each domain result declares equivalence as exact, tolerance-based numerical, observational, protocol, performance-only, or intentionally changed semantics.

## Framework differentiation

The [soundness analysis](paf-architecture-soundness-analysis.md#framework-comparison) records primary-source comparison links. LangGraph, AutoGen, CrewAI, MetaGPT, SWE-agent/mini-SWE-agent, OpenHands, and OpenAI Agents SDK each report some combinations of multi-agent orchestration, graphs or flows, sandboxes, handoffs, tracing, model selection, persistence, or human controls. PAF/SODA-EVOLVE does not claim any one feature as unique; its differentiation is the integrated combination of design/toolchain co-evolution, durable Objective and Work Graph evolution, autonomy/authority separation, access/funding/data-aware switching, exact-head independent review, federated ChangeSets, progressive-fidelity codesign, governed evidence and learning, and turnkey multi-environment operation.