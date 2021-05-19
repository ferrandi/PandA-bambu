/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/*
 * fdlibm kernel functions
 * files k_cos.c, k_rem_pio2.c, k_sin.c, k_standard.c, k_tan.c have been merged in this file
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
/*
 * __hide_kernel_rem_pio2(x,y,e0,nx,prec,ipio2)
 * double x[],y[]; int e0,nx,prec; int ipio2[];
 *
 * __hide_kernel_rem_pio2 return the last three digits of N with
 *		y = x - N*pi/2
 * so that |y| < pi/2.
 *
 * The method is to compute the integer (mod 8) and fraction parts of
 * (2/pi)*x without doing the full multiplication. In general we
 * skip the part of the product that are known to be a huge integer (
 * more accurately, = 0 mod 8 ). Thus the number of operations are
 * independent of the exponent of the input.
 *
 * (2/pi) is represented by an array of 24-bit integers in ipio2[].
 *
 * Input parameters:
 * 	x[]	The input value (must be positive) is broken into nx
 *		pieces of 24-bit integers in double precision format.
 *		x[i] will be the i-th 24 bit of x. The scaled exponent
 *		of x[0] is given in input parameter e0 (i.e., x[0]*2^e0
 *		match x's up to 24 bits.
 *
 *		Example of breaking a double positive z into x[0]+x[1]+x[2]:
 *			e0 = ilogb(z)-23
 *			z  = scalbn(z,-e0)
 *		for i = 0,1,2
 *			x[i] = floor(z)
 *			z    = (z-x[i])*2**24
 *
 *
 *	y[]	ouput result in an array of double precision numbers.
 *		The dimension of y[] is:
 *			24-bit  precision	1
 *			53-bit  precision	2
 *			64-bit  precision	2
 *			113-bit precision	3
 *		The actual value is the sum of them. Thus for 113-bit
 *		precison, one may have to do something like:
 *
 *		long double t,w,r_head, r_tail;
 *		t = (long double)y[2] + (long double)y[1];
 *		w = (long double)y[0];
 *		r_head = t+w;
 *		r_tail = w - (r_head - t);
 *
 *	e0	The exponent of x[0]
 *
 *	nx	dimension of x[]
 *
 *  	prec	an integer indicating the precision:
 *			0	24  bits (single)
 *			1	53  bits (double)
 *			2	64  bits (extended)
 *			3	113 bits (quad)
 *
 *	ipio2[]
 *		integer array, contains the (24*i)-th to (24*i+23)-th
 *		bit of 2/pi after binary point. The corresponding
 *		floating value is
 *
 *			ipio2[i] * 2^(-24(i+1)).
 *
 * External function:
 *	double scalbn(), floor();
 *
 *
 * Here is the description of some local variables:
 *
 * 	jk	jk+1 is the initial number of terms of ipio2[] needed
 *		in the computation. The recommended value is 2,3,4,
 *		6 for single, double, extended,and quad.
 *
 * 	jz	local integer variable indicating the number of
 *		terms of ipio2[] used.
 *
 *	jx	nx - 1
 *
 *	jv	index for pointing to the suitable ipio2[] for the
 *		computation. In general, we want
 *			( 2^e0*x[0] * ipio2[jv-1]*2^(-24jv) )/8
 *		is an integer. Thus
 *			e0-3-24*jv >= 0 or (e0-3)/24 >= jv
 *		Hence jv = max(0,(e0-3)/24).
 *
 *	jp	jp+1 is the number of terms in PIo2[] needed, jp = jk.
 *
 * 	q[]	double array with integral value, representing the
 *		24-bits chunk of the product of x and 2/pi.
 *
 *	q0	the corresponding exponent of q[0]. Note that the
 *		exponent for q[i] would be q0-24*i.
 *
 *	PIo2[]	double precision array, obtained by cutting pi/2
 *		into 24 bits chunks.
 *
 *	f[]	ipio2[] in floating point
 *
 *	iq[]	integer array by breaking up q[] in 24-bits chunk.
 *
 *	fq[]	final product of x*(2/pi) in fq[0],..,fq[jk]
 *
 *	ih	integer. If >0 it indicates q[] is >= 0.5, hence
 *		it also indicates the *sign* of the result.
 *
 */
