This directory show an example on how it is possible to write a C-based testbench to test a given kernel.
The kernel function is defined through the option --top-rtldesign-name.

This design flow requires to add two attributes to the kernel function:
  __attribute__ ((noinline)) __attribute__ ((used))  

and to insert this two timing functions:
        __builtin_bambu_time_start();
        __builtin_bambu_time_stop();

These two functions will start and stop a timer used by bambu to compute the total number of cycles spent in the kernel function.
The target device is a Zynq xc7z020,-1,clg484 and the back-end flow is based on yosys open source RTL synthesis tool (http://www.clifford.at/yosys/). 

