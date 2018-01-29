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

/* __ieee754_pow(x,y) return x**y
 *
 *		      n
 * Method:  Let x =  2   * (1+f)
 *	1. Compute and return log2(x) in two pieces:
 *		log2(x) = w1 + w2,
 *	   where w1 has 53-24 = 29 bit trailing zeros.
 *	2. Perform y*log2(x) = n+y' by simulating muti-precision 
 *	   arithmetic, where |y'|<=0.5.
 *	3. Return x**y = 2**n*exp(y'*log2)
 *
 * Special cases:
 *	1.  (anything) ** 0  is 1
 *	2.  (anything) ** 1  is itself
 *	3.  (anything) ** NAN is NAN
 *	4.  NAN ** (anything except 0) is NAN
 *	5.  +-(|x| > 1) **  +INF is +INF
 *	6.  +-(|x| > 1) **  -INF is +0
 *	7.  +-(|x| < 1) **  +INF is +0
 *	8.  +-(|x| < 1) **  -INF is +INF
 *	9.  +-1         ** +-INF is NAN
 *	10. +0 ** (+anything except 0, NAN)               is +0
 *	11. -0 ** (+anything except 0, NAN, odd integer)  is +0
 *	12. +0 ** (-anything except 0, NAN)               is +INF
 *	13. -0 ** (-anything except 0, NAN, odd integer)  is +INF
 *	14. -0 ** (odd integer) = -( +0 ** (odd integer) )
 *	15. +INF ** (+anything except 0,NAN) is +INF
 *	16. +INF ** (-anything except 0,NAN) is +0
 *	17. -INF ** (anything)  = -0 ** (-anything)
 *	18. (-anything) ** (integer) is (-1)**(integer)*(+anything**integer)
 *	19. (-anything except 0 and inf) ** (non-integer) is NAN
 *
 * Accuracy:
 *	pow(x,y) returns x**y nearly rounded. In particular
 *			pow(integer,integer)
 *	always returns the correct integer provided it is 
 *	representable.
 *
 * Constants :
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough 
 * to produce the hexadecimal values shown.
 */
