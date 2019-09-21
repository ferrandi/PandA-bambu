/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* Copyright (C) 2002 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "math_privatef.h"

float fdimf(float x, float y)
{
   int c = __fpclassifyf(x);
   if(c == FP_NAN)
      return (x);
   if(__fpclassifyf(y) == FP_NAN)
      return (y);
   if(c == FP_INFINITE)
      return HUGE_VALF;

   return x > y ? x - y : 0.0;
}

#ifdef _DOUBLE_IS_32BITS

double fdim(double x, double y)
{
   return (double)fdimf((float)x, (float)y);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
