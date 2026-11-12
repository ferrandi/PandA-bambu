# Bambu + Ramulator CI suite

The dedicated `Ramulator co-simulation` workflow builds Bambu with Clang 13,
then runs a single test job. The test job checks out the exact Ramulator
submodule revision, builds and installs it once, and reuses that installation
for all 24 cases (15 smoke, six intermediate HBM and three memory-isolation cases). The workflow assumes the pinned public Ramulator revision
provides the standard CMake installation layout. It never patches or overrides
the submodule revision and does not fall back to a different memory backend.

The reusable workflow is called by the standard Fast Regressions (`minimal.yml`)
after its existing Bambu build. Its result is included in the aggregate success
check. The existing Fast Regressions triggers are unchanged. The dedicated
workflow remains available for manual and weekly runs. Both callers share the
same test job, which uses Verilator and a C++20 host toolchain without vendor
tools. The standard caller's Bambu distribution is reused instead of rebuilding
Bambu.

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
For smoke cases, `RAMULATOR_OVERRIDE_CHANNELS=1` matches the DRAM channel count to the selected
Bambu channel count (four for banked cases, one otherwise).

## Pass criteria

Each smoke case must complete RTL-versus-C golden co-simulation with return value
zero, within 5000 simulation cycles and a 180-second wall timeout including
synthesis and simulator compilation. The driver must confirm Ramulator
initialization, at least 20 real read completions and positive DRAM latency.
The generated testbench must have the unified multichannel minimal memory model and
the expected number of AXI memory model instances (including both bus and dedicated bundles) for the selected interface.

Banked cases additionally emit a VCD. Sampling at rising clock edges must show
requests on at least two input memory channels simultaneously and activity on
at least two external banks. This prevents a nominal OpenMP/banked test from
passing while exercising only serialized memory traffic. The checker uses the
waveform to measure the actual concurrent traffic.
Smoke waveforms must expose four banks; HBM waveforms must expose all 32.

Each case has an isolated output directory. A failure does not skip the
remaining cases; the runner returns failure after writing JUnit, JSON and
Markdown summaries. CI uploads logs, commands, evidence and build provenance;
banked waveforms are uploaded on failure. Successful cases delete the full
waveform after recording measured evidence to limit disk usage.

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

Use `--list` to print the case IDs, `--group smoke|hbm|isolation` to select a group,
or repeat `--case ID` to run a subset.
`RAMULATOR_BUILD_JOBS` controls the one-time dependency build parallelism.
`LD_LIBRARY_PATH` is deliberately removed from the test subprocess environment:
the installed dependency must work through the MDPI driver's runtime path.

## Intermediate HBM group (six cases)

Three separate source files each run as `banked-minimal` and `banked-axi`:

| Source / case prefix | Elements per buffer | Access pattern |
|---|---:|---|
| `hbm_gather.cpp` / `hbm-gather` | 4096 | Permuted gather plus a second scrambled read |
| `hbm_scatter.cpp` / `hbm-scatter` | 4096 | Permuted scatter with disjoint destinations and strided/random reads |
| `hbm_chase.cpp` / `hbm-chase` | 2048 | Dependent index loads followed by random read-modify-write |

All six use explicit `bank_number=32`, 32 OpenMP threads, 32 Bambu memory
channels, and the same seeded permutation (`0x504E4E4C`). The input arrays are
larger than the smoke inputs and generated before synthesis; runtime random
number generation is unnecessary. Scatter uses a bijective permutation to
avoid data races between threads.

`hbm2.yaml` selects the actual HBM2 model (`HBM2_4Gb`, `HBM2_2Gbps`), 16 physical
channels with two pseudochannels each, and OpenRowPolicy. The accelerator runs
at 200 MHz and DRAM ticks at 1000 MHz (`clock_ratio=5`). For this group the driver
must preserve the YAML channel count (`RAMULATOR_OVERRIDE_CHANNELS=0`): forcing
32 physical HBM channels would double the intended pseudochannel count.
These are protocol/concurrency regressions, not a calibrated board bandwidth
benchmark or a guarantee of one-to-one physical address mapping.