double __attribute__((optimize("-fno-unsafe-math-optimizations"))) __hide_ieee754_pow(double x, double y)
{
	double z,ax,z_h,z_l,p_h,p_l;
	double y1,t1,t2,r,s,t,u,v,w;
	int i,j,k,yisint,n;
	int hx,hy,ix,iy;
	unsigned lx,ly;

	hx = GET_HI(x); lx = GET_LO(x);
	hy = GET_HI(y); ly = GET_LO(y);
	ix = hx&0x7fffffff;  iy = hy&0x7fffffff;

    /* y==zero: x**0 = 1 */
	if((iy|ly)==0) return one; 	
	if(x == 1.0) return one;

    /* +-NaN return x+y */
	if(ix > 0x7ff00000 || ((ix==0x7ff00000)&&(lx!=0)) ||
	   iy > 0x7ff00000 || ((iy==0x7ff00000)&&(ly!=0))) 
        return __builtin_nan("");

    /* determine if y is an odd int when x < 0
     * yisint = 0	... y is not an integer
     * yisint = 1	... y is an odd int
     * yisint = 2	... y is an even int
     */
	yisint  = 0;
	if(hx<0) {	
	    if(iy>=0x43400000) yisint = 2; /* even integer y */
	    else if(iy>=0x3ff00000) {
		k = (iy>>20)-0x3ff;	   /* exponent */
		if(k>20) {
		    j = ly>>(52-k);
		    if((j<<(52-k))==ly) yisint = 2-(j&1);
		} else if(ly==0) {
		    j = iy>>(20-k);
		    if((j<<(20-k))==iy) yisint = 2-(j&1);
		}
	    }		
	} 

    /* special value of y */
	if(ly==0) { 	
	    if (iy==0x7ff00000) {	/* y is +-inf */
	        if(((ix-0x3ff00000)|lx)==0)
		    return  one;	/* +-1**inf is 1 */
	        else if (ix >= 0x3ff00000)/* (|x|>1)**+-inf = inf,0 */
		    return (hy>=0)? y: zero;
	        else			/* (|x|<1)**-,+inf = inf,0 */
		    return (hy<0)?-y: zero;
	    } 
	    if(iy==0x3ff00000) {	/* y is  +-1 */
		if(hy<0) return one/x; else return x;
	    }
	    if(hy==0x40000000) return x*x; /* y is  2 */
	    if(hy==0x3fe00000) {	/* y is  0.5 */
		if(hx>=0)	/* x >= +0 */
        return sqrt(x);
	    }
	}

    ax   = fabs(x);
    /* special value of x */
	if(lx==0) {
	    if(ix==0x7ff00000||ix==0||ix==0x3ff00000){
		z = ax;			/*x is +-0,+-inf,+-1*/
		if(hy<0) z = one/z;	/* z = (1/|x|) */
		if(hx<0) {
		    if(((ix-0x3ff00000)|yisint)==0) {
			z = (z-z)/(z-z); /* (-1)**non-int is NaN */
		    } else if(yisint==1) 
			z = -z;		/* (x<0)**odd = -(|x|**odd) */
		}
		return z;
	    }
	}
    
	n = (hx>>31)+1;

    /* (x<0)**(non-int) is NaN */
    if((n|yisint)==0) return __builtin_nan("");

	s = one; /* s (sign of result -ve**odd) = -1 else = 1 */
	if((n|(yisint-1))==0) s = -one;/* (-ve)**(odd int) */

    /* |y| is huge */
	if(iy>0x41e00000) { /* if |y| > 2**31 */
	    if(iy>0x43f00000){	/* if |y| > 2**64, must o/uflow */
		if(ix<=0x3fefffff) return (hy<0)? huge*huge:tiny*tiny;
		if(ix>=0x3ff00000) return (hy>0)? huge*huge:tiny*tiny;
	    }
	/* over/underflow if x is not close to one */
	    if(ix<0x3fefffff) return (hy<0)? s*huge*huge:s*tiny*tiny;
	    if(ix>0x3ff00000) return (hy>0)? s*huge*huge:s*tiny*tiny;
	/* now |1-x| is tiny <= 2**-20, suffice to compute 
	   log(x) by x-x^2/2+x^3/3-x^4/4 */
	    t = ax-one;		/* t has 20 trailing zeros */
	    w = (t*t)*(0.5-t*(0.3333333333333333333333-t*0.25));
	    u = ivln2_h*t;	/* ivln2_h has 21 sig. bits */
	    v = t*ivln2_l-w*ivln2;
	    t1 = u+v;
	    SET_LOW_WORD(t1, 0);
	    t2 = v-(t1-u);
	} else {
	    double ss,s2,s_h,s_l,t_h,t_l;
	    n = 0;
	/* take care subnormal number */
	    if(ix<0x00100000)
		{ax *= two53; n -= 53; ix = GET_HI(ax); }
	    n  += ((ix)>>20)-0x3ff;
	    j  = ix&0x000fffff;
	/* determine interval */
	    ix = j|0x3ff00000;		/* normalize ix */
	    if(j<=0x3988E) k=0;		/* |x|<sqrt(3/2) */
	    else if(j<0xBB67A) k=1;	/* |x|<sqrt(3)   */
	    else {k=0;n+=1;ix -= 0x00100000;}
	    SET_HIGH_WORD(ax, ix);

	/* compute ss = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
	    u = ax-bp[k];		/* bp[0]=1.0, bp[1]=1.5 */
	    v = one/(ax+bp[k]);
	    ss = u*v;
	    s_h = ss;
	    SET_LOW_WORD(s_h, 0);
	/* t_h=ax+bp[k] High */
	    t_h = zero;
	    SET_HIGH_WORD(t_h, (((ix>>1)|0x20000000)+0x00080000+(k<<18))); 
	    t_l = ax - (t_h-bp[k]);
	    s_l = v*((u-s_h*t_h)-s_h*t_l);
	/* compute log(ax) */
	    s2 = ss*ss;
	    r = s2*s2*(L1+s2*(L2+s2*(L3+s2*(L4+s2*(L5+s2*L6)))));
	    r += s_l*(s_h+ss);
	    s2  = s_h*s_h;
	    t_h = 3.0+s2+r;
	    SET_LOW_WORD(t_h, 0);
	    t_l = r-((t_h-3.0)-s2);
	/* u+v = ss*(1+...) */
	    u = s_h*t_h;
	    v = s_l*t_h+t_l*ss;
	/* 2/(3log2)*(ss+...) */
	    p_h = u+v;
	    SET_LOW_WORD(p_h, 0);
	    p_l = v-(p_h-u);
	    z_h = cp_h*p_h;		/* cp_h+cp_l = 2/(3*log2) */
	    z_l = cp_l*p_h+p_l*cp+dp_l[k];
	/* log2(ax) = (ss+..)*2/(3*log2) = n + dp_h + z_h + z_l */
	    t = (double)n;
	    t1 = (((z_h+z_l)+dp_h[k])+t);
	    SET_LOW_WORD(t1, 0);
	    t2 = z_l-(((t1-t)-dp_h[k])-z_h);
	}

    /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
	y1  = y;
	SET_LOW_WORD(y1, 0);
	p_l = (y-y1)*t1+y*t2;
	p_h = y1*t1;
	z = p_l+p_h;
	j = GET_HI(z);
	i = GET_LO(z);
	if (j>=0x40900000) {				/* z >= 1024 */
	    if(((j-0x40900000)|i)!=0)			/* if z > 1024 */
		return s*huge*huge;			/* overflow */
	    else {
		if(p_l+ovt>z-p_h) return s*huge*huge;	/* overflow */
	    }
	} else if((j&0x7fffffff)>=0x4090cc00 ) {	/* z <= -1075 */
	    if(((j-0xc090cc00)|i)!=0) 		/* z < -1075 */
		return s*tiny*tiny;		/* underflow */
	    else {
		if(p_l<=z-p_h) return s*tiny*tiny;	/* underflow */
	    }
	}
    /*
     * compute 2**(p_h+p_l)
     */
	i = j&0x7fffffff;
	k = (i>>20)-0x3ff;
	n = 0;
	if(i>0x3fe00000) {		/* if |z| > 0.5, set n = [z+0.5] */
	    n = j+(0x00100000>>(k+1));
	    k = ((n&0x7fffffff)>>20)-0x3ff;	/* new k for n */
	    t = zero;
	    SET_HIGH_WORD(t, (n&~(0x000fffff>>k)));
	    n = ((n&0x000fffff)|0x00100000)>>(20-k);
	    if(j<0) n = -n;
	    p_h -= t;
	} 
	t = p_l+p_h;
	SET_LOW_WORD(t, 0);
	u = t*lg2_h;
	v = (p_l-(t-p_h))*lg2+t*lg2_l;
	z = u+v;
	w = v-(z-u);
	t  = z*z;
	t1  = z - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
	r  = (z*t1)/(t1-two)-(w+z*w);
	z  = one-(r-z);
	j  = GET_HI(z);
	j += (n<<20);
    if((j>>20)<=0) z = scalbn(z,n);	/* subnormal output */
	else 
	{
	  SET_HIGH_WORD(z, (GET_HI(z) + (n<<20)));
	}
	return s*z;
}

