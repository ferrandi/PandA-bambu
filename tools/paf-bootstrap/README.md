# PAF Cline Bootstrap Tools — Revision 3

These version-controlled tools build PAF one authorized task at a time while the permanent runtime is being implemented. They are temporary bootstrap infrastructure and the first concrete implementation of the PAF Runtime Specification.

## Commands

- `paf-cline-cycle` — bounded Sol → Terra → controller validation → checkpoint → Opus → Sonnet cycle.
- `paf-cline-next-task` — read-only Sol generator for the next dependency-eligible backlog item.
- `paf-cline-campaign` — locked bootstrap ledger and one-task-at-a-time driver.
- `paf-cline-monitor` — read-only status/watch/events interface.
- `paf-cline-preflight` — key, daemon, profile, Git, and publication preflight.
- `paf-cline-review-resume` — exact-head Opus/Sonnet stage-only review recovery.

## Important lifecycle boundary

Terra prepares the working tree. The controller validates authorized paths, runs long deterministic validation, creates the checkpoint, and coordinates exact-head review. Terra must not block because downstream checkpoint or review has not yet occurred.

Independent review is bounded and supervisor-enforced:

- controller validation evidence is included in review input;
- one shell command per tool call;
- no duplicate command beyond the configured limit;
- tool-call and iteration budgets;
- total, inactivity, and zombie-child supervision;
- stage-only exact-head review resume.

## Required environment

```bash
export LITELLM_API_KEY='...'
export OPENAI_API_KEY="$LITELLM_API_KEY"
```

Run preflight:

```bash
paf-cline-preflight \
  --repo /workspaces/PandA-bambu \
  --publish \
  --restart-daemon-on-key-mismatch
```

## Install

```bash
./install-paf-bootstrap.sh /workspaces/PandA-bambu
```

Backups are written under `~/.local/state/paf-bootstrap-install-backups/`, never into the repository. The installer does not stage, commit, push, create a PR, or merge.

## Initialize after the revision-3 plan/tooling PR is merged

```bash
cd /workspaces/PandA-bambu
git switch dev/panda
git pull --ff-only

paf-cline-campaign init --complete BS-000
paf-cline-campaign status
```

## Generate, inspect, authorize, and run

```bash
paf-cline-campaign next
less ~/.local/share/paf/tasks/paf-bootstrap-self-hosting/BS-010.md
cat ~/.local/share/paf/tasks/paf-bootstrap-self-hosting/BS-010.json
paf-cline-campaign authorize
paf-cline-campaign run
```

A generated task is bound to the exact base SHA and authorized backlog digest. It becomes stale rather than silently rebasing when the base changes before first execution.

Convenience after a prior merge:

```bash
paf-cline-campaign advance
# inspect and authorize, then:
paf-cline-campaign run
```

`advance --run --authorize` is available when intentional automated authorization is acceptable.

## Monitor

```bash
paf-cline-monitor status
paf-cline-monitor watch
paf-cline-monitor events --follow
# or:
paf-cline-campaign watch
```

The monitor shows stage, latest event, iteration, retained tool-call count, idle time, controller transitions, scoped process descendants, zombies, repeated commands, policy-budget warnings, and terminal summaries. It does not recursively print supplied prompts or accumulated output.

## Blocked, failed, or interrupted work

A blocked/failed implementation with preserved changes can be retried on the exact task branch:

```bash
paf-cline-campaign retry
```

An interrupted exact-head review does not rerun Sol or Terra:

```bash
paf-cline-campaign resume-review --from sonnet
# or when the review plan must also be regenerated:
paf-cline-campaign resume-review --from opus
```

Budget, hard quota, missing-key, and authentication failures are circuit-breaker events. Change/replenish the binding before retrying.

## After human review and merge

```bash
paf-cline-campaign reconcile
paf-cline-campaign status
```

## Authority boundary

The task generator is read-only. Generated task contracts require explicit operator authorization. Implementation agents cannot stage, commit, push, switch branches, or use network services. The controller creates only configured draft publication effects. Human authority remains required for merge, scope/Charter expansion, protected budget, credentials, data access, security exceptions, publication policy changes, and PAF promotion.
