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
 *
 */

#include "math_private.h"

extern int signgam;

/* __ieee754_lgamma_r(x, signgamp)
 * Reentrant version of the logarithm of the Gamma function
 * with user provide pointer for the sign of Gamma(x).
 *
 * Method:
 *   1. Argument Reduction for 0 < x <= 8
 * 	Since gamma(1+s)=s*gamma(s), for x in [0,8], we may
 * 	reduce x to a number in [1.5,2.5] by
 * 		lgamma(1+s) = log(s) + lgamma(s)
 *	for example,
 *		lgamma(7.3) = log(6.3) + lgamma(6.3)
 *			    = log(6.3*5.3) + lgamma(5.3)
 *			    = log(6.3*5.3*4.3*3.3*2.3) + lgamma(2.3)
 *   2. Polynomial approximation of lgamma around its
 *	minimun ymin=1.461632144968362245 to maintain monotonicity.
 *	On [ymin-0.23, ymin+0.27] (i.e., [1.23164,1.73163]), use
 *		Let z = x-ymin;
 *		lgamma(x) = -1.214862905358496078218 + z^2*poly(z)
 *	where
 *		poly(z) is a 14 degree polynomial.
 *   2. Rational approximation in the primary interval [2,3]
 *	We use the following approximation:
 *		s = x-2.0;
 *		lgamma(x) = 0.5*s + s*P(s)/Q(s)
 *	with accuracy
 *		|P/Q - (lgamma(x)-0.5s)| < 2**-61.71
 *	Our algorithms are based on the following observation
 *
 *                             zeta(2)-1    2    zeta(3)-1    3
 * lgamma(2+s) = s*(1-Euler) + --------- * s  -  --------- * s  + ...
 *                                 2                 3
 *
 *	where Euler = 0.5771... is the Euler constant, which is very
 *	close to 0.5.
 *
 *   3. For x>=8, we have
 *	lgamma(x)~(x-0.5)log(x)-x+0.5*log(2pi)+1/(12x)-1/(360x**3)+....
 *	(better formula:
 *	   lgamma(x)~(x-0.5)*(log(x)-1)-.5*(log(2pi)-1) + ...)
 *	Let z = 1/x, then we approximation
 *		f(z) = lgamma(x) - (x-0.5)(log(x)-1)
 *	by
 *	  			    3       5             11
 *		w = w0 + w1*z + w2*z  + w3*z  + ... + w6*z
 *	where
 *		|w - f(z)| < 2**-58.74
 *
 *   4. For negative x, since (G is gamma function)
 *		-x*G(-x)*G(x) = pi/sin(pi*x),
 * 	we have
 * 		G(x) = pi/(sin(pi*x)*(-x)*G(-x))
 *	since G(-x) is positive, sign(G(x)) = sign(sin(pi*x)) for x<0
 *	Hence, for x<0, signgam = sign(sin(pi*x)) and
 *		lgamma(x) = log(|Gamma(x)|)
 *			  = log(pi/(|x*sin(pi*x)|)) - lgamma(-x);
 *	Note: one should avoid compute pi*(-x) directly in the
 *	      computation of sin(pi*(-x)).
 *
 *   5. Special Cases
 *		lgamma(2+s) ~ s*(1-Euler) for tiny s
 *		lgamma(1)=lgamma(2)=0
 *		lgamma(x) ~ -log(x) for tiny x
 *		lgamma(0) = lgamma(inf) = inf
 *	 	lgamma(-integer) = +-inf
 *
 */

static double __hide_sin_pi(double x)
{
   double y, z;
   int n, ix;

   ix = 0x7fffffff & GET_HI(x);

   if(ix < 0x3fd00000)
      return __hide_kernel_sin(pi * x, zero, 0);
   y = -x; /* x is assume negative */

   /*
    * argument reduction, make sure inexact flag not raised if input
    * is an integer
    */
   z = floor(y);
   if(z != y)
   { /* inexact anyway */
      y *= 0.5;
      y = 2.0 * (y - floor(y)); /* y = |x| mod 2.0 */
      n = (int)(y * 4.0);
   }
   else
   {
      if(ix >= 0x43400000)
      {
         y = zero;
         n = 0; /* y must be even */
      }
      else
      {
         if(ix < 0x43300000)
            z = y + two52;  /* exact */
         n = GET_LO(z) & 1; /* lower word of z */
         y = n;
         n <<= 2;
      }
   }
   switch(n)
   {
      case 0:
         y = __hide_kernel_sin(pi * y, zero, 0);
         break;
      case 1:
      case 2:
         y = __hide_kernel_cos(pi * (0.5 - y), zero);
         break;
      case 3:
      case 4:
         y = __hide_kernel_sin(pi * (one - y), zero, 0);
         break;
      case 5:
      case 6:
         y = -__hide_kernel_cos(pi * (y - 1.5), zero);
         break;
      default:
         y = __hide_kernel_sin(pi * (y - 2.0), zero, 0);
         break;
   }
   return -y;
}