static inline int __hide_kernel_rem_pio2(double* x, double* y, int e0, int nx, int prec, const int* ipio2)
{
   static const int init_jk[] = {2, 3, 4, 6}; /* initial value for jk */

   static const double PIo2[] = {
       1.57079625129699707031e+00, /* 0x3FF921FB, 0x40000000 */
       7.54978941586159635335e-08, /* 0x3E74442D, 0x00000000 */
       5.39030252995776476554e-15, /* 0x3CF84698, 0x80000000 */
       3.28200341580791294123e-22, /* 0x3B78CC51, 0x60000000 */
       1.27065575308067607349e-29, /* 0x39F01B83, 0x80000000 */
       1.22933308981111328932e-36, /* 0x387A2520, 0x40000000 */
       2.73370053816464559624e-44, /* 0x36E38222, 0x80000000 */
       2.16741683877804819444e-51, /* 0x3569F31D, 0x00000000 */
   };
   int jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
   double z, fw, f[20], fq[20], q[20];

   /* initialize jk*/
   jk = init_jk[prec];
   jp = jk;

   /* determine jx,jv,q0, note that 3>q0 */
   jx = nx - 1;
   jv = (e0 - 3) / 24;
   if(jv < 0)
      jv = 0;
   q0 = e0 - 24 * (jv + 1);

   /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
   j = jv - jx;
   m = jx + jk;
   for(i = 0; i <= m; i++, j++)
      f[i] = (j < 0) ? zero : (double)ipio2[j];

   /* compute q[0],q[1],...q[jk] */
   for(i = 0; i <= jk; i++)
   {
      for(j = 0, fw = 0.0; j <= jx; j++)
         fw += x[j] * f[jx + i - j];
      q[i] = fw;
   }

   jz = jk;
recompute:
   /* distill q[] into iq[] reversingly */
   for(i = 0, j = jz, z = q[jz]; j > 0; i++, j--)
   {
      fw = (double)((int)(twon24 * z));
      iq[i] = (int)(z - two24 * fw);
      z = q[j - 1] + fw;
   }

   /* compute n */
   z = scalbn(z, q0);           /* actual value of z */
   z -= 8.0 * floor(z * 0.125); /* trim off integer >= 8 */
   n = (int)z;
   z -= (double)n;
   ih = 0;
   if(q0 > 0)
   { /* need iq[jz-1] to determine n */
      i = (iq[jz - 1] >> (24 - q0));
      n += i;
      iq[jz - 1] -= i << (24 - q0);
      ih = iq[jz - 1] >> (23 - q0);
   }
   else if(q0 == 0)
      ih = iq[jz - 1] >> 23;
   else if(z >= 0.5)
      ih = 2;

   if(ih > 0)
   { /* q > 0.5 */
      n += 1;
      carry = 0;
      for(i = 0; i < jz; i++)
      { /* compute 1-q */
         j = iq[i];
         if(carry == 0)
         {
            if(j != 0)
            {
               carry = 1;
               iq[i] = 0x1000000 - j;
            }
         }
         else
            iq[i] = 0xffffff - j;
      }
      if(q0 > 0)
      { /* rare case: chance is 1 in 12 */
         switch(q0)
         {
            case 1:
               iq[jz - 1] &= 0x7fffff;
               break;
            case 2:
               iq[jz - 1] &= 0x3fffff;
               break;
         }
      }
      if(ih == 2)
      {
         z = one - z;
         if(carry != 0)
            z -= scalbn(one, q0);
      }
   }

   /* check if recomputation is needed */
   if(z == zero)
   {
      j = 0;
      for(i = jz - 1; i >= jk; i--)
         j |= iq[i];
      if(j == 0)
      { /* need recomputation */
         for(k = 1; iq[jk - k] == 0; k++)
            ; /* k = no. of terms needed */

         for(i = jz + 1; i <= jz + k; i++)
         { /* add q[jz+1] to q[jz+k] */
            f[jx + i] = (double)ipio2[jv + i];
            for(j = 0, fw = 0.0; j <= jx; j++)
               fw += x[j] * f[jx + i - j];
            q[i] = fw;
         }
         jz += k;
         goto recompute;
      }
   }

   /* chop off zero terms */
   if(z == 0.0)
   {
      jz -= 1;
      q0 -= 24;
      while(iq[jz] == 0)
      {
         jz--;
         q0 -= 24;
      }
   }
   else
   { /* break z into 24-bit if necessary */
      z = scalbn(z, -q0);
      if(z >= two24)
      {
         fw = (double)((int)(twon24 * z));
         iq[jz] = (int)(z - two24 * fw);
         jz += 1;
         q0 += 24;
         iq[jz] = (int)fw;
      }
      else
         iq[jz] = (int)z;
   }

   /* convert integer "bit" chunk to floating-point value */
   fw = scalbn(one, q0);
   for(i = jz; i >= 0; i--)
   {
      q[i] = fw * (double)iq[i];
      fw *= twon24;
   }

   /* compute PIo2[0,...,jp]*q[jz,...,0] */
   for(i = jz; i >= 0; i--)
   {
      for(fw = 0.0, k = 0; k <= jp && k <= jz - i; k++)
         fw += PIo2[k] * q[i + k];
      fq[jz - i] = fw;
   }

   /* compress fq[] into y[] */
   switch(prec)
   {
      case 0:
         fw = 0.0;
         for(i = jz; i >= 0; i--)
            fw += fq[i];
         y[0] = (ih == 0) ? fw : -fw;
         break;
      case 1:
      case 2:
         fw = 0.0;
         for(i = jz; i >= 0; i--)
            fw += fq[i];
         y[0] = (ih == 0) ? fw : -fw;
         fw = fq[0] - fw;
         for(i = 1; i <= jz; i++)
            fw += fq[i];
         y[1] = (ih == 0) ? fw : -fw;
         break;
      case 3: /* painful */
         for(i = jz; i > 0; i--)
         {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
         }
         for(i = jz; i > 1; i--)
         {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
         }
         for(fw = 0.0, i = jz; i >= 2; i--)
            fw += fq[i];
         if(ih == 0)
         {
            y[0] = fq[0];
            y[1] = fq[1];
            y[2] = fw;
         }
         else
         {
            y[0] = -fq[0];
            y[1] = -fq[1];
            y[2] = -fw;
         }
   }
   return n & 7;
}

