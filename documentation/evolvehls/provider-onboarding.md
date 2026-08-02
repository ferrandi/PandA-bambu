# Provider configuration and canonical overlays

PAF-04A provides deterministic **declarative** provider configuration. It does
not contact a provider, send a task/inference request, inspect credentials, or
change global client configuration.

```text
provider specification
        ↓
validated non-secret provider configuration
        ↓
approved local profile/runtime overlays
        +
built-in canonical registry/runtime map
        ↓
computed effective canonical view
        ↓
canonical readiness → routing.resolve() → invocation_descriptor()
```

Local overlays do not create a second profile registry. They add a full
canonical profile which references the existing `generic-http` adapter and a
runtime entry which references the composed `(profile_id, adapter_id)` pair.
Routing and descriptors consume the effective canonical documents directly.

## Declarative commands

```bash
agentctl provider preview --spec provider.yaml
agentctl provider apply --spec provider.yaml
agentctl provider show provider-id
agentctl provider list
agentctl provider validate
agentctl provider remove provider-id --dry-run
agentctl provider remove provider-id
```

`preview` performs no writes and no network access. `apply` is idempotent for
the same material configuration. A changed endpoint, protocol, model, or
environment-variable reference fails unless `--replace` is explicitly passed.

`provider add` is deliberately deferred to PAF-04B and returns a classified
not-implemented result. PAF-04B also owns guided setup, dynamic discovery,
protocol detection, recommendations, and network readiness rechecks.

Provider command failures use the following exit codes:

| Exit code | Meaning |
| --- | --- |
| 3 | Validation or other classified general error |
| 9 | Specification parse error or unavailable YAML dependency |
| 10 | Persistence or transaction error |
| 11 | Collision or replacement-required error |
| 12 | Active-reference error |
| 13 | Intentionally deferred or not-implemented behavior |

## Specification formats

`--spec` accepts `.json`, `.yaml`, and `.yml`. JSON works with the standard
Python installation. YAML parsing uses a safe loader, rejects duplicate keys,
and requires a top-level mapping; it requires PyYAML. The repository validation
environment already installs the pinned supported version, and a normal
development environment can install it with:

```bash
python3 -m pip install PyYAML==6.0.2
```

JSON and YAML normalize to the same internal document.

```yaml
schema: evolvehls.agentic.provider-onboarding-spec
schema_version: "1.0"
provider_id: local-gateway
endpoint:
  origin: http://127.0.0.1:8080/v1
  protocol: openai-compatible
authentication:
  mode: none
model: coding-model
roles:
  - implementation
```

Remote endpoints must use HTTPS. Loopback HTTP is permitted. The configuration
contains no credential value: authentication can only name an external
environment-variable reference.

## Local state and composition

PAF-04A writes only ignored, restrictive-permission local state:

```text
.agentic-local/providers/<provider-id>.json
.agentic-local/overlays/profiles/<provider-id>.json
.agentic-local/overlays/runtime/<provider-id>.json
agentic-state/provider-setup/<provider-id>.json
```

Effective registry and runtime-map documents are computed, not materialized.
They carry built-in identities and versions; composition provenance is a
separate sidecar with source digests. Overlay ordering is deterministic.
Equivalent duplicates are idempotent; incompatible IDs and semantic collisions
fail closed. Removing a provider removes only its owned local configuration and
overlays, never built-in registry/runtime documents or credentials.

All writes/replacements/removals are transactional through `local_state`.
Replacement backups are restrictive local artifacts. Removal refuses active
readiness references unless `--force` is explicitly provided.

PAF-04A does not execute models or coding agents. Actual task execution remains
PAF-05.

## Deferred lifecycle and normalization follow-ups

PAF-04A removal protects active canonical readiness references only. PAF-04B
and PAF-05 should decide whether materialized invocation descriptors also need
reference protection before provider removal.

Endpoint normalization preserves explicitly written ports today. A future UX
follow-up should decide whether HTTPS `:443` and HTTP `:80` normalize to their
implicit-port forms.
