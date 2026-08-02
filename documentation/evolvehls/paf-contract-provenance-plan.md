# PAF Contract, Provenance, and Learning Architecture Plan

**Status:** planning snapshot captured 2026-08-02  
**Target milestone:** PAF-05A3a

## Purpose

Define the complete contract architecture required for autonomous development of PAF and EvolveHLS, compiler/HLS/architecture design-space exploration, complete audit and reproducibility, and governed reuse of evidence for improving PAF.

The existing v1 contracts remain compatibility fixtures while v2 and new contracts are introduced.

## Contract responsibilities

| Contract | Authoritative responsibility |
|---|---|
| Artifact | Immutable content, representation, integrity, location, governance, provenance |
| Workspace manifest | Repositories, revisions, topology, access, paths, dependency edges |
| Task | Desired outcome, scope, requirements, acceptance, requested resources |
| Plan | Steps, alternatives, assumptions, risks, dependencies |
| Workflow | Stages, transitions, fan-out, joins, retries, remediation, gates |
| Spawn request | Controlled request for child work |
| Policy decision | Authoritative permission, restriction, and budget decision |
| Execution request | One resolved, authorized execution attempt |
| Handoff | Exact context and artifacts exposed to an executor |
| Result | Semantic outcome and produced artifacts |
| Execution receipt | Actual process outcome, evidence, resource use, changes, retention |
| Review result | Verdict, findings, corrections, regressions |
| Change set | Coordinated repository changes and publication outcomes |
| Runtime checkpoint | Portable resumable state independent of client history |
| Exploration study | Search problem, parameters, objectives, constraints, strategy |
| Candidate | Immutable design point |
| Experiment request | Authorized candidate evaluation at specified fidelity |
| Measurement set | Metrics, units, provenance, validity, uncertainty |
| Campaign state | Pending/running/completed work, optimizer state, budget, Pareto state |
| Decision report | Evidence-backed recommendation and alternatives |
| Provenance event | Append-only state-transition record |
| Interaction trace | Context, messages, tools, approvals, compaction |
| Decision record | Alternatives, selection, rationale, evidence, policy |
| Evaluation record | Normalized quality and outcome signals |
| Feedback record | Human, CI, review, regression, downstream feedback |
| Dataset manifest | Governed immutable view over eligible evidence |
| Learning run | Reproducible offline policy/model improvement |
| Policy release | Promoted version with evaluation and rollback |

Contracts reference canonical identities and digests instead of copying partial objects unless the copy is explicitly content-addressed.

## Shared identity and lineage

Every object supports schema/version, canonical digest, creation time, workflow, root/parent task and execution, stage, attempt, remediation cycle, producer role, source artifacts, and decision references.

## Artifact contract

Anything consumed, produced, transformed, selected, rejected, reviewed, or evaluated is an artifact or provenance event.

Artifacts record type, media type, schema, digest, size, producer, derivation relations, transformation tool/version/parameters/fidelity, repository context, sensitivity, license, retention, evaluation eligibility, training eligibility, and redistribution eligibility.

Supported artifacts include tasks, plans, policies, prompts, responses, tool calls/results, source, patches, commits, build products, tests, CI, MLIR, LLVM IR, RTL, netlists, constraints, waveforms, reports, LEF/DEF, Liberty, SDC, SPEF, GDSII, images, PDFs, datasets, measurements, model catalogs, capability snapshots, routing decisions, and ledgers.

Whenever a model sees a derived representation, record original and derived digests, conversion operation, tool/version, parameters, fidelity/information loss, classification, and exact representation shown.

## Task v2

Task v2 describes intent and success. It does not identify a provider, client, access binding, credential, command line, or selected model.

Task types include analysis, planning, implementation, review, remediation, integration, validation, experiment, exploration, and decision.

Separate the actual objective from routing optimization preferences. Requirements and acceptance criteria have stable IDs. Inputs and outputs are typed artifact references. Scope and requested permissions are explicit but non-authoritative. Budgets are typed across money, quota, tokens, wall clock, turns, retries, children, concurrency, delegation depth, CI, compute, candidates, and protected reserves. Reproducibility records workspace, source revisions, toolchain, policies, catalog snapshot, context, and inputs.

## Workflow and multi-agent contracts

Support sequential and parallel stages, conditionals, dynamic fan-out/join, parent/child tasks, stage-specific policy, bounded retries, remediation loops, cancellation, aggregation, human interrupts, deterministic state transitions, restart, and resumption.

Spawn requests are proposals; the controller authorizes, restricts, queues, or denies them.

Portable workflows identify roles and capabilities, not Sol, Terra, Opus, Sonnet, Cline, Codex, Claude Code, or providers.

## Result, review, and change sets

Result v2 records outcome, plan adherence/deviation, artifacts, requirement/criterion status, validation, decisions, assumptions, diagnostics, blockers, proposed children/remediation, and provenance.

