/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_acosh.c -- float version of e_acosh.c.
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
 *
 */

#include "math_privatef.h"

static const float 
one	= 1.0,
ln2	= 6.9314718246e-01f;  /* 0x3f317218 */

float __hide_ieee754_acoshf(float x)
{	
	float t;
	int hx;
	GET_FLOAT_WORD(hx,x);
	if(hx<0x3f800000) {		/* x < 1 */
        return __builtin_nansf("");
	} else if(hx >=0x4d800000) {	/* x > 2**28 */
	    if(!FLT_UWORD_IS_FINITE(hx)) {	/* x is inf of NaN */
	        return x;
	    } else 
		return __hide_ieee754_logf(x)+ln2;	/* acosh(huge)=log(2x) */
	} else if (hx==0x3f800000) {
	    return 0.0;			/* acosh(1) = 0 */
	} else if (hx > 0x40000000) {	/* 2**28 > x > 2 */
	    t=x*x;
	    return __hide_ieee754_logf((float)2.0*x-one/(x+__hide_ieee754_sqrtf(t-one)));
	} else {			/* 1<x<2 */
	    t = x-one;
        return log1pf(t+__hide_ieee754_sqrtf((float)2.0*t+t*t));
	}
}
