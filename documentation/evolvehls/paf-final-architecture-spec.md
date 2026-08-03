# PAF Final Architecture Specification

Status: bootstrap implementation baseline
Version: 1.0
Date: 2026-08-03

## Mission

PAF is a portable, authority-governed, evidence-based engineering control plane that progressively refines high-level intent into executable, reviewable, recoverable engineering campaigns.

Its first proving grounds are EvolveHLS and FIZZ.

## Core invariants

1. Autonomy does not imply authority.
2. Planners propose; controllers accept and mutate.
3. Credentials and capability are not grants.
4. Agent completion claims do not establish Work Item completion.
5. Protected effects require commit-time authorization.
6. Semantic state is independent of any runtime or workflow backend.
7. Native framework traces are evidence inputs, not the source of truth.
8. Model, deployment, provider, framework, access, funding, workspace, tool, permission and resource identities remain distinct.
9. Unknown capabilities remain unknown.
10. PAF cannot approve its own promotion.
11. Human authority remains required for protected merge, publication and promotion.
12. EDA semantics live in SODA-EVOLVE extensions, not PAF core.

## Four graphs

### Refinement Graph

```text
Intent
→ Mission
→ Objectives
→ Capabilities
→ Requirements
→ Assumptions and Constraints
→ Candidate Strategies
→ Selected Strategy
→ Work Packages
```

Each refinement relation records source/destination revisions, rationale, assumptions, evidence, confidence, critique/review and authority.

### Semantic Work Graph

Authoritative graph of Work Items, dependencies, requirement references, acceptance criteria, authority, budgets, risk, scheduling, lifecycle and accepted mutations.

### Technical Design Graph

Domain graph for the engineered system:

```text
application
→ algorithm
→ MLIR/LLVM
→ dataflow
→ architecture
→ memory
→ RTL
→ netlist
→ physical design
→ FPGA/ASIC
→ measured result
```

### Execution Graph

Backend projection of model calls, coding-agent sessions, builds, tests, simulation, formal, synthesis, PPA, waits, retries and human decisions.

## Core contracts

Identity:
`LogicalId`, `RevisionId`, `TypedDigest`, `Namespace`, `SchemaVersion`, `ArtifactRef`.

Refinement:
`Intent`, `Mission`, `Objective`, `CapabilityGoal`, `Requirement`, `Assumption`, `Constraint`, `Strategy`, `RefinementRelation`, `RefinementGraphRevision`.

Work:
`WorkItem`, `WorkItemPlan`, `Step`, `AcceptanceCriterion`, `WorkProposal`, `Critique`, `GraphMutation`, `WorkGraphRevision`.

Authority:
`CampaignCharter`, `AssignmentGrant`, `ToolGrant`, `ContextReleaseDecision`, `BudgetAllocation`, `FundingAuthorization`, `PolicyDecision`, `CommitAuthorizationDecision`, `HumanDecision`.

Runtime/routing:
`ModelArtifact`, `ModelDeployment`, `FrameworkAdapter`, `ProviderAccess`, `WorkspaceDescriptor`, `PermissionProfile`, `ToolDescriptor`, `ResourceDescriptor`, `ResourceReservation`, `CapabilityEvidence`, `ResolvedExecutionRoute`, `RouteTransitionDecision`, `AttemptContract`.

Evidence:
`Attempt`, `RuntimeEvent`, `Result`, `Receipt`, `ValidationRecord`, `ReviewRecord`, `EvidencePackage`, `ReplayPackage`, `EvaluationRecord`, `CompletionAssessment`, `RecoveryDecision`.

EDA extension:
`ToolchainEpoch`, `DesignArtifact`, `TransformationRecord`, `EDAToolInvocation`, `SimulationResult`, `FormalResult`, `EquivalenceResult`, `SynthesisResult`, `TimingResult`, `AreaResult`, `PowerResult`, `Study`, `Experiment`, `Candidate`, `Metric`, `FidelityLevel`, `ParetoSet`, `SelectionDecision`.

## Runtime adapter taxonomy

```text
CodingFrameworkAdapter: Cline, Claude Code, OpenHands
AgentLoopAdapter: OpenAI Agents SDK, Microsoft Agent Framework, CrewAI
RemoteAgentAdapter: A2A
ToolGatewayAdapter: MCP and native tools
ExecutionBackendAdapter: local, Temporal, Dapr, Argo, LangGraph
ModelProviderAdapter: local endpoints, LiteLLM, hosted providers
```

Required runtime operations:

```text
probe
describe_capabilities
prepare_attempt
start_attempt
stream_events
inspect_health
cancel_attempt
terminate_attempt
collect_result
collect_receipts
finalize_attempt_evidence
cleanup_attempt
recover_attempt
```

## Durable execution backend

```text
create_execution
schedule_activity
await_signal
record_timer
checkpoint
query
pause
resume
cancel
replay
reconcile
archive
```

Reference backend: SQLite WAL plus content-addressed blobs.

Candidate production backends: Temporal, Dapr, Argo and LangGraph.

Model and EDA tool calls are nondeterministic activities. Replay uses recorded results or creates a new Attempt.

## Completion

Completion is criterion-level and fidelity-aware:

```text
authorized → planned → ready → executing → assessing
→ complete | incomplete | blocked | unverifiable | effect-ambiguous | superseded
```

EDA fidelity may require syntax, simulation, formal, synthesis, timing, area, power, physical design, FPGA execution or silicon measurement.

## Specialized models

Routing uses task family, capability evidence, assurance, data restrictions, tools, resources, funding, cost and latency.

SODA-EVOLVE supplies the EDA capability profile: SystemVerilog, MLIR/LLVM, PandA-Bambu, SODA-OPT, HLS, verification, synthesis, OpenROAD, memory architecture and PPA analysis.

Fine-tuned/local models are exact versioned Model Artifacts and Deployments with provenance and evaluation evidence.

## Context security

Every context transfer records source classification, destination, minimum necessary view, release policy, origin labels, transformation lineage, expiry/revocation and receipt.

Repository, web, MCP, A2A and tool content cannot grant authority or override policy.

## Commit-time authorization

```text
exact payload and target
+ current Charter/grants/policy
+ current data/funding/resource state
+ fresh dependency witnesses
→ CommitAuthorizationDecision
→ commit or refuse
```

## Interoperability

MCP: transport plus PAF admission, attestation, grants, effect classification, context release, sandbox and receipts.

A2A: Agent Cards/Tasks/Messages/Artifacts map into PAF remote-agent objects; remote completion remains a claim.

OpenTelemetry: export only; telemetry is non-authoritative.

OPA: optional PolicyDecisionProvider over PAF-defined facts and authority vocabulary.

in-toto/SLSA: standard provenance/test/release attestations plus PAF authority and completion predicates.

## EvolveHLS/FIZZ acceptance

EvolveHLS must refine a high-level compiler objective into reviewed tasks, use heterogeneous and EDA-specialized routes, modify Bambu/SODA-OPT, execute compiler/simulation/synthesis evidence, recover from failures and stop before human-protected merge.

FIZZ must support:

```text
intent → alternatives → candidates → experiment plan
→ HLS/RTL → synthesis/PPA → Pareto analysis
→ human selection → protected promotion
```
