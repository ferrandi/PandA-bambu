/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_cosh.c -- float version of e_cosh.c.
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

static const float one = 1.0, half=0.5, huge = 1.0e30;

float __hide_ieee754_coshf(float x)
{	
	float t,w;
	int ix;

	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;

    /* x is INF or NaN */
	if(!FLT_UWORD_IS_FINITE(ix)) return x*x;	

    /* |x| in [0,0.5*ln2], return 1+expm1(|x|)^2/(2*exp(|x|)) */
	if(ix<0x3eb17218) {
	    t = __builtin_expm1f(__builtin_fabsf(x));
	    w = one+t;
	    if (ix<0x24000000) return w;	/* cosh(tiny) = 1 */
	    return one+(t*t)/(w+w);
	}

    /* |x| in [0.5*ln2,22], return (exp(|x|)+1/exp(|x|)/2; */
	if (ix < 0x41b00000) {
		t = __hide_ieee754_expf(__builtin_fabsf(x));
		return half*t+half/t;
	}

    /* |x| in [22, log(maxdouble)] return half*exp(|x|) */
	if (ix <= FLT_UWORD_LOG_MAX)
	  return half*__hide_ieee754_expf(__builtin_fabsf(x));

    /* |x| in [log(maxdouble), overflowthresold] */
	if (ix <= FLT_UWORD_LOG_2MAX) {
	    w = __hide_ieee754_expf(half*__builtin_fabsf(x));
	    t = half*w;
	    return t*w;
	}

    /* |x| > overflowthresold, cosh(x) overflow */
	return huge*huge;
}