/* __hide_ieee754_rem_pio2(x,y)
 *
 * return the remainder of x rem pi/2 in y[0]+y[1]
 * use __hide_kernel_rem_pio2()
 */
int __hide_ieee754_rem_pio2(double x, double* y)
{
   /*
    * Table of constants for 2/pi, 396 Hex digits (476 decimal) of 2/pi
    */
   static const int two_over_pi[] = {
       0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62, 0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A, 0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129, 0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C,
       0x7026B4, 0x5F7E41, 0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8, 0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF, 0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5, 0xF17B3D, 0x0739F7,
       0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08, 0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3, 0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880, 0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
   };
   static const int npio2_hw[] = {
       0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C, 0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C, 0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB,
       0x403AB41B, 0x403C463A, 0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C, 0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB, 0x404858EB, 0x404921FB,
   };

   double z = 0.0, w, t, r, fn;
   double tx[3];
   int e0, i, j, nx, n, ix, hx;

   hx = GET_HI(x); /* high word of x */
   ix = hx & 0x7fffffff;
   if(ix <= 0x3fe921fb) /* |x| ~<= pi/4 , no need for reduction */
   {
      y[0] = x;
      y[1] = 0;
      return 0;
   }
   if(ix < 0x4002d97c)
   { /* |x| < 3pi/4, special case with n=+-1 */
      if(hx > 0)
      {
         z = x - pio2_1;
         if(ix != 0x3ff921fb)
         { /* 33+53 bit pi is good enough */
            y[0] = z - pio2_1t;
            y[1] = (z - y[0]) - pio2_1t;
         }
         else
         { /* near pi/2, use 33+33+53 bit pi */
            z -= pio2_2;
            y[0] = z - pio2_2t;
            y[1] = (z - y[0]) - pio2_2t;
         }
         return 1;
      }
      else
      { /* negative x */
         z = x + pio2_1;
         if(ix != 0x3ff921fb)
         { /* 33+53 bit pi is good enough */
            y[0] = z + pio2_1t;
            y[1] = (z - y[0]) + pio2_1t;
         }
         else
         { /* near pi/2, use 33+33+53 bit pi */
            z += pio2_2;
            y[0] = z + pio2_2t;
            y[1] = (z - y[0]) + pio2_2t;
         }
         return -1;
      }
   }
   if(ix <= 0x413921fb)
   { /* |x| ~<= 2^19*(pi/2), medium size */
      t = fabs(x);
      n = (int)(t * invpio2 + half);
      fn = (double)n;
      r = t - fn * pio2_1;
      w = fn * pio2_1t; /* 1st round good to 85 bit */
      if(n < 32 && ix != npio2_hw[n - 1])
      {
         y[0] = r - w; /* quick check no cancellation */
      }
      else
      {
         j = ix >> 20;
         y[0] = r - w;
         i = j - (((GET_HI(y[0])) >> 20) & 0x7ff);
         if(i > 16)
         { /* 2nd iteration needed, good to 118 */
            t = r;
            w = fn * pio2_2;
            r = t - w;
            w = fn * pio2_2t - ((t - r) - w);
            y[0] = r - w;
            i = j - (((GET_HI(y[0])) >> 20) & 0x7ff);
            if(i > 49)
            {         /* 3rd iteration need, 151 bits acc */
               t = r; /* will cover all possible cases */
               w = fn * pio2_3;
               r = t - w;
               w = fn * pio2_3t - ((t - r) - w);
               y[0] = r - w;
            }
         }
      }
      y[1] = (r - y[0]) - w;
      if(hx < 0)
      {
         y[0] = -y[0];
         y[1] = -y[1];
         return -n;
      }
      else
         return n;
   }
   /*
    * all other (large) arguments
    */
   if(ix >= 0x7ff00000)
   { /* x is inf or NaN */
      y[0] = y[1] = __builtin_nan("");
      return 0;
   }
   /* set z = scalbn(|x|,ilogb(x)-23) */
   SET_LOW_WORD(z, GET_LO(x));
   e0 = (ix >> 20) - 1046; /* e0 = ilogb(z)-23; */
   SET_HIGH_WORD(z, ix - (e0 << 20));
   for(i = 0; i < 2; i++)
   {
      tx[i] = (double)((int)(z));
      z = (z - tx[i]) * two24;
   }
   tx[2] = z;
   nx = 3;
   while(tx[nx - 1] == zero)
      nx--; /* skip zero term */
   n = __hide_kernel_rem_pio2(tx, y, e0, nx, 2, two_over_pi);
   if(hx < 0)
   {
      y[0] = -y[0];
      y[1] = -y[1];
      return -n;
   }
   return n;
}

