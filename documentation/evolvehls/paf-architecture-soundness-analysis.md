# PAF Architecture Soundness Analysis

**Status:** post-PR #25 advisory architecture review, 2026-08-03
**Scope:** documentation and recommendations only. The [requirements matrix](paf-contract-requirements-matrix.md) remains the normative completeness audit; the [roadmap](agentic-roadmap.md) assigns milestones; the [provenance plan](paf-contract-provenance-plan.md) explains the architecture; and the [turnkey plan](paf-turnkey-orchestration-plan.md) specifies operation.

## Current strengths and review conclusion

The accepted matrix already establishes provider/model neutrality, request-versus-authority separation, Result-versus-Receipt separation, immutable evidence, binding-scoped discovery, budget boundaries, portable checkpoints, human gates, DSE integrity, independent governance dimensions, and v1/v2 compatibility intent. This review finds that the next safe coding step is the compact kernel in **PAF-05A3a.2a**, once the documented decisions below are synchronized. It does not invalidate PR #25.

## Small semantic kernel

One authoritative top-level schema per board, role, methodology, or display would create duplicated authority and unbounded migration cost. The kernel should instead be compact:

| Kernel element | Responsibility |
|---|---|
| Contract Envelope / Contract/Object Reference / Version | Typed, versioned object boundary and reference resolution. |
| Canonicalization Profile / Typed Digest | Canonical bytes and domain-bound integrity claim. |
| Principal Reference / Attestation | Portable actor reference and verifiable claim. |
| Artifact / Representation | Immutable content and the form exposed or transformed. |
| Provenance Event | Append-only lineage/state transition. |
| Governance Decision | Exact-subject authority, restriction, or gate. |
| Error Record | Normalized, attributable failure without erasing prior evidence. |

Work, execution, DSE, learning, and extension profiles reuse this kernel; views remain projections.

## Identity, canonicalization, principals, and attestations

Logical identity persists across revisions; revision/version identity names an immutable state; content digest identifies canonical bytes. The current Python sorted/indented JSON helper is implementation-deterministic, but it is not a language-neutral standard. PAF-05A3a.2a must publish a versioned PAF canonical JSON profile covering duplicate-key rejection, UTF-8, Unicode/normalization, numeric restrictions, key ordering, array/set treatment, domain separation, schema/version binding, self-digest exclusion, and cross-language vectors. A digest is meaningful only with algorithm, profile, domain, schema, and version.

Portable principals are human, service, agent, controller, organization, and delegated role. Authority-bearing human or policy decisions cite principal, authority source, delegated scope, exact subject digest, operation, constraints, policy-input snapshot, and expiration/revocation when applicable. An Execution Receipt is only a structured claim until its issuer, executor, subject digest, and attestation validate; validation does not establish semantic correctness.

## Work Graph and decision soundness

The authoritative objects are Objective, Work Proposal, Work Item, Work Graph Snapshot, and Work Graph Mutation. Proposals may be generated from evidence but are not authorized work. Mutation operations cover split, merge, supersession, reprioritization, deferral, cancellation, dependency change, prerequisite insertion, reassignment, and regeneration. Every mutation includes prior revision/digest, ordered operations, preconditions, idempotency key, cited authority, resulting revision/digest, and conflict outcome. Completed evidence survives all replanning.

| Decision | May decide | Must not replace |
|---|---|---|
| Route Candidate Set | Viable alternatives | A route selection or authority. |
| Routing Decision | Initial permitted route | A transition or policy decision. |
| Transition Decision | Explicit protected route change | Work Graph mutation or authority expansion. |
| Workflow Adaptation Decision | Permitted execution-shape adjustment | Replanning or policy. |
| Replanning Decision | Authorized graph change | Routing or publication authority. |
| Policy Decision | Authority, restrictions, limits | Human judgment where a human gate is required. |
| Human Decision | Recorded human judgment/approval | Attested external effect. |

## Environment, resources, results, and publication

Access Binding cannot subsume environment, adapter/version, provider runtime, endpoint, model/version, credential reference, account/project/organization scope, funding source, data boundary, capability evidence, or catalog snapshot. A switch records source route, alternatives, capability evidence, policy decision, funding/budget impact, data/licensing/network evaluation, reproducibility impact, selection, receipt/failure, and checkpoint/rollback. It must never silently cross funding, credentials, data, licensing, network, account, reviewer-independence, or pinned-evaluation boundaries.

Budget Request, Budget Allocation, Reserve, Ledger Entry, Actual Consumption, Renewal/Increase Decision, Exhaustion Decision, and Resource Lease are separate facts. Leases include workspaces, branches, runners, clusters, EDA licenses, FPGA boards, datasets, and instruments.

Result owns semantic outcome; Validation Record owns checks; Review Result owns exact-subject findings/verdict; Evaluation Record owns normalized comparison; Feedback Record owns later external evidence; Execution Receipt owns attested effects; ChangeSet owns coordinated changes. Publication always follows request, authority decision, and attested receipt.

## Distributed-systems gaps

The runtime and contracts require the following invariants before durable/federated operation:

