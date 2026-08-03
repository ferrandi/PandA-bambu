# PAF Contract Requirements and Use-Case Matrix

**Status:** normative completeness audit for PAF-05A3a.1
**Scope:** requirements and contract ownership only; this document precedes all PAF v2 schema implementation.

## Purpose, authority, and scope

This matrix translates `agentic-roadmap.md`, `paf-contract-provenance-plan.md`, and `paf-turnkey-orchestration-plan.md` into implementation-ready scenario requirements. It is the normative completeness audit for PAF-05A3a.1, not runtime design or implementation authority. Current v1 documents are compatibility evidence, not proof of v2 completeness.

Authoritative inputs are the three documents above, every schema in `agentic/schemas/`, every tracked fixture in `agentic/fixtures/`, and semantic validators/tests in `tools/agentic/`. No provider, client, model, credential value, repository name, or local path named below is portable-schema vocabulary. Cline, Codex, Claude Code, SODA-OPT, and Bambu are scenario data or adapter/workspace identities only.

## Terminology and stable identifiers

Use-case IDs have the permanent form `PAF-UC-<FAMILY>-<NNN>`, where FAMILY is `CODE`, `ACCESS`, `BUDGET`, `DSE`, or `PROV`. IDs are never reused; renaming does not change an ID, and schema versions and milestones never appear in an ID. Every future fixture manifest references one or more IDs. Valid, invalid, and adversarial fixtures have distinct identities: `PAF-SC-<FAMILY>-<NNN>-VALID`, `...-INVALID-<REASON>`, and `...-ADVERSARIAL-<REASON>`. “Not applicable” is permitted only when explicit and justified; no matrix cell is blank.

### Matrix conventions

`I` means immutable identity plus canonical digest, root/parent/stage/attempt and decision lineage. `A` means immutable Artifact and typed representation references; `P` means append-only Provenance Events. `G` means four independent governance decisions: retention, evaluation, training, and redistribution. `R` means an Execution Receipt is the sole attestation of actual use. `CK` means a portable Runtime Checkpoint with deterministic next action. All fixture cells require a scenario manifest keyed by the displayed use-case ID; `V:` and `X:` name valid and invalid/adversarial variants. All lifecycle cells retain diagnostics and prior evidence; failure never rewrites prior Result, Receipt, measurement, or decision.

## Contract ownership (normative)

| Contract | Sole authoritative responsibility |
|---|---|
| Task | Intent, scope, requirements, success criteria, requested resources and permissions |
| Plan | Proposed steps, assumptions, alternatives, dependencies, and risks |
| Workflow | Collaboration graph, states/transitions, fan-out/join, retries, remediation, cancellation, and gates |
| Spawn Request | Proposal to create child work; never authority by itself |
| Policy Decision | Granted, restricted, denied, or human-gated authority and budget decisions |
| Execution Request | One resolved, authorized execution attempt |
| Handoff | Exact context and artifact representations visible to an executor |
| Result | Semantic outcome, requirement status, blockers, and produced artifacts |
| Review Result | Exact reviewed identity, structured findings, and verdict |
| Execution Receipt | What actually executed, consumed, changed, emitted, retained, or cleaned up |
| Artifact | Immutable content/representation identity and governance metadata |
| Provenance Event | Append-only transition or lineage event |
| Interaction Trace | Exact messages, tool interactions, approvals, and compaction/context history |
| Feedback Record | Human, review, CI, regression, and downstream feedback |
| Evaluation Record | Normalized evaluation outcomes and quality signals |
| Workspace Manifest | Repository topology, exact revisions, access, dependency edges, and workspace references |
| Change Set | Coordinated changes and publication intent/outcomes across repositories |
| Runtime Checkpoint | Portable resumable controller state |
| Access Binding | Binding, environment, account, project, funding, data-boundary identity and restrictions |
| Budget Policy | Thresholds, reserves, renewal/approval rules, and permitted exhaustion responses |
| Routing/Adaptation Decision | Selected route or adaptation, alternatives, rationale, evidence, and cited authority |
| Exploration contracts | Scientific search definition, immutable candidates, experiments, measurements, campaign state, recommendation |
| Dataset/Learning/Policy contracts | Governed evidence selection, offline learning, evaluation, promotion, release, and rollback |

Authority is not duplicated: a Task can request but not authorize budget; a Workflow can require but not grant a human gate; an Execution Request can cite but not create authority; a Result cannot attest execution; and retention never grants evaluation, training, or redistribution.

## Current-state compatibility inventory

The explicit v1 compatibility fixtures are `task.schema.json`, `result.schema.json`, `execution-request.schema.json`, `fixture-handoff.schema.json`, and `execution-receipt.schema.json`. Task v1 combines intent with one role and weakly typed budgets; Result v1 uses path/digest pairs rather than general artifacts and provenance; the Execution Request, Handoff, and Receipt are tied to the fixture executor, fixed operation, and fixture-local boundary.

The remaining current foundation is active, not obsolete: provider, catalog, selection, role, profile, readiness, discovery, onboarding, routing, adapter, and execution-plan schemas. The inspected repository contains **25** schema files and 12 tracked fixtures. Some current routing/catalog contracts use client-specific enums or identities and require later generalization.

Reusable validator patterns are exact-field validation, unsupported-version rejection, duplicate/reference checks, explicit routing/funding transitions, content digests and immutable execution bindings, and negative/adversarial tests. Gaps are a unified registry; centrally specified canonicalization; complete cross-contract digest resolution; authorization-monotonicity, budget, provenance-graph, DSE-comparability, and dataset-governance validators; and a matrix-driven conformance corpus.

## Linked scenario matrices

The four tables in each family use the same ID. Tables A–D respectively provide intent/ownership, identity/data/authority/budget, provenance/governance/lifecycle, and conformance/delivery. The completeness ledger at the end is normative.

### CODE — autonomous EvolveHLS coding

#### Table A — Intent and contract ownership
| Use-case ID | Scenario | User objective | Actors and agent roles | Authoritative contract(s) | Producer | Consumer |
|---|---|---|---|---|---|---|
| PAF-UC-CODE-001 | planner to implementer handoff | turn intent into bounded work | controller, planner, implementer | Task, Plan, Workflow, Handoff | planner/controller | implementer |
| PAF-UC-CODE-002 | parallel read-only investigation | gather independent evidence | controller, investigators | Task, Workflow, Spawn Request, Policy Decision | controller | investigators/join |
| PAF-UC-CODE-003 | isolated concurrent writers | change safely in parallel | controller, writers | Workflow, Workspace Manifest, Change Set | controller | writers/reviewer |
| PAF-UC-CODE-004 | exact-head independent review | review exact change independently | controller, reviewer | Change Set, Review Result, Policy Decision | writer/controller | reviewer |
| PAF-UC-CODE-005 | blocked review to remediation | correct structured findings | reviewer, planner, writer | Review Result, Plan, Workflow | reviewer/controller | remediation team |
| PAF-UC-CODE-006 | bounded correction cycles | stop repeated correction safely | controller, reviewer, writer | Workflow, Policy Decision, Runtime Checkpoint | controller | team |
| PAF-UC-CODE-007 | human-only merge | publish only with human approval | human, controller | Change Set, Policy Decision, Execution Receipt | controller/human | publication boundary |
| PAF-UC-CODE-008 | cross-repository SODA-OPT/Bambu change | coordinate two repositories | controller, writers, reviewer | Workspace Manifest, Change Set, Workflow | controller | repositories/reviewer |
| PAF-UC-CODE-009 | denied child spawn | prevent unauthorized delegation | parent, controller/policy | Spawn Request, Policy Decision, Workflow | parent | controller |
| PAF-UC-CODE-010 | cancellation and cleanup | stop safely and preserve evidence | controller, executor | Workflow, Execution Request, Execution Receipt | controller/executor | audit/resumer |
| PAF-UC-CODE-011 | controller restart and resume | continue deterministic work | controller, resumer | Runtime Checkpoint, Workflow | controller | resumer |
| PAF-UC-CODE-012 | malformed/ambiguous review verdict | fail closed rather than approve | reviewer, controller | Review Result, Workflow | reviewer | controller |
| PAF-UC-CODE-013 | CI/build/test failure | preserve failure and remediate | executor, reviewer, controller | Result, Execution Receipt, Workflow | executor | controller/reviewer |
| PAF-UC-CODE-014 | downstream regression feedback | append downstream evidence | downstream consumer, controller | Feedback Record, Result, Provenance Event | downstream | controller/audit |

