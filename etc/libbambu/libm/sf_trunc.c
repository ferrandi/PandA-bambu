/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "math_privatef.h"

float truncf(float x)
{
   int signbit, w, exponent_less_127;

   GET_FLOAT_WORD(w, x);

   /* Extract sign bit. */
   signbit = w & 0x80000000;

   /* Extract exponent field. */
   exponent_less_127 = ((w & 0x7f800000) >> 23) - 127;

   if(exponent_less_127 < 23)
   {
      if(exponent_less_127 < 0)
      {
         /* -1 < x < 1, so result is +0 or -0. */
         SET_FLOAT_WORD(x, signbit);
      }
      else
      {
         SET_FLOAT_WORD(x, signbit | (w & ~(0x007fffff >> exponent_less_127)));
      }
   }
   else
   {
      if(exponent_less_127 == 255)
         /* x is NaN or infinite. */
         return X_PLUS_X(x);

      /* All bits in the fraction field are relevant. */
   }
   return x;
}

#ifdef _DOUBLE_IS_32BITS

double trunc(double x)
{
   return (double)truncf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
