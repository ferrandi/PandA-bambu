# Turnkey provider and execution-profile design

The target user interface is:

```text
agentctl setup
agentctl doctor
agentctl profiles list
agentctl routing explain
agentctl run
```

PAF-03A implements only the bounded profile inspection and routing explanation portion. `setup`, detection, generated adapters, and `run` remain PAF-03B and later work.

## PAF-03A routing boundary

Execution profiles are portable tracked declarations, not a runtime configuration. They reference a client adapter and a symbolic provider-profile, native-session, or local-runtime binding. Local binding values, endpoints, executable paths, credentials, and discovered runtime facts remain ignored under `.agentic-local/` and `agentic-state/`.

The routing decision is deterministic and versioned. It reuses PAF-01 protocols and PAF-02 capability confidence/role requirements. It selects among profiles only after filtering mandatory role capabilities, allowed access/funding/authentication classes, privacy, cost, resources, readiness, and adapter compatibility. It records every rejected alternative.

A later preference tier requires an explicit fallback transition. Transitions across funding classes fail closed unless declared. Evaluation mode pins behavior and does not use a fallback. Independent review may require a different adapter and/or execution family from the prior implementer decision.

## Native-session safety

Codex-like and Claude-Code-like native account clients own their externally owned account sessions. PAF-03A never authenticates, reads, copies, exports, stores, or alters these sessions or their tokens. Credential references are identifiers only; credential values are never valid contract fields.

## Contract responsibilities

JSON Schema provides the versioned structural format. Python validators provide semantic checks: duplicate identifiers, unknown adapter/profile references, invalid access/funding/auth combinations, readiness cross-references, redaction, and fallback graph integrity. Tests exercise both responsibilities and fixture coverage.

## Bounded commands

```text
agentctl profiles validate
agentctl profiles list
agentctl profiles show
agentctl routing explain
agentctl readiness show
```

These commands only inspect supplied documents. They do not authenticate, collect secrets, create a native-client configuration, call a paid/remote model, probe automatically, launch a task, create a worktree, commit, push, or merge.