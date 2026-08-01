# Provider portability and execution profiles

PAF-01 defines generic provider discovery and capability evidence. PAF-02 defines roles, tasks, catalogs, and deterministic model selection. PAF-03A adds **non-executing portable execution-profile and routing contracts**; it does not authenticate, detect local installations, create configuration, probe, launch tasks, or persist a decision.

## Separation of concerns

An execution profile records these separate dimensions:

1. `access_class`: `api-gateway`, `native-account-client`, `local-server`, or `native-local-client`;
2. `funding_class`: `project`, `organization`, `subscription`, `personal-api`, or `local`;
3. `auth_mode`: `native-session`, `environment-token`, `token-helper`, or `none`;
4. client adapter identity, invocation class, and execution family;
5. symbolic provider-profile, native-session, or local-runtime binding;
6. model selector or pinned model reference;
7. privacy, cost, capability, resource, and availability constraints;
8. explicit fallback and escalation policy.

These are declarative identifiers, not a client configuration. A native-account-client binding means the native client owns its externally owned session. The framework never inspects, copies, exports, or persists Codex or Claude account tokens or session material.

Tracked fixtures are fictional and portable. They contain neither endpoints, institutional names, provider model IDs, account IDs, credentials, nor local machine paths. Actual local bindings belong under ignored `.agentic-local/`; runtime reports belong under ignored `agentic-state/`.

## Deterministic routing

`tools/agentic/routing.py` applies role mandatory capabilities and PAF-02 confidence ordering before ranking profiles. It filters access/funding/auth constraints, privacy data class, cost tier, resource availability, readiness, adapter protocol compatibility, and reviewer independence. It orders candidates by explicit policy preference then profile priority and stable identifiers.

A later preference tier is usable only through a declared fallback transition. Funding-boundary transitions are fail-closed. Evaluation mode disables fallback. Decisions record the selected profile, adapter, execution family, classes, binding, model selector/pin, protocol, capability evidence, fallback authorization, rejected alternatives, and resolver/policy/registry versions. An LLM never selects an execution profile.

## Contract validation

The JSON schemas define versioned document shape, required fields, and portable enum vocabulary. Python semantic validators in `tools/agentic/contracts.py` enforce constraints JSON Schema cannot conveniently express: duplicate identifiers, unknown references, class combinations, readiness cross-references, timestamp/redaction requirements, and policy transition integrity. Both layers are tested; schema/validator drift checks compare the canonical class enums with fixture coverage.

Read-only commands are intentionally bounded:

```text
agentctl profiles validate --registry <registry>
agentctl profiles list --registry <registry>
agentctl profiles show --registry <registry> --profile <id>
agentctl readiness show --registry <registry> --report <report>
agentctl routing explain --registry <registry> --policy <policy> --readiness <report> --task <task> --role <role>
```

They validate, inspect, and explain only. They do not authenticate, collect credentials, modify sessions, generate client configuration, call remote models, probe, launch tasks, create worktrees, commit, push, or merge. Input loading fails closed for non-regular or symlinked contract paths.