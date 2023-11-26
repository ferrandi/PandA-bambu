
Linear Algebra Library: Atan2() Example
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

Implementing the math function atan2 from the Linear Algebra Library 

[Full Documentation]

```
    void cordic_atan2( fix_coord_t y0, fix_coord_t x0, fix_phase_t *zn) 
    {
    fix_coord_t x, y, xp, yp; fix_phase_t z, zp; bool dneg; unsigned char i, quadrant = 0;
    static const fix_phase_t atan_2Mi[] = {45.0, 26.56, 14.03, 7.12, 3.57, 1.78, 0.89, 0.44};
    if (x0 < 0) x = -x0; else x = x0; if (y0 < 0) y = -y0; else y = y0; z=0;
    if (y0==0) { // SPECIAL CASE Y==0
    	if (x0<0) *zn = 180; else *zn = 0; return;
    }
    if (x0==0) { // SPECIAL CASE X==0
   		if (y0<0) *zn = -90; else *zn = 90; return;
    }
    if ((x0>0) & (y0>0)) quadrant = 1; else if ((x0<0) & (y0>0)) quadrant = 2; 
    else if ((x0<0) & (y0<0)) quadrant = 3; else if ((x0>0) & (y0<0)) quadrant = 4;
    LOOP1:for (i=0;i<=ROT;i++) {
    	dneg=  (y>0);
    	if (dneg) { xp = x+(y>>i); yp = y-(x>>i); zp = z + atan_2Mi[i];} 
    	else { xp = x - (y>>i); yp = y + (x>>i); zp = z - atan_2Mi[i]; }
    	x = xp; y = yp; z = zp;
    }
    if (quadrant==1) *zn = z; else if (quadrant==2) *zn = 180 - z; 
    else if (quadrant==3) *zn = z - 180; else if (quadrant==4) *zn = -z;
    }
``` 

## 2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS

Any Vivado HLS release from 2014.1 to 2016.1


## 3. DESIGN FILE HIERARCHY
```
    |   9a_cordic_atan2.pdf
    |   CONTRIBUTING.md
    |   cordic.h
    |   cordic_atan2.cpp
    |   cordic_test.cpp
    |   LICENSE.md
    |   README.md
    |   read_file.h
    |   run_atan2_hls_script.tcl
    |   
    +---M
    |       cpf_atan2.m
    |       cpf_atan2_data.txt
    |       db_cordic_atan2.m
    |       short_cpf_atan2.m
    |       
    \---test_data
            check_res.m
            input_data.txt
            ref_results.txt
            vector.dat
```

## 4. INSTALLATION AND OPERATING INSTRUCTIONS

The procedure to build the HLS project is as follows:

TCL file to run HLS tool:
```
	vivado_hls run_atan2_hls_script.tcl
```
## 5. OTHER INFORMATION

For more information check here: [Full Documentation][]
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
[Full Documentation]: 9a_cordic_atan2.pdf 
[Vivado HLS Forums]: https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/bd-p/hls 
[Vivado HLS User Guide]: http://www.xilinx.com/support/documentation/sw_manuals/xilinx2015_4/ug902-vivado-high-level-synthesis.pdf