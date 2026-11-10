# Bambu + Ramulator CI suite

The dedicated `Ramulator co-simulation` workflow builds Bambu with Clang 13,
then runs a single test job. The test job checks out the exact Ramulator
submodule revision, builds and installs it once, and reuses that installation
for all 15 cases. The workflow assumes the pinned public Ramulator revision
provides the standard CMake installation layout. It never patches or overrides
the submodule revision and does not fall back to a different memory backend.

The suite runs on pull requests, pushes to `main`/`dev/panda`, manual dispatch
and weekly. It uses Verilator and a C++20 host toolchain, without vendor tools.

## Test list

All inputs contain 20 signed integers, including zero and negative values.
Each of these three kernels runs in all five configurations below:

| Kernel | Operation |
|---|---|
| `add` | `c[i] = a[i] + b[i]` |
| `update` | `c[i] += 3*a[i] - b[i]`, exercising read-modify-write |
| `conditional` | Odd `a[i]`: sum; even `a[i]`: difference |

| Configuration | Interface | Parallelism |
|---|---|---|
| `minimal-bus` | Parameters explicitly on a common MINIMAL bus | Sequential, one memory channel |
| `axi-bus` | Parameters on a common AXI bus | Sequential, one memory channel |
| `banked-minimal` | Common MINIMAL bus, four banks, chunk size 8 | Four OpenMP threads and four memory channels |
| `banked-axi` | Common AXI bus, four banks, chunk size 8 | Four OpenMP threads and four memory channels |
| `axi-dedicated` | Three independent argument AXI interfaces, outside the common bus | Sequential, three interfaces |

This gives the following 15 test IDs:

```text
add/minimal-bus
add/axi-bus
add/banked-minimal
add/banked-axi
add/axi-dedicated
update/minimal-bus
update/axi-bus
update/banked-minimal
update/banked-axi
update/axi-dedicated
conditional/minimal-bus
conditional/axi-bus
conditional/banked-minimal
conditional/banked-axi
conditional/axi-dedicated
```

Bambu uses `--generate-interface=INFER` to honor the explicit interface and bus
pragmas. MINIMAL is the external protocol. Banked cases explicitly declare
`bank_number=4`; a channel count alone does not select banked memory.
`RAMULATOR_OVERRIDE_CHANNELS=1` matches the DRAM channel count to the selected
Bambu channel count (four for banked cases, one otherwise).

## Pass criteria

Each case must complete RTL-versus-C golden co-simulation with return value
zero, within 5000 simulation cycles and a 180-second wall timeout including
synthesis and simulator compilation. The driver must confirm Ramulator
initialization, at least 20 real read completions and positive DRAM latency.
The generated testbench must have the expected native/NoC memory model and
exactly 0, 1, 4 or 3 AXI memory model instances for the selected interface.

Banked cases additionally emit a VCD. Sampling at rising clock edges must show
requests on at least two input memory channels simultaneously and activity on
at least two external banks. This prevents a nominal OpenMP/banked test from
passing while exercising only serialized memory traffic. The checker uses the
waveform, not the shared profiling CSV, whose multiple writers can truncate it.

Each case has an isolated output directory. A failure does not skip the
remaining cases; the runner returns failure after writing JUnit, JSON and
Markdown summaries. CI uploads logs, commands, evidence and build provenance;
banked waveforms are uploaded on failure.

## Local use

```bash
# This creates a fresh build and installation outside the submodule.
# --checkout is for CI: it resets the submodule to the recorded Git revision.
# Omit it to test the current local submodule worktree without resetting it.
bash panda_regressions/hls/bambu_ramulator/prepare_ramulator.sh /tmp/ramulator-ci
source install/settings.sh
export RAMULATOR2_DIR=/tmp/ramulator-ci/install
python3 panda_regressions/hls/bambu_ramulator/run_tests.py \
  --bambu "$PWD/install/bin/bambu" --out "$PWD/out_ramulator_ci"
```

Use `--list` to print the case IDs, or repeat `--case ID` to run a subset.
`RAMULATOR_BUILD_JOBS` controls the one-time dependency build parallelism.
`LD_LIBRARY_PATH` is deliberately removed from the test subprocess environment:
the installed dependency must work through the MDPI driver's runtime path.

## Initial local baseline

The first complete run passed 12 of the 15 cases (220–1010 cycles); all six
banked cases showed four simultaneous input-channel requests and activity on
all four banks. The three `minimal-bus` cases currently fail before simulation
with `DUT done_port is missing`: the ordinary native bus wrapper does not
expose the completion port. They remain failures, not skips or expected passes.
This CI change records that defect; it does not modify the Bambu runtime or
rewrite the original Ramulator porting commit. The previous preliminary native
interface test did not exercise this explicit common-bus wrapper.
