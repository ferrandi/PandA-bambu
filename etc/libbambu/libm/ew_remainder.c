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
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#include "math_private.h"

/* __ieee754_remainder(x,p)
 * Return :                  
 * 	returns  x REM p  =  x - [x/p]*p as if in infinite 
 * 	precise arithmetic, where [x/p] is the (infinite bit) 
 *	integer nearest x/p (in half way case choose the even one).
 * Method : 
 *	Based on fmod() return x-[x/p]chopped*p exactlp.
 */
double __hide_ieee754_remainder(double x, double p)
{
	int hx,hp;
	unsigned sx,lx,lp;
	double p_half;

	hx = GET_HI(x);		/* high word of x */
	lx = GET_LO(x);		/* low  word of x */
	hp = GET_HI(p);		/* high word of p */
	lp = GET_LO(p);		/* low  word of p */
	sx = hx&0x80000000;
	hp &= 0x7fffffff;
	hx &= 0x7fffffff;

    /* purge off exception values */
    if((hp|lp)==0) return __builtin_nan(""); 	/* p = 0 */
	if((hx>=0x7ff00000)||			/* x not finite */
	  ((hp>=0x7ff00000)&&			/* p is NaN */
	  (((hp-0x7ff00000)|lp)!=0)))
        return __builtin_nan("");


	if (hp<=0x7fdfffff) x = __hide_ieee754_fmod(x,p+p);	/* now x < 2p */
	if (((hx-hp)|(lx-lp))==0) return zero*x;
    x  = fabs(x);
    p  = fabs(p);
	if (hp<0x00200000) {
	    if(x+x>p) {
		x-=p;
		if(x+x>=p) x -= p;
	    }
	} else {
	    p_half = 0.5*p;
	    if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	    }
	}
	SET_HIGH_WORD(x, (GET_HI(x) ^ sx));
	return x;
}

/* 
 * wrapper remainder(x,p)
 */
double remainder(double x, double y)	/* wrapper remainder */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_remainder(x,y);
#else
	double z;
	z = __hide_ieee754_remainder(x,y);
    if(_LIB_VERSION == _IEEE_ || isnan(y)) return z;
	if(y==0.0) 
	    return __hide_kernel_standard(x,y,28); /* remainder(x,0) */
	else
	    return z;
#endif
}
