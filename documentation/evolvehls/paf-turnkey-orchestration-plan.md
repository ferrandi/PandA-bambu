# PAF Turnkey Orchestration Plan

**Status:** post-PR #25 planning synchronization, 2026-08-03
**Targets:** PAF-05A3a.5, PAF-05B, PAF-05C, PAF-09, PAF-10, and PAF-12. The [roadmap](agentic-roadmap.md) allocates milestones; the [requirements matrix](paf-contract-requirements-matrix.md) is the normative completeness audit; see the [provenance plan](paf-contract-provenance-plan.md) and [soundness analysis](paf-architecture-soundness-analysis.md).

The bootstrap controller is useful A2 evidence, not the future supported PAF runtime. This document specifies operation and recovery requirements without creating a second contract authority.

## Turnkey surface

Normal operation remains:

```text
agentctl setup
agentctl doctor
agentctl run <objective-or-task>
```

The supported surface also includes:

```text
agentctl plan
agentctl status
agentctl explain
agentctl decisions
agentctl approve
agentctl pause
agentctl resume
agentctl evidence
agentctl doctor --refresh
```

`setup` establishes ignored local state after policy questions. `doctor` reports health; `doctor --refresh` refreshes capability/catalog evidence but never silently mutates a pinned run. `plan` creates Work Proposals or views, not silent authority. `run` resolves an authorized objective or Work Item. `status` exposes Work Graph and stage health. `explain` and `decisions` expose candidates, policy inputs, alternatives, and rationale. `approve` records an exact-subject governance decision. `pause` and `resume` use compatible checkpoints. `evidence` resolves retained artifacts, attestations, validation, and review.

## Durable bootstrap requirements

Setup and operation require deterministic task delivery; provider/profile/model preflight; ambient-secret presence and scope checks without recording values; adapter event-version detection; normalized terminal success, failure, cancellation, and ambiguity events; profile-state isolation; observable stage health; stale capability/catalog detection; and checkpoint compatibility plus recovery guidance. Portable semantics name no provider, model, client, endpoint, or profile.

Effective capability is the intersection of model, environment, access binding, policy, and task. Unknown quota is unknown, never infinite. Catalogs are binding-scoped and pinned for reproducibility-sensitive work.

## Work Graph operation and methodology

`agentctl plan` and adaptive execution operate on Objective, Work Proposal, Work Item, Work Graph Snapshot, and Work Graph Mutation as allocated by PAF-05A3a.3. Generated work remains a proposal until authorized. Graph mutations carry revisions, digests, preconditions, idempotency, authority, and conflict outcomes. Backlogs, boards, sprint plans, epic trees, and Kanban views project the graph only.

Scrum-like, Kanban-like, research-development, and hybrid research/hardening/release/maintenance profiles are switchable presentation and operating modes. A methodology transition is explicit and cannot alter objective, acceptance, authority, or evidence-retention semantics.

## Routes and explicit switching

The independently switchable dimensions are role; model and version; provider/runtime; agentic environment; endpoint; access binding; credential reference; account/project/organization; funding source; local/cloud/CI/HPC compute; adapter; repository/workspace; team topology; parallelism; methodology; DSE strategy; experiment fidelity; and context/artifact representation.

Every switch is an explicit Transition Decision with source route, alternatives, capability evidence, policy decision, budget and funding impact, data/licensing/network evaluation, reproducibility impact, selected transition, receipt or failure, and checkpoint or rollback behavior. It may never silently cross funding, credential, data, licensing, network, account, reviewer-independence, or pinned-evaluation boundaries. A route cannot expand authority.

## Parallel teams and independence

Operation supports read-only investigators, competing planners, isolated implementers, alternative implementations, parallel test/oracle development, independent and complementary reviewer ensembles, integration agents, DSE workers, and cross-repository/site-local teams. Independence is recorded and evaluated over session, model family, environment, access binding, mutable workspace, context visibility, prior conflicting roles, and route selection—not labels.

Fan-out has explicit join conditions. Quorum, dissent, conflict handling, adjudication, duplicate suppression, cancellation, partial completion, budget reconciliation, and retained evidence are required. A failed or cancelled branch cannot erase successful sibling evidence.

## Budgets, resources, checkpoints, and recovery

Budget Request, Budget Allocation, Reserve, Ledger Entry, Actual Consumption, Renewal/Increase Decision, and Exhaustion Decision are distinct facts. Resource Leases cover workspaces, branches, CI runners, local/cloud/HPC compute, clusters, EDA licenses, FPGA boards, datasets, and instruments. Retries and duplicate delivery cannot exceed an allocation; leases expire, renew, revoke, and fence stale workers.

A portable checkpoint retains exact work/graph revision, decisions and assumptions, artifact/repository digests, validation and findings, child state, budget/lease state, next action, portable context summary, and native references when available. Recovery describes retry safety, approved alternatives, reproducibility impact, rollback, and cross-environment resume.

## Acceptance requirements

Validate each CLI operation; preflight and incompatible event versions; normalized terminal outcomes; profile isolation; successful and failed explicit transitions; refusal of protected-boundary crossings; graph revision and conflicts; pause/resume and rollback; independence policy; partial success/cancellation; budget/lease exhaustion; cross-environment resume; and absence of named providers/models from portable semantics.