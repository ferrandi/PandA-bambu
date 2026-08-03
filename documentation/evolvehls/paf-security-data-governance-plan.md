# PAF Security, Data Governance, and Trust-Boundary Plan

Status: permanent cross-cutting plan  
Version: 2026-08-03 revision 3

## 1. Principle

Autonomy increases the impact of an incorrect permission, exposed credential, poisoned context, or ambiguous external effect. Security therefore begins before the first shadow-runtime event is persisted; it is not deferred until the generic executor is complete.

PAF applies defense in depth:

```text
Campaign Charter and policy
  ↓
data classification and context filtering
  ↓
route eligibility and credential binding
  ↓
outer process/filesystem/network sandbox
  ↓
framework-native permissions
  ↓
MCP/tool-specific grants
  ↓
receipts, evidence, monitoring, and recovery
```

No inner framework permission may weaken an outer PAF boundary.

## 2. Trust boundaries

PAF distinguishes at least:

- accountable human and organizational authority;
- PAF controller and policy engine;
- runtime supervisor;
- framework adapter;
- model/provider service;
- local model runtime;
- workspace and source repository;
- MCP server and individual tools;
- retrieval indexes and source systems;
- external publication/effect systems;
- monitoring, logs, exports, and GUI clients.

Every crossing records the principal, data classification, permission decision, route, operation, and receipt or explicit ambiguity.

## 3. Credential handling

Credentials are scoped capabilities, not authority.

Required rules:

- inject only credentials required by the resolved route;
- do not inherit the controller's entire environment by default;
- never place raw secrets in prompts, event payloads, manifests, or GUI projections;
- redact exact known secret values before persistence;
- record credential binding identity and scope, not secret material;
- detect stale Cline daemon processes that were started under another or missing key;
- revoke or rotate bindings without rewriting historical evidence;
- prohibit a fallback from silently changing account, funding class, data jurisdiction, or privilege.

## 4. Data classification and context release

Every Artifact, Context Package, Retrieval Record, prompt segment, log, and evidence export carries a classification and visibility policy.

A Context Release Decision determines:

- which sources may be included;
- which transformations are permitted;
- whether data may leave the local environment;
- which model/provider/framework routes are eligible;
- whether retention or export is allowed;
- required redaction and minimization.

A route is ineligible when it cannot prove the required data boundary.

## 5. Prompt injection and untrusted content

Retrieved documents, repository text, issue content, web pages, and MCP responses are untrusted data unless explicitly promoted by policy.

PAF must:

- label source and trust level;
- separate instructions from retrieved content;
- prevent retrieved text from granting tools or authority;
- constrain tool selection through controller policy rather than model text;
- retain the exact source and transformation lineage;
- detect attempts to exfiltrate secrets or override policy;
- require independent review for security-boundary changes.

## 6. Event, log, and monitoring security

Observability is a potential exfiltration surface.

Before durable persistence, the event pipeline must support:

- structured redaction;
- exact-secret replacement;
- field-level classification;
- payload truncation and content-addressed externalization;
- duplicate/accumulated-output suppression;
- retention and deletion policy;
- encryption at rest when required;
- role-based query access;
- tamper-evident event segment digests;
- export policy and audit receipts.

Hidden chain-of-thought is neither required nor collected. PAF stores structured decisions, evidence, alternatives, uncertainty, and policy basis.

## 7. Filesystem and process safety

The authoritative executor uses:

- dedicated worktrees;
- approved writable-path allowlists;
- read-only source areas where feasible;
- ephemeral homes and temporary directories;
- non-root execution;
- process groups and resource limits;
- no inherited SSH agent, cloud credentials, Docker socket, or unrelated tokens;
- deny-by-default network egress with explicit provider and MCP exceptions;
- preservation rather than destructive cleanup after failure.

The bootstrap cannot yet provide every outer-sandbox guarantee. It must report those gaps and enforce generated-task allowed paths before staging or committing.

## 8. Supply-chain integrity

Record and verify:

- framework and adapter versions;
- executable/container digests;
- model weights, tokenizer, quantization, and templates;
- MCP server and tool digests;
- package lockfiles and dependency sources;
- build/test toolchain identity;
- generated artifacts and transformation lineage.

Unpinned or unverifiable executable infrastructure lowers conformance and may make a route ineligible.

## 9. External effects

Effects are classified as:

```text
read-only
reversible-local
reversible-external
protected
irreversible
```

Protected and irreversible effects require explicit authority. Every mutating effect uses intent-before-action, idempotency where possible, receipt collection, reconciliation, and `effect-ambiguous` when success cannot be established.

## 10. Security acceptance gate

Before PAF becomes the authoritative executor, prove:

1. an unauthorized path cannot be modified and committed;
2. an unrelated inherited credential is unavailable to the agent;
3. retrieved prompt-injection text cannot expand tool or authority grants;
4. secrets do not appear in normalized events, logs, evidence, or monitoring output;
5. a denied network destination is unreachable;
6. a malicious or overbroad MCP tool is rejected;
7. supply-chain identities are recorded;
8. interrupted protected effects reconcile without blind repetition;
9. operator actions are authenticated, authorized, and audited;
10. security policy survives controller restart and replay.