/*
 * __hide_kernel_cos( x,  y )
 * kernel cos function on [-pi/4, pi/4], pi/4 ~ 0.785398164
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 *
 * Algorithm
 *	1. Since cos(-x) = cos(x), we need only to consider positive x.
 *	2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
 *	3. cos(x) is approximated by a polynomial of degree 14 on
 *	   [0,pi/4]
 *		  	                 4            14
 *	   	cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
 *	   where the remez error is
 *
 * 	|              2     4     6     8     10    12     14 |     -58
 * 	|cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
 * 	|    					               |
 *
 * 	               4     6     8     10    12     14
 *	4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
 *	       cos(x) = 1 - x*x/2 + r
 *	   since cos(x+y) ~ cos(x) - sin(x)*y
 *			  ~ cos(x) - x*y,
 *	   a correction term is necessary in cos(x) and hence
 *		cos(x+y) = 1 - (x*x/2 - (r - x*y))
 *	   For better accuracy when x > 0.3, let qx = |x|/4 with
 *	   the last 32 bits mask off, and if x > 0.78125, let qx = 0.28125.
 *	   Then
 *		cos(x+y) = (1-qx) - ((x*x/2-qx) - (r-x*y)).
 *	   Note that 1-qx and (x*x/2-qx) is EXACT here, and the
 *	   magnitude of the latter is at least a quarter of x*x/2,
 *	   thus, reducing the rounding error in the subtraction.
 */
