# PAF Contract and Provenance Architecture Plan

**Status:** final post-PR #26 ownership architecture; no v2 schema or runtime is implemented here.
**Authority:** contract layering, fact ownership, authority, evidence, provenance, and graph semantics. Milestones are [roadmap](agentic-roadmap.md); atomic test planning is [traceability](paf-requirements-traceability.md).

## Contract layers and fact ownership

The roadmap's 2a–2f, 3a–3f, 4, 5a–5e, 6, and 7 allocations are exclusive. The semantic model, normative encoding profile, canonical bytes, and typed digest/signature input are distinct layers. v2 uses a PAF-owned namespace to be selected in 2a; `evolvehls.agentic.*` is v1 compatibility lineage.

| Fact | Sole owner |
|---|---|
| Semantic claim/requirement outcome | Result |
| Attested execution or external effect | Execution Receipt |
| Check evidence | Validation Record |
| Finding/verdict | Review Result and Independence Assessment |
| Permission/gate/obligation | Charter, Grant, Policy or Human Decision |
| Aggregated immutable references and completeness | Execution Evidence Package Manifest |
| Resumable controller state | Checkpoint |

Storage, journals, indexes, dashboards, issue trackers, and boards are replaceable projections and never semantic authority.

## Authority and governed autonomy

A Campaign Charter is versioned standing authority declaring accountable human/organization/project authority, Objective and accepted revision, success/completion, allowed repositories/systems/services, prohibited scope, maximum autonomy, ceilings, protected data/credential/account/funding/licensing/network boundaries, permitted side effects, review/independence, publication/merge/release/fabrication/physical rules, human gates, stop/escalation/emergency-stop, duration/expiry, and amendment/revocation.

The controller validates **accountable authority → Charter → Authority Grants and policy → Capability/Assignment Grants → enforcement → agent execution**. Policy outcomes are permit, permit-with-obligations, deny, require-independent-review, or require-human-decision. Grants bound exact subjects, operations, scope, expiry, resources, and revocation. A Charter can delegate proposal generation, ordinary authorization, graph operations, eligible route/workspace selection, retries, isolated commits, draft PR creation, testing/CI, and resource consumption until completion/exhaustion/block/gate. Credentials and technical capability never grant authority. Human gates are Charter/policy configured, including protected changes, boundary crossings, security/independence weakening, publication and ambiguous effects.

## Evidence and provenance

Every success, failure, timeout, cancellation, refusal, validation/security failure, ambiguous effect, and interrupted cleanup finalizes an immutable content-addressed package, or records an explicit incomplete/unverifiable/ambiguous terminal state. The manifest references exact Objective/Requirement/Work/Workflow/Step revisions; attempt lineage; principals/services; resolved Execution Route; configuration/policy and Charter/grant decisions; request/handoff/context transfer; interactions/prompts; operation requests/receipts and retained native/normalized events; artifacts/workspace identities; Result, Receipt, Validation, Review and errors; retry/compensation/cleanup; budget/leases; time/clock quality; checkpoint/next action; retention/evaluation/training/redistribution; redaction/tombstone/completeness; schema/canonicalization/typed digest.

Attempt → Step → Work Item → Workflow/Campaign → Release/Publication packages reference immutable children without rewriting them. Each Step Definition and Operation Descriptor declares one required profile: advisory/read-only, local reversible coding, repository mutation, remote reversible/irreversible effect, publication/release, financial/funding, restricted data, laboratory/instrument, FPGA/ASIC fabrication, or PAF policy/capability evolution.

## Graphs, planning, and APIs

The Objective/Requirement graph, Work dependency graph, Workflow control graph, Team/communication graph, attempt lineage, Artifact/Provenance graph, ChangeSet dependency graph, DSE/Campaign graph, and Decision/supersession graph are distinct. “Work Graph” means authorized Work Items and dependencies only. Accepted Work Graph Mutations are authoritative transitions; snapshots are derived materializations.

The Adaptive Work Planning Cycle consumes graph revision, finalized packages, policies/budgets/resources/deadlines/human decisions; assesses evidence, completion and gaps; emits proposals and a candidate mutation; applies required critique; obtains authorization; then schedules execution. Triggers record source evidence, urgency, affected region, and mandatory/optional/prohibited status: initial acceptance, completion/failure/block/cancellation/ambiguity, validation/review/feedback, dependency/capability gaps, route availability, resource/deadline drift, external drift, human request, risk/no-progress, DSE result, epoch, and policy/security events. Proposals carry identity/lineage, rationale/scope, requirements/acceptance, evidence/validation/review, dependencies, workspaces/capabilities, risk/value/cost/priority/uncertainty, duplicate/freshness/team/route analysis, and generation evidence. Mutations record prior/result revisions/digests, ordered add/split/merge/supersede/reprioritize/defer/cancel/reopen/dependency/reassign/topology/regenerate/duplicate/integration/remediation/capability-gap/epoch-partition operations, preconditions, idempotency, cited evidence/authority, conflict, in-flight/budget/lease treatment, rollback/supersession.

Stability requires materiality, horizon, growth/mutation/replan limits, hysteresis, deduplication, no-progress and remediation bounds, reserves, fairness, escalation/safe stop, and immutable Objective/authority/protected boundaries. P0 is deterministic, P1 one planner, P2 independent critic, P3 competing planners, P4 human-governed. Planner and critic may change eligible routes but cannot acquire authority; only the deterministic controller transitions state.

Step Definitions declare inputs/outputs, preconditions/transitions, operations, effects, capability/authority/budget/resources, validation/review, checkpoint/retry/compensation, compatibility; Step Instances record exact execution. Operation Descriptors define stable semantic identity independent of transport, contracts/encodings, authority/data/effect/reversibility/idempotency/retry/compensation/cost/dry-run/evidence/compatibility. Mutating requests bind identity, idempotency, subject/revision, descriptor revision, authority/policy, budget/lease, inputs, effect, dry run, and causal identity; outcomes are accepted/rejected/human-gated/conflicted/running/completed/failed/cancelled/effect-ambiguous.

## Portability, operations, and resilience

An immutable Execution Route separately records model, provider/runtime, coding environment, framework adapter, access binding, workspace/executor, tool set, context representation, policy, and budget/resources. Descriptors cover catalog/provider/model/environment/runtime/adapter/binding/route; facts carry source, observation, confidence, expiry. Adapters support detect, inspect, doctor, discovery, prepare/session/invoke/events/input/cancel/checkpoint/resume/context/result/receipt/cleanup and report L0 detected through L5 portable transition. Context Package, Transfer Decision/Receipt, normalized events and immutable evidence provide continuity; hidden reasoning is neither required nor transferred. Cycle Policy defines eligible routes, limits, reserves, transition/independence/stability/stopping/gate rules.

Policy sets, layered configuration, replaceable stores, scheduler, external-system synchronization, and extension lifecycle have contracts but no backend owns facts. Configuration precedence is installation→organization→project→repository→user→session→run, with validation, secret references, provenance, immutable run snapshots, drift/migration/offline bundles. Security covers malicious inputs/adapters/models, exfiltration, supply chain, tenancy, deputy/authority/audit/resource/capability attacks; before 05B isolation, allowlists/limits, deny-by-default egress, scoped secrets, classification, supply-chain evidence, destructive authorization, cleanup/forensics are mandatory. Reproducibility grades are exact, deterministic-logical, environment-pinned, statistical, evidence-reconstructable, opaque. Incident and federated bundle contracts preserve containment, revocation, evidence, recovery, reconciliation, residency and delayed updates.
