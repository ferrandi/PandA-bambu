# PAF Responsibility-Transfer Matrix

Status: living cutover contract  
Version: 2026-08-03 revision 3

| Capability | Bootstrap authority initially | Shadow/transition mechanism | Cutover gate | PAF authority after cutover | Bootstrap retained role |
|---|---|---|---|---|---|
| Backlog selection | ordered campaign JSON and human merge ledger | PAF reads and predicts eligible work | hierarchical Work Graph/policy tests | Work Graph/controller | known-good recovery backlog |
| Generated task authorization | operator + deterministic metadata validator | PAF Task Contract shadow | Charter/task-contract authority tests | controller/policy | manual import/recovery |
| Allowed-path scope | task metadata + controller pre-commit check | Permission Profile translation | outer sandbox/path enforcement tests | policy/runtime | forensic scope check |
| Attempt identity and logs | timestamped run directory/native JSONL | emit redacted PAF Attempt/Event objects | attempt-total evidence conformance | durable state/evidence | forensic export |
| Security/redaction | bootstrap filters and explicit conformance gaps | security/event gateway shadow | secret/prompt-injection tests | security/data-governance plane | emergency deny configuration |
| Campaign state | locked JSON ledger + shell control flow | shadow event journal/snapshots | kill/replay/reconciliation suite | durable state service | launch/recover |
| Process/tool supervision | shell/Python supervisor + Cline | runtime health/process observations | Runtime Specification conformance | generic runtime supervisor | emergency watchdog |
| Review stage resume | standalone exact-head bootstrap reviewer | PAF stage/recovery records | stage-resume conformance | controller/runtime | emergency review resume |
| Monitoring | `paf-cline-monitor`, logs, process scope | normalized projections | R4-O reconstruction test | operations plane | low-level diagnostics |
| Cline invocation | hard-coded CLI/profile details | generic invocation shadow | Cline adapter/sandbox tests | PAF runtime adapter | launch-known-good |
| Credentials/daemon | bootstrap preflight and scoped bindings | runtime Credential Binding | isolation/mismatch/redaction tests | security/runtime | key/daemon repair |
| Permissions | Cline command policy/controller checks | portable Permission Profile translation | outer sandbox enforcement tests | policy + runtime | emergency restriction |
| Deterministic validation | controller validation file | Validation Records | reproducible runtime execution | validation service | fallback runner |
| Checkpoint commit | controller | effect intent/receipt shadow | idempotency/recovery tests | controller/effect service | human recovery |
| Completion | agent marker + exact-head review | criterion-level assessment shadow | seeded mismatch tests | Completion Assessment | none |
| Retry/switching | fixed cycle + non-retryable circuit breakers | route-transition recommendations | failure/progress/retry conformance | execution decision service/controller | emergency retry |
| Capability discovery | static profiles | descriptors/probes | catalog snapshot tests | capability service | configuration bootstrap |
| Local models/resources | manual endpoint setup | deployment/resource probes | offline/local route tests | runtime/scheduler | known-good local launcher |
| Retrieval/MCP | framework-native setup | governed tool/MCP records | provenance/isolation/injection tests | retrieval/runtime/security services | disable/recover |
| Planning | fixed Sol plan | Work Proposals in shadow | reviewed mutation tests | hierarchical planner/controller | manual task import |
| Review coordination | fixed Opus/Sonnet loop with enforced budgets | policy-generated review requests | exact-subject bounded review tests | review service/controller | standalone emergency review |
| GUI/TUI/CLI observation | bootstrap monitor | stable read-only API | reconstructed/redacted status after restart | operations plane | low-level CLI |
| Operator controls | controller terminal/human action | audited command records | secure-runtime/policy tests | operations/controller | emergency stop |
| Push/PR effects | controller/GitHub CLI | effect intent/reconciliation | duplicate-effect injection tests | controller, human-authorized | recovery verification |
| Merge/promotion | human | none | intentionally not automated by bootstrap | human/organizational authority | unchanged |