double __hide_kernel_cos(double x, double y)
{
   double a, hz, z, r, qx;
   int ix;
   ix = GET_HI(x) & 0x7fffffff; /* ix = |x|'s high word*/
   if(ix < 0x3e400000)
   { /* if x < 2**27 */
      if(((int)x) == 0)
         return one; /* generate inexact */
   }
   z = x * x;
   r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
   if(ix < 0x3FD33333) /* if |x| < 0.3 */
      return one - (0.5 * z - (z * r - x * y));
   else
   {
      if(ix > 0x3fe90000)
      { /* x > 0.78125 */
         qx = 0.28125;
      }
      else
      {
         INSERT_WORDS(qx, ix - 0x00200000, 0); /* x/4 */
      }
      hz = 0.5 * z - qx;
      a = one - qx;
      return a - (hz - (z * r - x * y));
   }
}

/* __hide_kernel_sin( x, y, iy)
 * kernel sin function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input iy indicates whether y is 0. (if iy=0, y assume to be 0).
 *
 * Algorithm
 *	1. Since sin(-x) = -sin(x), we need only to consider positive x.
 *	2. if x < 2^-27 (hx<0x3e400000 0), return x with inexact if x!=0.
 *	3. sin(x) is approximated by a polynomial of degree 13 on
 *	   [0,pi/4]
 *		  	         3            13
 *	   	sin(x) ~ x + S1*x + ... + S6*x
 *	   where
 *
 * 	|sin(x)         2     4     6     8     10     12  |     -58
 * 	|----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
 * 	|  x 					           |
 *
 *	4. sin(x+y) = sin(x) + sin'(x')*y
 *		    ~ sin(x) + (1-x*x/2)*y
 *	   For better accuracy, let
 *		     3      2      2      2      2
 *		r = x *(S2+x *(S3+x *(S4+x *(S5+x *S6))))
 *	   then                   3    2
 *		sin(x) = x + (S1*x + (x *(r-y/2)+y))
 */
double __hide_kernel_sin(double x, double y, int iy)
{
   double z, r, v;
   int ix;
   ix = GET_HI(x) & 0x7fffffff; /* high word of x */
   if(ix < 0x3e400000)          /* |x| < 2**-27 */
   {
      if((int)x == 0)
         return x;
   } /* generate inexact */
   z = x * x;
   v = z * x;
   r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
   if(iy == 0)
      return x + v * (S1 + z * r);
   else
      return x - ((z * (half * y - v * r) - y) - v * S1);
}

#ifndef HAVE_FLOPOCO