#### Table B — Identity, data, authority, and budget
| Use-case ID | Required identities and lineage | Required inputs | Required outputs | Permission and authorization boundary | Budget and reserve behavior |
|---|---|---|---|---|---|
| PAF-UC-CODE-001 | I: task→plan→handoff | Task, accepted artifacts | Plan, exact Handoff | planner has no execution authority | requested only; policy allocates |
| PAF-UC-CODE-002 | I: child/read-only bindings | question, artifact refs | evidence Results | Policy grants read-only only | bounded child allocations |
| PAF-UC-CODE-003 | I: writer/worktree/namespace | Task, workspace refs | Change Sets | isolated mutable worktree/output namespace | per-writer cap; no shared reserve |
| PAF-UC-CODE-004 | I: task/change-set/commit/artifact head | immutable head, review policy | structured Review Result | independent authorized reviewer route | protected review reserve only |
| PAF-UC-CODE-005 | I: finding→cycle→attempt | blocked review/finding digests | remediation Plan/Result | only granted remediation scope | cycle allocation; reserve protected |
| PAF-UC-CODE-006 | I: monotonic cycle count | cycle limit, prior evidence | stop or CK | no attempt after limit | hard cycle/budget limit |
| PAF-UC-CODE-007 | I: change-set/publication decision | approved head, human decision | publication Receipt | human-only merge authority | merge spend separately authorized |
| PAF-UC-CODE-008 | I: repos/revisions/dependency edges | workspace topology, access | coordinated Change Set | per-repository least privilege | split binding/repo ledgers |
| PAF-UC-CODE-009 | I: parent/spawn/denial | Spawn Request, policy | denial event | denial creates no child authority | no allocation consumed |
| PAF-UC-CODE-010 | I: attempt/cancel/cleanup | active request, cancellation | cancelled Result/Receipt | controller cancellation authority | stop charging; retain cleanup reserve |
| PAF-UC-CODE-011 | I: checkpoint snapshot | CK, current policies | resumed decision/attempt | resumer may not expand authority | restore remaining ledgers/reserves |
| PAF-UC-CODE-012 | I: review/head/verdict parse | Review Result bytes | invalid-verdict event | no implicit approval | review reserve remains accounted |
| PAF-UC-CODE-013 | I: request/result/receipt | validation evidence | failed Result, findings | remediation needs new authorization | actual use from Receipt |
| PAF-UC-CODE-014 | I: original result→feedback | downstream evidence | Feedback Record | feedback cannot alter original | separately authorized follow-up |

#### Table C — Provenance, governance, and lifecycle
| Use-case ID | Artifact and representation provenance | Interaction and decision provenance | Audit retention | Evaluation eligibility | Training eligibility | Redistribution eligibility | Failure/cancellation/retry/resumption behavior |
|---|---|---|---|---|---|---|---|
| PAF-UC-CODE-001 | A: plan/exact handoff | P: planning/routing | retain | independent G | independent G | independent G | blocked handoff stops; retry new attempt |
| PAF-UC-CODE-002 | A: each source/evidence representation | P: fan-out/join | retain | independent G | independent G | independent G | child failure isolated; join records it |
| PAF-UC-CODE-003 | A: worktree diffs/outputs | P: workspace allocation | retain | independent G | independent G | independent G | conflict fails safely; no shared write |
| PAF-UC-CODE-004 | A: exact reviewed bytes/digests | P: independence/routing/verdict | retain | independent G | independent G | independent G | mismatch blocks; re-review exact head |
| PAF-UC-CODE-005 | A: findings/derived patches | P: remediation/cycle | retain | independent G | independent G | independent G | append cycle; never overwrite review |
| PAF-UC-CODE-006 | A: all cycles | P: exhaustion | retain | independent G | independent G | independent G | exhaustion stops or CK deterministically |
| PAF-UC-CODE-007 | A: approved head/publication | P: human approval | retain | independent G | independent G | independent G | absent approval stops; no merge |
| PAF-UC-CODE-008 | A: per-repo revisions/artifacts | P: cross-repo decisions | retain | independent G | independent G | independent G | partial failure checkpoints state |
| PAF-UC-CODE-009 | A: spawn request | P: denial rationale | retain | independent G | independent G | independent G | denied; deterministic alternative |
| PAF-UC-CODE-010 | A: logs/cleanup evidence | P: cancellation cause | retain diagnostics | independent G | independent G | independent G | R attests cleanup; CK if resumable |
| PAF-UC-CODE-011 | A: CK references all state | P: restart/resume route | retain | independent G | independent G | independent G | one deterministic next action |
| PAF-UC-CODE-012 | A: raw malformed verdict | P: parse failure | retain | independent G | independent G | independent G | fail closed; request replacement |
| PAF-UC-CODE-013 | A: CI/build/test logs | P: failure classification | retain | independent G | independent G | independent G | retain failure; authorized remediation |
| PAF-UC-CODE-014 | A: immutable feedback derivative | P: downstream linkage | retain | independent G | independent G | independent G | append only; optional workflow |

#### Table D — Conformance and delivery
| Use-case ID | Required valid fixture | Required invalid/adversarial fixture | Required semantic invariant | Intended milestone |
|---|---|---|---|---|
| PAF-UC-CODE-001 | V: plan/handoff | X: plan used as request | visibility closure | PR1/3/4/6 |
| PAF-UC-CODE-002 | V: two read-only children | X: child write attempt | authorization monotonicity | PR4 |
| PAF-UC-CODE-003 | V: isolated writers | X: shared worktree | workspace isolation | PR4 |
| PAF-UC-CODE-004 | V: matching independent review | X: stale/self review | exact-head review; reviewer independence | PR4/5 |
| PAF-UC-CODE-005 | V: findings remediation | X: rewritten finding | lineage closure | PR4 |
| PAF-UC-CODE-006 | V: last permitted cycle | X: cycle N+1 | budget non-expansion; checkpoint consistency | PR4/5 |
| PAF-UC-CODE-007 | V: human-approved merge | X: automated merge | human authority | PR4 |
| PAF-UC-CODE-008 | V: two-repo set | X: revision mismatch | identity closure | PR4 |
| PAF-UC-CODE-009 | V: denied spawn | X: denied child runs | authorization monotonicity | PR4 |
| PAF-UC-CODE-010 | V: cancelled cleanup | X: missing receipt | receipt truth boundary | PR4/6 |
| PAF-UC-CODE-011 | V: restart snapshot | X: ambiguous next action | checkpoint consistency | PR5 |
| PAF-UC-CODE-012 | V: valid verdict | X: ambiguous approval | fail-closed validation | PR4 |
| PAF-UC-CODE-013 | V: failed CI receipt | X: failure marked pass | measurement integrity | PR3/6 |
| PAF-UC-CODE-014 | V: linked regression | X: original mutation | lineage closure | PR8 |

