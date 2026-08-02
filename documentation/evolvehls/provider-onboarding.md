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

## Guided PAF-04B onboarding and bounded discovery

`agentctl provider add` is the interactive, non-secret onboarding path. It
requires a TTY; scripts must use declarative `provider preview` and `provider
apply`. The wizard collects a display name, deterministic provider ID, endpoint,
authentication mode, only an environment-variable *name*, endpoint protocol,
an explicitly user-confirmed canonical execution protocol, models, and role
assignments for `planning`, `implementation`, and `review`.

The wizard performs a PAF-04A `apply(..., dry_run=True)` preview before asking
for final confirmation. It then invokes the same PAF-04A transactional apply
path. Cancellation, EOF, and confirmation refusal leave no persistent state.
No secret value is stored in configuration, overlays, receipts, evidence,
identities, digests, or diagnostics.

Manual onboarding is always available. It records `manual` discovery evidence
and explicitly states that neither model listing nor model execution occurred.

```bash
agentctl provider add
agentctl provider discover <provider-id>
agentctl provider discover <provider-id> --allow-private-network
```

`provider discover` is explicitly network-authorized and inspect-only. It
supports only OpenAI-compatible `GET` model-list paths; it does not send a
prompt, completion, chat, response, embedding, POST body, or any other
inference request. It does not alter configured models or role assignments.

Model-list evidence proves only endpoint reachability, authentication acceptance
for that list request, and model visibility. It does **not** establish execution
readiness. Canonical execution readiness remains `unknown` until a future
PAF-05 execution probe succeeds. A listing that omits an assigned model may be
reported as unavailable by later lifecycle policy, but discovery never creates a
second readiness document or silently changes assignments.

PAF-04B normalizes OpenAI-compatible model-list responses with bounded model
counts and fields, bounded response size and JSON depth, strict JSON decoding,
and deterministic lexical ordering/truncation. Capabilities are not inferred
from model names. The initial recommendation is deliberately conservative: when
no endpoint-reported distinction exists it recommends the same deterministic
model for all roles and explains why. A user-confirmed execution protocol is
recorded with `declared` confidence; a list response never confirms an inference
interface.

The discovery transport is direct standard-library socket/SSL HTTP. It validates
resolved addresses before connecting and connects to that validated address,
retaining the original hostname for HTTPS SNI/certificate verification and
`Host`. It does not inherit proxy settings. Supported schemes are HTTPS and
loopback HTTP only. URL credentials, query strings, fragments, remote HTTP,
link-local/metadata addresses, multicast, unspecified, reserved addresses,
mixed allowed/prohibited DNS answers, cross-origin redirects, and HTTPS-to-HTTP
redirects are rejected. Private HTTPS addresses require explicit
`--allow-private-network` opt-in. Authorization is read from an environment
variable only during the authorized request and is never persisted.

PAF-04B also owns guided setup, dynamic discovery, protocol detection, and
recommendations. PAF-05 owns actual model execution.

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