| Failure mode | Required invariant |
|---|---|
| Retry or duplicate delivery | Idempotency keys; retry safety; exactly-once external effects where feasible, otherwise deduplication and reconciliation. |
| Partial success or compensation | Retain every branch result; record compensation and reconciliation without rewriting evidence. |
| Reordering/concurrency | Causal ordering, optimistic concurrency/preconditions, explicit conflict outcome, and graph revision fencing. |
| Expired/stale workers | Lease expiry, renewal, revocation, fencing, and rejection of late results after cancellation. |
| Deadlock/livelock/no progress/task explosion | Detect cycles, bounded retries, progress thresholds, graph-size/rate limits, escalation, and safe stop. |
| Upstream drift/external sync conflict | Pinned inputs, drift detection, reconciliation policy, and preserved competing evidence. |
| Checkpoint mismatch | Version compatibility/migration checks; unsupported state fails closed. |
| Revocation between authorization and effect | Revalidate relevant policy before an external effect and record the race outcome. |

## Security, trust, and governance gaps

Repositories, issues, papers, logs, and artifacts are potential prompt-injection carriers. Inputs need untrusted-input classification and propagation. Adapter/plugin identity and version, package/container/tool provenance, signed releases and receipts, and SBOMs where appropriate require a future trust design. That design must also define tenant/organization isolation, delegated authority, separation of duties, retention minima/maxima, legal/institutional hold, deletion/tombstones, lineage-preserving redaction, licensing and export restrictions, and knowledge freshness, epistemic status, and confidence.

No cryptographic assurance is promised before selection of signing formats, key management, trust anchors, rotation, and revocation procedures.

## Migration, extraction, and boundary review

v2 uses a PAF-owned namespace; v1 `evolvehls.agentic.*` documents remain compatibility fixtures. Migration derives a new object with source lineage and preserves missing facts as missing. Relocation cannot alter portable identity or digest. Consumer adapters isolate repository behavior, while independent tests, dependency-boundary tests, standalone installation/import/CLI smoke tests, licensing, and contributor provenance prove extraction readiness.

PAF portable semantics contain no Bambu, SODA-EVOLVE, SODA-OPT, MLIR, HLS, provider, model, client, path, or private endpoint type. Compiler, HLS, codesign, fidelity, toolchain epoch, and semantic-equivalence concepts belong in SODA-EVOLVE adapters and namespaced extension profiles, preserving the roadmap's dependency arrows.

## Framework comparison

The following are project/vendor capability claims, checked against primary sources on 2026-08-03, not independent benchmarks.

| Project | Source-backed capability relevant here | Primary source |
|---|---|---|
| LangGraph | Graph-based stateful agents with persistence and human-in-the-loop features. | [LangGraph overview](https://langchain-ai.github.io/langgraph/) |
| AutoGen | Conversational/event-driven multi-agent applications and extensions. | [AutoGen documentation](https://microsoft.github.io/autogen/) |
| CrewAI | Agents/crews and stateful event-driven flows. | [CrewAI documentation](https://docs.crewai.com/) |
| MetaGPT | Role-based multi-agent software workflows. | [MetaGPT repository](https://github.com/geekan/MetaGPT) |
| SWE-agent / mini-SWE-agent | Tool-using repository-task agents; project documentation recommends mini-SWE-agent. | [SWE-agent documentation](https://swe-agent.com/latest/) |
| OpenHands | Software-agent SDK and repository/local/cloud execution offerings. | [OpenHands documentation](https://docs.all-hands.dev/) |
| OpenAI Agents SDK | Agents, tools, handoffs, guardrails, sessions, tracing, sandboxing, and human controls. | [OpenAI Agents SDK documentation](https://openai.github.io/openai-agents-python/) |

PAF/SODA-EVOLVE differentiates only through its integrated design/toolchain co-evolution, durable Objective/Work Graph evolution, autonomy/authority separation, access/funding/data-aware switching, exact-head independent review, federated ChangeSets, progressive-fidelity codesign, governed evidence/learning, and turnkey multi-environment operation. It makes no universal uniqueness claim about an individual feature.

## Recommendation allocation and blocking conditions

| Recommendation | Milestone |
|---|---|
| Kernel, identity, canonicalization, principals, attestations, errors, namespace decision | PAF-05A3a.2a |
| Reusable governance/budget/lineage/resource primitives | Later PAF-05A3a.2 slices |
| Work Graph authority, result/review/validation/publication | PAF-05A3a.3 |
| DSE fidelity and epochs | PAF-05A3a.4 and SODA-EVOLVE X-A–X-E |
| Environment/access/switching/leases | PAF-05A3a.5 and PAF-12 |
| Learning and promotion | PAF-05A3a.6, PAF-13, X-H |
| Cross-language/adversarial/portability conformance | PAF-05A3a.7 and PAF-05C |
| Supported A2, extraction, adaptive/federated operation | PAF-05B, PAF-06, PAF-09, PAF-10; X-F/X-G |

Before PAF-05A3a.2a implementation, agree on the kernel boundary, three identity layers, v2 namespace ownership, canonicalization decision process, principal/attestation minimum fields, error model, no Bambu/SODA-EVOLVE dependency, and migration/conformance responsibilities. With those synchronized, PAF-05A3a.2a is the next coding task.