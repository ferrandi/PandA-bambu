# PAF Architecture Soundness Analysis

**Status:** final post-PR #26 architecture closure assessment, 2026-08-03.
**Review scope:** documentation architecture only; no v2 runtime/schema claim. This assessment reconciles the [roadmap](agentic-roadmap.md), [provenance plan](paf-contract-provenance-plan.md), [turnkey plan](paf-turnkey-orchestration-plan.md), [legacy matrix](paf-contract-requirements-matrix.md), and [atomic traceability](paf-requirements-traceability.md).

## Closure conclusion and boundary analysis

The closed product stack is SODA-SPRITZ → PAF API/CLI → SODA-EVOLVE → SODA-FIZZ + EvolveHLS → implementation tools. PAF is generic; SODA-EVOLVE owns compiler, HLS, hardware, Toolchain Epoch, semantic equivalence and physical-fidelity semantics. Operational work runs campaigns and effects; evolution work proposes schemas, descriptors, workflows, methodology, routes, prompts, adapters, validators, evidence selection and extensions through ordinary Work Items, evidence, review and promotion. There is no privileged self-modification path.

The deterministic controller enforces the chain accountable authority → Charter → grants/policy → capability/assignment grants → execution. Credentials/capabilities are not authority. Planners/replanners interpret evidence and propose; critics challenge; P0–P4 assurance selects deterministic through human-governed planning. Protocol contracts own facts and transitions; stores, boards and dashboards are projections.

## Reconciled kernel and state authority

| Slice | Review finding |
|---|---|
| 2a | Identity/canonical form only: envelope, references, logical/revision/schema identity, profiles/digests, registry/negotiation, migration envelope, vectors. |
| 2b–2f | Artifacts/evidence; authority/policy/attestation; events/time/errors; budgets/resources; agents/context/tools respectively. |
| 3a–3f | Requirement/work/planning, scheduling, review/result, release, and adaptive planning contracts. |
| 4–7 | Generic DSE; execution/routes/security; governed evolution; integrated conformance. |

Immutable contracts are facts, accepted Work Graph Mutations are transitions, snapshots are derived, Results own semantic claims, Receipts own effects, and Events connect facts/transitions. A planner cannot directly mutate a graph. Evidence packages finalize every terminal attempt or explicitly fail closed as incomplete, unverifiable, or ambiguous; package aggregates never replace Result, Receipt, Validation, Review, or authority ownership.

## Portability and operational soundness

Execution Routes independently represent model, provider/runtime, coding environment, adapter, binding, workspace/executor, tools, context, policy and resources. Dynamic registration, negotiation, versioned adapters, conformance and honest L0–L5 degradation avoid closed provider/framework enumerations. Context packages and receipts—not hidden reasoning—permit cross-framework continuation. Policy/configuration/storage/scheduler/external sync and extension lifecycle have replaceable implementations but stable contract semantics.

The threat model covers malicious repositories/artifacts, injection, compromised adapters/models/systems, secret leakage, supply chain, tenancy, deputy/authority/audit attacks, exhaustion, poisoned capability facts and malicious generated work. PAF-05B is gated on isolation, allowlisted files, process limits, deny-by-default network, scoped secrets, classification, supply-chain evidence, destructive authorization and forensics. Reproducibility grades and incident/federation bundles are specified in the provenance plan.

## Unresolved decision ledger

| Decision ID and decision | Accountable owner | Gate | Required resolution evidence |
|---|---|---|
| PAF-DEC-001 namespace spelling | PAF architecture authority | 2a implementation completion | namespace reservation and migration examples |
| PAF-DEC-002 canonical JSON, Unicode and numeric domain | PAF architecture authority | 2a implementation completion | normative profile and cross-language vectors |
| PAF-DEC-003 identity format and digest separation | PAF architecture authority | 2a implementation completion | reference vectors and negative cases |
| PAF-DEC-004 schema/version negotiation and registry behavior | PAF architecture authority | 2a implementation completion | compatibility decision and reject vectors |
| PAF-DEC-005 migration envelope and vector languages | PAF architecture authority | 2a implementation completion | migration fixtures and implementation plan |
| PAF-DEC-006 signing/trust/rotation/revocation | security governance | 2c completion | security design; no cryptographic assurance is claimed |
| PAF-DEC-007 policy engine, storage backend, scheduler algorithm | operations architecture authority | 5/05B completion | replaceable implementation selection |
| PAF-DEC-008 extension and federation trust model | security/federation governance | 5e/07/08 completion | threat-model and interoperability review |

## Readiness criteria

Architecture closure is satisfied when the six documents remain consistent, every traceability row has positive/negative planned coverage and honest status, and the legacy corpus remains intact. PAF-05A3a.2a may start by recording PAF-DEC-001 through PAF-DEC-005 as its first bounded decision activities; those decisions gate implementation completion, not task start. Extraction readiness requires dependency-boundary, standalone import, migration, audit, adversarial and cross-language evidence; this closure does not claim it has been implemented.