### ACCESS — environments and bindings

#### Table A — Intent and contract ownership
| Use-case ID | Scenario | User objective | Actors and agent roles | Authoritative contract(s) | Producer | Consumer |
|---|---|---|---|---|---|---|
| PAF-UC-ACCESS-001 | Cline environment | use discovered environment | setup, adapter, router | Access Binding, catalog/capability | adapter/setup | router |
| PAF-UC-ACCESS-002 | Codex environment | use discovered environment | setup, adapter, router | Access Binding, catalog/capability | adapter/setup | router |
| PAF-UC-ACCESS-003 | Claude Code environment | use discovered environment | setup, adapter, router | Access Binding, catalog/capability | adapter/setup | router |
| PAF-UC-ACCESS-004 | direct API | route authorized API | setup, binding, router | Access Binding, Policy Decision | setup | router |
| PAF-UC-ACCESS-005 | gateway | route authorized gateway | setup, gateway, router | Access Binding, catalog | gateway/setup | router |
| PAF-UC-ACCESS-006 | local runtime | use local runtime | setup, local adapter | Access Binding, capability | setup | router |
| PAF-UC-ACCESS-007 | multiple credentials | keep keys separate | setup, bindings | Access Binding, ledger | setup | router/audit |
| PAF-UC-ACCESS-008 | account/project/funding scopes | enforce scopes | policy, router | Access Binding, Policy Decision | policy | router |
| PAF-UC-ACCESS-009 | dynamic discovery per binding | discover safely | discovery, binding | catalog/capability evidence | discovery | router |
| PAF-UC-ACCESS-010 | stale cache/model pin | fallback safely | discovery, policy, router | catalog, Policy Decision | router | executor |
| PAF-UC-ACCESS-011 | binding capability evidence | prove capability | probe, discovery | capability evidence | discovery | router |
| PAF-UC-ACCESS-012 | restricted-data denial | reject route | policy, router | Policy Decision, Access Binding | policy | router |
| PAF-UC-ACCESS-013 | subscription-to-metered denial | prevent funding switch | policy, router | Policy Decision, Routing/Adaptation Decision | policy | router |
| PAF-UC-ACCESS-014 | portable checkpoint elsewhere | resume elsewhere | controller, resumer | Runtime Checkpoint, Access Binding | controller | resumer |

#### Table B — Identity, data, authority, and budget
| Use-case ID | Required identities and lineage | Required inputs | Required outputs | Permission and authorization boundary | Budget and reserve behavior |
|---|---|---|---|---|---|
| PAF-UC-ACCESS-001 | I: environment/binding/catalog | adapter evidence | binding status | client is scenario data, not enum | binding ledger |
| PAF-UC-ACCESS-002 | I: environment/binding/catalog | adapter evidence | binding status | client is scenario data, not enum | binding ledger |
| PAF-UC-ACCESS-003 | I: environment/binding/catalog | adapter evidence | binding status | client is scenario data, not enum | binding ledger |
| PAF-UC-ACCESS-004 | I: endpoint/opaque credential ref | policy, endpoint evidence | authorized binding | no credential value/path portable | binding ledger |
| PAF-UC-ACCESS-005 | I: gateway/upstream confidence | gateway catalog | scoped catalog | upstream not assumed | binding ledger |
| PAF-UC-ACCESS-006 | I: runtime/environment | local evidence | local binding | local data/network policy | local ledger |
| PAF-UC-ACCESS-007 | I: credential refs distinct | two bindings | separate catalogs/ledgers | same provider grants nothing | no pooling |
| PAF-UC-ACCESS-008 | I: account/project/funding/data | scope policy | eligible route | policy intersects scopes | separate reserves |
| PAF-UC-ACCESS-009 | I: model/environment/binding | authorized discovery | snapshot/status | probes require authority | discovery allocation |
| PAF-UC-ACCESS-010 | I: snapshot age/pin/decision | cache, pin, policy | route or denial | pin cannot override restrictions | permitted ledger only |
| PAF-UC-ACCESS-011 | I: tuple/source/time | probe evidence | capability status | unknown is not capability | no inferred quota |
| PAF-UC-ACCESS-012 | I: data class/denial | restricted task/policy | denial | no restricted crossing | no spend |
| PAF-UC-ACCESS-013 | I: old/new funding | routes, policy | denial | transition requires decision | preserve reserve |
| PAF-UC-ACCESS-014 | I: CK/old/new binding | CK, new evidence | resumed route | no authority expansion | restore, do not transfer ledger |

#### Table C — Provenance, governance, and lifecycle
| Use-case ID | Artifact and representation provenance | Interaction and decision provenance | Audit retention | Evaluation eligibility | Training eligibility | Redistribution eligibility | Failure/cancellation/retry/resumption behavior |
|---|---|---|---|---|---|---|---|
| PAF-UC-ACCESS-001 | A: adapter/environment evidence | P: binding selection | retain | independent G | independent G | independent G | unavailable explicit |
| PAF-UC-ACCESS-002 | A: adapter/environment evidence | P: binding selection | retain | independent G | independent G | independent G | unavailable explicit |
| PAF-UC-ACCESS-003 | A: adapter/environment evidence | P: binding selection | retain | independent G | independent G | independent G | unavailable explicit |
| PAF-UC-ACCESS-004 | A: endpoint evidence/no secret | P: authorization | retain | independent G | independent G | independent G | auth failure halts/refreshes |
| PAF-UC-ACCESS-005 | A: advertised catalog/confidence | P: gateway selection | retain | independent G | independent G | independent G | policy alternatives only |
| PAF-UC-ACCESS-006 | A: local evidence | P: local route | retain | independent G | independent G | independent G | no network fallback |
| PAF-UC-ACCESS-007 | A: separate catalogs | P: separate routing | retain | independent G | independent G | independent G | failure does not merge state |
| PAF-UC-ACCESS-008 | A: scope evidence | P: policy route | retain | independent G | independent G | independent G | ineligible denied |
| PAF-UC-ACCESS-009 | A: timestamped snapshot | P: source/confidence | retain | independent G | independent G | independent G | stale/unknown explicit |
| PAF-UC-ACCESS-010 | A: cache/pin | P: fallback decision | retain | independent G | independent G | independent G | pin fails if policy fails |
| PAF-UC-ACCESS-011 | A: probe artifacts | P: capability rationale | retain | independent G | independent G | independent G | failure retained unknown |
| PAF-UC-ACCESS-012 | A: classification/policy | P: denial | retain | independent G | independent G | independent G | deterministic deny/CK |
| PAF-UC-ACCESS-013 | A: route/funding evidence | P: transition denial | retain | independent G | independent G | independent G | stop/CK, never switch |
| PAF-UC-ACCESS-014 | A: CK/new route evidence | P: resume decision | retain | independent G | independent G | independent G | records new execution route |

