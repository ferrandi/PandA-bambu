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

/* __ieee754_log10(x)
 * Return the base 10 logarithm of x
 * 
 * Method :
 *	Let log10_2hi = leading 40 bits of log10(2) and
 *	    log10_2lo = log10(2) - log10_2hi,
 *	    ivln10   = 1/log(10) rounded.
 *	Then
 *		n = ilogb(x), 
 *		if(n<0)  n = n+1;
 *		x = scalbn(x,-n);
 *		log10(x) := n*log10_2hi + (n*log10_2lo + ivln10*log(x))
 *
 * Note 1:
 *	To guarantee log10(10**n)=n, where 10**n is normal, the rounding 
 *	mode must set to Round-to-Nearest.
 * Note 2:
 *	[1/log(10)] rounded to 53 bits has error  .198   ulps;
 *	log10 is monotonic at all binary break points.
 *
 * Special cases:
 *	log10(x) is NaN with signal if x < 0; 
 *	log10(+INF) is +INF with no signal; log10(0) is -INF with signal;
 *	log10(NaN) is that NaN with no signal;
 *	log10(10**N) = N  for N=0,1,...,22.
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

double __hide_ieee754_log10(double x)
{
	double y,z;
	int i,k,hx;
	unsigned lx;

	hx = GET_HI(x);	/* high word of x */
	lx = GET_LO(x);	/* low word of x */

        k=0;
        if (hx < 0x00100000) {                  /* x < 2**-1022  */
            if (((hx&0x7fffffff)|lx)==0)
                return -__builtin_inf();             /* log(+-0)=-inf */
            if (hx<0) return __builtin_nans("");        /* log(-#) = NaN */
            k -= 54; x *= two54; /* subnormal number, scale up x */
            hx = GET_HI(x);                /* high word of x */
        }
	if (hx >= 0x7ff00000) return x;
	k += (hx>>20)-1023;
	i  = ((unsigned)k&0x80000000)>>31;
        hx = (hx&0x000fffff)|((0x3ff-i)<<20);
        y  = (double)(k+i);
        SET_HIGH_WORD(x,hx);
	z  = y*log10_2lo + ivln10*__hide_ieee754_log(x);
	return  z+y*log10_2hi;
}
/* 
 * wrapper log10(X)
 */
double __builtin_log10(double x)		/* wrapper log10 */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_log10(x);
#else
	double z;
	z = __hide_ieee754_log10(x);
	if(_LIB_VERSION == _IEEE_ || __builtin_isnan(x)) return z;
	if(x<=0.0) {
	    if(x==0.0)
	        return __hide_kernel_standard(x,x,18); /* log10(0) */
	    else 
	        return __hide_kernel_standard(x,x,19); /* log10(x<0) */
	} else
	    return z;
#endif
}
