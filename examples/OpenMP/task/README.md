# OpenMP task regressions

These tests require the LLVM 19 frontend (`--compiler=I386_CLANG19`) and
`--bambu-parameter=enable-task=1`. Source the installed Bambu `settings.sh` first.

Run the same suites used by CI, for example:

```sh
./examples/OpenMP/task/openmp_task_arb.sh --returnfail -j 3 -c=--simulator=VERILATOR
./examples/OpenMP/task/openmp_task_banked.sh --returnfail -j 3 -c=--simulator=VERILATOR
```

The arbiter suite covers one and multiple memory channels, resource duplication,
normal and pipelined buses, and all three priority encoder implementations. The banked
suite uses the bank directives in sparse LU and MNIST. The evaluation suites also
cover AXI interfaces. The suites are invoked from `minimal.yml` and
`reusable-openmp.yml`; `full-clang.yml` builds LLVM 19 support for the latter.

The small testbenches check expected array values. `taskNestedCalls` checks
independent call graphs and local memories beneath two distinct task entries,
with two levels of non-inlined callees. `taskMathCalls` exercises `expf` and
`sqrtf` under task-private callees, without any function-name exception in the
frontend; its testbench checks a relative tolerance and allows 16 ULPs in the
co-simulation comparison.

MNIST dimension assertions are disabled only when compiling the synthesized
kernel (`__SYNTHESIS__`); host and testbench assertions stay active.

MNIST defaults to deterministic synthetic data and small network dimensions, so
the regression suites need no external datasets. Inference compares every final
output value with the host implementation, in addition to its aggregate score;
training rejects nonfinite losses before comparing them. `openmp_task_perf.sh` is a manual
performance suite and includes real-size/data configurations. For configurations
with `USE_REAL_DATASET`, extract the supplied dataset archives and set
`BAMBU_MNIST_TRAIN` to the training CSV, `BAMBU_MNIST_TEST` to the test CSV, and
`BAMBU_MNIST_MODEL` to a model directory for inference. The testbench reports a
missing configuration before attempting to read files.

The standalone priority encoder regression is:

```sh
python3 etc/scripts/test_priority_encoders.py
```

It is also registered as CTest `priority_encoders`. Add `--yosys /path/to/yosys`
to run the formal checks.

Project source headers use the same Apache-2.0 WITH LLVM-exception wording as the
rest of Bambu. The original third-party BOTS copyright and GPL notice inside the
sparse LU benchmark are preserved. Technology-library RTL retains the MIT license
used by Bambu's existing IP library.

Choose encoders with `--priority-encoder=hierarchical|tree|gates` (default:
`hierarchical`). Each family supports every width from 1 to 512. `TREE` and `GATES`
configurations in the arbiter and banked scripts exercise the alternative families.