static double __hide_ieee754_lgamma_r(double x, int* signgamp)
{
   double t, y, z, nadj = 0.0, p, p1, p2, p3, q, r, w;
   int i, hx, lx, ix;

   hx = GET_HI(x);
   lx = GET_LO(x);

   /* purge off +-inf, NaN, +-0, and negative arguments */
   *signgamp = 1;
   ix = hx & 0x7fffffff;
   if(ix == 0x7ff00000 && lx == 0)
      return __builtin_inf();
   if(ix >= 0x7ff00000)
      return __builtin_nan("");
   if((ix | lx) == 0)
   {
      if(hx < 0)
         *signgamp = -1;
      return __builtin_inf();
   }
   if(ix < 0x3b900000)
   { /* |x|<2**-70, return -log(|x|) */
      if(hx < 0)
      {
         *signgamp = -1;
         return -__hide_ieee754_log(-x);
      }
      else
         return -__hide_ieee754_log(x);
   }
   if(hx < 0)
   {
      if(ix >= 0x43300000) /* |x|>=2**52, must be -integer */
         return __builtin_inf();
      t = __hide_sin_pi(x);
      if(t == zero)
         return one / zero; /* -integer */
      nadj = __hide_ieee754_log(pi / fabs(t * x));
      if(t < zero)
         *signgamp = -1;
      x = -x;
   }

   /* purge off 1 and 2 */
   if((((ix - 0x3ff00000) | lx) == 0) || (((ix - 0x40000000) | lx) == 0))
      r = 0;
   /* for x < 2.0 */
   else if(ix < 0x40000000)
   {
      if(ix <= 0x3feccccc)
      { /* lgamma(x) = lgamma(x+1)-log(x) */
         r = -__hide_ieee754_log(x);
         if(ix >= 0x3FE76944)
         {
            y = one - x;
            i = 0;
         }
         else if(ix >= 0x3FCDA661)
         {
            y = x - (tc - one);
            i = 1;
         }
         else
         {
            y = x;
            i = 2;
         }
      }
      else
      {
         r = zero;
         if(ix >= 0x3FFBB4C3)
         {
            y = 2.0 - x;
            i = 0;
         } /* [1.7316,2] */
         else if(ix >= 0x3FF3B4C4)
         {
            y = x - tc;
            i = 1;
         } /* [1.23,1.73] */
         else
         {
            y = x - one;
            i = 2;
         }
      }
      switch(i)
      {
         case 0:
            z = y * y;
            p1 = a0 + z * (a2 + z * (a4 + z * (a6 + z * (a8 + z * a10))));
            p2 = z * (a1 + z * (a3 + z * (a5 + z * (a7 + z * (a9 + z * a11)))));
            p = y * p1 + p2;
            r += (p - 0.5 * y);
            break;
         case 1:
            z = y * y;
            w = z * y;
            p1 = t0 + w * (t3 + w * (t6 + w * (t9 + w * t12))); /* parallel comp */
            p2 = t1 + w * (t4 + w * (t7 + w * (t10 + w * t13)));
            p3 = t2 + w * (t5 + w * (t8 + w * (t11 + w * t14)));
            p = z * p1 - (tt - w * (p2 + y * p3));
            r += (tf + p);
            break;
         case 2:
            p1 = y * (u0 + y * (u1 + y * (u2 + y * (u3 + y * (u4 + y * u5)))));
            p2 = one + y * (v1 + y * (v2 + y * (v3 + y * (v4 + y * v5))));
            r += (-0.5 * y + p1 / p2);
      }
   }
   else if(ix < 0x40200000)
   { /* x < 8.0 */
      i = (int)x;
      t = zero;
      y = x - (double)i;
      p = y * (s0 + y * (s1 + y * (s2 + y * (s3 + y * (s4 + y * (s5 + y * s6))))));
      q = one + y * (r1 + y * (r2 + y * (r3 + y * (r4 + y * (r5 + y * r6)))));
      r = half * y + p / q;
      z = one; /* lgamma(1+s) = log(s) + lgamma(s) */
      switch(i)
      {
         case 7:
            z *= (y + 6.0); /* FALLTHRU */
         case 6:
            z *= (y + 5.0); /* FALLTHRU */
         case 5:
            z *= (y + 4.0); /* FALLTHRU */
         case 4:
            z *= (y + 3.0); /* FALLTHRU */
         case 3:
            z *= (y + 2.0); /* FALLTHRU */
            r += __hide_ieee754_log(z);
            break;
      }
      /* 8.0 <= x < 2**58 */
   }
   else if(ix < 0x43900000)
   {
      t = __hide_ieee754_log(x);
      z = one / x;
      y = z * z;
      w = w0 + z * (w1 + y * (w2 + y * (w3 + y * (w4 + y * (w5 + y * w6)))));
      r = (x - half) * (t - one) + w;
   }
   else
      /* 2**58 <= x <= inf */
      r = x * (__hide_ieee754_log(x) - one);
   if(hx < 0)
      r = nadj - r;
   return r;
}