#### Table D — Conformance and delivery
| Use-case ID | Required valid fixture | Required invalid/adversarial fixture | Required semantic invariant | Intended milestone |
|---|---|---|---|---|
| PAF-UC-ACCESS-001 | V: Cline descriptor | X: client enum core | portability | PR5/9 |
| PAF-UC-ACCESS-002 | V: Codex descriptor | X: client enum core | portability | PR5/9 |
| PAF-UC-ACCESS-003 | V: Claude Code descriptor | X: client enum core | portability | PR5/9 |
| PAF-UC-ACCESS-004 | V: API opaque ref | X: credential/path | portability | PR5 |
| PAF-UC-ACCESS-005 | V: gateway tuple | X: assumed upstream | identity closure | PR5 |
| PAF-UC-ACCESS-006 | V: local binding | X: implicit network | no silent boundary transition | PR5 |
| PAF-UC-ACCESS-007 | V: two bindings | X: shared ledger | budget non-expansion | PR5 |
| PAF-UC-ACCESS-008 | V: scoped route | X: scope crossing | authorization monotonicity | PR5 |
| PAF-UC-ACCESS-009 | V: bound discovery | X: global catalog reuse | identity closure | PR5 |
| PAF-UC-ACCESS-010 | V: stale explicit pin | X: pin bypass | no silent boundary transition | PR5 |
| PAF-UC-ACCESS-011 | V: tuple capability | X: provider-wide claim | identity closure | PR5 |
| PAF-UC-ACCESS-012 | V: restricted denial | X: public route | authorization monotonicity | PR5 |
| PAF-UC-ACCESS-013 | V: transition denial | X: automatic metered | no silent boundary transition | PR5 |
| PAF-UC-ACCESS-014 | V: alternate resume | X: CK state loss | checkpoint consistency | PR5/9 |

### BUDGET — budgets and adaptation

#### Table A — Intent and contract ownership
| Use-case ID | Scenario | User objective | Actors and agent roles | Authoritative contract(s) | Producer | Consumer |
|---|---|---|---|---|---|---|
| PAF-UC-BUDGET-001 | soft-threshold rerouting | reconsider safely | controller/router | Budget Policy, Routing/Adaptation Decision | controller | router |
| PAF-UC-BUDGET-002 | hard-threshold stop | prohibit overspend | controller | Budget Policy, Runtime Checkpoint | controller | resumer |
| PAF-UC-BUDGET-003 | independent-review reserve | preserve review | policy, controller | Policy Decision, ledger | policy | controller |
| PAF-UC-BUDGET-004 | final-validation reserve | preserve validation | policy, controller | Policy Decision, ledger | policy | controller |
| PAF-UC-BUDGET-005 | unknown subscription quota | avoid false certainty | binding, router | binding status, ledger | binding | router |
| PAF-UC-BUDGET-006 | rate limit/unavailability | adapt safely | binding, controller | status, Routing/Adaptation Decision | binding | controller |
| PAF-UC-BUDGET-007 | reduced concurrency | reduce resource use | controller | Workflow, Routing/Adaptation Decision | controller | workers |
| PAF-UC-BUDGET-008 | reduced delegation | reduce child work | controller | Workflow, Policy Decision | controller | parent |
| PAF-UC-BUDGET-009 | approved local fallback | use permitted local route | policy, router | Policy Decision, Access Binding | policy | router |
| PAF-UC-BUDGET-010 | no permitted route | stop deterministically | controller | Routing/Adaptation Decision, Runtime Checkpoint | controller | audit/resumer |
| PAF-UC-BUDGET-011 | approved boundary transition | make transition explicit | human/policy, router | Policy Decision, Routing/Adaptation Decision | human/policy | router |

#### Table B — Identity, data, authority, and budget
| Use-case ID | Required identities and lineage | Required inputs | Required outputs | Permission and authorization boundary | Budget and reserve behavior |
|---|---|---|---|---|---|
| PAF-UC-BUDGET-001 | I: binding/ledger/decision | threshold, alternatives | reroute decision | permitted routes only | soft triggers reconsideration |
| PAF-UC-BUDGET-002 | I: ledger/threshold/stop | actual R | stop/CK | no request after hard limit | hard limit absolute |
| PAF-UC-BUDGET-003 | I: review reserve | review requirement | allocation | unrelated stages denied | review only |
| PAF-UC-BUDGET-004 | I: validation reserve | validation requirement | allocation | unrelated stages denied | validation only |
| PAF-UC-BUDGET-005 | I: binding/quota evidence | unknown status | cautious route/stop | unknown grants no permission | neither zero nor unlimited |
| PAF-UC-BUDGET-006 | I: status/attempt/backoff | rate-limit evidence | adaptation/CK | retry policy-controlled | actual use from R |
| PAF-UC-BUDGET-007 | I: workflow/concurrency | worker state | reduced fan-out | no authority expansion | recorded reallocation |
| PAF-UC-BUDGET-008 | I: delegation decision | child plan/policy | reduced spawn plan | denied child never runs | reserve preserved |
| PAF-UC-BUDGET-009 | I: old/new binding | local binding/policy | approved fallback | explicit local/data/network authority | separate local ledger |
| PAF-UC-BUDGET-010 | I: route-set/exhaustion CK | alternatives | terminal stop/CK | no invented route | preserve reserve |
| PAF-UC-BUDGET-011 | I: approval/route | old/new scope, decision | transition | explicit authority | no silent ledger pooling |

#### Table C — Provenance, governance, and lifecycle
| Use-case ID | Artifact and representation provenance | Interaction and decision provenance | Audit retention | Evaluation eligibility | Training eligibility | Redistribution eligibility | Failure/cancellation/retry/resumption behavior |
|---|---|---|---|---|---|---|---|
| PAF-UC-BUDGET-001 | A: ledger/R | P: threshold rationale | retain | independent G | independent G | independent G | reconsider or CK |
| PAF-UC-BUDGET-002 | A: R/ledger | P: hard-stop | retain | independent G | independent G | independent G | deterministic stop/CK |
| PAF-UC-BUDGET-003 | A: allocation/R | P: reserve | retain | independent G | independent G | independent G | only review consumes reserve |
| PAF-UC-BUDGET-004 | A: allocation/R | P: reserve | retain | independent G | independent G | independent G | only validation consumes reserve |
| PAF-UC-BUDGET-005 | A: quota status | P: uncertainty | retain | independent G | independent G | independent G | refresh/restrict/stop |
| PAF-UC-BUDGET-006 | A: rate-limit evidence | P: backoff | retain | independent G | independent G | independent G | bounded retry or CK |
| PAF-UC-BUDGET-007 | A: worker state | P: adaptation | retain | independent G | independent G | independent G | resume reduced state |
| PAF-UC-BUDGET-008 | A: spawn records | P: adaptation | retain | independent G | independent G | independent G | denied work remains absent |
| PAF-UC-BUDGET-009 | A: local evidence | P: approval | retain | independent G | independent G | independent G | policy alternatives only |
| PAF-UC-BUDGET-010 | A: considered routes | P: no-route rationale | retain | independent G | independent G | independent G | terminal CK/human action |
| PAF-UC-BUDGET-011 | A: scopes/ledger | P: approval | retain | independent G | independent G | independent G | denial returns/CK |

