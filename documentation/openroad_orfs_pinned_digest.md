# OpenROAD ORFS Pinned Digest

This repository pins the OpenROAD-flow-scripts (ORFS) revision used by the Bambu OpenROAD backend (repo mode).
It also pins the default Docker image tag used to run the flow.

## Pinned ORFS Digest

- ORFS git commit digest (SHA-1): `a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8`
- Runtime override variable: `OPENROAD_ORFS_PINNED_DIGEST`
- Default Docker image: `openroad/orfs:v3.0-4639-ga2bb042b6`
- Runtime override variable: `OR_IMAGE`

## Backend Modes

The OpenROAD backend now supports two execution modes:

1. `image-only` mode (default when `OPENROAD_USE_DOCKER=1` and `OPENROAD_FLOW_DATA_INSTALLDIR` is unset)
2. `repo` mode (enabled when `OPENROAD_FLOW_DATA_INSTALLDIR` is set)

Notes:

- `image-only` mode uses ORFS files already present in the Docker image.
- `repo` mode uses `OPENROAD_FLOW_DATA_INSTALLDIR/flow` mounted into the container.
- Non-docker mode (`OPENROAD_USE_DOCKER=0`) requires `repo` mode.

## Why This Is Pinned

The Bambu OpenROAD backend is validated against a specific ORFS revision. Running with a different ORFS revision can break the flow because scripts, Tcl APIs, and metadata formats may change over time.

In `repo` mode, Bambu validates the checked-out ORFS commit against the pinned digest above.
In `image-only` mode, Bambu relies on the pinned Docker image and does not require a local ORFS clone.

If needed, you can override the digest (repo mode) with `OPENROAD_ORFS_PINNED_DIGEST` and/or the image with `OR_IMAGE`.

## Optional Module Retiming Controls

The Bambu OpenROAD backend forwards the ORFS synthesis variables
`SYNTH_KEEP_MODULES` and `SYNTH_RETIME_MODULES`.

By default, Bambu adds the following logical module names to both variables:

- `mul_node_FU`
- `widen_mul_node_FU`
- `ui_widen_mul_node_FU`
- `ui_mul_node_FU`

When the value is a simple whitespace-separated module list, Bambu first runs
the Yosys canonicalization step and resolves each logical name against
`1_1_yosys_canonicalize.rtlil`.

That means a user-facing name such as:

```bash
export SYNTH_KEEP_MODULES=mul_node_FU
export SYNTH_RETIME_MODULES=mul_node_FU
```

is translated to the exact elaborated Yosys module name actually present in the
design, for example `$paramod$...\\mul_node_FU`.

Behavior notes:

1. Default module names are optional matches: if one of the four module families
   is not present in a design, it is silently skipped.
2. User-supplied simple module names are required matches: if a requested module
   cannot be resolved, Bambu stops with an explicit error.
3. `SYNTH_KEEP_MODULES` and resolved `SYNTH_RETIME_MODULES` are passed to ORFS
   as lists of exact elaborated module names.
4. For resolved retiming lists, Bambu generates a temporary `SYNTH_SCRIPT`
   override so ORFS retimes each selected module with `select -module`.
5. If you provide a more complex raw ORFS expression instead of a simple module
   list, Bambu forwards it unchanged and does not try to resolve names.

## Image Migration Note

The OpenROAD project replaced `openroad/flow-ubuntu22.04-builder` with `openroad/orfs` (see ORFS issue #2760 comment: <https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues/2760#issuecomment-3214592060>).

The default pair above is validated for CTS on our current host setup.

## Runtime Validation

At backend startup, Bambu checks:

`repo` mode checks:

1. `OPENROAD_FLOW_DATA_INSTALLDIR` is absolute and points to an existing ORFS directory.
2. The directory is a git clone of ORFS.
3. The checked-out commit is exactly the pinned digest above.
4. The ORFS working tree has no local tracked-file modifications.

If any check fails, Bambu stops and prints recovery instructions.

`image-only` mode does not perform ORFS git digest validation because ORFS files are consumed from the container image.

## Optional Digest Override (Repo Mode)

To override the default pinned digest:

```bash
export OPENROAD_ORFS_PINNED_DIGEST=<40-hex-commit-digest>
```

Example:

```bash
export OPENROAD_ORFS_PINNED_DIGEST=a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
```

## Recovery Procedure (Repo Mode)

Use one of the following methods.

### A) Fresh clone at the pinned digest (recommended)

```bash
git clone --no-checkout --depth 1 https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts.git /path/to/OpenROAD-flow-scripts
git -C /path/to/OpenROAD-flow-scripts fetch --depth 1 origin a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
git -C /path/to/OpenROAD-flow-scripts checkout --detach a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
export OPENROAD_FLOW_DATA_INSTALLDIR=/path/to/OpenROAD-flow-scripts
```

### B) Convert an existing ORFS clone to the pinned digest

```bash
git -C "$OPENROAD_FLOW_DATA_INSTALLDIR" fetch --depth 1 origin a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
git -C "$OPENROAD_FLOW_DATA_INSTALLDIR" checkout --detach a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
git -C "$OPENROAD_FLOW_DATA_INSTALLDIR" reset --hard a2bb042b6ea0811b4a63b6be41dcac5dc49e72f8
```

## Quick Verification (Repo Mode)

```bash
git -C "$OPENROAD_FLOW_DATA_INSTALLDIR" rev-parse HEAD
git -C "$OPENROAD_FLOW_DATA_INSTALLDIR" status --porcelain --untracked-files=no
```

`rev-parse HEAD` must match the pinned digest and `status` must print nothing.
