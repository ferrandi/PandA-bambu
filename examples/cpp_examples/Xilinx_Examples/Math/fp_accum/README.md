
Floating Point Accumluation Operation and HLS Optimizations
======================================

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

Optimization of a Floating Point Accumluation Operation.

    
[Full Documentation]

## 2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS

Any Vivado HLS release from 2014.1 to 2016.1

## 3. DESIGN FILE HIERARCHY
```
    3_FP_accum.pdf
    CONTRIBUTING.md
    fp_accum.cpp
    LICENSE.md
    README.md
    run_3_FP_accum_script.tcl
```

## 4. INSTALLATION AND OPERATING INSTRUCTIONS

The procedure to build the HLS project is as follows:
```
	vivado_hls run_3_FP_accum_script.tcl
```

## 5. OTHER INFORMATION

For more information check here: 
[Full Documentation][]
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
JAN2014		|	1.0					|	Initial Xilinx release
24MAR2016	|	1.1					|	Verified for 2016.1



[Contributing]: CONTRIBUTING.md 
[3-Clause BSD License]: LICENSE.md
[Full Documentation]: 3_FP_accum.pdf 
[Vivado HLS Forums]: https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/bd-p/hls 
[Vivado HLS User Guide]: http://www.xilinx.com/support/documentation/sw_manuals/xilinx2015_4/ug902-vivado-high-level-synthesis.pdf