#### Table D — Conformance and delivery
| Use-case ID | Required valid fixture | Required invalid/adversarial fixture | Required semantic invariant | Intended milestone |
|---|---|---|---|---|
| PAF-UC-BUDGET-001 | V: permitted reroute | X: boundary crossing | no silent boundary transition | PR5 |
| PAF-UC-BUDGET-002 | V: hard stop | X: post-limit request | budget non-expansion | PR5 |
| PAF-UC-BUDGET-003 | V: protected review | X: implementation spends it | budget non-expansion | PR5 |
| PAF-UC-BUDGET-004 | V: protected validation | X: review spends it | budget non-expansion | PR5 |
| PAF-UC-BUDGET-005 | V: unknown quota | X: unknown=infinite | fail-closed validation | PR5 |
| PAF-UC-BUDGET-006 | V: rate-limit backoff | X: unbounded retry | budget non-expansion | PR5 |
| PAF-UC-BUDGET-007 | V: reduced workers | X: unrecorded reduction | checkpoint consistency | PR5 |
| PAF-UC-BUDGET-008 | V: reduced children | X: unauthorized spawn | authorization monotonicity | PR5 |
| PAF-UC-BUDGET-009 | V: approved local | X: local policy bypass | no silent boundary transition | PR5 |
| PAF-UC-BUDGET-010 | V: no-route CK | X: nondeterministic route | checkpoint consistency | PR5 |
| PAF-UC-BUDGET-011 | V: approved transition | X: missing decision | no silent boundary transition | PR5 |

### DSE — design-space exploration

#### Table A — Intent and contract ownership
| Use-case ID | Scenario | User objective | Actors and agent roles | Authoritative contract(s) | Producer | Consumer |
|---|---|---|---|---|---|---|
| PAF-UC-DSE-001 | exhaustive search | enumerate space | campaign controller, evaluator | Exploration Study, Parameter Space, Campaign State | controller | evaluator |
| PAF-UC-DSE-002 | sampled search | sample reproducibly | strategy, controller | Exploration Study, Strategy, Campaign State | strategy | evaluator |
| PAF-UC-DSE-003 | conditional parameter space | constrain legal points | study author, validator | Parameter Space, Candidate | author | generator |
| PAF-UC-DSE-004 | immutable candidate/duplicate suppression | avoid duplicate work | generator, controller | Candidate, Experiment Request | generator | controller |
| PAF-UC-DSE-005 | feasibility rejection | reject cheaply | filter, controller | Candidate, Measurement Set | filter | campaign |
| PAF-UC-DSE-006 | functional failure | preserve functional evidence | evaluator | Experiment Request, Measurement Set | evaluator | campaign |
| PAF-UC-DSE-007 | synthesis failure | preserve synthesis evidence | evaluator | Experiment Request, Measurement Set | evaluator | campaign |
| PAF-UC-DSE-008 | mixed fidelity | retain non-equivalence | controller, evaluator | Exploration Study, Measurement Set | evaluator | campaign |
| PAF-UC-DSE-009 | parallel evaluation | evaluate safely in parallel | controller, evaluators | Workflow, Experiment Request | controller | evaluators |
| PAF-UC-DSE-010 | repeated/noisy measurements | characterize uncertainty | evaluator | Measurement Set | evaluator | optimizer |
| PAF-UC-DSE-011 | comparability break | exclude invalid comparison | validator, controller | Measurement Set, Decision Report | validator | optimizer |
| PAF-UC-DSE-012 | optimizer/campaign restart | resume search | controller, resumer | Campaign State, Checkpoint | controller | resumer |
| PAF-UC-DSE-013 | agent-proposed candidate | govern proposal | agent, policy, controller | Candidate, Policy Decision | agent | controller |
| PAF-UC-DSE-014 | cross-repository candidate | coordinate compiler/architecture | controller, evaluators | Workspace Manifest, Candidate | controller | campaign |
| PAF-UC-DSE-015 | budget-driven batch reduction | adapt batch size | controller | Campaign State, Routing/Adaptation Decision | controller | evaluator |
| PAF-UC-DSE-016 | fidelity escalation | spend on evidence | controller, policy | Experiment Request, Policy Decision | controller | evaluator |
| PAF-UC-DSE-017 | FPGA/ASIC human gate | authorize expensive work | human, policy, controller | Policy Decision, Experiment Request | human/policy | evaluator |
| PAF-UC-DSE-018 | Pareto recommendation | recommend with evidence | controller, reviewer | Result Set, Decision Report | controller | human |

#### Table B — Identity, data, authority, and budget
| Use-case ID | Required identities and lineage | Required inputs | Required outputs | Permission and authorization boundary | Budget and reserve behavior |
|---|---|---|---|---|---|
| PAF-UC-DSE-001 | I: study/space/candidate/seed | canonical space | complete candidates | authorized study only | bounded campaign budget |
| PAF-UC-DSE-002 | I: strategy/random state/seed | sampling strategy | selected candidates | strategy cannot authorize runs | sample allocation |
| PAF-UC-DSE-003 | I: conditional rule digest | parameters/conditions | legal candidate | validator rejects undefined branch | no invalid experiment spend |
| PAF-UC-DSE-004 | I: canonical candidate fingerprint | params/revisions/toolchain/input/target/constraints/seed | candidate or duplicate ref | duplicate has no new authorization | duplicate consumes no second allocation |
| PAF-UC-DSE-005 | I: candidate/filter/result | feasibility evidence | rejected status | filter scope only | low-cost filter allocation |
| PAF-UC-DSE-006 | I: candidate/experiment/trial | test evidence | failed functional measurement | receipt/tool evidence authoritative | failure records actual use |
| PAF-UC-DSE-007 | I: candidate/experiment/tool | synthesis evidence | failed synthesis measurement | receipt/tool evidence authoritative | failure records actual use |
| PAF-UC-DSE-008 | I: fidelity/fingerprint | measurements | typed fidelity result | no implicit equivalence | budget per fidelity |
| PAF-UC-DSE-009 | I: experiment/workspace | immutable candidates | parallel measurements | isolated evaluator/output scope | per-experiment cap |
| PAF-UC-DSE-010 | I: trial/seed/repetition | repeated raw evidence | uncertainty measurement | no fabricated aggregate | repeated-trial allocation |
| PAF-UC-DSE-011 | I: fingerprint/comparison | incompatible evidence | excluded comparison | validator controls ranking | no spend for invalid ranking |
| PAF-UC-DSE-012 | I: campaign/optimizer/checkpoint | CK/random state | resumed campaign | resume cannot mutate past | restore remaining budget |
| PAF-UC-DSE-013 | I: proposer/candidate/decision | proposal evidence | authorized/rejected candidate | agent proposal not authority | authorization before run |
| PAF-UC-DSE-014 | I: repo revisions/candidate | workspace topology | cross-repo candidate | per-repo authority | split ledgers |
| PAF-UC-DSE-015 | I: campaign/batch decision | budget state | reduced batch | policy bounds adaptation | reserve verification budget |
| PAF-UC-DSE-016 | I: candidate/fidelity approval | promising evidence | escalated request | explicit policy required | higher-fidelity allocation |
| PAF-UC-DSE-017 | I: human decision/experiment | expensive request | authorized/denied request | human gate mandatory | no expensive reserve without approval |
| PAF-UC-DSE-018 | I: candidates/measurements/report | comparable measurements | Pareto report | report cannot authorize work | recommendation cites spent budget |

