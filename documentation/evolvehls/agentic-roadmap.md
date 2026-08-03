# EvolveHLS Portable Agent Framework Roadmap

**Status:** final post-PR #26 architecture and requirements closure, 2026-08-03.
**Authority:** product boundary, sequencing, milestones, and implementation gates. Contract ownership is in the [provenance plan](paf-contract-provenance-plan.md); atomic coverage is in [traceability](paf-requirements-traceability.md); the preserved v1 corpus is in the [requirements matrix](paf-contract-requirements-matrix.md); operational realization is in the [turnkey plan](paf-turnkey-orchestration-plan.md).

## Product boundary

```text
SODA-SPRITZ
  user-facing intent, steering, explanation, approvals and campaign interaction
        ↓
PAF API and CLI (`paf`)
  domain-independent autonomous engineering control plane
        ↓
SODA-EVOLVE extension
  compiler/HLS/hardware/system codesign semantics
        ↓
SODA-FIZZ + EvolveHLS
        ↓
SODA-OPT, PandA-Bambu and implementation tools
```

PAF core has no dependency on Bambu; any SODA component; MLIR, HLS, or hardware concepts; named providers, models, coding clients, or private endpoints; repository-local paths; or one orchestration framework. SODA-EVOLVE is the flagship proving ground, never the owner of portable PAF semantics. Existing `agentic/`, `tools/agentic/`, and `evolvehls.agentic.*` identities remain unchanged v1 compatibility evidence.

## Closure invariants

- Credentials, capability, and possession of a tool are never authority. The authority chain is accountable authority → Campaign Charter → Authority Grants and policy → Capability/Assignment Grants → deterministic controller enforcement → execution.
- A Charter is versioned standing authority: accountable authority; accepted Objective/revision and completion criteria; allowed systems/services and prohibited scope; autonomy, budget/resource, data/credential/account/funding/licensing/network, and side-effect limits; review/independence; publication/merge/release/fabrication/physical rules; gates; stopping/escalation/emergency stop; expiry; amendment and revocation.
- The controller validates that chain, schemas, revisions, idempotency, budgets, protected boundaries, and evidence; it does not invent authority. A planner proposes only; an accepted Work Graph Mutation is the sole graph transition.
- Every attempt ends in an immutable, content-addressed Evidence Package Manifest or explicitly `evidence-incomplete`, `unverifiable`, or `effect-ambiguous`. Cancellation, merge, and supersession retain completed evidence.
- Logical ID, immutable revision ID, and typed digest are separate. Missing evidence is missing/unknown/`null`, never zero.
- Immutable contracts are facts; accepted mutations are transitions; snapshots, dashboards, boards, and external systems are derived projections.
- Evolution is ordinary governed work, never privileged self-modification or self-promotion. Conformance starts in every slice.

## Final PAF-05A3a allocation

| Slice | Exclusive allocation |
|---|---|
| 2a | Wire identity and canonical form: Contract Envelope, Object Reference, logical/revision/schema identity, encoding and Canonicalization Profile, Typed Digest, registry, namespace/version negotiation, v1 migration envelope, cross-language vectors. Nothing else. |
| 2b | Artifacts, Representations, storage references, manifests, Evidence Package Manifest, lifecycle references. |
| 2c | Principals, Campaign Charter, grants, policy inputs/decisions/obligations/revocation, attestation, separation of duties. |
| 2d | Event Envelope, causal/correlation identity, provenance, time, audit reconstruction, Error Record, retryability, package finalization. |
| 2e | Typed quantities; money, tokens, time, compute, energy, attention; budgets, governance, Resource Descriptor and Lease. |
| 2f | Agent Definition/Instance, Session, Team, messages, Context Package/View, memory/Knowledge Claim, Tool/API Operation Descriptor and Invocation Request/Receipt. |
| 3a–3f | Objectives/Requirements/assessments; proposals, Work Items and mutations; workflow/scheduling/teams/methodology; Result/Validation/Review/Finding/independence; ChangeSets/integration/publication/release; Adaptive Work Planning, generation, portfolio and stability. |
| 4 | Generic research/experiment contracts: Study, Parameter Space, Candidate, Experiment, Measurement, generic Fidelity, Uncertainty, Strategy, Campaign, Recommendation, Stop Decision and generic comparability. |
| 5a–5e | Normalized execution/invocation; route components/catalogs; adapter SDK/conformance; Cycle Policy, transitions, Context transfer, checkpoints/recovery/reconciliation; security, sandbox, network, secrets, supply chain. |
| 6 | Evidence selection, learning/evaluation, Evolution Proposals, promotion, canary/shadow, rollback, deprecation, retirement. |
| 7 | Integrated cross-language, migration, audit, adversarial, portability, and evolution conformance. |

## Later milestones and proof gates

`PAF-05A3b` trusted contract-driven executor; `PAF-05B` supported generic A2 cycle and coding profile; `PAF-05C` second framework/binding/local-model/cross-language proof; `PAF-06` standalone extraction; `PAF-07` federated workspaces and ChangeSets; `PAF-08` organizations, tenants, authority, funding, data and security; `PAF-09` A3 adaptive campaigns and Work Graph evolution; `PAF-10` A4 durable/distributed operation; `PAF-11` knowledge/capability intelligence; `PAF-12` route cycling/portfolio optimization; `PAF-13` A5 governed self-evolution.

PAF-05C proves two materially different coding frameworks, two access-binding/provider families, one local-model route, one task through multiple routes, portable Context Package transfer, normalized evidence, independent cross-framework review, honest degradation, and a standalone-packaging rehearsal.

## Generic DSE and SODA-EVOLVE boundary

PAF owns only generic experiment semantics listed in 4. SODA-EVOLVE owns source/application/algorithm intent, compiler and IR lineage, HLS schedule/binding, hardware/memory architecture, Toolchain Epoch, semantic-equivalence categories, RTL/FPGA/ASIC/physical fidelity, timing/area/power, package/system, fabrication, and silicon measurements. Results across epochs are never silently comparable.

SODA-EVOLVE retains X-A through X-H and adds **X-I Physical realization and fabrication** (PDK/IP/tool licensing, restricted artifacts, signoff, DFT, packaging/chiplets, FPGA prototypes, MPW/tapeout, authority, cost/schedule, bring-up and reconciliation) and **X-J Domain benchmarks and Open Evidence Plane** (reference campaigns, baselines, failed retention, eligibility, reproducible bundles, open/restricted evidence, governance, contamination control and release).

The capability-gap handshake is: SODA-FIZZ campaign → Capability Gap → EvolveHLS Objective and Work Proposals → validated toolchain release → new Toolchain Epoch → explicit transition decision → campaign partition/resume.

## Immediate handoff

Once this closure is merged, the next coding task is **PAF-05A3a.2a — wire identity and canonical form**, suggested branch `agent/paf-05a3a2a-wire-identity`. It is limited to the PAF-owned v2 namespace, envelope/reference/identity layers, encoding and canonicalization, Typed Digest, registry/negotiation, v1 migration envelope, vectors, and dependency-boundary/standalone-import tests. It must not implement Artifacts, authority, Work Graphs, execution, providers, or orchestration. The blocking decisions are recorded in the soundness analysis.
