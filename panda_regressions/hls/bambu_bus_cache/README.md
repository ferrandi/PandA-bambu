# Sequential bus and cache regressions

The `interface-test` Verilator job in `minimal.yml` runs this suite once using
its Bambu distribution. All kernels are sequential: no `-fopenmp` is passed.
The existing `OpenMP-support/banked_cache` job covers parallel banked caches.
These tests use the fixed-latency AXI memory model and do not need Ramulator.

The 12 cases cover:

- Nonbanked caches with `MEM_ACC_11` (one channel) and `MEM_ACC_N1` (two direct
  channels, one indirect channel), each with and without `--bus-pipelined`.
- Sequential four-bank caches with two `MEM_ACC_NN` channels, with and without
  `--bus-pipelined`.
- Rejection of nonbanked caches using `MEM_ACC_NN` with two channels, with and
  without `--bus-pipelined`. Independent caches have no coherence mechanism.
  Rejection must contain the specific configuration diagnostic.
- One-channel `MEM_ACC_NN` requests, with and without `--bus-pipelined`: existing
  parameter checking normalizes these to `MEM_ACC_11`, so they remain valid.
- Uncached AXI bus adapters using `MEM_ACC_N1` and `MEM_ACC_NN`, each configured
  with two channels.

Successful cases must complete co-simulation and match the C reference within
5,000 cycles. The cache kernel reads and updates arrays across cache-line and
bank boundaries. Logs and exact commands are kept per case; JUnit is included
in the existing interface-test reports.

After sourcing the Bambu installation's `settings.sh`, run:

```sh
python3 -B panda_regressions/hls/bambu_bus_cache/run_tests.py \
  --out out_bambu_bus_cache --jobs 2 --junit test-reports/bus-cache.xml
```

Use a fresh output directory for each run.
