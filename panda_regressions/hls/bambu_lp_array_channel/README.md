# Array interface memory model

This is specifically the Xilinx HLS `ap_memory` contract, not a general RAM
assumption. The array ports expose an address, chip enable (`ce`), write enable
(`we`), write data, and read data. The external memory must stall and keep its
output data valid while `d_ce` is low, as specified by [AMD UG1399, Port-Level
I/O: Memory Interface Protocol](https://docs.amd.com/r/en-US/ug1399-vitis-hls/Port-Level-I-O-Memory-Interface-Protocol).

The generated array model marks the interface as stallable by default. Its
testbench model holds each channel's `q` value when that channel is not making a
read (`ce` low or `we` high), and updates read data according to `READ_DELAY`.
Writes update memory without replacing the read-data value. The regression
`test_array_memory_model.py` checks both channels reading together, independent
reads, writes, the configured read delays 1, 2, and 3, and output stability
during and after CE-low intervals.

This is the synchronous `ap_memory` interface contract, not an unrestricted RAM
model. A custom array interface whose output does not remain stable while its
consumer is stalled must declare `Stallable=false` in its functional-unit XML
metadata. Bambu can then use the timed-capture fallback for that interface;
this metadata exception does not change the default behavior for inferred
arrays.
