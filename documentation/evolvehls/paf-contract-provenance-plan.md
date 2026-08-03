# PAF Contract and Provenance Architecture Plan

**Status:** explanatory architecture plan synchronized after PR #25, 2026-08-03
**Authority chain:** the [requirements matrix](paf-contract-requirements-matrix.md) is the normative completeness audit; the [roadmap](agentic-roadmap.md) allocates milestones; this document explains contract/provenance architecture; the [soundness analysis](paf-architecture-soundness-analysis.md) records advisory gaps and recommendations; the [turnkey plan](paf-turnkey-orchestration-plan.md) specifies operation. Concrete v2 schemas remain future work.

New v2 contracts require a PAF-owned namespace whose spelling is decided by PAF-05A3a.2a. `evolvehls.agentic.*` remains a v1 compatibility namespace and fixture family.

## Layered architecture and ownership

Avoid one top-level schema per UI, methodology, or convenience view.

1. **Small semantic kernel (PAF-05A3a.2a):** Contract Envelope, Contract/Object Reference, Version, Canonicalization Profile, Typed Digest, Principal Reference, Artifact, Representation, Attestation, Provenance Event, Governance Decision, and Error Record.
2. **Work and Work Graph (PAF-05A3a.3):** Objective, Work Proposal, Work Item, Plan, Workflow, snapshots, mutations, Result, Validation Record, Review Result, ChangeSet, and publication requests.
3. **Execution and environment (PAF-05A3a.5):** execution, handoff, receipt, trace, environment/access identities, routing, transition, leases, and recovery.
4. **DSE (PAF-05A3a.4):** studies, immutable candidates, experiments, measurements, campaigns, fidelity, and comparability.
5. **Learning/governance (PAF-05A3a.6):** eligibility, datasets, evaluation, promotion, rollback, retention, and redaction.
6. **Domain extension profiles:** SODA-EVOLVE semantics through adapters, not portable PAF types.

## Identity, canonicalization, and migration

Logical object identity is stable across revisions. Revision/version identity identifies one immutable state of that object. A content digest identifies canonical bytes and is not a substitute for either identity. Existing sorted/indented Python JSON is implementation-deterministic only, not a language-neutral v2 standard.

PAF-05A3a.2a must select and publish a versioned canonical JSON profile with duplicate-key rejection, UTF-8 encoding, Unicode handling/normalization decision, numeric restrictions, key ordering, array/set treatment, digest domain separation, schema/version binding, self-digest exclusion, and cross-language conformance vectors. Unsupported versions and unknown authoritative fields fail closed.

Migration creates a derived object and preserves source lineage; missing v1 evidence remains missing/unknown/`null`. Package or repository relocation cannot change portable identity or digest. Compatibility adapters are boundary components. Portable v2 contracts cannot name Bambu or SODA-EVOLVE types.

## Principals, authority, and attestations

Portable principal kinds are human, service, agent, controller, organization, and delegated role. An authority-bearing human or policy decision cites principal, authority source, delegated scope, exact subject digest, operation, constraints, policy-input snapshot, and applicable expiration/revocation. Separation of duties is evaluated from recorded principals and policy.

An Execution Receipt starts as a structured claim. It is trusted evidence only after issuer, executor, exact subject digest, and attestation validate; even then, it establishes execution effects, not semantic correctness.

## Non-overlapping evidence and decisions

| Object | Sole responsibility |
|---|---|
| Result | Semantic outcome and requirement status. |
| Validation Record | Test/check evidence. |
| Review Result | Findings and verdict against an exact subject. |
| Evaluation Record | Normalized quality/performance comparison. |
| Feedback Record | Later external evidence. |
| Execution Receipt | Attested execution effects and consumption. |
| ChangeSet | Coordinated changes, not publication authority. |
| Publication request / decision / receipt | Requested effect / authority / attested external effect. |

Route Candidate Set lists viable routes; Routing Decision selects a route; Transition Decision changes route; Workflow Adaptation Decision changes permitted execution shape; Replanning Decision changes the Work Graph; Policy Decision grants/restricts authority; Human Decision records human judgment. A generic rationale wrapper may reference them but cannot compete with their authority.

## Work Graph, environment, and resources

PAF-05A3a.3 owns Objective, Work Proposal, Work Item, Work Graph Snapshot, and Work Graph Mutation. A mutation records prior revision/digest, ordered operations, preconditions, idempotency key, policy/human decision, resulting revision/digest, and conflict outcome. It covers split, merge, supersession, reprioritization, deferral, cancellation, dependency/assignment changes, prerequisite insertion, and regeneration.

Access Binding does not replace environment, adapter/version, provider/runtime, endpoint, model/version, credential reference, account/project/organization scope, funding source, data boundary, capability evidence, or catalog snapshot; each has an independent reference.

Budget Request, Budget Allocation, Reserve, Ledger Entry, Actual Consumption, Renewal/Increase Decision, Exhaustion Decision, and Resource Lease are distinct. Leases include workspaces, branches, runners, compute, clusters, EDA licenses, boards, datasets, and instruments.

## Validation and extraction

Conformance verifies schema/version resolution, identity/digest resolution, authorization monotonicity, idempotency, budget non-expansion, lineage, canonicalization vectors, artifact reachability, governance, protected transitions, fidelity/comparability, migration, adversarial inputs, and second-environment portability. PAF-05A3a.7 owns this work. Extraction requires independent tests, dependency-boundary tests, standalone installation/import/CLI smoke tests, and licensing/contributor-provenance review.