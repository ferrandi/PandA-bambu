# AXI burst Verilog regressions

Following the Verilog regression layout used for priority encoders, this family
contains tests that exercise AXI burst hardware directly:

- `rtl_tests/`: extracted XML components, FIFO/core tests, standalone memory
  models, and Bambu-generated accelerator/wrapper RTL driven by dedicated
  Verilog testbenches. Includes ABC analysis and equivalence helpers.
- `t4b_tests/`: extracted-core and matched-slave floor/knee simulations, their
  extractor and offline checks.
- `run_duplex_iteration_tests.sh` and `check_duplex_iteration_results.py`: generated-kernel iteration mapping and observation checks.
- Root-level `test_*.py`: offline tests for generated-RTL runners and iteration checks.

The shared C/C++ kernel fixtures, compiler checks, native Bambu end-to-end tests,
and T4B runs using Bambu's own generated testbench remain in
`panda_regressions/hls/bambu_axi_burst/`. The HLS regression and development
entry points still dispatch the Verilog gates from this directory.

Examples, from the repository root (use fresh output directories):

```bash
python3 -m unittest discover -s panda_regressions/verilog/axi_burst/rtl_tests
python3 -m unittest discover -s panda_regressions/verilog/axi_burst/t4b_tests
python3 -m unittest discover -s panda_regressions/verilog/axi_burst -p 'test_*.py'
bash panda_regressions/verilog/axi_burst/rtl_tests/run_read_burst_modelsim.sh /tmp/axi-read all
BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 bash panda_regressions/verilog/axi_burst/rtl_tests/run_write_burst_engine.sh /tmp/axi-write
```

Set `BURST_TEST_TMPDIR` to place offline-test artifacts in a selected directory.
Existing simulator guard variables and parameter checks are unchanged.
