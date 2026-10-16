# Priority encoder families

All three families are combinational, select the least significant asserted input,
and assert `empty` with index zero when all inputs are zero. Each family has every
power-of-two width from 1 to 512, with the same `i`, `o` and `empty` interface.

| Option | Module prefix | Implementation |
| --- | --- | --- |
| `--priority-encoder=hierarchical` (default) | `hierarchical_` | Composition of smaller encoder modules |
| `--priority-encoder=tree` | `tree_` | Balanced tree of explicit gate equations, without encoder submodules |
| `--priority-encoder=gates` | none | Constant masks and vector reductions computing each output bit independently |

The 1- and 2-input base cases reduce to the same minimal logic. Hierarchical 4..64
compose two smaller encoders, while 128..512 reuse 64-input groups. Tree encoders
use the balanced gate tree at every larger width; gates encoders use the masked
reduction equations at every larger width. The library keeps all 30 definitions.

## Static selection

Consumer RTL in the XML references the default hierarchical family. During library
loading, `technology_manager::xload` replaces that prefix in both
`VERILOG_PROVIDED` and `IP_COMPONENT` metadata. It skips definitions of all encoder
families, preserving their contents and internal dependencies. Only the selected
family's dependencies are emitted for each consumer. No Verilog selection
parameter is required. `technology_manager::get_priority_encoder_prefix` is the
shared selection function; task-generated resource managers call it too because
they are generated after library loading. This mechanism is also explained in
comments at both C++ call sites.

## Verification

```sh
python3 etc/scripts/test_priority_encoders.py --yosys /path/to/yosys \
  --synthesis-report priority-encoders.json
```

Verilator checks all 30 modules against an independent reference, including zero,
walking bits, multiple active bits and deterministic random patterns. With
`--yosys`, ten SAT miters prove all three implementations against an independent
behavioral priority specification at every width, including `empty` and the zero
input. This proves equivalence and catches a bug shared by multiple families.
The script checks complete families and valid encoder dependencies. The RTL
regression remains registered in CTest and minimal CI; the task arbiter and banked
CI scripts now also exercise the `TREE` configuration.

Bambu was rebuilt with GCC 13 and warnings-as-errors/assertions enabled. Twelve
RTL co-simulations passed: ordinary OpenMP critical managers with and without
context switching, banked task controllers with duplicate resources, and tasks
calling `expf`/`sqrtf`, each with all three families. The hierarchical cases use the
default without an explicit selector. The emitted RTL was checked for the selected
prefix; both arbiters and both critical managers were covered for every family.
Help and CLI acceptance/rejection checks also passed. Exact integration outcomes
are in `openmp_task_validation_results.json` under `priority_encoder_families`.

## Synthesis comparison

Yosys 0.33 (git 2584903a060), bundled ABC, flattened designs, generic 6-input LUTs:

```text
read_verilog encoders.v
synth -top <encoder_module> -flatten -lut 6
stat
ltp -noff
```

The table reports **LUTs / longest topological path in LUT levels** from the actual
library definitions and their dependencies.

| Inputs | Hierarchical | Tree | Gates |
| --- | --- | --- | --- |
| 1 | 1 / 1 | 1 / 1 | 1 / 1 |
| 2 | 2 / 1 | 2 / 1 | 2 / 1 |
| 4 | 3 / 1 | 3 / 1 | 3 / 1 |
| 8 | 7 / 2 | 7 / 2 | 7 / 2 |
| 16 | 17 / 3 | 17 / 3 | 23 / 3 |
| 32 | 37 / 3 | 37 / 3 | 64 / 4 |
| 64 | 79 / 4 | 79 / 4 | 138 / 5 |
| 128 | 157 / 5 | 159 / 5 | 322 / 6 |
| 256 | 319 / 6 | 322 / 5 | 729 / 7 |
| 512 | 638 / 7 | 644 / 6 | 1652 / 8 |

The hierarchical default has the smallest mapped area for the larger widths.
At 256 and 512, the explicit tree has slightly more LUTs but one fewer level than
the hierarchical implementation in this run. The mask/reduction family maps to
more LUTs and levels here. These are mapping results without device timing,
placement or routing; they do not establish operating frequencies or a universal
winner across targets. Synthesis heuristics can also produce small differences
between equivalent composed and explicit descriptions after flattening.

[Current synthesis data](openmp_task_priority_encoders_results.json) are stored
with the source. The [earlier optimization experiments](openmp_task_priority_encoders_optimization.json)
retain the historical two-family results before this split; their old `gates`
family mixed small trees and large masked reductions. The [Yosys synthesis documentation](https://yosyshq.readthedocs.io/projects/yosys/en/v0.54/cmd/synth.html)
describes the mapping flow; Intel's [design recommendations](https://www.intel.com/programmable/technical-pdfs/683323.pdf)
discuss HDL structure and implementation quality.
