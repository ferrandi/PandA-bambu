# Turnkey provider and execution-profile design

PAF-03B is a **foundation** for portable client detection, safe local
configuration metadata, canonical PAF-03A routing integration, and future
PAF-05 invocation descriptors. It does not launch coding tasks, plan or
implement repository changes, create worktrees, run coding-task validation,
repair, commit, push, create PRs, or choose an automatic fallback.

## Canonical PAF-03A contract boundary

PAF-03A remains the sole authority for:

- `evolvehls.agentic.client-adapter`;
- `evolvehls.agentic.profile-registry`;
- `evolvehls.agentic.readiness-report`;
- `evolvehls.agentic.stage-routing-decision`;
- canonical adapter and profile identifiers;
- profile selection, ranking, fallback authorization, and readiness semantics.

Execution profiles are portable tracked declarations, not runtime
configuration. The canonical registry owns adapter identity, execution family,
access/funding/authentication class, provider-or-runtime binding, model binding,
protocol, capabilities, privacy, cost, resources, availability, and priority.

The deterministic PAF-03A resolver selects a canonical profile after filtering
role capabilities, policy, privacy, cost, resources, readiness, and adapter
compatibility. PAF-03B never implements a second routing algorithm and never
creates an alternate profile identifier space.

## Client runtime supplements

PAF-03B adds `evolvehls.agentic.client-runtime-map`, a versioned runtime
supplement rather than a profile registry. Each entry has a runtime-entry ID and
references one existing canonical `(profile_id, adapter_id)` pair. It maps that
pair to an internal bounded client-template key and execution path.

The runtime map may describe client-specific invocation mode, protocol
compatibility, environment-variable references, non-secret local configuration
metadata, and future PAF-05 handoff placeholders. It must not redefine
canonical access/funding/authentication classes, priority, readiness, profile
selection, bindings, models, or capabilities. Unknown, mismatched, or duplicate
canonical references fail validation.

The descriptor flow is:

```text
PAF-01 discovery/evidence
        ↓
PAF-02 task and role selection
        ↓
PAF-03A registry, canonical readiness, routing.resolve() decision
        ↓
PAF-03B client-runtime-map entry
        ↓
PAF-03B future PAF-05 invocation descriptor
```

Descriptors preserve the selected canonical `profile_id`, `adapter_id`,
runtime-map and runtime-entry IDs, execution path, canonical readiness
observation, and the actual routing decision provenance tuple. PAF-03A does
not define a routing-decision document ID, so no synthetic one is created.

## Detection, readiness, and timestamps

Detection is bounded to executable presence, bounded `--version` output, and a
conservative static version-capability map. It never reads native session
storage, browser state, token caches, credential stores, OAuth databases,
cookies, account identities, or global client configuration. Unsupported client
versions fail closed.

Static version maps are template metadata, not per-installation capability
proof. Native-account executable presence does not verify a native login; such
paths remain `unknown` without canonical readiness evidence.

PAF-03B translates its bounded observations into the one canonical
`evolvehls.agentic.readiness-report` shape. Files under
`agentic-state/readiness/<registry_id>.json` must always validate against that
canonical contract. PAF-03B never persists a portable readiness document.

Every setup/config operation captures one actual timezone-aware UTC observation
timestamp. Tests may inject a clock or timestamp; production paths never use a
fixed observation time. A readiness age is evaluated only when an explicit
maximum-age policy is supplied; PAF-03A itself does not prescribe a TTL.

## Setup and local ownership

The local ignored layout is:

```text
.agentic-local/
  generated/
agentic-state/
  readiness/
  setup/
  catalogs/
  probes/
```

`agentctl setup --spec <local-spec>` accepts canonical registry, runtime-map,
and canonical routing-decision inputs without secret values. It writes a
canonical readiness report, one selected-profile descriptor, and a receipt.
Writes are confined to approved ignored roots; identifiers are validated before
path construction; traversal, symlink, and non-regular targets are rejected;
writes are restrictive and atomic; overwrite is refused unless explicitly
authorized.

Probe authorization is explicit. The receipt distinguishes `not-requested` from
`authorized-not-performed` and records `probes_performed: false`. This
foundation does not create a second probe implementation or imply that a probe
ran.

## Commands and PAF-05 boundary

```text
agentctl setup --spec local-setup.json [--dry-run]
agentctl doctor
agentctl adapters detect
agentctl adapters list
agentctl adapters show --adapter codex
agentctl profiles list|show|validate
agentctl readiness show
agentctl routing explain
agentctl config preview|generate|validate --spec local-setup.json
```

Errors are classified and redacted. Generated descriptors deliberately retain
the following future PAF-05 placeholders:

- input handoff;
- working-directory behavior;
- result collection;
- client-specific launch execution.

PAF-03B does not execute Codex, Claude Code, Cline, LiteLLM, direct HTTP, or
any other client. It does not acquire credentials, inspect authentication
sessions, alter client-global configuration, scrape native-login status, or
perform result collection.