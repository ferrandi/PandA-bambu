# AXI burst Verilog regressions

Following the Verilog regression layout used for priority encoders, this family
contains tests that exercise AXI burst hardware directly:

- `rtl_tests/`: extracted XML components, FIFO/core tests, standalone memory
  models, and Bambu-generated accelerator/wrapper RTL driven by dedicated
  Verilog testbenches.
  `run_std_fifo.sh` checks the complete shared FIFO across depths and data widths, including
  ordering, full replacement, empty bypass, clear and collision forwarding.
- `t4b_tests/`: extracted-core and matched-slave floor/knee simulations, their
  extractor and offline checks.
- `run_duplex_iteration_tests.sh` and `check_duplex_iteration_results.py`: generated-kernel iteration mapping and observation checks.
- Root-level `test_*.py`: offline tests for generated-RTL runners and iteration checks.

The shared C/C++ kernel fixtures, compiler checks, native Bambu end-to-end tests,
and T4B runs using Bambu's own generated testbench remain in
`panda_regressions/hls/bambu_axi_burst/`. The `verilog-axi-burst` CI job runs the functional component tests through
`panda_regressions/verilog/axi_burst.sh`; the HLS job retains the existing AXI master, compiler and
generated-accelerator integration checks. Component tests require ModelSim,
Python and standard shell tools. They do not require Bambu, Vivado, ABC or Yosys.

Examples, from the repository root (use fresh output directories):

```bash
bash panda_regressions/verilog/axi_burst.sh /tmp/axi-components
python3 -m unittest discover -s panda_regressions/verilog/axi_burst/rtl_tests
python3 -m unittest discover -s panda_regressions/verilog/axi_burst/t4b_tests
python3 -m unittest discover -s panda_regressions/verilog/axi_burst -p 'test_*.py'
bash panda_regressions/verilog/axi_burst/rtl_tests/run_read_burst_modelsim.sh /tmp/axi-read all
BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 bash panda_regressions/verilog/axi_burst/rtl_tests/run_write_burst_engine.sh /tmp/axi-write
```

Set `BURST_TEST_TMPDIR` to place offline-test artifacts in a selected directory.
Existing simulator guard variables and parameter checks are unchanged.

The burst masters and `kmp_bambu_general_memory_interface` use the complete
`STD_FIFO` technology component from `etc/libtech/NC_MEM_IPs.xml`. It owns both
pointers and the occupancy count, and exposes `push`, `pop`, `level`, `empty`
and `full`. Depth uses the memory-library parameter name `n_elements`.
`BITSIZE_level` must accommodate `$clog2(n_elements+1)` bits; payload widths
are supplied through `BITSIZE_data_in` and `BITSIZE_data_out`.

The output presents the current head before consumption. On `pop`, synchronous
prefetch selects the next head without adding an output stage. A push into an
empty queue makes its word available after that edge. The caller supplies
accepted commands and prevents overflow/underflow; the FIFO does not silently
reject commands. Simultaneous push/pop is supported at full capacity, and at
empty for a bypass implemented by the caller. Reset and synchronous `clear`
discard the queue and take priority over commands. Reset polarity and
synchronous/asynchronous reset follow the library placeholders. Payload is not
reset and is only valid while the FIFO is nonempty.

The component uses `STD_SDP_BRAMFW` with write forwarding, `HIGH_LATENCY=0`,
`READ_ONLY_MEMORY=0`, and an empty initialization filename. The shared memory's
existing zero initialization remains enabled. Vendor libraries can specialize
the storage primitive; FPGA synthesis may choose LUTRAM or BRAM without vendor
primitives in the portable implementation. `simple_fifo` and the private
`AXIBurstFIFOStorage` wrapper have been replaced by this common component.

The OpenMP caller now consumes a head only when the selected destination accepts
it, replacing its previous popped-word/valid pipeline. Its external request
latency is preserved. The existing context-capacity requirement and upstream
backpressure behavior remain unchanged.

The extractor resolves cross-library dependencies and prefers a colocated
`NC_MEM_IPs.xml` snapshot when available.

The burst reader bypasses an empty payload FIFO when a valid LOAD can consume
a successful AXI read beat on the same clock edge. Otherwise the beat is buffered
in the synchronous storage. The RAM write remains enabled on bypass, while FIFO
push and consumption cancel in the occupancy count. `RREADY` is independent of
the incoming response and the bypass path. The reader regression includes
`SKID_STREAM` at depths 1, 2, 16, 64, 512, and 4096, checking first-word latency,
queued data, pointer wrap, and pending LOADs. Duplex checks sample LOAD data and
completion at the active clock edge to observe the earlier completion.

The shared `STD_SDP_BRAMFW` selects its forwarding implementation with a generate:
for fewer than 512 words it registers one combined collision/write-enable bit;
from 512 words it registers the address-match bit and write enable separately.
The threshold is based on the measured 32-bit Artix-7 profiles, not a universal
mapping guarantee. Both forms replace the complete registered addresses. The
write payload is still sampled unconditionally. Initialization, pipeline modes,
width conversions, and the existing forwarding behavior in read-only mode are
preserved.

The CI component suite checks:

- `STD_FIFO`: 21 depth/width profiles with an independent queue scoreboard,
  including full replacement, empty bypass, pointer wrap and clear.
- `STD_SDP_BRAMFW`: 48 profiles against an independent cycle-level model,
  including four-state data, file initialization, width conversions, pipeline
  modes and both sides of the forwarding threshold.
- OpenMP memory interface: ten profiles comparing the adapted caller with the
  previous controller and an independent RAM model, including backpressure,
  reset, routing and an independent request-order scoreboard.

The runner retains per-component logs and writes `junit.xml`, including failures.
The pre-existing AXI master, Verilog compilation, analysis and synthesis tests
are preserved. The new CI selection adds functional component checks only; it
does not add area/frequency benchmarks or require synthesis/formal tools.
