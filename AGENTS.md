# PandA-bambu agent guidance

PandA-bambu is the HLS backend used by EvolveHLS. Development is evidence-driven:
preserve synthesis, simulation, and structured CI reporting. The default integration
branch is `dev/panda`.

## Workflow

1. Inspect and explain relevant code before modifying it.
2. Use one narrowly scoped `agent/<task>` branch and avoid unrelated refactoring.
3. Run focused tests before broader applicable validation.
4. Open a draft PR; never merge automatically, modify repository rules, force-push,
   or weaken branch protection.

## Cost discipline

- Use at most two concurrent subagents, only for independent bounded work. Do not
  ask agents to rediscover the same code.
- Reuse existing CI logs, structured bundles, and artifacts. Never rerun a
  successful expensive PandA build without a concrete reason.
- Allow at most two attempts for one implementation approach and at most two
  CI-repair iterations for one failure. If the same failure persists twice, stop
  and report it.
- Do not broaden scope without approval. Use low or medium reasoning for routine
  work; reserve high reasoning for architecture, difficult CI failures, and final
  review.

## Validation

Run only the applicable subset:

```bash
actionlint -shellcheck=
python3 .github/scripts/validate_ci.py
python3 .github/scripts/test_validate_ci.py
python3 -m unittest discover -s .github/scripts -p 'test_*.py'
bash -n .github/actions/build-panda/entrypoint.sh
git diff --check
```

Do not run irrelevant expensive checks merely to complete a fixed checklist.

## CI and safety

- Correctness failures block acceptance. Never mask build, synthesis, simulation,
  or verification failures.
- Keep missing evidence missing or `null`; never fabricate zero. Preserve
  diagnostics and candidate evidence after failure.
- Persistent hosted caches must be CPU-portable. Compare candidates only with
  validated, compatible baselines.
- Do not use `sudo`, install packages system-wide, place credentials in tracked
  files, or delete outside repository-owned temporary directories.
- Do not access outside this repository except for explicitly required read-only
  configuration discovery.