#### Table C — Provenance, governance, and lifecycle
| Use-case ID | Artifact and representation provenance | Interaction and decision provenance | Audit retention | Evaluation eligibility | Training eligibility | Redistribution eligibility | Failure/cancellation/retry/resumption behavior |
|---|---|---|---|---|---|---|---|
| PAF-UC-DSE-001 | A: canonical space/candidates | P: enumeration | retain | independent G | independent G | independent G | CK campaign on stop |
| PAF-UC-DSE-002 | A: strategy/state | P: sample selection | retain | independent G | independent G | independent G | restart from state |
| PAF-UC-DSE-003 | A: rules/rejections | P: validation | retain | independent G | independent G | independent G | invalid point retained rejected |
| PAF-UC-DSE-004 | A: candidate/dedup match | P: duplicate decision | retain | independent G | independent G | independent G | reuse evidence; no retry |
| PAF-UC-DSE-005 | A: filter evidence | P: rejection | retain | independent G | independent G | independent G | rejection distinct from failure |
| PAF-UC-DSE-006 | A: tests/logs | P: failure classification | retain | independent G | independent G | independent G | failed remains failed, not zero |
| PAF-UC-DSE-007 | A: synthesis logs | P: failure classification | retain | independent G | independent G | independent G | failed remains failed, not zero |
| PAF-UC-DSE-008 | A: fidelity evidence | P: equivalence decision | retain | independent G | independent G | independent G | incompatible comparison excluded |
| PAF-UC-DSE-009 | A: per-evaluator evidence | P: fan-out/join | retain | independent G | independent G | independent G | isolated failure recorded |
| PAF-UC-DSE-010 | A: raw trials | P: aggregation method | retain | independent G | independent G | independent G | retain seed/uncertainty; repeat authorized |
| PAF-UC-DSE-011 | A: fingerprints | P: comparability break | retain | independent G | independent G | independent G | exclude, never delete evidence |
| PAF-UC-DSE-012 | A: campaign CK | P: restart | retain | independent G | independent G | independent G | deterministic resume |
| PAF-UC-DSE-013 | A: proposal artifacts | P: proposal/decision | retain | independent G | independent G | independent G | rejected proposal retained |
| PAF-UC-DSE-014 | A: multi-repo evidence | P: topology decision | retain | independent G | independent G | independent G | checkpoint partial result |
| PAF-UC-DSE-015 | A: budget/batch state | P: adaptation | retain | independent G | independent G | independent G | reduced batch resumes |
| PAF-UC-DSE-016 | A: low/high fidelity links | P: escalation | retain | independent G | independent G | independent G | denied escalation retains low fidelity |
| PAF-UC-DSE-017 | A: request/evidence | P: human decision | retain | independent G | independent G | independent G | denial stops expensive work |
| PAF-UC-DSE-018 | A: measurements/alternatives | P: recommendation rationale | retain | independent G | independent G | independent G | uncertainty/comparability retained |

#### Table D — Conformance and delivery
| Use-case ID | Required valid fixture | Required invalid/adversarial fixture | Required semantic invariant | Intended milestone |
|---|---|---|---|---|
| PAF-UC-DSE-001 | V: full enumeration | X: missing point | candidate immutability | PR7 |
| PAF-UC-DSE-002 | V: seeded sample | X: missing strategy state | checkpoint consistency | PR7 |
| PAF-UC-DSE-003 | V: legal conditional | X: illegal branch | identity closure | PR7 |
| PAF-UC-DSE-004 | V: duplicate reference | X: duplicate run | candidate immutability | PR7 |
| PAF-UC-DSE-005 | V: infeasible status | X: rejected as failed | measurement integrity | PR7 |
| PAF-UC-DSE-006 | V: functional failure | X: zero success metric | measurement integrity | PR7 |
| PAF-UC-DSE-007 | V: synthesis failure | X: zero success metric | measurement integrity | PR7 |
| PAF-UC-DSE-008 | V: tagged fidelity | X: silent mix | comparability enforcement | PR7 |
| PAF-UC-DSE-009 | V: parallel evaluators | X: shared output | workspace isolation | PR7 |
| PAF-UC-DSE-010 | V: repeated trials | X: dropped uncertainty | measurement integrity | PR7 |
| PAF-UC-DSE-011 | V: excluded comparison | X: invalid Pareto rank | comparability enforcement | PR7 |
| PAF-UC-DSE-012 | V: resumed campaign | X: state mismatch | checkpoint consistency | PR7 |
| PAF-UC-DSE-013 | V: authorized proposal | X: auto-run proposal | authorization monotonicity | PR7 |
| PAF-UC-DSE-014 | V: cross-repo point | X: revision drift | identity closure | PR7 |
| PAF-UC-DSE-015 | V: reduced batch | X: reserve breach | budget non-expansion | PR7 |
| PAF-UC-DSE-016 | V: approved escalation | X: unapproved fidelity | authorization monotonicity | PR7 |
| PAF-UC-DSE-017 | V: human-gated run | X: automatic ASIC run | human authority | PR7 |
| PAF-UC-DSE-018 | V: evidence Pareto | X: narrative-only rank | comparability enforcement | PR7 |

### PROV — provenance and governed evolution

#### Table A — Intent and contract ownership
| Use-case ID | Scenario | User objective | Actors and agent roles | Authoritative contract(s) | Producer | Consumer |
|---|---|---|---|---|---|---|
| PAF-UC-PROV-001 | exact model-visible representation | reproduce visibility | controller, executor | Artifact, Handoff, Interaction Trace | controller | audit/evaluator |
| PAF-UC-PROV-002 | transformation fidelity/loss | explain derivation | transformer, controller | Artifact, Provenance Event | transformer | audit |
| PAF-UC-PROV-003 | trace/context compaction | reproduce interaction | controller, executor | Interaction Trace, Artifact | controller | audit |
| PAF-UC-PROV-004 | routing/adaptation decision | justify route | router, controller | Routing/Adaptation Decision, Policy Decision | router | audit |
| PAF-UC-PROV-005 | human override | preserve intervention | human, controller | Policy Decision, Provenance Event | human/controller | audit |
| PAF-UC-PROV-006 | redacted derivative | safely share derivative | redactor | Artifact, transformation event | redactor | permitted consumer |
| PAF-UC-PROV-007 | audit-only evidence | retain without reuse | governance, audit | Artifact governance | governance | audit |
| PAF-UC-PROV-008 | training-ineligible restricted evidence | protect restricted evidence | governance | Artifact governance | governance | dataset selector |
| PAF-UC-PROV-009 | governed dataset manifest | select eligible evidence | curator | Dataset Manifest | curator | learning/evaluation |
| PAF-UC-PROV-010 | offline learning run | improve policy reproducibly | learner | Learning Run, Dataset Manifest | learner | evaluator |
| PAF-UC-PROV-011 | held-out/adversarial evaluation | evaluate without leakage | evaluator | Policy Evaluation, Evaluation Record | evaluator | human |
| PAF-UC-PROV-012 | human promotion/rollback | govern policy release | human, release manager | Promotion, Policy Release, Rollback | human | controller |

