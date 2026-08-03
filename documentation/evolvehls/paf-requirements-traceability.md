# PAF Requirements Traceability

**Status:** final post-PR #26 atomic requirements-to-test planning authority. No planned fixture or test below exists merely by being named; statuses are deliberately not implementation claims.

## Conventions and automated review

IDs are immutable `PAF-REQ-<FAMILY>-<NNN>` and do not encode versions/milestones. `PF-` and `NF-` are stable planned positive/negative fixture identities; `CT-` is a planned conformance test identity. A planned traceability conformance check (`CT-TRACEABILITY-001`) must reject duplicate IDs, blank table cells, unknown family/status/milestone, absent positive or negative coverage, unresolved legacy references, and an implementation claim based solely on schema presence. Controlled statuses: `v1-compatibility-evidence`, `specified`, `planned`, `blocked-decision`, `partially-implemented`, `implemented-and-tested`.

| Requirement ID | Requirement | Source/use cases | Authoritative contract owner | Producers | Consumers | Milestone | Valid fixture | Invalid/adversarial fixture | Semantic invariant | Conformance test | Implementation status |
|---|---|---|---|---|---|---|---|---|---|---|---|
| PAF-REQ-IDENTITY-001 | Separate logical ID, immutable revision, canonical bytes and typed digest. | [paf-contract-requirements-matrix.md — Canonicalization and digest expectations](paf-contract-requirements-matrix.md#canonicalization-and-digest-expectations); PAF-UC-PROV-001 | 2a Contract Envelope | registry | all contracts | 2a | PF-IDENTITY-001 | NF-IDENTITY-001 | identity-layer-separation | CT-IDENTITY-001 | blocked-decision |
| PAF-REQ-AUTHORITY-001 | Charter and grants cover every effect; capability is not authority. | [paf-contract-provenance-plan.md — Authority and governed autonomy](paf-contract-provenance-plan.md#authority-and-governed-autonomy); PAF-UC-CODE-002, PAF-UC-CODE-007 | 2c Charter/Policy Decision | authority/policy | controller | 2c | PF-AUTHORITY-001 | NF-AUTHORITY-001 | authority-monotonicity | CT-AUTHORITY-001 | specified |
| PAF-REQ-WORK-001 | Only accepted mutations alter authorized Work Items/dependencies. | [paf-contract-provenance-plan.md — Graphs, planning, and APIs](paf-contract-provenance-plan.md#graphs-planning-and-apis); PAF-UC-CODE-005 | 3b Work Graph Mutation | controller | scheduler | 3b | PF-WORK-001 | NF-WORK-001 | snapshot-non-authority | CT-WORK-001 | specified |
| PAF-REQ-PLANNING-001 | Triggered planning produces proposal, assessment and authorized mutation only. | [paf-contract-provenance-plan.md — Graphs, planning, and APIs](paf-contract-provenance-plan.md#graphs-planning-and-apis); PAF-UC-CODE-001, PAF-UC-CODE-005 | 3f Planning Cycle | planner/critic | controller | 3f | PF-PLANNING-001 | NF-PLANNING-001 | planner-proposes-only | CT-PLANNING-001 | planned |
| PAF-REQ-EVIDENCE-001 | Every terminal attempt finalizes a package or explicit incomplete state. | [paf-contract-provenance-plan.md — Evidence and provenance](paf-contract-provenance-plan.md#evidence-and-provenance); PAF-UC-PROV-004 | 2b Evidence Package Manifest | controller | audit/replanner | 2b | PF-EVIDENCE-001 | NF-EVIDENCE-001 | attempt-total-evidence | CT-EVIDENCE-001 | specified |
| PAF-REQ-EXECUTION-001 | Step Instance records exact route, request, result, receipt and cleanup. | [paf-contract-provenance-plan.md — Graphs, planning, and APIs](paf-contract-provenance-plan.md#graphs-planning-and-apis); PAF-UC-CODE-010 | 5a Step Instance | executor | evidence/audit | 5a | PF-EXECUTION-001 | NF-EXECUTION-001 | transport-not-effect | CT-EXECUTION-001 | planned |
| PAF-REQ-API-001 | Mutations bind subject revision, idempotency, descriptor, authority and outcome. | [paf-contract-provenance-plan.md — Graphs, planning, and APIs](paf-contract-provenance-plan.md#graphs-planning-and-apis); PAF-UC-CODE-007 | 2f Operation Descriptor | API/controller | clients | 2f | PF-API-001 | NF-API-001 | idempotent-exact-subject | CT-API-001 | specified |
| PAF-REQ-CONTEXT-001 | Transfer uses Context Package decision/receipt, never hidden reasoning. | [paf-contract-provenance-plan.md — Portability, operations, and resilience](paf-contract-provenance-plan.md#portability-operations-and-resilience); PAF-UC-CODE-001, PAF-UC-ACCESS-014 | 2f Context Package | agent | successor agent | 2f | PF-CONTEXT-001 | NF-CONTEXT-001 | visible-context-lineage | CT-CONTEXT-001 | specified |
| PAF-REQ-MODEL-001 | Model identity/catalog facts include source, confidence and expiry. | [paf-contract-provenance-plan.md — Portability, operations, and resilience](paf-contract-provenance-plan.md#portability-operations-and-resilience); PAF-UC-ACCESS-001 | 5b Model Descriptor | discovery adapter | route resolver | 5b | PF-MODEL-001 | NF-MODEL-001 | honest-opaque-identity | CT-MODEL-001 | v1-compatibility-evidence |
| PAF-REQ-FRAMEWORK-001 | Adapters negotiate capabilities and report L0–L5 honestly. | [paf-contract-provenance-plan.md — Portability, operations, and resilience](paf-contract-provenance-plan.md#portability-operations-and-resilience); PAF-UC-ACCESS-003 | 5c Adapter Descriptor | adapter | controller | 5c | PF-FRAMEWORK-001 | NF-FRAMEWORK-001 | honest-conformance | CT-FRAMEWORK-001 | planned |
| PAF-REQ-ROUTING-001 | Route eligibility intersects ten independent axes and policy/resources. | [paf-turnkey-orchestration-plan.md — Routes, adapters, scheduling, and continuity](paf-turnkey-orchestration-plan.md#routes-adapters-scheduling-and-continuity); PAF-UC-ACCESS-013 | 5b Execution Route | resolver | scheduler | 5b | PF-ROUTING-001 | NF-ROUTING-001 | route-axis-independence | CT-ROUTING-001 | specified |
| PAF-REQ-BUDGET-001 | Allocations, reserves and consumption never silently expand. | [paf-contract-requirements-matrix.md — BUDGET — budgets and adaptation](paf-contract-requirements-matrix.md#budget--budgets-and-adaptation); PAF-UC-BUDGET-001 | 2e Budget Ledger | controller | scheduler | 2e | PF-BUDGET-001 | NF-BUDGET-001 | budget-non-expansion | CT-BUDGET-001 | v1-compatibility-evidence |
| PAF-REQ-RESOURCE-001 | Leases fence stale workers and record fit/reservation lifecycle. | [paf-turnkey-orchestration-plan.md — Routes, adapters, scheduling, and continuity](paf-turnkey-orchestration-plan.md#routes-adapters-scheduling-and-continuity); PAF-UC-BUDGET-007 | 2e Resource Lease | scheduler | executor | 2e | PF-RESOURCE-001 | NF-RESOURCE-001 | lease-fencing | CT-RESOURCE-001 | specified |
| PAF-REQ-POLICY-001 | Replaceable policy returns permit/obligation/deny/review/human decisions. | [paf-contract-provenance-plan.md — Authority and governed autonomy](paf-contract-provenance-plan.md#authority-and-governed-autonomy); PAF-UC-CODE-007, PAF-UC-DSE-017 | 2c Policy Decision | policy engine | controller | 2c | PF-POLICY-001 | NF-POLICY-001 | fail-closed-authority | CT-POLICY-001 | specified |
| PAF-REQ-CONFIG-001 | Layered configuration has deterministic precedence and immutable run snapshot. | [paf-contract-provenance-plan.md — Portability, operations, and resilience](paf-contract-provenance-plan.md#portability-operations-and-resilience); PAF-UC-ACCESS-014 | 5d Effective Configuration | resolver | controller | 5d | PF-CONFIG-001 | NF-CONFIG-001 | deterministic-precedence | CT-CONFIG-001 | planned |
| PAF-REQ-STORAGE-001 | Stores preserve addressing/integrity but never own semantic authority. | [paf-contract-provenance-plan.md — Contract layers and fact ownership](paf-contract-provenance-plan.md#contract-layers-and-fact-ownership); PAF-UC-PROV-001 | 2b Storage Reference | store | audit | 2b | PF-STORAGE-001 | NF-STORAGE-001 | storage-non-authority | CT-STORAGE-001 | specified |
| PAF-REQ-SCHEDULER-001 | Scheduling is fair, lease-aware and replayably explained. | [paf-turnkey-orchestration-plan.md — Routes, adapters, scheduling, and continuity](paf-turnkey-orchestration-plan.md#routes-adapters-scheduling-and-continuity); PAF-UC-BUDGET-007 | 3c Scheduling Decision | scheduler | controller | 3c | PF-SCHEDULER-001 | NF-SCHEDULER-001 | no-starvation-within-policy | CT-SCHEDULER-001 | planned |
| PAF-REQ-SECURITY-001 | 05B requires isolation, restricted network/secrets and forensic cleanup. | [paf-turnkey-orchestration-plan.md — Protected operation](paf-turnkey-orchestration-plan.md#protected-operation); PAF-UC-ACCESS-012 | 5e Security Profile | controller | executor | 5e/05B | PF-SECURITY-001 | NF-SECURITY-001 | deny-by-default-egress | CT-SECURITY-001 | planned |
| PAF-REQ-REPRO-001 | Runs declare reproducibility grade and observed nondeterminism. | [paf-contract-provenance-plan.md — Portability, operations, and resilience](paf-contract-provenance-plan.md#portability-operations-and-resilience); PAF-UC-PROV-010 | 2d Provenance Event | executor | evaluator | 2d | PF-REPRO-001 | NF-REPRO-001 | no-fabricated-replay | CT-REPRO-001 | specified |
| PAF-REQ-FEDERATION-001 | Portable bundles retain provenance through disconnected reconciliation. | [paf-turnkey-orchestration-plan.md — Protected operation](paf-turnkey-orchestration-plan.md#protected-operation); PAF-UC-CODE-008 | 5d Transfer Bundle | exporter/importer | federation controller | 5d/07 | PF-FEDERATION-001 | NF-FEDERATION-001 | cross-site-lineage | CT-FEDERATION-001 | planned |
| PAF-REQ-LEARNING-001 | Evidence selection records retention/evaluation/training/redistribution decisions. | [paf-contract-requirements-matrix.md — PROV — provenance and governed evolution](paf-contract-requirements-matrix.md#prov--provenance-and-governed-evolution); PAF-UC-PROV-009 | 6 Evidence Selection Decision | evaluator | learning pipeline | 6 | PF-LEARNING-001 | NF-LEARNING-001 | governance-separation | CT-LEARNING-001 | planned |
| PAF-REQ-EVOLUTION-001 | Evolution follows proposal, review, canary, promotion and rollback. | [paf-contract-requirements-matrix.md — PROV — provenance and governed evolution](paf-contract-requirements-matrix.md#prov--provenance-and-governed-evolution); [agentic-roadmap.md — Closure invariants](agentic-roadmap.md#closure-invariants); PAF-UC-PROV-012 | 6 Evolution Proposal | agents | promotion authority | 6/13 | PF-EVOLUTION-001 | NF-EVOLUTION-001 | no-self-promotion | CT-EVOLUTION-001 | planned |
| PAF-REQ-DSE-001 | Generic experiments retain candidate, measurement, uncertainty and comparability. | [paf-contract-requirements-matrix.md — DSE — design-space exploration](paf-contract-requirements-matrix.md#dse--design-space-exploration); [agentic-roadmap.md — Generic DSE and SODA-EVOLVE boundary](agentic-roadmap.md#generic-dse-and-soda-evolve-boundary); PAF-UC-DSE-001 | 4 Experiment | campaign controller | recommender | 4 | PF-DSE-001 | NF-DSE-001 | generic-comparability | CT-DSE-001 | v1-compatibility-evidence |
| PAF-REQ-SODA-EVOLVE-001 | Epoch/equivalence/physical semantics remain extension-owned and partition results. | [agentic-roadmap.md — Generic DSE and SODA-EVOLVE boundary](agentic-roadmap.md#generic-dse-and-soda-evolve-boundary); PAF-UC-DSE-008, PAF-UC-DSE-017 | SODA-EVOLVE extension | domain adapter | campaign | X-A–X-J | PF-SODAEVOLVE-001 | NF-SODAEVOLVE-001 | no-silent-cross-epoch-comparison | CT-SODAEVOLVE-001 | specified |
| PAF-REQ-TURNKEY-001 | The public operations are setup, doctor, plan, run, status, explain, decisions, approve, pause, resume and evidence. | [paf-turnkey-orchestration-plan.md — PAF Turnkey Orchestration Plan](paf-turnkey-orchestration-plan.md#paf-turnkey-orchestration-plan); PAF-UC-CODE-011 | Operational API/CLI | controller | user/SODA-SPRITZ | 05B | PF-TURNKEY-001 | NF-TURNKEY-001 | public-command-completeness | CT-TURNKEY-001 | planned |

## Decision ledger

| Decision ID | Affected requirements | Decision owner | Blocking milestone | Alternatives | Required resolution evidence |
|---|---|---|---|---|---|
| PAF-DEC-001 | IDENTITY-001 | PAF architecture authority | 2a completion | namespace spellings | registered namespace and migration sample |
| PAF-DEC-002 | IDENTITY-001 | PAF architecture authority | 2a completion | canonical JSON profiles | Unicode/numeric rules and vectors |
| PAF-DEC-003 | IDENTITY-001 | PAF architecture authority | 2a completion | ID/digest formats | positive/negative reference vectors |
| PAF-DEC-004 | IDENTITY-001, API-001 | PAF architecture authority | 2a completion | negotiation/registry behaviors | compatibility and reject vectors |
| PAF-DEC-005 | IDENTITY-001 | PAF architecture authority | 2a completion | migration envelope/vector languages | migration fixtures and implementation plan |
| PAF-DEC-006 | AUTHORITY-001 | security governance | 2c completion | signing/trust models | trust/rotation/revocation design |
| PAF-DEC-007 | POLICY-001, STORAGE-001, SCHEDULER-001 | operations architecture authority | 5/05B completion | replaceable engine/backend/algorithm choices | implementation selection record |
| PAF-DEC-008 | FEDERATION-001, SECURITY-001 | security/federation governance | 5e/07/08 completion | extension/federation trust models | threat model and interoperability review |

## Legacy use-case crosswalk

This explicit many-to-many crosswalk resolves every preserved legacy scenario to applicable atomic requirements. It is planning traceability, not evidence that a fixture or test exists; each destination row supplies its planned fixture, invariant, test, milestone, and controlled status. `CT-TRACEABILITY-001` is planned to validate this table against the 69 legacy IDs and all required cells.

| Legacy use-case ID | Applicable atomic requirement IDs |
|---|---|
| PAF-UC-CODE-001 | PAF-REQ-PLANNING-001, PAF-REQ-CONTEXT-001, PAF-REQ-AUTHORITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-002 | PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-BUDGET-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-003 | PAF-REQ-WORK-001, PAF-REQ-EXECUTION-001, PAF-REQ-RESOURCE-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-004 | PAF-REQ-IDENTITY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-005 | PAF-REQ-WORK-001, PAF-REQ-PLANNING-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-006 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-CONTEXT-001 |
| PAF-UC-CODE-007 | PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-API-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-008 | PAF-REQ-WORK-001, PAF-REQ-EXECUTION-001, PAF-REQ-RESOURCE-001, PAF-REQ-FEDERATION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-009 | PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-010 | PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-011 | PAF-REQ-EXECUTION-001, PAF-REQ-CONTEXT-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-012 | PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-CODE-013 | PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-WORK-001 |
| PAF-UC-CODE-014 | PAF-REQ-EVIDENCE-001, PAF-REQ-LEARNING-001, PAF-REQ-EVOLUTION-001 |
| PAF-UC-ACCESS-001 | PAF-REQ-MODEL-001, PAF-REQ-FRAMEWORK-001, PAF-REQ-CONFIG-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-002 | PAF-REQ-MODEL-001, PAF-REQ-FRAMEWORK-001, PAF-REQ-CONFIG-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-003 | PAF-REQ-MODEL-001, PAF-REQ-FRAMEWORK-001, PAF-REQ-CONFIG-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-004 | PAF-REQ-FRAMEWORK-001, PAF-REQ-ROUTING-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-005 | PAF-REQ-MODEL-001, PAF-REQ-FRAMEWORK-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-006 | PAF-REQ-FRAMEWORK-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-007 | PAF-REQ-CONFIG-001, PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-008 | PAF-REQ-ROUTING-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-BUDGET-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-009 | PAF-REQ-MODEL-001, PAF-REQ-FRAMEWORK-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-010 | PAF-REQ-MODEL-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-EXECUTION-001, PAF-REQ-CONTEXT-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-011 | PAF-REQ-FRAMEWORK-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-012 | PAF-REQ-ROUTING-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-SECURITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-013 | PAF-REQ-ROUTING-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-BUDGET-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-ACCESS-014 | PAF-REQ-ROUTING-001, PAF-REQ-CONTEXT-001, PAF-REQ-EXECUTION-001, PAF-REQ-CONFIG-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-001 | PAF-REQ-BUDGET-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-002 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-003 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-004 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-005 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-006 | PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-007 | PAF-REQ-BUDGET-001, PAF-REQ-WORK-001, PAF-REQ-SCHEDULER-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-008 | PAF-REQ-BUDGET-001, PAF-REQ-WORK-001, PAF-REQ-POLICY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-009 | PAF-REQ-BUDGET-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-BUDGET-010 | PAF-REQ-BUDGET-001, PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-CONTEXT-001 |
| PAF-UC-BUDGET-011 | PAF-REQ-BUDGET-001, PAF-REQ-ROUTING-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-001 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-BUDGET-001 |
| PAF-UC-DSE-002 | PAF-REQ-DSE-001, PAF-REQ-PLANNING-001, PAF-REQ-EVIDENCE-001, PAF-REQ-BUDGET-001 |
| PAF-UC-DSE-003 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-004 | PAF-REQ-DSE-001, PAF-REQ-IDENTITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-005 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-POLICY-001 |
| PAF-UC-DSE-006 | PAF-REQ-DSE-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-007 | PAF-REQ-DSE-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-008 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-POLICY-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-009 | PAF-REQ-DSE-001, PAF-REQ-WORK-001, PAF-REQ-EXECUTION-001, PAF-REQ-RESOURCE-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-010 | PAF-REQ-DSE-001, PAF-REQ-EXECUTION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-BUDGET-001 |
| PAF-UC-DSE-011 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-POLICY-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-012 | PAF-REQ-DSE-001, PAF-REQ-EXECUTION-001, PAF-REQ-CONTEXT-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-013 | PAF-REQ-DSE-001, PAF-REQ-WORK-001, PAF-REQ-PLANNING-001, PAF-REQ-POLICY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-014 | PAF-REQ-DSE-001, PAF-REQ-WORK-001, PAF-REQ-IDENTITY-001, PAF-REQ-FEDERATION-001, PAF-REQ-EVIDENCE-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-015 | PAF-REQ-DSE-001, PAF-REQ-BUDGET-001, PAF-REQ-WORK-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-DSE-016 | PAF-REQ-DSE-001, PAF-REQ-EXECUTION-001, PAF-REQ-BUDGET-001, PAF-REQ-POLICY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-EVIDENCE-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-017 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-RESOURCE-001, PAF-REQ-BUDGET-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-DSE-018 | PAF-REQ-DSE-001, PAF-REQ-EVIDENCE-001, PAF-REQ-POLICY-001, PAF-REQ-SODA-EVOLVE-001 |
| PAF-UC-PROV-001 | PAF-REQ-IDENTITY-001, PAF-REQ-CONTEXT-001, PAF-REQ-STORAGE-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-002 | PAF-REQ-IDENTITY-001, PAF-REQ-STORAGE-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-003 | PAF-REQ-IDENTITY-001, PAF-REQ-CONTEXT-001, PAF-REQ-STORAGE-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-004 | PAF-REQ-ROUTING-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-005 | PAF-REQ-IDENTITY-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-006 | PAF-REQ-IDENTITY-001, PAF-REQ-STORAGE-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-007 | PAF-REQ-LEARNING-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-008 | PAF-REQ-LEARNING-001, PAF-REQ-POLICY-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-009 | PAF-REQ-IDENTITY-001, PAF-REQ-LEARNING-001, PAF-REQ-POLICY-001, PAF-REQ-REPRO-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-010 | PAF-REQ-IDENTITY-001, PAF-REQ-LEARNING-001, PAF-REQ-REPRO-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001, PAF-REQ-EVOLUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-011 | PAF-REQ-LEARNING-001, PAF-REQ-POLICY-001, PAF-REQ-EVOLUTION-001, PAF-REQ-EVIDENCE-001 |
| PAF-UC-PROV-012 | PAF-REQ-IDENTITY-001, PAF-REQ-EVIDENCE-001, PAF-REQ-LEARNING-001, PAF-REQ-EVOLUTION-001, PAF-REQ-AUTHORITY-001, PAF-REQ-POLICY-001 |
