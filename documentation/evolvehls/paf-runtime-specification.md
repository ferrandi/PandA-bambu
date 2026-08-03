# PAF Runtime Specification

Status: normative bootstrap target  
Version: 2026-08-03 revision 3

## 1. Purpose

This specification defines the stable runtime contract that every PAF execution environment must satisfy. Cline is the first conforming runtime adapter. Claude Code, local coding agents, API-only model routes, MCP-backed tools, and eventually the self-hosted PAF runtime must conform to the same lifecycle, event, supervision, permission, evidence-component, monitoring, and recovery rules.

The runtime is separate from:

- hierarchical planning;
- model capability ranking;
- policy and authority;
- Work Graph ownership;
- semantic completion assessment;
- final Evidence Package ownership;
- publication.

A runtime executes an already-authorized Attempt Contract. It does not invent objectives, grant authority, declare a Work Item complete, mutate the Work Graph, or promote a runtime release.

## 2. Runtime boundary

```text
Campaign Controller
  ├── resolved Execution Route
  ├── Context Package and Context Release Decision
  ├── Permission Profile
  ├── Resource Reservation
  ├── Runtime and Review Policy
  └── Attempt Contract
           ↓
PAF Runtime
  ├── framework adapter
  ├── process/tool supervisor
  ├── sandbox and scoped credential injection
  ├── normalized event stream
  ├── health and progress observations
  ├── cancellation, timeout, and stage resume
  └── evidence components and receipts
           ↓
Cline / Claude Code / local agent / API / MCP / tools
```

The controller owns the decision to dispatch, retry, switch routes, checkpoint, review, publish, pause, or stop. The Evidence service owns the final immutable Evidence Package Manifest. The runtime owns faithful, bounded, observable execution of one attempt.

## 3. Normative runtime interfaces

A conforming runtime exposes:

```text
probe()
describe_capabilities()
prepare_attempt()
start_attempt()
stream_events()
inspect_health()
inspect_progress()
cancel_attempt()
terminate_attempt()
collect_result()
collect_receipts()
collect_evidence_components()
cleanup_attempt()
recover_attempt()
resume_stage()
```

Every mutating operation has:

- stable operation ID;
- idempotency key where possible;
- expected-state revision or fencing token;
- typed success and failure outcomes;
- authority, security, and permission references;
- event and evidence obligations;
- explicit ambiguous-effect behavior.

## 4. Attempt lifecycle

```text
CREATED
→ PREPARING
→ READY
→ DISPATCH_INTENT_RECORDED
→ STARTING
→ RUNNING
→ FINALIZING
→ terminal
```

Observable substates include:

```text
waiting-for-resource
waiting-for-provider
reasoning
requesting-tool
tool-running
validating
checkpointing
reviewing
recovering
```

Terminal runtime outcomes are:

```text
process-complete
process-failed
refused
cancelled
timed-out
runtime-unavailable
credential-or-budget-block
evidence-incomplete
effect-ambiguous
```

These are not Work Item completion outcomes. Completion is assessed separately from acceptance criteria and evidence.

## 5. Tool-call lifecycle

Every tool or MCP invocation follows:

```text
TOOL_REQUESTED
→ TOOL_AUTHORIZED | TOOL_DENIED
→ TOOL_STARTED
→ TOOL_RESULT | TOOL_FAILED | TOOL_TIMED_OUT |
  TOOL_CANCELLED | TOOL_EFFECT_AMBIGUOUS
```

The invocation records tool identity, arguments digest, effect class, permission decision, start/end time, process or remote request identity, output/evidence references, and receipt.

## 6. Normalized event model

Every runtime emits ordered, timestamped events with:

- Campaign, Work Item, Step, Attempt, Stage, and Operation IDs;
- route and adapter identity;
- source sequence number and controller ingestion sequence;
- event type and schema revision;
- source monotonic and wall-clock time where available;
- causal parent, correlation, and idempotency IDs;
- data classification and redaction status;
- payload digest and evidence reference.

Required event families include:

```text
attempt.*
stage.*
framework.*
model.*
message.*
tool.*
process.*
filesystem.*
network.*
resource.*
budget.*
validation.*
review.*
health.*
progress.*
recovery.*
evidence.*
security.*
operator.*
```

Raw native framework output remains evidence when policy permits. Controller logic and operator interfaces consume normalized events.

## 7. Event volume, backpressure, and retention

The runtime/event pipeline must:

- suppress repeated accumulated-output snapshots;
- chunk and content-address large payloads;
- preserve bounded previews in projections;
- apply queue and disk quotas;
- report dropped or truncated data explicitly;
- support consumer cursors and replay;
- prevent a slow GUI from blocking execution;
- attach retention and export policy;
- produce tamper-evident segment digests.

Log growth is not treated as semantic progress.

## 8. Supervision requirements

A runtime must:

- use a dedicated process group or equivalent isolation;
- record executable, command, environment-name, configuration, and working-directory digests;
- supervise total time and inactivity independently;
- enforce role-specific tool-call, iteration, duplicate-action, and budget limits;
- distinguish model/provider waiting from tool execution;
- track descendant process state, including zombies and unreaped children;
- support graceful cancellation followed by bounded forced termination;
- retain partial logs and worktree state after failure;
- never perform destructive cleanup automatically;
- expose heartbeat, last-event time, last-material-progress time, and current operation;
- support stage-only resume against an immutable checkpoint.

Framework-native timeouts are defense in depth. The PAF supervisor is authoritative.

## 9. Progress and stall assessment

Progress is not equivalent to output activity. Runtime observations include:

- unique tool actions;
- acceptance evidence acquired;
- criteria resolved;
- new artifacts or valid worktree changes;
- failure-signature changes;
- provider and tool latency;
- repeated commands and equivalent action signatures;
- repeated prompts or accumulated-output echoes;
- descendant-process health;
- budget consumed since material progress.