Review results identify exact reviewed tasks, change sets, commits, and digests; verdict; stable findings; severity; violated requirements; affected locations; behavior; impact; reproducer; evidence; correction; regression; and reviewer independence/routing provenance.

Change sets represent one or more repositories, bases, branches, files, commits, dependency order, coordinated publication, CI, review, push/PR/merge intent and outcome, and approvals.

## Execution contracts

Execution request binds task, role, workflow/stage, workspace/revisions, selected model/environment/adapter/endpoint/binding, catalog/capability evidence, authorization, limits, retention, handoff, result collection, and routing/adaptation provenance.

Handoff references canonical task, role, request, workspace, inputs, policies, output namespaces, and exact artifact representations exposed.

Receipt records state, process outcome, streams, environment/tool identity, token/cost/quota/compute/time use, changes, validation/result digests, checkpoints, retention, publication attempts/outcomes, invariants, diagnostics, and cleanup evidence.

## Access bindings and dynamic discovery

A model is discovered per access binding and environment. Distinct concepts are environment, provider/runtime, endpoint, access binding, credential reference, account/project scope, funding class, discovered model, and composite capability.

Multiple credentials for one provider remain separate. Gateways may expose several upstream providers; exact advertised IDs and upstream-identity confidence are preserved.

Each binding records environment compatibility, endpoint, opaque credential reference/fingerprint, account/project references, funding, owner, data boundary, unattended permission, discovery capabilities, quota/billing semantics, local/network execution, and priority. Credential values never appear in portable artifacts.

Discovery proceeds through installed environment/session detection, endpoint resolution, model listing when supported, capability enrichment, policy filtering, immutable catalog snapshot, and routing. Fallback uses environment-native listing, endpoint API, validated cache, setup-approved model, or explicit pin. Unknown or stale state is reported explicitly.

## Capability model

Effective capability is the intersection of model, environment, binding, policy, and task. Evidence records source, timestamp, confidence, probes, environment, and catalog snapshot.

## Budgets and adaptation

Budget sources include personal/project APIs, institutional gateways, subscription quota, cloud compute, CI, local compute, cluster allocation, and experimental resources.

Each resource supports soft, hard, reserve, renewal, and approval thresholds.

Within policy, adaptation may switch model/environment/binding, reduce concurrency/delegation, use cache, alter DSE batch/fidelity, preserve reserves, checkpoint, or stop. It may never silently cross funding, data, credential, account, network, or authorization boundaries.

## Portable checkpoint

Opaque client history is not authoritative. At boundaries persist canonical task/workflow, plan, decisions, assumptions, accepted/rejected changes, repository/artifact digests, validation, findings, children, budgets, exact next action, portable context summary, and native session references when available.

## Design-space exploration

Exploration study defines objectives, feasibility, baseline, parameter space, strategy, fidelity, budget, stopping, evidence, and comparability.

Candidates are immutable and content-addressed from parameters, revisions, toolchain, inputs, target, constraints, and seed.

Measurements record metric, value, unit, status, tool/version, evidence, fidelity, uncertainty, repetition/seed, and comparability fingerprint.

Campaign state supports restart with optimizer/random state, pending/running/completed/failed candidates, budget, Pareto set, and stop rationale.

## Provenance and interaction

Append-only events cover task creation/normalization, planning, routing, spawning, authorization, tool execution, artifact creation/transformation, validation, findings, remediation, human intervention, candidate proposal/evaluation, adaptation, checkpoint, and stop.

Interaction traces record exact system/role instructions, task representation, selected context, messages, tools, approvals, mode changes, token/cost use, truncation/compaction, and outcome. Redaction creates a derived artifact preserving lineage.

Evaluation records normalize correctness, security, maintainability, efficiency, cost, latency, reviewer quality, remediation burden, and downstream outcomes.

## Governance and learning

Audit retention does not imply training permission. Each artifact independently identifies retention, evaluation eligibility, training eligibility, redistribution eligibility, sensitivity, ownership/license, consent/approval, and restrictions.

Dataset manifests select only eligible immutable evidence and record exclusions. Learning runs are offline. Candidate policies undergo independent held-out and adversarial evaluation. Promotion is human-authorized and reversible.

## Validation

Validation includes JSON Schema, semantic checks, cross-contract identity/digest resolution, authorization monotonicity, budget non-expansion, lineage constraints, deterministic canonicalization, artifact reachability, dataset governance, binding-transition policy, fidelity/comparability consistency, and v1 migration compatibility.

## Definition of complete

The architecture is complete only when every autonomous coding and DSE scenario has valid and adversarial fixtures; the same task runs through two environments without schema changes; multiple keys and funding scopes remain distinct; routing, adaptation, budget, and artifact visibility are attributable; all evidence is governed; workflows resume through another environment; and no participating provider, model, client, repository, or tool needs to extend core schemas.