/* 
 * wrapper pow(x,y) return x**y
 */
double pow(double x, double y)	/* wrapper pow */
{
#ifdef _IEEE_LIBM
	return  __hide_ieee754_pow(x,y);
#else
	double z;
	z=__hide_ieee754_pow(x,y);
    if(_LIB_VERSION == _IEEE_|| isnan(y)) return z;
    if(isnan(x)) {
	    if(y==0.0) 
	        return __hide_kernel_standard(x,y,42); /* pow(NaN,0.0) */
	    else 
		return z;
	}
	if(x==0.0){ 
	    if(y==0.0)
	        return __hide_kernel_standard(x,y,20); /* pow(0.0,0.0) */
	    if(__finite(y)&&y<0.0)
	        return __hide_kernel_standard(x,y,23); /* pow(0.0,negative) */
	    return z;
	}
	if(!__finite(z)) {
	    if(__finite(x)&&__finite(y)) {
            if(isnan(z))
	            return __hide_kernel_standard(x,y,24); /* pow neg**non-int */
	        else 
	            return __hide_kernel_standard(x,y,21); /* pow overflow */
	    }
	} 
	if(z==0.0&&__finite(x)&&__finite(y))
	    return __hide_kernel_standard(x,y,22); /* pow underflow */
	return z;
#endif
}
