# EvolveHLS portable agentic framework roadmap

Each PR is narrow and excludes credentials, private endpoints/models, user configuration changes, branch-rule changes, and automatic merging.

| PR | Objective | Dependency | Expected branch | Proposed PR title | Acceptance criterion | Explicit non-goals |
|---|---|---|---|---|---|---|
| PAF-01 | Provider contract and safe capability probe | None | `agent/portable-provider-core` | `build: add portable agent provider core` | Examples validate; doctor and mocked probes pass without secrets or paid access | Client adapters, roles, launchers, Bambu changes |
| PAF-02 | Neutral roles, tasks, and results | PAF-01 | `agent/portable-task-contracts` | `build: add portable agent task contracts` | Client-neutral task/result fixtures validate | Client-specific configuration |
| PAF-03 | Generated Codex, Claude Code, and Cline adapters | PAF-02 | `agent/portable-client-adapters` | `build: generate portable client adapters` | Deterministic adapters validate in all three formats | Launching tasks |
| PAF-04 | Durable architecture and inspection knowledge | PAF-02 | `agent/portable-knowledge` | `docs: add portable architecture knowledge` | Versioned knowledge is inspectable and source-linked | Runtime orchestration |
| PAF-05 | Isolated portable task launcher | PAF-03, PAF-04 | `agent/portable-task-launcher` | `build: add isolated portable task launcher` | Fixture tasks run in isolated worktrees | Hosted production execution |
| PAF-06 | Shared guardrails and secret handling | PAF-05 | `agent/portable-guardrails` | `security: add portable agent guardrails` | Cross-client policy and leakage tests pass | Weakening sandbox or approvals |
| PAF-07 | Cross-client evaluation harness | PAF-06 | `agent/portable-evaluation` | `test: add cross-client agent evaluation` | Reproducible fixture evaluation compares clients/providers | Benchmark claims without evidence |
| PAF-08 | Research evidence and paper scaffold | PAF-07 | `agent/portable-research` | `docs: add agentic research evidence scaffold` | Evidence provenance and paper sections validate | Fabricated or unpublished results |
| PAF-09 | SPARTA and GraphSAGE acceptance pilots | PAF-08 | `agent/portable-acceptance-pilots` | `test: add SPARTA GraphSAGE agent pilots` | Approved pilots preserve synthesis/simulation evidence | New accelerator implementation or synthesis changes |

SPARTA is a Bambu mode. The eventual GraphSAGE pilot targets Bambu's SPARTA path; PAF-01 adds neither SPARTA nor GraphSAGE code.
