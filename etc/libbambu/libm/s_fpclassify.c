/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* Copyright (C) 2002, 2007 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "math_private.h"

int __fpclassify(double x)
{
   unsigned msw, lsw;

   lsw = GET_LO(x);
   msw = GET_HI(x);

   if((msw == 0x00000000 && lsw == 0x00000000) || (msw == 0x80000000 && lsw == 0x00000000))
      return FP_ZERO;
   else if((msw >= 0x00100000 && msw <= 0x7fefffff) || (msw >= 0x80100000 && msw <= 0xffefffff))
      return FP_NORMAL;
   else if((msw >= 0x00000000 && msw <= 0x000fffff) || (msw >= 0x80000000 && msw <= 0x800fffff))
      /* zero is already handled above */
      return FP_SUBNORMAL;
   else if((msw == 0x7ff00000 && lsw == 0x00000000) || (msw == 0xfff00000 && lsw == 0x00000000))
      return FP_INFINITE;
   else
      return FP_NAN;
}
