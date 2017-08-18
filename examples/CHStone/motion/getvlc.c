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
/* getvlc.c, variable length decoding                                       */

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

int
Get_motion_code ()
{
  int code;

  if (Get_Bits1 ())
    {
      return 0;
    }

  if ((code = Show_Bits (9)) >= 64)
    {
      code >>= 6;
      Flush_Buffer (MVtab0[code][1]);

      return Get_Bits1 ()? -MVtab0[code][0] : MVtab0[code][0];
    }

  if (code >= 24)
    {
      code >>= 3;
      Flush_Buffer (MVtab1[code][1]);

      return Get_Bits1 ()? -MVtab1[code][0] : MVtab1[code][0];
    }

  if ((code -= 12) < 0)
    return 0;

  Flush_Buffer (MVtab2[code][1]);
  return Get_Bits1 ()? -MVtab2[code][0] : MVtab2[code][0];
}

/* get differential motion vector (for dual prime prediction) */
int
Get_dmvector ()
{

  if (Get_Bits (1))
    {
      return Get_Bits (1) ? -1 : 1;
    }
  else
    {
      return 0;
    }
}
