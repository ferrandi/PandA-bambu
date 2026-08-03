# PAF Bootstrap Operational Invariants

Status: normative for the temporary Cline bootstrap  
Version: 2026-08-03 revision 3

## 1. Purpose

These invariants capture operational lessons from the requirements-closure campaign and prevent the bootstrap from becoming an unsafe parallel implementation of PAF.

## 2. Role and lifecycle invariants

- Sol proposes a task plan; it does not edit.
- Terra edits and runs implementation-stage checks; it does not commit or obtain downstream review.
- The controller validates allowed paths, runs controller-owned deterministic validation, creates the checkpoint, and coordinates exact-head review.
- Opus creates a bounded criterion-linked review plan.
- Sonnet verifies the exact immutable head and returns a verdict.
- Human authority is required for merge and protected scope, budget, credential, data, and publication changes.

The absence of a commit, base-to-head digest, PR, or independent review is never a Terra blocker.

## 3. Generated-task invariants

A generated task is bound to:

- one authorized backlog item and digest;
- one exact base-branch SHA;
- an allowed-path set that is no broader than backlog authorization;
- explicit acceptance criteria and non-goals;
- bounded validation commands;
- a review-assurance level;
- a runtime/tool budget.

A task becomes stale when its exact base changes before execution. It must be regenerated or explicitly superseded; it is not silently rebased.

Generated tasks require an explicit operator authorization before execution. Authorization approves the task contract, not the implementation result.

## 4. Workspace invariants

- A clean base is required for task generation.
- A dirty worktree may be adopted only on the exact task branch through an explicit flag.
- Every modified, deleted, renamed, or untracked path must match the authorized task metadata before staging.
- `git add -A` is permitted only after allowed-path validation.
- The bootstrap never performs automatic `reset --hard`, `git clean`, destructive checkout, or unbounded cleanup.
- Reviewer worktrees are detached at the exact subject and retained when residue or ambiguity exists.

## 5. Runtime and credential invariants

- Both LiteLLM and OpenAI-compatible key variables are checked before a model invocation.
- A stale Cline Hub daemon with missing or mismatched credentials is detected before execution.
- Budget, quota, missing-key, and authentication failures are non-retryable until the binding changes.
- Review stages default to one automatic attempt; their exact checkpoint can be resumed without rerunning planning or implementation.
- Framework output growth is not equivalent to semantic progress.

## 6. Supervision invariants

The bootstrap supervisor enforces, rather than merely prompts for:

- total and inactivity timeouts;
- review tool-call and iteration budgets;
- duplicate-command limits;
- bounded termination of process groups;
- preservation of raw and normalized evidence;
- classification of non-retryable provider errors;
- explicit failure when a supervisor policy budget is exhausted.

## 7. Validation and review invariants

- Long deterministic validation is controller-owned and runs outside the agent's tool budget.
- Validation command, exit status, toolchain context, and bounded output are supplied to reviewers.
- Reviewers do not rerun broad suites without a criterion-specific reason.
- Review actions are one command per tool invocation by default.
- Review can be resumed at Opus or Sonnet against the same exact head.
- An approved verdict is valid only when the primary and, when published, remote head still match the reviewed SHA and the worktrees are clean.

## 8. Publication invariants

- Merge is always human-controlled.
- Draft publication is a configurable bootstrap effect, not a prerequisite for review.
- Push and PR effects are recorded and reconciled; duplicate creation is forbidden.
- Publication never changes the exact review subject silently.

## 9. Campaign-ledger invariants

- Only one state-mutating campaign command holds the ledger lock at a time.
- A failed or interrupted cycle can be recovered from its summary and state directory.
- `mark-complete` is not a routine bypass; any operator override records reason, principal, and time.
- A blocked task with preserved changes can be retried on its exact branch without requiring a clean checkout.
- The campaign cannot advance until the approved exact head is an ancestor of the authoritative base branch.
