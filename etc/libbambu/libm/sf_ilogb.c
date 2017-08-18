/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_ilogb.c -- float version of s_ilogb.c.
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

#include <limits.h>
#include "math_privatef.h"

int __builtin_ilogbf(float x)
{
	int hx,ix;

	GET_FLOAT_WORD(hx,x);
	hx &= 0x7fffffff;
	if(FLT_UWORD_IS_ZERO(hx))
	    return FP_ILOGB0;	/* ilogb(0) = special case error */
	if(FLT_UWORD_IS_SUBNORMAL(hx)) {
	    for (ix = -126,hx<<=8; hx>0; hx<<=1) ix -=1;
	    return ix;
	}
	#if FP_ILOGBNAN != INT_MAX
	else if (FLT_UWORD_IS_NAN(hx)) return FP_ILOGBNAN;	/* NAN */
	#endif
	else if (!FLT_UWORD_IS_FINITE(hx)) return INT_MAX;
	else return (hx>>23)-127;
}

#ifdef _DOUBLE_IS_32BITS

int __builtin_ilogb(double x)
{
	return __builtin_ilogbf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
