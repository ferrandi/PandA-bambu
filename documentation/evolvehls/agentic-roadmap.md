# EvolveHLS portable agentic framework roadmap

Each PR is narrow and excludes credentials, private endpoints/models, account identifiers, user configuration changes, branch-rule changes, automatic merging, and task execution unless explicitly stated.

| PR | Objective | Dependency | Expected branch | Proposed PR title | Acceptance criterion | Explicit non-goals |
|---|---|---|---|---|---|---|
| PAF-01 | Provider contract and safe capability probe | None | `agent/portable-provider-core` | `build: add portable agent provider core` | Examples validate; doctor and mocked probes pass without secrets or paid access | Client adapters, roles, launchers |
| PAF-02 | Neutral roles, tasks, results, catalogs, and deterministic model selection | PAF-01 | `agent/portable-task-contracts` | `build: add portable agent task contracts` | Client-neutral fixtures validate | Client-specific configuration |
| PAF-03A | Execution-profile and routing contracts | PAF-02 | `agent/portable-execution-profiles` | `build: add portable execution profile contracts` | Deterministic, explained non-executing routing fixtures validate | Bootstrap, detection, generated adapters, launching |
| PAF-03B | Turnkey bootstrap, detection, and generated adapters | PAF-03A | `agent/portable-client-adapters` | `build: generate portable client adapters` | Authorized local bindings can be detected and generated safely | Running tasks |
| PAF-04 | Durable architecture and inspection knowledge | PAF-02 | `agent/portable-knowledge` | `docs: add portable architecture knowledge` | Versioned knowledge is inspectable and source-linked | Runtime orchestration |
| PAF-05 | Isolated one-task launcher | PAF-03B, PAF-04 | `agent/portable-task-launcher` | `build: add isolated portable task launcher` | Approved fixture task runs in an isolated worktree | Hosted production execution |
| PAF-06 | Guardrails, budgets, permissions, and funding boundaries | PAF-05 | `agent/portable-guardrails` | `security: add portable agent guardrails` | Cross-client funding and leakage tests pass | Weakening sandbox or approvals |
| PAF-07 | Autonomous multi-stage workflow controller | PAF-06 | `agent/portable-workflow-controller` | `build: add portable autonomous workflow controller` | Staged controller consumes explicit decisions and evidence | Implicit profile selection |
| PAF-08 | Evaluation | PAF-07 | `agent/portable-evaluation` | `test: add cross-client agent evaluation` | Reproducible fixture evaluation validates pinned decisions | Benchmark claims without evidence |
| PAF-09 | Research evidence | PAF-08 | `agent/portable-research` | `docs: add agentic research evidence scaffold` | Evidence provenance and paper sections validate | Fabricated or unpublished results |
| PAF-10 | SPARTA and GraphSAGE pilots | PAF-09 | `agent/portable-acceptance-pilots` | `test: add SPARTA GraphSAGE agent pilots` | Approved pilots preserve synthesis/simulation evidence | New accelerator implementation or synthesis changes |

## Target turnkey flow

```text
agentctl setup
agentctl doctor
agentctl profiles list
agentctl routing explain
agentctl run
```

Only PAF-03A’s `profiles` and `routing explain` operations exist here, and they are inspection-only. `setup`, generated adapters, and `run` remain future work.

PAF-03A keeps profile choice deterministic and outside an LLM. It separates access class, funding class, authentication mode, adapter identity/invocation family, provider-or-runtime binding, model selector/pin, constraints, and explicit fallback authorization. Evaluation mode never falls back.