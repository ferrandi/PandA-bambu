/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_c_isnan.c -- float version of s_c_isnan.c.
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
 * isnanf(x) returns 1 is x is nan, else 0;
 *
 * isnanf is an extension declared in <ieeefp.h>.
 */

#include "math_privatef.h"
 
int __isnanf(float x)
{
	int ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	return FLT_UWORD_IS_NAN(ix);
}

int __builtin_isnanf(float x)
{
   return __isnanf(x);
}


#ifdef _DOUBLE_IS_32BITS

int __builtin_isnan(double x)
{
	return __builtin_isnanf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
