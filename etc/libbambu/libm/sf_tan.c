/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_tan.c -- float version of s_tan.c.
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

float __builtin_tanf(float x)
{
	float y[2],z=0.0;
	int n,ix;

	GET_FLOAT_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;
	if(ix <= 0x3f490fda) return __hide_kernel_tanf(x,z,1);

    /* tan(Inf or NaN) is NaN */
	else if (!FLT_UWORD_IS_FINITE(ix)) return __builtin_nanf("");		/* NaN */

    /* argument reduction needed */
	else {
	    n = __hide_ieee754_rem_pio2f(x,y);
	    return __hide_kernel_tanf(y[0],y[1],1-((n&1)<<1)); /*   1 -- n even
							      -1 -- n odd */
	}
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_tan(double x)
{
	return (double) __builtin_tanf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
