# PAF Turnkey Adaptive Orchestration Plan

**Status:** planning snapshot captured 2026-08-02  
**Targets:** PAF-05B, PAF-05C, and PAF-08 through PAF-12

## Product requirement

A user describes the objective and funding boundaries once. PAF discovers available environments and access modes, selects and adapts the agent team, manages budgets and evidence, checkpoints safely when resources end, and presents only decisions requiring human authority.

Normal use:

```text
agentctl setup
agentctl doctor
agentctl run <task>
```

The user should not need to hand-write provider configuration, know model IDs, switch models manually, or understand client-specific profile formats.

## Runtime abstraction

Keep separate role, model, agentic environment, provider/runtime, endpoint, access binding, credential reference, account scope, funding class, and adapter.

The portable workflow requests roles and capabilities. Routing selects the exact model–environment–binding tuple.

## Turnkey setup

`agentctl setup` should:

1. discover Cline, Codex, Claude Code, local runtimes, and direct API adapters;
2. detect authenticated sessions and configured endpoints;
3. discover multiple access bindings, including multiple credentials for the same provider;
4. query models dynamically where supported;
5. probe only when authorized;
6. infer no unavailable facts;
7. ask minimal policy questions;
8. create ignored local configuration;
9. produce a safe default routing and budget policy;
10. verify with `agentctl doctor`.

Minimal questions concern local/subscription/API use, funding ownership, data classifications, unattended limits, publication permissions, and human gates. Advanced YAML and flags remain optional.

## Multiple keys, accounts, and providers

Two keys for one provider are distinct because organization/project scope, model visibility, quota, billing, data policy, region, unattended authorization, and funding may differ.

Example bindings:

```text
openai-personal-api
openai-pnnl-project-api
codex-subscription
pnnl-litellm
local-ollama
```

They do not share one catalog, ledger, or policy merely because model names overlap.

Gateway bindings may expose multiple upstream providers. Preserve exact advertised IDs and upstream identity confidence.

## Dynamic model discovery

Discovery is per binding and refreshes during setup, on request, before long unattended runs, after authentication/model/quota errors, after deprecation signals, when metadata expires, and before long-delayed resumption.

An active workflow pins a catalog snapshot. Discovery changes do not silently alter reproducibility-sensitive runs.

Fallback order:

1. environment-native listing;
2. endpoint model API;
3. validated cache;
4. setup-approved model;
5. manually pinned ID.

Stale, unavailable, and unknown states are explicit.

## Capability discovery

Effective capability is:

```text
model ∩ environment ∩ binding ∩ policy ∩ task
```

Record text, image, structured output, tools, source editing, terminal, network, context, local/offline operation, resumable sessions, artifact modalities, and confidence.

## Default EvolveHLS coding policy

Initial user-local preference:

```text
coding planning        → Sol
coding implementation  → Terra
review planning        → Opus
review execution       → Sonnet
```

These are not hard-coded. The controller may adapt for availability, capability, limits, budget, performance, review strength, local sufficiency, or data boundaries.

## Adaptive routing

Before each stage consider role/capabilities, modalities/classification, repository/task criticality, context, environment compatibility, authorization, availability/rate limits, estimated/remaining budget, reserves, cost/latency, current-cycle failures, historical performance, and reproducibility mode.

Reconsider at stage boundaries, controlled failures, checkpoints, or budget/status changes.

## Budget model

Budget dimensions include monetary limits by funding source, subscription/provider quota, tokens, CI minutes, local/cluster compute, wall clock, turns, cycles, candidates, and protected reserves.

Every binding has a separate ledger. Thresholds are soft, hard, reserve, renewal, and approval. Unknown subscription quota is unknown, not infinite.

## Exhaustion behavior

Policy-approved actions include checkpoint/stop, another authorized binding, authorized local model, lower capability for noncritical work, reduced concurrency/delegation, cached evidence, smaller DSE batches, lower fidelity with provenance, preserved review reserve, or explicit additional authorization.

Never silently switch subscription to metered API, personal to project funding, restricted code to a public endpoint, weak reviewer for mandatory security review, lower pinned evaluation fidelity, enable network, or exceed hard limits.

## Binding transitions

Every transition has an explicit policy decision. Funding-boundary changes may require human approval.

## Portable checkpointing

Persist canonical task/workflow, plan, decisions/assumptions, accepted/rejected changes, repository/artifact digests, validation, findings, child state, budgets, next action, portable context summary, and native session references when available.

A checkpoint must permit resumption through another environment.

## Coding cycle

```text
plan
→ implement
→ independent review plan
→ review execution
→ approved: human merge gate
→ blocked: remediation plan and implementation
→ repeat within limits
```

Requirements include isolated worktrees, exact-head review, structured findings, independent reviewer route, protected review budget, maximum cycles, deterministic handling of malformed verdicts, no automatic merge, and full provenance.

## Design-space exploration

Adaptive DSE generates/samples candidates, rejects duplicates, applies low-cost feasibility filters, evaluates at selected fidelity, updates optimizer/Pareto state, adapts batch/concurrency to budget, escalates promising candidates, preserves verification budget, gates expensive FPGA/ASIC work, and checkpoints on stop.

Deterministic tool evidence is authoritative for functional, synthesis, timing, area, power, and physical results.

## `agentctl doctor`

Report per environment and binding: installation/health, authentication, endpoint, model discovery, discovered models, capability confidence, quota visibility, rate-limit state, funding/data boundary, unattended permission, eligible roles, missing requirements, and last verification.

Statuses include available, temporarily unavailable, rate-limited, budget-exhausted, authentication-required, capability-insufficient, policy-ineligible, and status-unknown.

## Guided recovery

Errors identify the component, retry safety, required action, approved alternatives, reproducibility effect, checkpoint location, and resume procedure.

## Provenance

Every execution records task/workflow, role, model ID, catalog snapshot, environment, adapter/version, endpoint, access binding, account/project scope, funding, policy/authorization, budget snapshot, alternatives, selection rationale, adaptations, actual consumption, artifacts, and evidence.

## Acceptance criteria

Demonstrate guided subscription setup, local setup, BYO API credential without repository storage, multiple credentials for one provider, multiple providers/gateways, Cline/Codex/Claude Code discovery, dynamic model discovery, explicit stale fallback, unchanged task through two environments, soft-budget reconsideration, hard-budget checkpoint, protected review reserve, restricted-data denial, DSE concurrency/fidelity adaptation, complete provenance, cross-environment resume, and minimal user questions.
