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
agentctl models select --role implementer --objective balanced-quality-cost
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