#### Table B — Identity, data, authority, and budget
| Use-case ID | Required identities and lineage | Required inputs | Required outputs | Permission and authorization boundary | Budget and reserve behavior |
|---|---|---|---|---|---|
| PAF-UC-PROV-001 | I: bytes/representation/handoff | exact visible bytes | recoverable representation | visibility only as Handoff declares | representation storage policy |
| PAF-UC-PROV-002 | I: source/derived/tool/params | source, transform metadata | derivative/fidelity/loss | transform cannot erase source | transformation allocation |
| PAF-UC-PROV-003 | I: trace/source/compacted repr | messages/context/tool calls | compacted artifact | compaction does not expand visibility | context budget recorded |
| PAF-UC-PROV-004 | I: alternatives/decision/policy | route evidence, policy | decision record | router cannot grant authority | binding budget snapshot |
| PAF-UC-PROV-005 | I: original/override decision | controller decision, human action | override event | human authority explicit | override spend separately authorized |
| PAF-UC-PROV-006 | I: source/redacted/tool/params | source, redaction policy | derivative artifact | derivative permissions independently checked | redaction allocation |
| PAF-UC-PROV-007 | I: artifact/governance decision | audit evidence | audit-only metadata | no evaluation/training grant | retention cost only |
| PAF-UC-PROV-008 | I: artifact/classification | restricted evidence | training denial | selector must exclude | no learning allocation |
| PAF-UC-PROV-009 | I: manifest/evidence/exclusions | eligible immutable refs | manifest | curator cannot override governance | dataset build allocation |
| PAF-UC-PROV-010 | I: dataset/run/policy candidate | pinned manifest/code/config | offline run output | no online controller mutation | learning budget separate |
| PAF-UC-PROV-011 | I: heldout/eval/run | held-out/adversarial refs | evaluation record | train/eval sets disjoint | evaluation reserve |
| PAF-UC-PROV-012 | I: candidate/evaluation/release/rollback | pinned evaluation, human decision | release/rollback | human authority mandatory | promotion budget separate |

#### Table C — Provenance, governance, and lifecycle
| Use-case ID | Artifact and representation provenance | Interaction and decision provenance | Audit retention | Evaluation eligibility | Training eligibility | Redistribution eligibility | Failure/cancellation/retry/resumption behavior |
|---|---|---|---|---|---|---|---|
| PAF-UC-PROV-001 | A: exact bytes/digest | P: exposure event | retain | independent G | independent G | independent G | missing bytes blocks reproduction |
| PAF-UC-PROV-002 | A: source/derived/fidelity/loss | P: transform tool/version/params | retain | independent G | independent G | independent G | loss explicit; source immutable |
| PAF-UC-PROV-003 | A: trace/compacted bytes | P: compaction rationale | retain | independent G | independent G | independent G | compacted form recoverable/linked |
| PAF-UC-PROV-004 | A: evidence/snapshot | P: alternatives/rationale | retain | independent G | independent G | independent G | adaptation appends decision |
| PAF-UC-PROV-005 | A: original and override | P: human override | retain | independent G | independent G | independent G | original never erased |
| PAF-UC-PROV-006 | A: source/derivative relation | P: redaction event | retain per source policy | independent G | independent G | derivative-specific G | failure retains source lineage |
| PAF-UC-PROV-007 | A: audit artifact | P: governance decision | retain | explicitly ineligible | explicitly ineligible | explicitly ineligible | no reuse on retention alone |
| PAF-UC-PROV-008 | A: restricted artifact | P: training denial | retain | policy decision | explicitly ineligible | policy decision | exclusion retained |
| PAF-UC-PROV-009 | A: manifest and excluded refs | P: selection rules | retain | manifest decision | eligible only | manifest decision | rebuild from pinned refs |
| PAF-UC-PROV-010 | A: run/code/config/output | P: offline run decision | retain | independent G | outputs policy decision | independent G | failure retains reproducibility evidence |
| PAF-UC-PROV-011 | A: disjoint eval data/results | P: evaluation method | retain | eligible by policy | ineligible for training selection | policy decision | adversarial failure retained |
| PAF-UC-PROV-012 | A: release/evaluation/rollback | P: human promotion/rollback | retain | independent G | independent G | release policy decision | rollback reversible and complete |

#### Table D — Conformance and delivery
| Use-case ID | Required valid fixture | Required invalid/adversarial fixture | Required semantic invariant | Intended milestone |
|---|---|---|---|---|
| PAF-UC-PROV-001 | V: recoverable visible bytes | X: digest-only visibility | visibility closure | PR2/6 |
| PAF-UC-PROV-002 | V: loss-tagged derivative | X: missing transform data | redaction lineage | PR2 |
| PAF-UC-PROV-003 | V: linked compaction | X: source trace omitted | visibility closure | PR6 |
| PAF-UC-PROV-004 | V: rationale record | X: route without policy | no silent boundary transition | PR5 |
| PAF-UC-PROV-005 | V: override event | X: replaced original | lineage closure | PR2/8 |
| PAF-UC-PROV-006 | V: redacted derivative | X: source mutation | redaction lineage | PR2 |
| PAF-UC-PROV-007 | V: audit-only artifact | X: retained→train | governance independence | PR8 |
| PAF-UC-PROV-008 | V: restricted exclusion | X: training inclusion | governance independence | PR8 |
| PAF-UC-PROV-009 | V: pinned manifest | X: mutable/uneligible ref | governance independence | PR8 |
| PAF-UC-PROV-010 | V: offline run | X: online policy mutation | human authority | PR8 |
| PAF-UC-PROV-011 | V: held-out evaluation | X: train/eval leakage | governance independence | PR8 |
| PAF-UC-PROV-012 | V: approved release/rollback | X: automatic promotion | human authority | PR8 |

## Cross-contract invariants (normative)

1. **Identity closure:** every reference resolves to exactly one identity and digest.
2. **Digest verification:** recompute declared digests before trust.
3. **Lineage closure:** root, parent, child, stage, attempt, and remediation graphs are acyclic and valid.
4. **Authorization monotonicity:** downstream contracts cannot expand upstream authority.
5. **Budget non-expansion:** attempts/children stay within authorized remaining budget and protected reserves.
6. **Exact-head review:** reviewed revisions and artifacts equal immutable Change Set head.
7. **Reviewer independence:** resolve required independence from routing/binding evidence, never self-assertion.
8. **Workspace isolation:** concurrent writers cannot share mutable worktree or output namespace.
9. **Visibility closure:** Handoff lists every representation exposed to its executor.
10. **Receipt truth boundary:** only Receipts attest execution and consumption.
11. **No silent boundary transition:** funding, credential, account, data, network, or authorization changes cite Policy Decision.
12. **Governance independence:** retention never implies evaluation, training, or redistribution eligibility.
13. **Redaction lineage:** redacted derivatives retain source and transformation references.
14. **Candidate immutability:** candidate identity cannot change once evaluation begins.
15. **Measurement integrity:** failed/missing observations are never successful zero values.
16. **Comparability enforcement:** incompatible fingerprints cannot directly rank or dominate.
17. **Checkpoint consistency:** a checkpoint is a mutually consistent snapshot with one deterministic next action.
18. **Human authority:** merge, expensive FPGA/ASIC work, and policy promotion require mandated explicit human authority.
19. **Fail-closed versions:** unsupported versions and unknown fields are rejected.
20. **Portability:** core contracts require no provider, model, client, repository, local-path, or DSE-tool extension.

## Versioning, canonicalization, and migration

### Coexistence and required v2 semantics

