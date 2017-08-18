/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_frexp.c -- float version of s_frexp.c.
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

#include "math_privatef.h"

static const float
two25 =  3.3554432000e+07; /* 0x4c000000 */

float __builtin_frexpf(float x, int *eptr)
{
	int hx, ix;
	GET_FLOAT_WORD(hx,x);
	ix = 0x7fffffff&hx;
	*eptr = 0;
	if(!FLT_UWORD_IS_FINITE(ix)||FLT_UWORD_IS_ZERO(ix)) return x;	/* 0,inf,nan */
	if (FLT_UWORD_IS_SUBNORMAL(ix)) {		/* subnormal */
	    x *= two25;
	    GET_FLOAT_WORD(hx,x);
	    ix = hx&0x7fffffff;
	    *eptr = -25;
	}
	*eptr += (ix>>23)-126;
	hx = (hx&0x807fffff)|0x3f000000;
	SET_FLOAT_WORD(x,hx);
	return x;
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_frexp(double x, int *eptr)
{
	return (double) __builtin_frexpf((float) x, eptr);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
