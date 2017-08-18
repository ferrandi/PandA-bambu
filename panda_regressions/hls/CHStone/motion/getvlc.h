/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/* getvlc.h, variable length code tables                                    */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

/* NOTE: #define constants such as MACROBLOCK_QUANT are upper case 
   as per C programming convention. However, the MPEG document 
   (ISO/IEC 13818-2) lists them in all lower case (e.g. Annex B) */

/* NOTE: the VLC tables are in a flash format---a transformation
   of the tables in Annex B to a form more convenient towards 
   parallel (more than one-bit-at-a-time) decoding */


/* Table B-10, motion_code, codes 0001 ... 01xx */
const char MVtab0[8][2] = {
  {ERROR, 0}, {3, 3}, {2, 2}, {2, 2},
  {1, 1}, {1, 1}, {1, 1}, {1, 1}
};

/* Table B-10, motion_code, codes 0000011 ... 000011x */
const char MVtab1[8][2] = {
  {ERROR, 0}, {ERROR, 0}, {ERROR, 0}, {7, 6},
  {6, 6}, {5, 6}, {4, 5}, {4, 5}
};

/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
const char MVtab2[12][2] = {
  {16, 9}, {15, 9}, {14, 9}, {13, 9}, {12, 9}, {11, 9},
  {10, 8}, {10, 8}, {9, 8}, {9, 8}, {8, 8}, {8, 8}
};
