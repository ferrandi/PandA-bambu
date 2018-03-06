/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_nextafter.c -- float version of s_nextafter.c.
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

float nextafterf(float x, float y)
{
    int hx,hy,ix,iy;

    GET_FLOAT_WORD(hx,x);
    GET_FLOAT_WORD(hy,y);
    ix = hx&0x7fffffff;		/* |x| */
    iy = hy&0x7fffffff;		/* |y| */

    if((ix>0x7f800000) ||   /* x is nan */
            (iy>0x7f800000))     /* y is nan */
        return x+y;
    if(x==y) return y;		/* x=y, return y */
    if(ix==0) {				/* x == 0 */
        float u;
        SET_FLOAT_WORD(x,(hy&0x80000000)|1);/* return +-minsubnormal */
        u = math_opt_barrier (x);
        u = u*u;
        math_force_eval (u);		/* raise underflow flag */
        return x;
    }
    if(hx>=0) {				/* x > 0 */
        if(hx>hy) {				/* x > y, x -= ulp */
            hx -= 1;
        } else {				/* x < y, x += ulp */
            hx += 1;
        }
    } else {				/* x < 0 */
        if(hy>=0||hx>hy){			/* x < y, x -= ulp */
            hx -= 1;
        } else {				/* x > y, x += ulp */
            hx += 1;
        }
    }
    hy = hx&0x7f800000;
    if(hy>=0x7f800000) {
        float u = x+x;	/* overflow  */
        math_force_eval (u);
    }
    if(hy<0x00800000) {
        float u = x*x;			/* underflow */
        math_force_eval (u);		/* raise underflow flag */
    }
    SET_FLOAT_WORD(x,hx);
	return x;
}

#ifdef _DOUBLE_IS_32BITS

double nextafter(double x, double y)
{
    return (double) nextafterf((float) x, (float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
