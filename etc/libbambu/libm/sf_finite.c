/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_finite.c -- float version of s_finite.c.
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
 * finitef(x) returns 1 is x is finite, else 0;
 * no branching!
 */

#include "math_privatef.h"

int __finitef(float x)
{
   int ix;
   GET_FLOAT_WORD(ix, x);
   ix &= 0x7fffffff;
   return (FLT_UWORD_IS_FINITE(ix));
}

int finitef(float x)
{
   __finitef(x);
}

#ifdef _DOUBLE_IS_32BITS

int __finite(double x)
{
   return __finitef((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
