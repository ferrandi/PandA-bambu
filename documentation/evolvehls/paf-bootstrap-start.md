# Start the PAF Bootstrap Coder

## Recommendation

Use the corrected Cline bootstrap for BS-010 onward. Do not switch frameworks yet.

Role split:

```text
Sol → plan
Terra → implement and run focused checks
Controller → validate, enforce scope, checkpoint and create draft PR
Opus → bounded review plan
Sonnet → exact-head independent verification
Human → review and merge
```

## First task

`BS-010 — Wire identity and canonical form`

Do not include runtime adapters, planning, MCP, GUI or EDA behavior in BS-010.

## Workflow

```text
generate task
→ inspect metadata
→ authorize
→ run bounded cycle
→ monitor
→ exact-head review
→ human merge
→ reconcile
→ generate next eligible task
```

## Framework introduction gates

- OpenHands: after generic runtime and Cline adapter.
- OpenAI Agents SDK or Microsoft Agent Framework: after Message/Context/policy/event contracts.
- Temporal/Dapr/LangGraph/Argo: after DurableExecutionBackend SPI.
- Claude Code: after Cline conforms to the common runtime contract.

## Cutover rule

Transfer one bootstrap responsibility only after PAF passes shadow comparison, failure injection, recovery, monitoring and rollback, followed by human approval.