The target is approximately 10K accelerator cycles, with a hard cap of 12000
and a 600-second wall timeout including compile/simulation. The VCD must expose
all 32 logical banks, show at least eight simultaneous input requests, and
activity on **all 32** external banks. Source, generated input, seed, bank count,
cycle limit and observed evidence are recorded per case. No case is skipped
when a preceding test fails.

```text
hbm-gather/banked-minimal
hbm-gather/banked-axi
hbm-scatter/banked-minimal
hbm-scatter/banked-axi
hbm-chase/banked-minimal
hbm-chase/banked-axi
```

## Independent memory interfaces

The shared bus owns one Ramulator backend, including all channels of a banked
bus, regardless of whether the external protocol is MINIMAL or AXI. Each
dedicated AXI bundle owns another independent backend. Parameters in the same
dedicated bundle share that memory. All backends advance from a single
accelerator clock source in SystemMEM, once per cycle, including idle cycles.

Three additional cases exercise a bus and a dedicated AXI bundle:

- `add/mixed-axi`: sequential kernel; `a` uses the AXI bus and `b`/`c` share
  one dedicated AXI bundle.
- `add/mixed-banked-minimal` and `add/mixed-banked-axi`: `mixed_banked.cpp`
  first reads `b` through a dedicated AXI bundle in the sequential section.
  Four OpenMP threads then gather from `a` and write `c` on the four-bank bus.
  No dedicated interface is accessed inside the parallel section. Both cases
  must demonstrate parallel requests and activity on multiple banks in the VCD.

The checker requires two initialized, active independent backends, each serving reads or writes,
with completed reads where applicable, equal accelerator cycle counts and exactly three DRAM ticks
per cycle (the smoke DDR4 configuration). The existing dedicated cases also
require three active independent backends. Backend summaries are part of the
normal simulation evidence; missing summaries or a shared backend fail the test.

## Standalone memory-model checks

The same reusable job also runs the production Verilog models against a fake
RTL accelerator, the production MDPI IPC/driver and the installed Ramulator.
These tests do not invoke Bambu. Both models are extracted directly from the
technology library. The dependency is built only once for both suites.

```bash
python3 -B panda_regressions/verilog/ramulator/run_tests.py \
  --bambu-prefix "$PWD/install" --out /tmp/ramulator-rtl \
  --junit /tmp/ramulator-rtl/junit.xml
```

`RAMULATOR2_DIR` must name the standard installation prefix. The suite checks
real read/write rejection at capacity, including absence of writes on rejection;
minimal request/response queues of depth 1, 2, 4 and 64; multiple channels and
internal-memory response merging; and AXI transaction queues of depth 1, 2 and 4.
The AXI fixture exercises independent AW/W, FIXED and INCR bursts up to 256 beats,
sparse and empty strobes, and long independent stalls on R/B. A burst occupies
one AR/AW slot; response storage reserves all its beats. Minimal responses use
full driver tags and a FIFO with bypass. Writes complete on Ramulator acceptance,
with no additional fixed delay. Rejected requests stay in bounded RTL queues.

The MDPI contract checks also cover read-data snapshots, partial-byte writes,
4096-bit payloads, adjacent mapped arguments, unmapped cache-line padding,
channel tags, invalid requests and fractional DRAM/accelerator clock ratios.
The classical backend is checked both with and without the Ramulator library.
`RAMULATOR2_CONFIG` selects a YAML file; relative `!include` paths resolve from
the containing file. An explicit invalid configuration fails initialization.
`MemorySystem.clock_ratio` is the number of DRAM ticks per accelerator cycle,
including fractional values; there is no upper limit of six.

The original DDR3, DDR4, DDR5 and HBM presets run without changing their row
policies. DDR3 ClosedRowPolicy treats the absent bank-group level as one implicit
group and uses the model's existing PRE command. Directed row-policy checks
exercise repeated accesses, row closing and reopening, multiple banks/ranks,
and counter reset on refresh, without changing DRAM timing tables.
The four-channel DDR5 preset also guards against duplicate formatted statistic names.
