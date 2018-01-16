/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_atanh.c -- float version of e_atanh.c.
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

static const float one = 1.0f, huge = 1e30f;

float __hide_ieee754_atanhf(float x)
{
	float t;
	int hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if (ix>0x3f800000) 		/* |x|>1 */
        return nansf("");
	if(ix==0x3f800000)
        return (hx>>31) ? -inff() : inff();
    if(ix<0x31800000)
    {
        math_force_eval (huge + x);
        return x;	/* x<2**-28 */
    }
	SET_FLOAT_WORD(x,ix);
	if(ix<0x3f000000) {		/* x < 0.5 */
	    t = x+x;
        t = (float)0.5*log1pf(t+t*x/(one-x));
	} else 
        t = (float)0.5*log1pf((x+x)/(one-x));
	if(hx>>31) return -t; else return t;
}
