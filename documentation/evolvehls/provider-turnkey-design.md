# Turnkey provider and execution-profile design

PAF-03B configures and validates execution units. **It does not launch coding
tasks**, plan or implement repository changes, create worktrees, run coding-task
validation, repair, commit, push, create PRs, or choose an automatic fallback.

## Client adapters and execution paths

A client is not an authentication mode, provider, protocol, or funding class.
A versioned adapter exposes one or more independently measurable paths:

```text
Codex
  native-account      native client session / subscription
  configured-api      API or compatible gateway

Claude Code
  native-account      native client session / subscription
  configured-api      API or compatible gateway
```

A failed or unavailable path never disables a sibling path. Direct HTTP against
the same provider is separately represented from Codex, Claude Code, or Cline
using that provider.

Supported adapter templates are:

| Adapter | Execution paths |
| --- | --- |
| Claude Code | native account; configured Anthropic-compatible API/gateway |
| Codex | native ChatGPT account; configured OpenAI-compatible API/gateway |
| Cline | configured API; local compatible server; separate Plan/Act models when the detected configuration surface supports them |
| Direct HTTP | OpenAI Responses/Chat Completions; Anthropic Messages |
| Local compatible runtime | OpenAI-compatible or Anthropic-compatible local server |
| Future native local client | extensible `native-local-cli` execution-path contract |

A LiteLLM gateway is not assumed to implement every protocol. A profile binds
one protocol explicitly and the path validator rejects incompatible protocol,
access, funding, and authentication combinations.

## Funding and authentication

The generic framework uses only `project`, `organization`, `subscription`,
`personal-api`, and `local`. Client/provider names do not change these
classifications.

Native account paths use `native-session`. The native client owns login state.
PAF never reads, exports, copies, decodes, writes, logs, or persists browser
state, credential stores, cookies, OAuth material, session databases, token
caches, account names, emails, organization IDs, or subscription details. It
does not log users in or out. Executable presence does not prove a native login.

Configured paths retain credential references only: environment variable names
or argv token-helper references. Secret values are not stored in setup state,
generated descriptors, reports, fixtures, command arguments, or diagnostics.
Helpers are invoked only with `shell=False` and never during dry-run.

## Detection and readiness

Detection is bounded and deterministic:

1. executable presence;
2. bounded `--version` output;
3. conservative version-capability map;
4. documented bounded status surface when added to a template;
5. explicitly approved non-secret project-local configuration;
6. unknown/request-confirmation if safe evidence is unavailable.

Readiness states are `available`, `authenticated-or-ready`,
`configuration-required`, `unsupported-version`,
`execution-path-unsupported`, `unavailable`, and `unknown`.

All free-form readiness and diagnostic text passes through the centralized
sanitizer. It removes credential-shaped strings, authorization/cookie/session
material, URLs and query credentials, account identifiers, environment values,
and raw provider/subprocess bodies. `redacted: true` is metadata only; it is not
accepted as proof that content is safe. Useful reason classifications survive:
authentication failure, configuration invalid, protocol unsupported, transport
failure, timeout, client unavailable, version unsupported, and unknown.

## Setup and local ownership

The local ignored layout is:

```text
.agentic-local/
  adapters/ providers/ profiles/ bindings/ generated/ setup/ backups/
agentic-state/
  readiness/ catalogs/ probes/ setup/
```

Tracked files remain generic and fictional. The local trees contain real
deployment references only. Framework-owned ignored state is distinct from
generated repository-local client configuration and external user-owned global
or native-client configuration. PAF never modifies unrelated global client,
shell, browser, OAuth, or credential settings.

`agentctl setup --spec <local-spec>` accepts a validated noninteractive
specification containing references but no secret values. It is resumable and
idempotent: unchanged repeated input creates no changes. `--dry-run` performs
no writes, probes, token-helper calls, or network work.

Safe writes remain under the two approved ignored roots, validate identifiers
before path construction, reject traversal/symlinks/non-regular targets, use
restrictive permissions and atomic replacement, refuse overwrite by default,
and create a backup only for explicit authorized replacement.

## Commands

PAF-03B exposes bounded setup/inspection operations:

```text
agentctl setup --spec local-setup.json [--dry-run]
agentctl doctor
agentctl adapters detect
agentctl adapters list
agentctl adapters show --adapter codex
agentctl profiles list|show|validate
agentctl readiness show
agentctl config preview|generate|validate --spec local-setup.json
agentctl routing explain
```

Machine-readable JSON and deterministic ordering are provided by the CLI
output. Errors use classified, redacted failures. Existing PAF-01 through
PAF-03A catalog, profile, readiness, and routing inspection commands remain
available.

## Probes, discovery, and provenance

PAF-01 discovery, normalized catalogs, protocol-specific synthetic probes, and
ignored probe caches remain the provider capability source. Probes require
explicit authorization, are bounded, redacted, disabled in dry-run, and never
silently invoke fallback. When authorized catalog discovery is unavailable,
setup requires a manually authorized model reference.

Each generated PAF-05 descriptor records non-secret provenance: adapter and
version, execution path, invocation class/family, access/funding/auth/protocol,
provider/runtime reference, model reference, template/setup version, capability
evidence, readiness, and configuration ownership. Confidential model IDs use a
digest where required.

The descriptor defines the handoff fields PAF-05 will consume—input handoff,
working-directory behavior, result collection, structured-output, resume,
timeout, and cancellation—but PAF-03B does not invoke them.