/* double gamma(double x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call gamma_r
 */
double gamma(double x)
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_lgamma_r(x, &signgam);
#else
   double y;
   y = __hide_ieee754_lgamma_r(x, &signgam);
   if(_LIB_VERSION == _IEEE_)
      return y;
   if(!finite(y) && finite(x))
   {
      if(floor(x) == x && x <= 0.0)
         return __hide_kernel_standard(x, x, 41); /* gamma pole */
      else
         return __hide_kernel_standard(x, x, 40); /* gamma overflow */
   }
   else
      return y;
#endif
}

/*
 * wrapper double gamma_r(double x, int *signgamp)
 */
double gamma_r(double x, int* signgamp) /* wrapper lgamma_r */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_lgamma_r(x, signgamp);
#else
   double y;
   y = __hide_ieee754_lgamma_r(x, signgamp);
   if(_LIB_VERSION == _IEEE_)
      return y;
   if(!(y) && (x))
   {
      if(floor(x) == x && x <= 0.0)
         return __hide_kernel_standard(x, x, 41); /* gamma pole */
      else
         return __hide_kernel_standard(x, x, 40); /* gamma overflow */
   }
   else
      return y;
#endif
}

/* double lgamma(double x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgamma_r
 */
double lgamma(double x)
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_lgamma_r(x, &signgam);
#else
   double y;
   y = __hide_ieee754_lgamma_r(x, &signgam);
   if(_LIB_VERSION == _IEEE_)
      return y;
   if(!(y) && (x))
   {
      if(floor(x) == x && x <= 0.0)
         return __hide_kernel_standard(x, x, 15); /* lgamma pole */
      else
         return __hide_kernel_standard(x, x, 14); /* lgamma overflow */
   }
   else
      return y;
#endif
}

/*
 * wrapper double lgamma_r(double x, int *signgamp)
 */
double lgamma_r(double x, int* signgamp) /* wrapper lgamma_r */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_lgamma_r(x, signgamp);
#else
   double y;
   y = __hide_ieee754_lgamma_r(x, signgamp);
   if(_LIB_VERSION == _IEEE_)
      return y;
   if(!(y) && (x))
   {
      if(floor(x) == x && x <= 0.0)
         return __hide_kernel_standard(x, x, 15); /* lgamma pole */
      else
         return __hide_kernel_standard(x, x, 14); /* lgamma overflow */
   }
   else
      return y;
#endif
}

/* double gamma(double x)
 * Return  the logarithm of the Gamma function of x or the Gamma function of x,
 * depending on the library mode.
 */
double tgamma(double x)
{
   int hx;
   unsigned lx;

   hx = GET_HI(x);
   lx = GET_LO(x);

   if(__builtin_expect(((hx & 0x7fffffff) | lx) == 0, 0))
   {
      /* Return value for x == 0 is Inf with divide by zero exception.  */
      return 1.0 / x;
   }
   if(__builtin_expect(hx < 0, 0) && (unsigned)hx < 0xfff00000 && rint(x) == x)
   {
      /* Return value for integer x < 0 is NaN with invalid exception.  */
      return (x - x) / (x - x);
   }
   if(__builtin_expect((unsigned int)hx == 0xfff00000 && lx == 0, 0))
   {
      /* x == -Inf.  According to ISO this is NaN.  */
      return x - x;
   }
   if(__builtin_expect((hx & 0x7ff00000) == 0x7ff00000, 0))
   {
      /* Positive infinity (return positive infinity) or NaN (return
      NaN).  */
      return X_PLUS_X(x);
   }
   double y;
   int local_signgam = 0;
   y = __hide_ieee754_exp(__hide_ieee754_lgamma_r(x, &local_signgam));
   if(local_signgam < 0)
      y = -y;
#ifdef _IEEE_LIBM
   return y;
#else
   if(_LIB_VERSION == _IEEE_)
      return y;

   if(!(y) && (x))
   {
      if(floor(x) == x && x <= 0.0)
         return __hide_kernel_standard(x, x, 41); /* tgamma pole */
      else
         return __hide_kernel_standard(x, x, 40); /* tgamma overflow */
   }
   return y;
#endif
}
