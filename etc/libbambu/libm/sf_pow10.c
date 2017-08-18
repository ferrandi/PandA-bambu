/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_pow10.c -- float version of s_pow10.c.
 * Modification of sf_pow10.c by Yaakov Selkowitz 2007.
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
 * wrapper pow10f(x)
 */

#include "math_privatef.h"

float __builtin_pow10f(float x)		/* wrapper pow10f */
{
  return __builtin_powf(10.0, x);
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_pow10(double x)
{
	return (double) __builtin_pow10f((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
