/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
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

float __builtin_roundf(float x)
{
    int i0, j0;

    GET_FLOAT_WORD (i0, x);
    j0 = ((i0 >> 23) & 0xff) - 0x7f;
    if (j0 < 23)
    {
        if (j0 < 0)
        {
            math_force_eval (huge + x);

            i0 &= 0x80000000;
            if (j0 == -1)
                i0 |= 0x3f800000;
        }
        else
        {
            unsigned int i = 0x007fffff >> j0;
            if ((i0 & i) == 0)
                /* X is integral.  */
                return x;
            math_force_eval (huge + x);

            /* Raise inexact if x != 0.  */
            i0 += 0x00400000 >> j0;
            i0 &= ~i;
        }
    }
    else
    {
        if (j0 == 0x80)
            /* Inf or NaN.  */
            return X_PLUS_X(x);
        else
            return x;
    }

    SET_FLOAT_WORD (x, i0);
    return x;
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_round(double x)
{
	return (double) __builtin_roundf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
