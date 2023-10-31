## Auto-Regressive Filter (ARF) benchmark

ARF is a digital implementation of an auto-regressive lattice filter. 
The benchmark consists of 16 multiplication and 12 addition operations. 
The ARF benchmark operates in a loop. 
This feature is exploited by exercising the simulation as closed loop system.
   
Derived from:
```     
N. Mukherjee, "Built-in in Self-Test for Functional Blocks in Data-Path Architectures", PhD thesis, McGill University, Montreal, 1996.
```
where it is referred to:
```
R. Jain, "High-Level Area-Delay Prediction with Application to Behavioral Synthesis", PhD thesis, University of Southern California, Los Angeles, Usa, 1989.
```
This directory includes a simple example of RTL synthesis and simulation of the ARF benchmark.

Bambu HLS generates several dot files by passing the option `--print-dot`; in this case you may pass the option to the `generic_arf.sh` script as `--c=--print-dot`.
The scheduling of the arf function is stored in file _HLS_output/dot/arf/HLS_scheduling.dot_ while the FSM of the arf function annotated with the C statements is stored in file _HLS_output/dot/arf/HLS_STGraph.dot_.
