/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_scalb.c -- float version of e_scalb.c.
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
#include <limits.h>

#ifdef _SCALB_INT
	float __hide_ieee754_scalbf(float x, int fn)
#else
	float __hide_ieee754_scalbf(float x, float fn)
#endif
{
#ifdef _SCALB_INT
	return __builtin_scalbnf(x,fn);
#else
	if (__builtin_isnan(x)||__builtin_isnan(fn)) return x*fn;
	if (!__finitef(fn)) {
	    if(fn>(float)0.0) return x*fn;
	    else       return x/(-fn);
	}
	if (__builtin_rintf(fn)!=fn) return (fn-fn)/(fn-fn);
#if INT_MAX > 65000
	if ( fn > (float)65000.0) return __builtin_scalbnf(x, 65000);
	if (-fn > (float)65000.0) return __builtin_scalbnf(x,-65000);
#else
	if ( fn > (float)32000.0) return __builtin_scalbnf(x, 32000);
	if (-fn > (float)32000.0) return __builtin_scalbnf(x,-32000);
#endif
	return __builtin_scalbnf(x,(int)fn);
#endif
}
