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
/* global.h, global variables                                               */

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

#include "mpeg2dec.h"

/* choose between declaration (GLOBAL undefined)
 * and definition (GLOBAL defined)
 * GLOBAL is defined in exactly one file mpeg2dec.c)
 */


/* Get_Bits.c */
void Fill_Buffer _ANSI_ARGS_ ((void));
unsigned int Show_Bits _ANSI_ARGS_ ((int n));
unsigned int Get_Bits1 _ANSI_ARGS_ ((void));
void Flush_Buffer _ANSI_ARGS_ ((int n));
unsigned int Get_Bits _ANSI_ARGS_ ((int n));
int Get_Byte _ANSI_ARGS_ ((void));

/* getvlc.c */
int Get_motion_code _ANSI_ARGS_ ((void));
int Get_dmvector _ANSI_ARGS_ ((void));
int Get_coded_block_pattern _ANSI_ARGS_ ((void));


/* motion.c */
void motion_vector
_ANSI_ARGS_ ((int *PMV, int *dmvector, int h_r_size, int v_r_size, int dmv,
	      int mvscale, int full_pel_vector));

int System_Stream_Flag;

unsigned char ld_Rdbfr[2048];
unsigned char *ld_Rdptr, *ld_Rdmax;
unsigned int ld_Bfr;
int ld_Incnt;