V1 documents remain immutable and valid under current validators. V2 uses distinct schema versions and semantic validators; a v1 document is never v2 because fields look similar. A conversion creates a derived artifact with migration provenance and never invents missing identity, authorization, budget, or provenance facts; information remains unknown, missing, or `null` as the target requires. Compatibility adapters are boundary components, not portable core semantics.

Task v2, Result v2, generalized Handoff, generalized Execution Request, generalized Execution Receipt, and routing/catalog/access abstractions that must remove client-specific enums and represent binding-scoped discovery, authority, budgets, and provenance require v2 semantics. New families are shared primitives; Artifact and Provenance Event; Plan, Workflow, Spawn Request, Policy Decision; Review Result and Change Set; Workspace Manifest and Runtime Checkpoint; Access Binding, catalog/capability/binding status and budget records; DSE contracts; and Dataset, Learning, Policy Candidate/Evaluation/Release, Promotion, and Rollback contracts.

### Canonicalization and digest expectations

Canonical bytes use one versioned deterministic JSON canonicalization algorithm, centrally documented and tested byte-for-byte before dependent schemas merge. Digests are explicit, initially SHA-256, lowercase hexadecimal, and domain-separated by contract identity and schema version. A self-digest field is excluded from its own digest input. Object member order is canonical; array order remains significant unless a contract declares an unordered set and normalizes it before hashing. Immutable references include canonical identity and digest. Transformations/migrations create new digests and preserve source lineage. V1 digest algorithms remain unchanged for compatibility even where they differ. PR 1 chooses and publishes the precise algorithm and vectors.

### Migration and deprecation principles

A version or field is deprecated only after its replacement contract, validators, migration behavior, and compatibility fixtures exist. Deprecation is explicit in version and schema documentation; it is never inferred from age. Deprecated v1 and replacement v2 contracts coexist for a documented support window and until identified consumers have a migration path. During that window, new producers stop emitting the deprecated version while consumers continue validating it under the applicable version-specific rules.

Retirement is an explicit governed change, never a silent reinterpretation. After retirement, active consumers reject the retired version fail-closed, while immutable historical evidence and pinned compatibility fixtures remain available for audit and reproduction. Each migration creates a derived artifact, preserves source lineage and digests, and never invents missing evidence.

### Fail-closed behavior

Unknown fields and unsupported versions fail schema and semantic validation. Unknown enum values fail unless a contract explicitly defines safe `unknown`. Missing authorization, compatibility, capability, quota, or governance evidence never defaults to permission; consumers never discard a newer field silently. Version negotiation occurs outside contract interpretation.

## Planning-document consistency disposition

The three plans agree on ownership, binding-scoped discovery, budget boundaries, checkpoint portability, human gates, DSE evidence, and governance separation. The roadmap’s completed foundations are narrow v1 foundations, not PAF-05A3a completion. Named local model preferences are user-local examples, never portable vocabulary. Current v1 client-specific enums are compatibility limitations for later v2 generalization. No authoritative planning-document edit is required: the observed 25-schema count corrects only the supplied audit’s incidental inventory count and creates no plan contradiction.

## Dependency-ordered follow-up PRs

### PR 1 — Shared primitives
**Scope:** versioned IDs/references/timestamps/event order; canonicalization/digests; lineage; repository/workspace references; permissions/authorization; budgets/reserves/ledgers; governance; units/dimensions/uncertainty; model/environment/runtime/binding identities.
**Non-goals:** no Task v2, workflows, routing engine, execution, or DSE optimizer.
**Acceptance:** strict schemas/validators, canonical byte/digest vectors, unknown-field/version failures, lineage/reference fixtures, and no provider/client/local-path hard-coding.

### PR 2 — Artifact and provenance
**Scope:** Artifact, typed representations, transformations/loss, Provenance Event, redaction lineage, and reachability.
**Non-goals:** no orchestration, interaction execution, or learning.
**Acceptance:** exact-visible-representation and redacted-derivative fixtures; invalid digest/lineage/transformation fixtures; independent governance fields.

### PR 3 — Task and Result v2
**Scope:** Task v2, Result v2, stable requirement/acceptance IDs, typed artifacts, requested permissions/budgets, requirement outcomes, and v1 migration adapters/fixtures.
**Non-goals:** no workflow state machine, authority grant, or execution runtime.
**Acceptance:** v1/v2 coexist; migration invents no evidence; Task stays provider/client/model neutral; Result does not claim execution facts.

### PR 4 — Workflow, spawn, review, and change sets
**Scope:** Plan, Workflow, Spawn Request, Review Result, remediation lineage, Workspace Manifest, Change Set, publication intent/outcomes, cancellation, and bounded cycles.
**Non-goals:** no merge automation, runtime adapter, or routing implementation.
**Acceptance:** planner/implementer, investigators, isolated writers, exact-head review, denied spawn, remediation, cross-repo, and human-merge fixtures; deterministic transition validation.

### PR 5 — Access binding, discovery, routing, budgets, checkpoints
**Scope:** Access Binding/credential reference; endpoint/account/project/funding; catalog/discovered model; capability/status; Policy and Routing/Adaptation Decisions; budgets/ledgers; Runtime Checkpoint; transition policy.
**Non-goals:** no live provider calls, credential storage, or execution-adapter changes.
**Acceptance:** separate credential state; binding/environment-scoped discovery; stale-cache, restricted-data, transition-denial, reserve, no-route, and cross-environment checkpoint fixtures.

### PR 6 — Execution integration contracts
**Scope:** generalized Execution Request, Handoff, Execution Receipt, Interaction Trace, artifact visibility, and execution/provenance validation.
**Non-goals:** no PAF-05A3b executor, process launch, or routing behavior changes.
**Acceptance:** one authorized attempt/request; exact visible context; receipt/request identity closure; cancellation/cleanup/consumption evidence; fixture-local v1 remains valid.

### PR 7 — DSE contracts
**Scope:** Exploration Study, Parameter Space, Candidate, Experiment Request, Measurement Set, Strategy, Campaign State, Pareto/Result Set, Decision Report.
**Non-goals:** no optimizer, synthesis execution, or FPGA/ASIC allocation.
**Acceptance:** all 18 DSE scenarios get valid/adversarial fixtures; deterministic candidates, duplicate suppression, failure integrity, fidelity/comparability validation, checkpointable campaign state.

### PR 8 — Learning and governance
**Scope:** Dataset Manifest, Evaluation/Feedback Record, Learning Run, Policy Candidate/Evaluation/Release, Promotion, and Rollback.
**Non-goals:** no online learning, autonomous promotion, or training pipeline.
**Acceptance:** audit-only/restricted exclusion; reproducible selection/exclusions; held-out/adversarial separation; human promotion and complete rollback.

### PR 9 — Conformance suite
**Scope:** manifests for all 69 IDs; valid/invalid/adversarial fixtures; schema/semantic parity; graph validation; v1/v2 migration; portability proofs.
**Non-goals:** no live network, production execution, or expensive PandA build.
**Acceptance:** every ID coverage; every invariant positive/negative; same Task/Workflow through two environment descriptors without extension; fail-closed version/field checks; no generated residue.

## Completeness ledger

The matrix declares exactly 69 IDs: CODE 001–014, ACCESS 001–014, BUDGET 001–011, DSE 001–018, and PROV 001–012. Each occurs exactly once in each of its family’s Tables A, B, C, and D. The scenarios collectively map every required supplied scenario, and every Table D row names both V and X fixture requirements, an invariant, and an implementation milestone.
