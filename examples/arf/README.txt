This directory includes a simple example of HLS synthesis and generation of RTL simulation&synthesis scripts.
The results of the HLS synthesis could be inspected by looking into testbench/hls_summary_0.xml.
The result of the scheduling could be graphically viewed exploiting a viewer of dot files (e.g., xdot or dotty). 
In particular, bambu generates several dot files by passing the option --print-dot.
The scheduling of the arf function is stored in file HLS_output/dot/arf/HLS_scheduling.dot while the FSM of the arf function annotated with the C statements is stored in file HLS_output/dot/arf/HLS_STGraph.dot.
