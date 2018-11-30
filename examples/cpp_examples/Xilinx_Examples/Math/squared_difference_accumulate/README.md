
Squared Difference Accumulate Using Vivado High-Level Synthesis
================================================================

This readme file contains these sections:

1. OVERVIEW
2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS
3. DESIGN FILE HIERARCHY
4. INSTALLATION AND OPERATING INSTRUCTIONS
5. OTHER INFORMATION (OPTIONAL)
6. SUPPORT
7. LICENSE
8. CONTRIBUTING
9. Acknowledgements
10. REVISION HISTORY

## 1. OVERVIEW

This simple example shows how to use Vivado HLS to code a "Squared Difference Accumulate" function and ensure the new squaring MUX feature within the UltraScale DSP48E2 slice is utilized to allow the subtraction, multiplication, (i.e. squaring of the output of the pre-Adder in the DSP48E2 slice, and the accumulation to be implemented in 1 slices without using general FPGA logic for the subtraction as was necessary in previous architectures.


## 2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS

* Xilinx Vivado 2015.4 Design Suite with High-Level Synthesis

## 3. DESIGN FILE HIERARCHY
```
	|   CONTRIBUTING.md
	|   LICENSE.md
	|   README.md
	|   run.tcl
	|   
	+---src							: This folder contains C++ design files and header file.
	|       diff_sq_acc.cpp
	|       diff_sq_acc.h
	|       
	+---tb							: This folder contains a C++ design file that serves as the test bench. 
	|       diff_sq_acc_tb.cpp
	|       

```

## 4. INSTALLATION AND OPERATING INSTRUCTIONS

1. Install the Xilinx Vivado tools, version 2015.4 or later.
2. Unzip the design files into a clean directory.
3. In the Vivado HLS command line window:
	a. `cd` to the root of the design directory.
	b. Type `vivado_hls run.tcl`
	c. Check that the synthesized design meets expectations.

## 5. OTHER INFORMATION

For more information check here: 

[Vivado HLS User Guide][]

## 6. SUPPORT

For questions and to get help on this project or your own projects, visit the [Vivado HLS Forums][]. 

## 7. License
The source for this project is licensed under the [3-Clause BSD License][]

## 8. Contributing code
Please refer to and read the [Contributing][] document for guidelines on how to contribute code to this open source project. The code in the `/master` branch is considered to be stable, and all pull-requests should be made against the `/develop` branch.

## 9. Acknowledgements
The Library is written by developers at [Xilinx](http://www.xilinx.com/) with other contributors listed below:

## 10. REVISION HISTORY

Date		|	Readme Version		|	Revision Description
------------|-----------------------|-------------------------
April2016	|	1.0					|	Initial Xilinx release




[Contributing]: CONTRIBUTING.md 
[3-Clause BSD License]: LICENSE.md
[Vivado HLS Forums]: https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/bd-p/hls 
[Vivado HLS User Guide]: http://www.xilinx.com/support/documentation/sw_manuals/xilinx2015_4/ug902-vivado-high-level-synthesis.pdf