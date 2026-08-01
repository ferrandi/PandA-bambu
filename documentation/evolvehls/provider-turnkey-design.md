# Turnkey provider setup design

The target user interface is:

```bash
python3 tools/agentic/agentctl.py provider add --adapter auto --profile work
```

Only a gateway base URL and an API key entered through hidden input should be required. PAF-01 defines the contracts and generic interfaces needed by that command; it does not yet collect or persist credentials, generate client configuration, or launch work.

## Pipeline boundary

The eventual controller performs these deterministic stages:

1. auto-detect a generic or approved provider adapter;
2. try ordered model discovery methods;
3. ask for one authorized model ID only if listing is unavailable;
4. normalize metadata into a local catalog;
5. apply ignored local policy overlays;
6. probe only eligible role candidates with unknown or untrusted evidence;
7. resolve client + provider + model + protocol + effort + fallback chain;
8. generate supported client configuration;
9. emit a redacted readiness report.

Discovery and capability are separate. Listing a model never makes it eligible for autonomous work. Every capability carries status, provenance, and confidence: `declared`, `inferred`, `observed`, `historically-validated`, or `unknown`.

Generic discovery may try `GET /v1/models`, `GET /v1/model/info`, an adapter-provided list surface, or an imported local catalog. Missing, rejected, and malformed surfaces fall through without assuming model-name patterns. The generic interface returns `requires_model_id` when manual authorized-model fallback is needed.

## Staged probing and cache

Setup lists all models, applies eligibility rules, probes likely role candidates, and defers other probes until a capability query needs them. Probe records cover basic text, streaming, tools, structured output, context-limit behavior, usage, and embeddings where applicable. Successful observations are cached under `agentic-state/probes/` with timestamps and a profile-controlled TTL. Cache records are redacted and local-only.

## Resolution and modes

The resolver is deterministic and versioned; no LLM chooses a model. It filters mandatory capabilities and policies before ranking, explains the selected plan, records every rejected candidate and reason, preserves overrides, and never silently falls back. Implementation and independent review may select different clients.

Development mode may refresh stale evidence and use approved, explained fallbacks. Evaluation pins catalog snapshot, execution plan, provider protocol, client version, effort, task version, context hash, base revision, and budgets; it neither refreshes nor falls back. Ablation varies only declared dimensions.

The reserved future command surface is:

```text
agentctl catalog sync --profile work
agentctl models query --catalog <catalog-file> --role agentic/roles/implementer.yaml
agentctl models select --catalog <catalog-file> --role agentic/roles/implementer.yaml
agentctl models explain --latest
agentctl run --task <task-file> --select-execution-plan
```

## Configuration separation

Tracked files contain only generic schemas, adapters, probes, policies, and fictional examples. User-local configuration is ignored under:

- `.agentic-local/providers/`
- `.agentic-local/catalogs/`
- `.agentic-local/overlays/`
- `.agentic-local/credentials/`

Runtime discovery, probe, and selection records are ignored under `agentic-state/`. No organizational endpoint, credential, provider-specific model identifier, or model-name heuristic belongs in the generic core.

## PAF-02 portable task contracts

PAF-02 adds versioned, client-neutral role, task, result, policy-overlay, and
selection contracts under `agentic/schemas/`. JSON Schemas define the portable
structural contract. Python validators enforce both those structural checks and
semantic invariants that JSON Schema cannot conveniently express, including
uniqueness by `model_id` and cross-field relationships. Tracked roles and
fixtures contain only fictional data. Task and result contracts are declarative
records; they do not configure a client or launch work. Validation evidence
remains missing or `null` when it was not collected.

`agentctl catalog sync` accepts an explicitly supplied imported catalog and an
optional local overlay, then writes a deterministic normalized snapshot.
Overlays target exact discovered model identifiers, are fail-closed for unknown
models or invalid execution units, and record policy-overlay provenance.
Discovery alone cannot make a candidate eligible.

`agentctl models query` reads a catalog and role without probing. It reports
eligibility, unavailable mandatory capabilities, and capabilities that need
stronger evidence. `agentctl models select` invokes the versioned deterministic
resolver and persists a selection under ignored `agentic-state/selections/`.
`agentctl models explain --latest` reads only a valid persisted selection.

PAF-02 has no `agentctl run`, no credential collection, no generated client
configuration, and no automatic capability probe. Those behaviors remain
reserved for dependent PAF work.
