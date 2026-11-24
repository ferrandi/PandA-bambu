# OpenMP hardware predication regressions

Run from a Bambu installation environment with Verilator available:

```sh
python3 -B panda_regressions/hls/bambu_omp_predication/run_tests.py \
  --out out_omp_predication --jobs 2 --junit omp-predication.xml
```

The suite runs in the Verilator interface job of the minimal workflow:

- Directed RTL checks extract the seven synchronization and reduction primitives from
  `C_OPENMP_IPs.xml`. They cover skipped and executed starts, side effects,
  immediate and delayed acknowledgments, changes to the predicate while waiting,
  consecutive operations, independent contexts and reset of pending waits.
  All four combinations of reset polarity and synchronous/asynchronous reset run.
- `critical.cpp`: conditional contended updates to shared memory.
- `barrier.cpp`: a uniform dynamic condition guards a barrier and a subsequent
  read of data written by other threads. The two output halves avoid data races.
- `phases.cpp`: repeated skipped and executed synchronization phases in a loop.
- `fork.cpp`: two consecutive parallel regions separated by conditional scalar
  computations; the second region consumes the first region’s results and the
  parent checks their completion.
- `reduction.cpp`: a uniform dynamic condition guards two reductions, exercising
  skipped and executed thread-local reduction-data accessors with and without
  context switching.

Critical, barrier and phase tests run with one thread, four threads, and four
threads with two contexts per accelerator. Fork, reduction and memory-initialization
tests run with four threads, with and without context switching. Both false and true dynamic conditions are simulated,
with a limit of 15,000 cycles per invocation. Single-thread cases use one memory
channel; other cases use two. End-to-end cases compare the RTL results against
software execution. Checks on the generated netlist additionally require dynamic
IR predicates to reach native lock, barrier and wait ports, including shared FUs,
while their start ports receive the original controller commands.

`--case NAME` selects a case (for example `critical-4t-cs2`); `--skip-rtl` skips
the directed checks when investigating a co-simulation case. Use a fresh output
folder for each run.

Code motion distinguishes synchronization ordering from hardware predication.
Locks, barriers and reduction-data handoffs order memory accesses and other
synchronizations, while independent computations can still move across them.
Fork calls remain anchored and block every operation from crossing them, including
across intermediate control-flow blocks. Thread-count setup is consumed by
OpenMP call-graph expansion rather than emitted as hardware.