/* __ieee754_exp(x)
 * Returns the exponential of x.
 *
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *	Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.
 *
 *      Here r will be represented as r = hi-lo for better
 *	accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *	the interval [0,0.34658]:
 *	Write
 *	    R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Remes algorithm on [0,0.34658] to generate
 * 	a polynomial of degree 5 to approximate R. The maximum error
 *	of this polynomial approximation is bounded by 2**-59. In
 *	other words,
 *	    R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *  	(where z=r*r, and the values of P1 to P5 are listed below)
 *	and
 *	    |                  5          |     -59
 *	    | 2.0+P1*z+...+P5*z   -  R(z) | <= 2
 *	    |                             |
 *	The computation of exp(r) thus becomes
 *                             2*r
 *		exp(r) = 1 + -------
 *		              R - r
 *                                 r*R1(r)
 *		       = 1 + r + ----------- (for better accuracy)
 *		                  2 - R1(r)
 *	where
 *			         2       4             10
 *		R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *
 *   3. Scale back to obtain exp(x):
 *	From step 1, we have
 *	   exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF) is 0, and
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double
 *	    if x >  7.09782712893383973096e+02 then exp(x) overflow
 *	    if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */
double __hide_ieee754_exp(double x) /* default IEEE double exp */
{
   double y, hi = 0, lo = 0, c, t;
   int k = 0, xsb;
   unsigned hx;

   hx = GET_HI(x);       /* high word of x */
   xsb = (hx >> 31) & 1; /* sign bit of x */
   hx &= 0x7fffffff;     /* high word of |x| */

   /* filter out non-finite argument */
   if(hx >= 0x40862E42)
   { /* if |x|>=709.78... */
      if(hx >= 0x7ff00000)
      {
         if(((hx & 0xfffff) | GET_LO(x)) != 0)
            return x; /* NaN */
         else
            return (xsb == 0) ? x : 0.0; /* exp(+-inf)={inf,0} */
      }
      if(x > o_threshold)
         return huge * huge; /* overflow */
      if(x < u_threshold)
         return twom1000 * twom1000; /* underflow */
   }

   /* argument reduction */
   if(hx > 0x3fd62e42)
   { /* if  |x| > 0.5 ln2 */
      if(hx < 0x3FF0A2B2)
      { /* and |x| < 1.5 ln2 */
         hi = x - ln2HI[xsb];
         lo = ln2LO[xsb];
         k = 1 - xsb - xsb;
      }
      else
      {
         k = (int)(invln2 * x + halF[xsb]);
         t = k;
         hi = x - t * ln2HI[0]; /* t*ln2HI is exact here */
         lo = t * ln2LO[0];
      }
      x = hi - lo;
   }
   else if(hx < 0x3e300000)
   { /* when |x|<2**-28 */
      if(huge + x > one)
         return one + x; /* trigger inexact */
   }
   else
      k = 0;

   /* x is now in primary range */
   t = x * x;
   c = x - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
   if(k == 0)
      return one - ((x * c) / (c - 2.0) - x);
   else
      y = one - ((lo - (x * c) / (2.0 - c)) - hi);
   if(k >= -1021)
   {
      SET_HIGH_WORD(y, GET_HI(y) + (k << 20)); /* add k to y's exponent */
      return y;
   }
   else
   {
      SET_HIGH_WORD(y, GET_HI(y) + ((k + 1000) << 20)); /* add k to y's exponent */
      return y * twom1000;
   }
}

/* __ieee754_log(x)
 * Return the logarithm of x
 *
 * Method :
 *   1. Argument Reduction: find k and f such that
 *			x = 2^k * (1+f),
 *	   where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *   2. Approximation of log(1+f).
 *	Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *		 = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *	     	 = 2s + s*R
 *      We use a special Reme algorithm on [0,0.1716] to generate
 * 	a polynomial of degree 14 to approximate R The maximum error
 *	of this polynomial approximation is bounded by 2**-58.45. In
 *	other words,
 *		        2      4      6      8      10      12      14
 *	    R(z) ~ Lg1*s +Lg2*s +Lg3*s +Lg4*s +Lg5*s  +Lg6*s  +Lg7*s
 *  	(the values of Lg1 to Lg7 are listed in the program)
 *	and
 *	    |      2          14          |     -58.45
 *	    | Lg1*s +...+Lg7*s    -  R(z) | <= 2
 *	    |                             |
 *	Note that 2s = f - s*f = f - hfsq + s*hfsq, where hfsq = f*f/2.
 *	In order to guarantee error in log below 1ulp, we compute log
 *	by
 *		log(1+f) = f - s*(f - R)	(if f is not too large)
 *		log(1+f) = f - (hfsq - s*(hfsq+R)).	(better accuracy)
 *
 *	3. Finally,  log(x) = k*ln2 + log(1+f).
 *			    = k*ln2_hi+(f-(hfsq-(s*(hfsq+R)+k*ln2_lo)))
 *	   Here ln2 is split into two floating point number:
 *			ln2_hi + ln2_lo,
 *	   where n*ln2_hi is always exact for |n| < 2000.
 *
 * Special cases:
 *	log(x) is NaN with signal if x < 0 (including -INF) ;
 *	log(+INF) is +INF; log(0) is -INF with signal;
 *	log(NaN) is that NaN with no signal.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */
