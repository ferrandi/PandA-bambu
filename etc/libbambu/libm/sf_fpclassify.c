/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* Copyright (C) 2002,2007 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "math_privatef.h"

int __fpclassifyf(float x)
{
   unsigned w;

   GET_FLOAT_WORD(w, x);

   if(w == 0x00000000 || w == 0x80000000)
      return FP_ZERO;
   else if((w >= 0x00800000 && w <= 0x7f7fffff) || (w >= 0x80800000 && w <= 0xff7fffff))
      return FP_NORMAL;
   else if((w >= 0x00000001 && w <= 0x007fffff) || (w >= 0x80000001 && w <= 0x807fffff))
      return FP_SUBNORMAL;
   else if(w == 0x7f800000 || w == 0xff800000)
      return FP_INFINITE;
   else
      return FP_NAN;
}
