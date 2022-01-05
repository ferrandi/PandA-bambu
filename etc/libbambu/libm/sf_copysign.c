/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_copysign.c -- float version of s_copysign.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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

/*
 * copysignf(float x, float y)
 * copysignf(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "math_privatef.h"

float copysignf(float x, float y)
{
   unsigned ix, iy;
   GET_FLOAT_WORD(ix, x);
   GET_FLOAT_WORD(iy, y);
   SET_FLOAT_WORD(x, (ix & 0x7fffffff) | (iy & 0x80000000));
   return x;
}

#ifdef _DOUBLE_IS_32BITS

double copysign(double x, double y)
{
   return (double)copysignf((float)x, (float)y);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