double __hide_ieee754_log(double x)
{
   double hfsq, f, s, z, R, w, t1, t2, dk;
   int k, hx, i, j;
   unsigned lx;

   hx = GET_HI(x); /* high word of x */
   lx = GET_LO(x); /* low  word of x */

   k = 0;
   if(hx < 0x00100000)
   { /* x < 2**-1022  */
      if(((hx & 0x7fffffff) | lx) == 0)
         return -__builtin_inf(); /* log(+-0)=-inf */
      if((hx >> 31) & 1)
         return __builtin_nans(""); /* log(-#) = NaN */
      k -= 54;
      x *= two54;     /* subnormal number, scale up x */
      hx = GET_HI(x); /* high word of x */
   }
   if(hx >= 0x7ff00000)
      return x;
   k += (hx >> 20) - 1023;
   hx &= 0x000fffff;
   i = (hx + 0x95f64) & 0x100000;
   SET_HIGH_WORD(x, hx | (i ^ 0x3ff00000)); /* normalize x or x/2 */
   k += (i >> 20);
   f = x - 1.0;
   if((0x000fffff & (2 + hx)) < 3)
   { /* |f| < 2**-20 */
      if(f == zero)
      {
         if(k == 0)
            return zero;
         else
         {
            dk = (double)k;
            return dk * ln2_hi + dk * ln2_lo;
         }
      }
      R = f * f * (0.5 - 0.33333333333333333 * f);
      if(k == 0)
         return f - R;
      else
      {
         dk = (double)k;
         return dk * ln2_hi - ((R - dk * ln2_lo) - f);
      }
   }
   s = f / (2.0 + f);
   dk = (double)k;
   z = s * s;
   i = hx - 0x6147a;
   w = z * z;
   j = 0x6b851 - hx;
   t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
   t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
   i |= j;
   R = t2 + t1;
   if(i > 0)
   {
      hfsq = 0.5 * f * f;
      if(k == 0)
         return f - (hfsq - s * (hfsq + R));
      else
         return dk * ln2_hi - ((hfsq - (s * (hfsq + R) + dk * ln2_lo)) - f);
   }
   else
   {
      if(k == 0)
         return f - s * (f - R);
      else
         return dk * ln2_hi - ((s * (f - R) - dk * ln2_lo) - f);
   }
}
#endif

/*
 * __ieee754_scalb(x, fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */
#ifdef _SCALB_INT
double __hide_ieee754_scalb(double x, int fn)
#else
double __hide_ieee754_scalb(double x, double fn)
#endif
{
#ifdef _SCALB_INT
   return scalbn(x, fn);
#else
   if(isnan(x) || isnan(fn))
      return x * fn;
   if(!finite(fn))
   {
      if(fn > 0.0)
         return x * fn;
      else
         return x / (-fn);
   }
   if(rint(fn) != fn)
      return (fn - fn) / (fn - fn);
   if(fn > 65000.0)
      return scalbn(x, 65000);
   if(-fn > 65000.0)
      return scalbn(x, -65000);
   return scalbn(x, (int)fn);
#endif
}