Stall classes include:

```text
provider-wait
tool-wait
tool-supervisor-stall
repeated-action-loop
no-semantic-progress
resource-starvation
credential-or-budget-block
unknown-runtime-stall
```

The runtime reports observations and enforced policy-limit events. The controller or deterministic execution policy decides whether to wait, cancel, retry, switch, or escalate.

## 10. Provider and budget circuit breakers

The runtime classifies at least:

- missing or mismatched credential;
- authentication/authorization failure;
- exhausted project/key budget;
- hard quota exhaustion;
- transient rate limit;
- provider outage;
- request timeout;
- model unavailable or mismatched.

Missing-key, authentication, exhausted-budget, hard-quota, and policy-denial failures are non-retryable until the relevant binding or policy changes. They must not consume automatic retry attempts.

## 11. Permissions and sandboxing

The runtime translates a portable Permission Profile into:

1. authoritative OS/runtime sandbox controls;
2. framework-native permissions;
3. MCP/tool grants;
4. scoped credential exposure;
5. data-release and network-egress policy.

It reports each requested boundary as:

```text
enforced-by-outer-sandbox
enforced-natively
enforced-by-both
partially-enforced
unenforceable
```

A route is ineligible when a required boundary is unenforceable.

## 12. Credential and environment isolation

A runtime does not inherit the controller's full environment by default. It constructs an allowlisted child environment containing only runtime necessities and resolved credential bindings.

It must:

- omit unrelated cloud, GitHub, SSH-agent, container-socket, and user tokens;
- redact exact known secret values before persistence;
- detect stale daemon/session processes with missing or mismatched credentials;
- record credential binding identity and scope without secret material;
- prevent tool output or retrieved text from exposing secrets through monitoring or evidence.

## 13. Tools and MCP

MCP servers and tools are execution resources, not trusted extensions.

For every enabled server or tool, record:

- identity, version, executable or endpoint digest;
- transport;
- allowed operations and effect class;
- filesystem, network, credential, and data boundaries;
- invocation request and receipt;
- timeout, cancellation, and ambiguous-effect semantics.

Local MCP servers execute inside the same sandbox or a stricter child sandbox. Tool output is untrusted content until classified and released into context.

## 14. Local-model runtime requirements

A local-model attempt identifies:

- weights, tokenizer, quantization, and prompt template;
- inference runtime and build/container digest;
- serving configuration and endpoint;
- hardware placement and reservation;
- context and generation limits;
- warm/cold state;
- observed memory, latency, and throughput;
- deployment health.

Local-specific failures such as OOM, context overflow, endpoint cold start, runtime crash, and resource contention remain distinct from semantic model failure.

## 15. Review runtime policy

Independent review is an execution role with stronger default bounds:

- exact immutable review subject;
- read-only detached workspace;
- controller-supplied deterministic validation evidence;
- one command per tool call by default;
- enforced tool-call, iteration, duplicate-action, wall-clock, and cost budgets;
- no broad repository exploration without criterion linkage;
- no publication or mutation effects;
- explicit finding and verdict schema;
- stage-only resume without rerunning implementation.

Prompt instructions are not enforcement. The runtime supervisor terminates policy-violating review attempts and records the reason.

## 16. Recovery

Recovery is deterministic where facts are mechanically observable.

A recovering runtime must:

1. acquire a valid controller/session lease;
2. verify campaign, attempt, workspace, route, and process identities;
3. determine whether a matching process survives;
4. inspect durable output and terminal events;
5. reconcile tool and external-effect receipts;
6. preserve dirty worktrees and partial evidence;
7. emit a Recovery Decision;
8. reattach, resume a stage, finalize, cancel, retry, or mark ambiguity.

No hidden framework conversation is required for recovery. Portable Context Packages, exact checkpoints, and durable events are authoritative.

## 17. Monitoring API

The runtime exposes read-only projections suitable for CLI, TUI, GUI, and alerts:

```text
get_campaign_status()
get_attempt_status()
get_current_operation()
get_timeline()
get_health()
get_progress()
get_budget_usage()
get_resource_usage()
stream_normalized_events()
get_logs()
get_evidence_refs()
```

State-changing operator actions use separate authority-bearing controller interfaces and become enabled only after the secure runtime gate:

```text
pause_campaign()
resume_campaign()
cancel_attempt()
request_retry()
approve_transition()
```

Every operator action is journaled and policy-evaluated.

## 18. Evidence ownership

The runtime produces Results, process/tool receipts, native logs, normalized events, resource observations, and evidence components. The Evidence service verifies references and finalizes the immutable Attempt Evidence Package Manifest.

A runtime cannot mark its own incomplete evidence as complete. Finalization failure yields `evidence-incomplete` or `unverifiable`.

## 19. Conformance ladder

```text
L0 identified runtime
L1 bounded invocation and terminal result
L2 normalized, redacted events and evidence components
L3 portable context, checkpoint, and stage recovery
L4 cancellation, sandbox, tools, credentials, and effect reconciliation
L5 full monitoring, health, budgets, retry, and campaign-runtime conformance
```

A route may operate below L5 only when the Campaign Charter accepts the reduced guarantees.

## 20. Bootstrap cutover

The shell bootstrap initially implements the runtime contract externally. Responsibilities move to PAF only after conformance gates pass:

```text
bootstrap supervision and monitor
→ shadow normalized events and security filters
→ durable state and recovery
→ PAF generic executor
→ Cline adapter
→ additional adapters
→ self-hosted runtime
```

The bootstrap remains a known-good recovery and review-resume path after normal execution moves to PAF.
