/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_exp10.c -- float version of s_exp10.c.
 * Modification of sf_exp2.c by Yaakov Selkowitz 2007.
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
 * wrapper exp10f(x)
 */

#include "math_privatef.h"

float exp10f(float x)		/* wrapper exp10f */
{
  return powf(10.0, x);
}

#ifdef _DOUBLE_IS_32BITS

double exp10(double x)
{
    return (double) exp10f((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
