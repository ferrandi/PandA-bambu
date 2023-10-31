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

/* __ieee754_j0(x), __ieee754_y0(x)
 * Bessel function of the first and second kinds of order zero.
 * Method -- j0(x):
 *	1. For tiny x, we use j0(x) = 1 - x^2/4 + x^4/64 - ...
 *	2. Reduce x to |x| since j0(x)=j0(-x),  and
 *	   for x in (0,2)
 *		j0(x) = 1-z/4+ z^2*R0/S0,  where z = x*x;
 *	   (precision:  |j0-1+z/4-z^2R0/S0 |<2**-63.67 )
 *	   for x in (2,inf)
 * 		j0(x) = sqrt(2/(pi*x))*(p0(x)*cos(x0)-q0(x)*sin(x0))
 * 	   where x0 = x-pi/4. It is better to compute sin(x0),cos(x0)
 *	   as follow:
 *		cos(x0) = cos(x)cos(pi/4)+sin(x)sin(pi/4)
 *			= 1/sqrt(2) * (cos(x) + sin(x))
 *		sin(x0) = sin(x)cos(pi/4)-cos(x)sin(pi/4)
 *			= 1/sqrt(2) * (sin(x) - cos(x))
 * 	   (To avoid cancellation, use
 *		sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
 * 	    to compute the worse one.)
 *
 *	3 Special cases
 *		j0(nan)= nan
 *		j0(0) = 1
 *		j0(inf) = 0
 *
 * Method -- y0(x):
 *	1. For x<2.
 *	   Since
 *		y0(x) = 2/pi*(j0(x)*(ln(x/2)+Euler) + x^2/4 - ...)
 *	   therefore y0(x)-2/pi*j0(x)*ln(x) is an even function.
 *	   We use the following function to approximate y0,
 *		y0(x) = U(z)/V(z) + (2/pi)*(j0(x)*ln(x)), z= x^2
 *	   where
 *		U(z) = u00 + u01*z + ... + u06*z^6
 *		V(z) = 1  + v01*z + ... + v04*z^4
 *	   with absolute approximation error bounded by 2**-72.
 *	   Note: For tiny x, U/V = u0 and j0(x)~1, hence
 *		y0(tiny) = u0 + (2/pi)*ln(tiny), (choose tiny<2**-27)
 *	2. For x>=2.
 * 		y0(x) = sqrt(2/(pi*x))*(p0(x)*cos(x0)+q0(x)*sin(x0))
 * 	   where x0 = x-pi/4. It is better to compute sin(x0),cos(x0)
 *	   by the method mentioned above.
 *	3. Special cases: y0(0)=-inf, y0(x<0)=NaN, y0(inf)=0.
 */

static double __hide_pzero(double), __hide_qzero(double);

static const double invsqrtpi = 5.64189583547756279280e-01, /* 0x3FE20DD7, 0x50429B6D */
    tpi = 6.36619772367581382433e-01,                       /* 0x3FE45F30, 0x6DC9C883 */
    /* R0/S0 on [0, 2.00] */
    R02 = 1.56249999999999947958e-02,  /* 0x3F8FFFFF, 0xFFFFFFFD */
    R03 = -1.89979294238854721751e-04, /* 0xBF28E6A5, 0xB61AC6E9 */
    R04 = 1.82954049532700665670e-06,  /* 0x3EBEB1D1, 0x0C503919 */
    R05 = -4.61832688532103189199e-09, /* 0xBE33D5E7, 0x73D63FCE */
    S01 = 1.56191029464890010492e-02,  /* 0x3F8FFCE8, 0x82C8C2A4 */
    S02 = 1.16926784663337450260e-04,  /* 0x3F1EA6D2, 0xDD57DBF4 */
    S03 = 5.13546550207318111446e-07,  /* 0x3EA13B54, 0xCE84D5A9 */
    S04 = 1.16614003333790000205e-09;  /* 0x3E1408BC, 0xF4745D8F */

double __hide_ieee754_j0(double x)
{
   double z, s, c, ss, cc, r, u, v;
   int hx, ix;

   hx = GET_HI(x);
   ix = hx & 0x7fffffff;
   if(ix >= 0x7ff00000)
      return one / (x * x);
   x = fabs(x);
   if(ix >= 0x40000000)
   { /* |x| >= 2.0 */
      s = sin(x);
      c = cos(x);
      ss = s - c;
      cc = s + c;
      if(ix < 0x7fe00000)
      { /* make sure x+x not overflow */
         z = -cos(x + x);
         if((s * c) < zero)
            cc = z / ss;
         else
            ss = z / cc;
      }
      /*
       * j0(x) = 1/sqrt(pi) * (P(0,x)*cc - Q(0,x)*ss) / sqrt(x)
       * y0(x) = 1/sqrt(pi) * (P(0,x)*ss + Q(0,x)*cc) / sqrt(x)
       */
      if(ix > 0x48000000)
         z = (invsqrtpi * cc) / sqrt(x);
      else
      {
         u = __hide_pzero(x);
         v = __hide_qzero(x);
         z = invsqrtpi * (u * cc - v * ss) / sqrt(x);
      }
      return z;
   }
   if(ix < 0x3f200000)
   { /* |x| < 2**-13 */
      if(huge + x > one)
      { /* raise inexact if x != 0 */
         if(ix < 0x3e400000)
            return one; /* |x|<2**-27 */
         else
            return one - 0.25 * x * x;
      }
   }
   z = x * x;
   r = z * (R02 + z * (R03 + z * (R04 + z * R05)));
   s = one + z * (S01 + z * (S02 + z * (S03 + z * S04)));
   if(ix < 0x3FF00000)
   { /* |x| < 1.00 */
      return one + z * (-0.25 + (r / s));
   }
   else
   {
      u = 0.5 * x;
      return ((one + u) * (one - u) + z * (r / s));
   }
}

static const double u00 = -7.38042951086872317523e-02, /* 0xBFB2E4D6, 0x99CBD01F */
    u01 = 1.76666452509181115538e-01,                  /* 0x3FC69D01, 0x9DE9E3FC */
    u02 = -1.38185671945596898896e-02,                 /* 0xBF8C4CE8, 0xB16CFA97 */
    u03 = 3.47453432093683650238e-04,                  /* 0x3F36C54D, 0x20B29B6B */
    u04 = -3.81407053724364161125e-06,                 /* 0xBECFFEA7, 0x73D25CAD */
    u05 = 1.95590137035022920206e-08,                  /* 0x3E550057, 0x3B4EABD4 */
    u06 = -3.98205194132103398453e-11,                 /* 0xBDC5E43D, 0x693FB3C8 */
    v01 = 1.27304834834123699328e-02,                  /* 0x3F8A1270, 0x91C9C71A */
    v02 = 7.60068627350353253702e-05,                  /* 0x3F13ECBB, 0xF578C6C1 */
    v03 = 2.59150851840457805467e-07,                  /* 0x3E91642D, 0x7FF202FD */
    v04 = 4.41110311332675467403e-10;                  /* 0x3DFE5018, 0x3BD6D9EF */

double __hide_ieee754_y0(double x)
{
   double z, s, c, ss, cc, u, v;
   int hx, ix, lx;

   hx = GET_HI(x);
   ix = 0x7fffffff & hx;
   lx = GET_LO(x);
   /* Y0(NaN) is NaN, y0(-inf) is Nan, y0(inf) is 0  */
   if(ix >= 0x7ff00000)
      return one / (x + x * x);
   if((ix | lx) == 0)
      return -one / zero;
   if(hx < 0)
      return zero / zero;
   if(ix >= 0x40000000)
   { /* |x| >= 2.0 */
      /* y0(x) = sqrt(2/(pi*x))*(p0(x)*sin(x0)+q0(x)*cos(x0))
       * where x0 = x-pi/4
       *      Better formula:
       *              cos(x0) = cos(x)cos(pi/4)+sin(x)sin(pi/4)
       *                      =  1/sqrt(2) * (sin(x) + cos(x))
       *              sin(x0) = sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
       *                      =  1/sqrt(2) * (sin(x) - cos(x))
       * To avoid cancellation, use
       *              sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
       * to compute the worse one.
       */
      s = sin(x);
      c = cos(x);
      ss = s - c;
      cc = s + c;
      /*
       * j0(x) = 1/sqrt(pi) * (P(0,x)*cc - Q(0,x)*ss) / sqrt(x)
       * y0(x) = 1/sqrt(pi) * (P(0,x)*ss + Q(0,x)*cc) / sqrt(x)
       */
      if(ix < 0x7fe00000)
      { /* make sure x+x not overflow */
         z = -cos(x + x);
         if((s * c) < zero)
            cc = z / ss;
         else
            ss = z / cc;
      }
      if(ix > 0x48000000)
         z = (invsqrtpi * ss) / sqrt(x);
      else
      {
         u = __hide_pzero(x);
         v = __hide_qzero(x);
         z = invsqrtpi * (u * ss + v * cc) / sqrt(x);
      }
      return z;
   }
   if(ix <= 0x3e400000)
   { /* x < 2**-27 */
      return (u00 + tpi * __hide_ieee754_log(x));
   }
   z = x * x;
   u = u00 + z * (u01 + z * (u02 + z * (u03 + z * (u04 + z * (u05 + z * u06)))));
   v = one + z * (v01 + z * (v02 + z * (v03 + z * v04)));
   return (u / v + tpi * (__hide_ieee754_j0(x) * __hide_ieee754_log(x)));
}

/* The asymptotic expansions of pzero is
 *	1 - 9/128 s^2 + 11025/98304 s^4 - ...,	where s = 1/x.
 * For x >= 2, We approximate pzero by
 * 	pzero(x) = 1 + (R/S)
 * where  R = pR0 + pR1*s^2 + pR2*s^4 + ... + pR5*s^10
 * 	  S = 1 + pS0*s^2 + ... + pS4*s^10
 * and
 *	| pzero(x)-1-R/S | <= 2  ** ( -60.26)
 */
static const double pR8[6] = {
    /* for x in [inf, 8]=1/[0,0.125] */
    0.00000000000000000000e+00,  /* 0x00000000, 0x00000000 */
    -7.03124999999900357484e-02, /* 0xBFB1FFFF, 0xFFFFFD32 */
    -8.08167041275349795626e+00, /* 0xC02029D0, 0xB44FA779 */
    -2.57063105679704847262e+02, /* 0xC0701102, 0x7B19E863 */
    -2.48521641009428822144e+03, /* 0xC0A36A6E, 0xCD4DCAFC */
    -5.25304380490729545272e+03, /* 0xC0B4850B, 0x36CC643D */
};

static const double pS8[5] = {
    1.16534364619668181717e+02, /* 0x405D2233, 0x07A96751 */
    3.83374475364121826715e+03, /* 0x40ADF37D, 0x50596938 */
    4.05978572648472545552e+04, /* 0x40E3D2BB, 0x6EB6B05F */
    1.16752972564375915681e+05, /* 0x40FC810F, 0x8F9FA9BD */
    4.76277284146730962675e+04, /* 0x40E74177, 0x4F2C49DC */
};

static const double pR5[6] = {
    /* for x in [8,4.5454]=1/[0.125,0.22001] */
    -1.14125464691894502584e-11, /* 0xBDA918B1, 0x47E495CC */
    -7.03124940873599280078e-02, /* 0xBFB1FFFF, 0xE69AFBC6 */
    -4.15961064470587782438e+00, /* 0xC010A370, 0xF90C6BBF */
    -6.76747652265167261021e+01, /* 0xC050EB2F, 0x5A7D1783 */
    -3.31231299649172967747e+02, /* 0xC074B3B3, 0x6742CC63 */
    -3.46433388365604912451e+02, /* 0xC075A6EF, 0x28A38BD7 */
};

static const double _pS5[5] = {
    6.07539382692300335975e+01, /* 0x404E6081, 0x0C98C5DE */
    1.05125230595704579173e+03, /* 0x40906D02, 0x5C7E2864 */
    5.97897094333855784498e+03, /* 0x40B75AF8, 0x8FBE1D60 */
    9.62544514357774460223e+03, /* 0x40C2CCB8, 0xFA76FA38 */
    2.40605815922939109441e+03, /* 0x40A2CC1D, 0xC70BE864 */
};

static const double pR3[6] = {
    /* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
    -2.54704601771951915620e-09, /* 0xBE25E103, 0x6FE1AA86 */
    -7.03119616381481654654e-02, /* 0xBFB1FFF6, 0xF7C0E24B */
    -2.40903221549529611423e+00, /* 0xC00345B2, 0xAEA48074 */
    -2.19659774734883086467e+01, /* 0xC035F74A, 0x4CB94E14 */
    -5.80791704701737572236e+01, /* 0xC04D0A22, 0x420A1A45 */
    -3.14479470594888503854e+01, /* 0xC03F72AC, 0xA892D80F */
};

static const double _pS3[5] = {
    3.58560338055209726349e+01, /* 0x4041ED92, 0x84077DD3 */
    3.61513983050303863820e+02, /* 0x40769839, 0x464A7C0E */
    1.19360783792111533330e+03, /* 0x4092A66E, 0x6D1061D6 */
    1.12799679856907414432e+03, /* 0x40919FFC, 0xB8C39B7E */
    1.73580930813335754692e+02, /* 0x4065B296, 0xFC379081 */
};

static const double pR2[6] = {
    /* for x in [2.8570,2]=1/[0.3499,0.5] */
    -8.87534333032526411254e-08, /* 0xBE77D316, 0xE927026D */
    -7.03030995483624743247e-02, /* 0xBFB1FF62, 0x495E1E42 */
    -1.45073846780952986357e+00, /* 0xBFF73639, 0x8A24A843 */
    -7.63569613823527770791e+00, /* 0xC01E8AF3, 0xEDAFA7F3 */
    -1.11931668860356747786e+01, /* 0xC02662E6, 0xC5246303 */
    -3.23364579351335335033e+00, /* 0xC009DE81, 0xAF8FE70F */
};

static const double _pS2[5] = {
    2.22202997532088808441e+01, /* 0x40363865, 0x908B5959 */
    1.36206794218215208048e+02, /* 0x4061069E, 0x0EE8878F */
    2.70470278658083486789e+02, /* 0x4070E786, 0x42EA079B */
    1.53875394208320329881e+02, /* 0x40633C03, 0x3AB6FAFF */
    1.46576176948256193810e+01, /* 0x402D50B3, 0x44391809 */
};

static double __hide_pzero(double x)
{
   const double *p, *q;
   double z, r, s;
   int ix;
   ix = 0x7fffffff & GET_HI(x);
   if(ix >= 0x40200000)
   {
      p = pR8;
      q = pS8;
   }
   else if(ix >= 0x40122E8B)
   {
      p = pR5;
      q = _pS5;
   }
   else if(ix >= 0x4006DB6D)
   {
      p = pR3;
      q = _pS3;
   }
   else
   {
      p = pR2;
      q = _pS2;
   }
   z = one / (x * x);
   r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
   s = one + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * q[4]))));
   return one + r / s;
}

/* For x >= 8, the asymptotic expansions of qzero is
 *	-1/8 s + 75/1024 s^3 - ..., where s = 1/x.
 * We approximate pzero by
 * 	qzero(x) = s*(-1.25 + (R/S))
 * where  R = qR0 + qR1*s^2 + qR2*s^4 + ... + qR5*s^10
 * 	  S = 1 + qS0*s^2 + ... + qS5*s^12
 * and
 *	| qzero(x)/s +1.25-R/S | <= 2  ** ( -61.22)
 */
static const double qR8[6] = {
    /* for x in [inf, 8]=1/[0,0.125] */
    0.00000000000000000000e+00, /* 0x00000000, 0x00000000 */
    7.32421874999935051953e-02, /* 0x3FB2BFFF, 0xFFFFFE2C */
    1.17682064682252693899e+01, /* 0x40278952, 0x5BB334D6 */
    5.57673380256401856059e+02, /* 0x40816D63, 0x15301825 */
    8.85919720756468632317e+03, /* 0x40C14D99, 0x3E18F46D */
    3.70146267776887834771e+04, /* 0x40E212D4, 0x0E901566 */
};

static const double qS8[6] = {
    1.63776026895689824414e+02,  /* 0x406478D5, 0x365B39BC */
    8.09834494656449805916e+03,  /* 0x40BFA258, 0x4E6B0563 */
    1.42538291419120476348e+05,  /* 0x41016652, 0x54D38C3F */
    8.03309257119514397345e+05,  /* 0x412883DA, 0x83A52B43 */
    8.40501579819060512818e+05,  /* 0x4129A66B, 0x28DE0B3D */
    -3.43899293537866615225e+05, /* 0xC114FD6D, 0x2C9530C5 */
};

static const double qR5[6] = {
    /* for x in [8,4.5454]=1/[0.125,0.22001] */
    1.84085963594515531381e-11, /* 0x3DB43D8F, 0x29CC8CD9 */
    7.32421766612684765896e-02, /* 0x3FB2BFFF, 0xD172B04C */
    5.83563508962056953777e+00, /* 0x401757B0, 0xB9953DD3 */
    1.35111577286449829671e+02, /* 0x4060E392, 0x0A8788E9 */
    1.02724376596164097464e+03, /* 0x40900CF9, 0x9DC8C481 */
    1.98997785864605384631e+03, /* 0x409F17E9, 0x53C6E3A6 */
};

static const double qS5[6] = {
    8.27766102236537761883e+01,  /* 0x4054B1B3, 0xFB5E1543 */
    2.07781416421392987104e+03,  /* 0x40A03BA0, 0xDA21C0CE */
    1.88472887785718085070e+04,  /* 0x40D267D2, 0x7B591E6D */
    5.67511122894947329769e+04,  /* 0x40EBB5E3, 0x97E02372 */
    3.59767538425114471465e+04,  /* 0x40E19118, 0x1F7A54A0 */
    -5.35434275601944773371e+03, /* 0xC0B4EA57, 0xBEDBC609 */
};

static const double qR3[6] = {
    /* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
    4.37741014089738620906e-09, /* 0x3E32CD03, 0x6ADECB82 */
    7.32411180042911447163e-02, /* 0x3FB2BFEE, 0x0E8D0842 */
    3.34423137516170720929e+00, /* 0x400AC0FC, 0x61149CF5 */
    4.26218440745412650017e+01, /* 0x40454F98, 0x962DAEDD */
    1.70808091340565596283e+02, /* 0x406559DB, 0xE25EFD1F */
    1.66733948696651168575e+02, /* 0x4064D77C, 0x81FA21E0 */
};

static const double _qS3[6] = {
    4.87588729724587182091e+01,  /* 0x40486122, 0xBFE343A6 */
    7.09689221056606015736e+02,  /* 0x40862D83, 0x86544EB3 */
    3.70414822620111362994e+03,  /* 0x40ACF04B, 0xE44DFC63 */
    6.46042516752568917582e+03,  /* 0x40B93C6C, 0xD7C76A28 */
    2.51633368920368957333e+03,  /* 0x40A3A8AA, 0xD94FB1C0 */
    -1.49247451836156386662e+02, /* 0xC062A7EB, 0x201CF40F */
};

static const double qR2[6] = {
    /* for x in [2.8570,2]=1/[0.3499,0.5] */
    1.50444444886983272379e-07, /* 0x3E84313B, 0x54F76BDB */
    7.32234265963079278272e-02, /* 0x3FB2BEC5, 0x3E883E34 */
    1.99819174093815998816e+00, /* 0x3FFFF897, 0xE727779C */
    1.44956029347885735348e+01, /* 0x402CFDBF, 0xAAF96FE5 */
    3.16662317504781540833e+01, /* 0x403FAA8E, 0x29FBDC4A */
    1.62527075710929267416e+01, /* 0x403040B1, 0x71814BB4 */
};
static const double _qS2[6] = {
    3.03655848355219184498e+01,  /* 0x403E5D96, 0xF7C07AED */
    2.69348118608049844624e+02,  /* 0x4070D591, 0xE4D14B40 */
    8.44783757595320139444e+02,  /* 0x408A6645, 0x22B3BF22 */
    8.82935845112488550512e+02,  /* 0x408B977C, 0x9C5CC214 */
    2.12666388511798828631e+02,  /* 0x406A9553, 0x0E001365 */
    -5.31095493882666946917e+00, /* 0xC0153E6A, 0xF8B32931 */
};

static double __hide_qzero(double x)
{
   const double *p, *q;
   double s, r, z;
   int ix;
   ix = 0x7fffffff & GET_HI(x);
   if(ix >= 0x40200000)
   {
      p = qR8;
      q = qS8;
   }
   else if(ix >= 0x40122E8B)
   {
      p = qR5;
      q = qS5;
   }
   else if(ix >= 0x4006DB6D)
   {
      p = qR3;
      q = _qS3;
   }
   else
   {
      p = qR2;
      q = _qS2;
   }
   z = one / (x * x);
   r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
   s = one + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * (q[4] + z * q[5])))));
   return (-.125 + r / s) / x;
}

/* __ieee754_j1(x), __ieee754_y1(x)
 * Bessel function of the first and second kinds of order zero.
 * Method -- j1(x):
 *	1. For tiny x, we use j1(x) = x/2 - x^3/16 + x^5/384 - ...
 *	2. Reduce x to |x| since j1(x)=-j1(-x),  and
 *	   for x in (0,2)
 *		j1(x) = x/2 + x*z*R0/S0,  where z = x*x;
 *	   (precision:  |j1/x - 1/2 - R0/S0 |<2**-61.51 )
 *	   for x in (2,inf)
 * 		j1(x) = sqrt(2/(pi*x))*(p1(x)*cos(x1)-q1(x)*sin(x1))
 * 		y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x1)+q1(x)*cos(x1))
 * 	   where x1 = x-3*pi/4. It is better to compute sin(x1),cos(x1)
 *	   as follow:
 *		cos(x1) =  cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
 *			=  1/sqrt(2) * (sin(x) - cos(x))
 *		sin(x1) =  sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
 *			= -1/sqrt(2) * (sin(x) + cos(x))
 * 	   (To avoid cancellation, use
 *		sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
 * 	    to compute the worse one.)
 *
 *	3 Special cases
 *		j1(nan)= nan
 *		j1(0) = 0
 *		j1(inf) = 0
 *
 * Method -- y1(x):
 *	1. screen out x<=0 cases: y1(0)=-inf, y1(x<0)=NaN
 *	2. For x<2.
 *	   Since
 *		y1(x) = 2/pi*(j1(x)*(ln(x/2)+Euler)-1/x-x/2+5/64*x^3-...)
 *	   therefore y1(x)-2/pi*j1(x)*ln(x)-1/x is an odd function.
 *	   We use the following function to approximate y1,
 *		y1(x) = x*U(z)/V(z) + (2/pi)*(j1(x)*ln(x)-1/x), z= x^2
 *	   where for x in [0,2] (abs err less than 2**-65.89)
 *		U(z) = U0[0] + U0[1]*z + ... + U0[4]*z^4
 *		V(z) = 1  + v0[0]*z + ... + v0[4]*z^5
 *	   Note: For tiny x, 1/x dominate y1 and hence
 *		y1(tiny) = -2/pi/tiny, (choose tiny<2**-54)
 *	3. For x>=2.
 * 		y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x1)+q1(x)*cos(x1))
 * 	   where x1 = x-3*pi/4. It is better to compute sin(x1),cos(x1)
 *	   by method mentioned above.
 */

static double __hide_pone(double), __hide_qone(double);

static const double
    /* R0/S0 on [0,2] */
    r00 = -6.25000000000000000000e-02, /* 0xBFB00000, 0x00000000 */
    r01 = 1.40705666955189706048e-03,  /* 0x3F570D9F, 0x98472C61 */
    r02 = -1.59955631084035597520e-05, /* 0xBEF0C5C6, 0xBA169668 */
    r03 = 4.96727999609584448412e-08,  /* 0x3E6AAAFA, 0x46CA0BD9 */
    s01 = 1.91537599538363460805e-02,  /* 0x3F939D0B, 0x12637E53 */
    s02 = 1.85946785588630915560e-04,  /* 0x3F285F56, 0xB9CDF664 */
    s03 = 1.17718464042623683263e-06,  /* 0x3EB3BFF8, 0x333F8498 */
    s04 = 5.04636257076217042715e-09,  /* 0x3E35AC88, 0xC97DFF2C */
    s05 = 1.23542274426137913908e-11;  /* 0x3DAB2ACF, 0xCFB97ED8 */

double __hide_ieee754_j1(double x)
{
   double z, s, c, ss, cc, r, u, v, y;
   int hx, ix;

   hx = GET_HI(x);
   ix = hx & 0x7fffffff;
   if(ix >= 0x7ff00000)
      return one / x;
   y = fabs(x);
   if(ix >= 0x40000000)
   { /* |x| >= 2.0 */
      s = sin(y);
      c = cos(y);
      ss = -s - c;
      cc = s - c;
      if(ix < 0x7fe00000)
      { /* make sure y+y not overflow */
         z = cos(y + y);
         if((s * c) > zero)
            cc = z / ss;
         else
            ss = z / cc;
      }
      /*
       * j1(x) = 1/sqrt(pi) * (P(1,x)*cc - Q(1,x)*ss) / sqrt(x)
       * y1(x) = 1/sqrt(pi) * (P(1,x)*ss + Q(1,x)*cc) / sqrt(x)
       */
      if(ix > 0x48000000)
         z = (invsqrtpi * cc) / sqrt(y);
      else
      {
         u = __hide_pone(y);
         v = __hide_qone(y);
         z = invsqrtpi * (u * cc - v * ss) / sqrt(y);
      }
      if(hx < 0)
         return -z;
      else
         return z;
   }
   if(ix < 0x3e400000)
   { /* |x|<2**-27 */
      if(huge + x > one)
         return 0.5 * x; /* inexact if x!=0 necessary */
   }
   z = x * x;
   r = z * (r00 + z * (r01 + z * (r02 + z * r03)));
   s = one + z * (s01 + z * (s02 + z * (s03 + z * (s04 + z * s05))));
   r *= x;
   return (x * 0.5 + r / s);
}

static const double U0[5] = {
    -1.96057090646238940668e-01, /* 0xBFC91866, 0x143CBC8A */
    5.04438716639811282616e-02,  /* 0x3FA9D3C7, 0x76292CD1 */
    -1.91256895875763547298e-03, /* 0xBF5F55E5, 0x4844F50F */
    2.35252600561610495928e-05,  /* 0x3EF8AB03, 0x8FA6B88E */
    -9.19099158039878874504e-08, /* 0xBE78AC00, 0x569105B8 */
};
static const double V0[5] = {
    1.99167318236649903973e-02, /* 0x3F94650D, 0x3F4DA9F0 */
    2.02552581025135171496e-04, /* 0x3F2A8C89, 0x6C257764 */
    1.35608801097516229404e-06, /* 0x3EB6C05A, 0x894E8CA6 */
    6.22741452364621501295e-09, /* 0x3E3ABF1D, 0x5BA69A86 */
    1.66559246207992079114e-11, /* 0x3DB25039, 0xDACA772A */
};

double __hide_ieee754_y1(double x)
{
   double z, s, c, ss, cc, u, v;
   int hx, ix, lx;

   hx = GET_HI(x);
   ix = 0x7fffffff & hx;
   lx = GET_LO(x);
   /* if Y1(NaN) is NaN, Y1(-inf) is NaN, Y1(inf) is 0 */
   if(ix >= 0x7ff00000)
      return one / (x + x * x);
   if((ix | lx) == 0)
      return -one / zero;
   if(hx < 0)
      return zero / zero;
   if(ix >= 0x40000000)
   { /* |x| >= 2.0 */
      s = sin(x);
      c = cos(x);
      ss = -s - c;
      cc = s - c;
      if(ix < 0x7fe00000)
      { /* make sure x+x not overflow */
         z = cos(x + x);
         if((s * c) > zero)
            cc = z / ss;
         else
            ss = z / cc;
      }
      /* y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x0)+q1(x)*cos(x0))
       * where x0 = x-3pi/4
       *      Better formula:
       *              cos(x0) = cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
       *                      =  1/sqrt(2) * (sin(x) - cos(x))
       *              sin(x0) = sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
       *                      = -1/sqrt(2) * (cos(x) + sin(x))
       * To avoid cancellation, use
       *              sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
       * to compute the worse one.
       */
      if(ix > 0x48000000)
         z = (invsqrtpi * ss) / sqrt(x);
      else
      {
         u = __hide_pone(x);
         v = __hide_qone(x);
         z = invsqrtpi * (u * ss + v * cc) / sqrt(x);
      }
      return z;
   }
   if(ix <= 0x3c900000)
   { /* x < 2**-54 */
      return (-tpi / x);
   }
   z = x * x;
   u = U0[0] + z * (U0[1] + z * (U0[2] + z * (U0[3] + z * U0[4])));
   v = one + z * (V0[0] + z * (V0[1] + z * (V0[2] + z * (V0[3] + z * V0[4]))));
   return (x * (u / v) + tpi * (__hide_ieee754_j1(x) * __hide_ieee754_log(x) - one / x));
}

/* For x >= 8, the asymptotic expansions of pone is
 *	1 + 15/128 s^2 - 4725/2^15 s^4 - ...,	where s = 1/x.
 * We approximate pone by
 * 	pone(x) = 1 + (R/S)
 * where  R = pr0 + pr1*s^2 + pr2*s^4 + ... + pr5*s^10
 * 	  S = 1 + ps0*s^2 + ... + ps4*s^10
 * and
 *	| pone(x)-1-R/S | <= 2  ** ( -60.06)
 */

static const double pr8[6] = {
    /* for x in [inf, 8]=1/[0,0.125] */
    0.00000000000000000000e+00, /* 0x00000000, 0x00000000 */
    1.17187499999988647970e-01, /* 0x3FBDFFFF, 0xFFFFFCCE */
    1.32394806593073575129e+01, /* 0x402A7A9D, 0x357F7FCE */
    4.12051854307378562225e+02, /* 0x4079C0D4, 0x652EA590 */
    3.87474538913960532227e+03, /* 0x40AE457D, 0xA3A532CC */
    7.91447954031891731574e+03, /* 0x40BEEA7A, 0xC32782DD */
};
static const double ps8[5] = {
    1.14207370375678408436e+02, /* 0x405C8D45, 0x8E656CAC */
    3.65093083420853463394e+03, /* 0x40AC85DC, 0x964D274F */
    3.69562060269033463555e+04, /* 0x40E20B86, 0x97C5BB7F */
    9.76027935934950801311e+04, /* 0x40F7D42C, 0xB28F17BB */
    3.08042720627888811578e+04, /* 0x40DE1511, 0x697A0B2D */
};

static const double pr5[6] = {
    /* for x in [8,4.5454]=1/[0.125,0.22001] */
    1.31990519556243522749e-11, /* 0x3DAD0667, 0xDAE1CA7D */
    1.17187493190614097638e-01, /* 0x3FBDFFFF, 0xE2C10043 */
    6.80275127868432871736e+00, /* 0x401B3604, 0x6E6315E3 */
    1.08308182990189109773e+02, /* 0x405B13B9, 0x452602ED */
    5.17636139533199752805e+02, /* 0x40802D16, 0xD052D649 */
    5.28715201363337541807e+02, /* 0x408085B8, 0xBB7E0CB7 */
};

static const double ps5[5] = {
    5.92805987221131331921e+01, /* 0x404DA3EA, 0xA8AF633D */
    9.91401418733614377743e+02, /* 0x408EFB36, 0x1B066701 */
    5.35326695291487976647e+03, /* 0x40B4E944, 0x5706B6FB */
    7.84469031749551231769e+03, /* 0x40BEA4B0, 0xB8A5BB15 */
    1.50404688810361062679e+03, /* 0x40978030, 0x036F5E51 */
};

static const double pr3[6] = {
    3.02503916137373618024e-09, /* 0x3E29FC21, 0xA7AD9EDD */
    1.17186865567253592491e-01, /* 0x3FBDFFF5, 0x5B21D17B */
    3.93297750033315640650e+00, /* 0x400F76BC, 0xE85EAD8A */
    3.51194035591636932736e+01, /* 0x40418F48, 0x9DA6D129 */
    9.10550110750781271918e+01, /* 0x4056C385, 0x4D2C1837 */
    4.85590685197364919645e+01, /* 0x4048478F, 0x8EA83EE5 */
};

static const double ps3[5] = {
    3.47913095001251519989e+01, /* 0x40416549, 0xA134069C */
    3.36762458747825746741e+02, /* 0x40750C33, 0x07F1A75F */
    1.04687139975775130551e+03, /* 0x40905B7C, 0x5037D523 */
    8.90811346398256432622e+02, /* 0x408BD67D, 0xA32E31E9 */
    1.03787932439639277504e+02, /* 0x4059F26D, 0x7C2EED53 */
};

static const double pr2[6] = {
    /* for x in [2.8570,2]=1/[0.3499,0.5] */
    1.07710830106873743082e-07, /* 0x3E7CE9D4, 0xF65544F4 */
    1.17176219462683348094e-01, /* 0x3FBDFF42, 0xBE760D83 */
    2.36851496667608785174e+00, /* 0x4002F2B7, 0xF98FAEC0 */
    1.22426109148261232917e+01, /* 0x40287C37, 0x7F71A964 */
    1.76939711271687727390e+01, /* 0x4031B1A8, 0x177F8EE2 */
    5.07352312588818499250e+00, /* 0x40144B49, 0xA574C1FE */
};

static const double ps2[5] = {
    2.14364859363821409488e+01, /* 0x40356FBD, 0x8AD5ECDC */
    1.25290227168402751090e+02, /* 0x405F5293, 0x14F92CD5 */
    2.32276469057162813669e+02, /* 0x406D08D8, 0xD5A2DBD9 */
    1.17679373287147100768e+02, /* 0x405D6B7A, 0xDA1884A9 */
    8.36463893371618283368e+00, /* 0x4020BAB1, 0xF44E5192 */
};

static double __hide_pone(double x)
{
   const double *p, *q;
   double z, r, s;
   int ix;
   ix = 0x7fffffff & GET_HI(x);
   if(ix >= 0x40200000)
   {
      p = pr8;
      q = ps8;
   }
   else if(ix >= 0x40122E8B)
   {
      p = pr5;
      q = ps5;
   }
   else if(ix >= 0x4006DB6D)
   {
      p = pr3;
      q = ps3;
   }
   else
   {
      p = pr2;
      q = ps2;
   }
   z = one / (x * x);
   r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
   s = one + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * q[4]))));
   return one + r / s;
}

/* For x >= 8, the asymptotic expansions of qone is
 *	3/8 s - 105/1024 s^3 - ..., where s = 1/x.
 * We approximate pone by
 * 	qone(x) = s*(0.375 + (R/S))
 * where  R = qr1*s^2 + qr2*s^4 + ... + qr5*s^10
 * 	  S = 1 + qs1*s^2 + ... + qs6*s^12
 * and
 *	| qone(x)/s -0.375-R/S | <= 2  ** ( -61.13)
 */

static const double qr8[6] = {
    /* for x in [inf, 8]=1/[0,0.125] */
    0.00000000000000000000e+00,  /* 0x00000000, 0x00000000 */
    -1.02539062499992714161e-01, /* 0xBFBA3FFF, 0xFFFFFDF3 */
    -1.62717534544589987888e+01, /* 0xC0304591, 0xA26779F7 */
    -7.59601722513950107896e+02, /* 0xC087BCD0, 0x53E4B576 */
    -1.18498066702429587167e+04, /* 0xC0C724E7, 0x40F87415 */
    -4.84385124285750353010e+04, /* 0xC0E7A6D0, 0x65D09C6A */
};

static const double qs8[6] = {
    1.61395369700722909556e+02,  /* 0x40642CA6, 0xDE5BCDE5 */
    7.82538599923348465381e+03,  /* 0x40BE9162, 0xD0D88419 */
    1.33875336287249578163e+05,  /* 0x4100579A, 0xB0B75E98 */
    7.19657723683240939863e+05,  /* 0x4125F653, 0x72869C19 */
    6.66601232617776375264e+05,  /* 0x412457D2, 0x7719AD5C */
    -2.94490264303834643215e+05, /* 0xC111F969, 0x0EA5AA18 */
};

static const double qr5[6] = {
    /* for x in [8,4.5454]=1/[0.125,0.22001] */
    -2.08979931141764104297e-11, /* 0xBDB6FA43, 0x1AA1A098 */
    -1.02539050241375426231e-01, /* 0xBFBA3FFF, 0xCB597FEF */
    -8.05644828123936029840e+00, /* 0xC0201CE6, 0xCA03AD4B */
    -1.83669607474888380239e+02, /* 0xC066F56D, 0x6CA7B9B0 */
    -1.37319376065508163265e+03, /* 0xC09574C6, 0x6931734F */
    -2.61244440453215656817e+03, /* 0xC0A468E3, 0x88FDA79D */
};

static const double qs5[6] = {
    8.12765501384335777857e+01,  /* 0x405451B2, 0xFF5A11B2 */
    1.99179873460485964642e+03,  /* 0x409F1F31, 0xE77BF839 */
    1.74684851924908907677e+04,  /* 0x40D10F1F, 0x0D64CE29 */
    4.98514270910352279316e+04,  /* 0x40E8576D, 0xAABAD197 */
    2.79480751638918118260e+04,  /* 0x40DB4B04, 0xCF7C364B */
    -4.71918354795128470869e+03, /* 0xC0B26F2E, 0xFCFFA004 */
};

static const double qr3[6] = {
    -5.07831226461766561369e-09, /* 0xBE35CFA9, 0xD38FC84F */
    -1.02537829820837089745e-01, /* 0xBFBA3FEB, 0x51AEED54 */
    -4.61011581139473403113e+00, /* 0xC01270C2, 0x3302D9FF */
    -5.78472216562783643212e+01, /* 0xC04CEC71, 0xC25D16DA */
    -2.28244540737631695038e+02, /* 0xC06C87D3, 0x4718D55F */
    -2.19210128478909325622e+02, /* 0xC06B66B9, 0x5F5C1BF6 */
};

static const double qs3[6] = {
    4.76651550323729509273e+01,  /* 0x4047D523, 0xCCD367E4 */
    6.73865112676699709482e+02,  /* 0x40850EEB, 0xC031EE3E */
    3.38015286679526343505e+03,  /* 0x40AA684E, 0x448E7C9A */
    5.54772909720722782367e+03,  /* 0x40B5ABBA, 0xA61D54A6 */
    1.90311919338810798763e+03,  /* 0x409DBC7A, 0x0DD4DF4B */
    -1.35201191444307340817e+02, /* 0xC060E670, 0x290A311F */
};

static const double qr2[6] = {
    /* for x in [2.8570,2]=1/[0.3499,0.5] */
    -1.78381727510958865572e-07, /* 0xBE87F126, 0x44C626D2 */
    -1.02517042607985553460e-01, /* 0xBFBA3E8E, 0x9148B010 */
    -2.75220568278187460720e+00, /* 0xC0060484, 0x69BB4EDA */
    -1.96636162643703720221e+01, /* 0xC033A9E2, 0xC168907F */
    -4.23253133372830490089e+01, /* 0xC04529A3, 0xDE104AAA */
    -2.13719211703704061733e+01, /* 0xC0355F36, 0x39CF6E52 */
};

static const double qs2[6] = {
    2.95333629060523854548e+01,  /* 0x403D888A, 0x78AE64FF */
    2.52981549982190529136e+02,  /* 0x406F9F68, 0xDB821CBA */
    7.57502834868645436472e+02,  /* 0x4087AC05, 0xCE49A0F7 */
    7.39393205320467245656e+02,  /* 0x40871B25, 0x48D4C029 */
    1.55949003336666123687e+02,  /* 0x40637E5E, 0x3C3ED8D4 */
    -4.95949898822628210127e+00, /* 0xC013D686, 0xE71BE86B */
};

static double __hide_qone(double x)
{
   const double *p, *q;
   double s, r, z;
   int ix;
   ix = 0x7fffffff & GET_HI(x);
   if(ix >= 0x40200000)
   {
      p = qr8;
      q = qs8;
   }
   else if(ix >= 0x40122E8B)
   {
      p = qr5;
      q = qs5;
   }
   else if(ix >= 0x4006DB6D)
   {
      p = qr3;
      q = qs3;
   }
   else
   {
      p = qr2;
      q = qs2;
   }
   z = one / (x * x);
   r = p[0] + z * (p[1] + z * (p[2] + z * (p[3] + z * (p[4] + z * p[5]))));
   s = one + z * (q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * (q[4] + z * q[5])))));
   return (.375 + r / s) / x;
}

/*
 * __ieee754_jn(n, x), __ieee754_yn(n, x)
 * floating point Bessel's function of the 1st and 2nd kind
 * of order n
 *
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with division by zero signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *
 */
double __hide_ieee754_jn(int n, double x)
{
   int i, hx, ix, lx, sgn;
   double a, b, temp, di;
   double z, w;

   /* J(-n,x) = (-1)^n * J(n, x), J(n, -x) = (-1)^n * J(n, x)
    * Thus, J(-n,x) = J(n,-x)
    */
   hx = GET_HI(x);
   ix = 0x7fffffff & hx;
   lx = GET_LO(x);
   /* if J(n,NaN) is NaN */
   if((ix | ((unsigned)(lx | -lx)) >> 31) > 0x7ff00000)
      return X_PLUS_X(x);
   if(n < 0)
   {
      n = -n;
      x = -x;
      hx ^= 0x80000000;
   }
   if(n == 0)
      return (__hide_ieee754_j0(x));
   if(n == 1)
      return (__hide_ieee754_j1(x));
   sgn = (n & 1) & (hx >> 31); /* even n -- 0, odd n -- sign(x) */
   x = fabs(x);
   if((ix | lx) == 0 || ix >= 0x7ff00000) /* if x is 0 or inf */
      b = zero;
   else if((double)n <= x)
   {
      /* Safe to use J(n+1,x)=2n/x *J(n,x)-J(n-1,x) */
      if(ix >= 0x52D00000)
      { /* x > 2**302 */
         /* (x >> n**2)
          *	    Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
          *	    Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
          *	    Let s=sin(x), c=cos(x),
          *		xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
          *
          *		   n	sin(xn)*sqt2	cos(xn)*sqt2
          *		----------------------------------
          *		   0	 s-c		 c+s
          *		   1	-s-c 		-c+s
          *		   2	-s+c		-c-s
          *		   3	 s+c		 c-s
          */
         switch(n & 3)
         {
            case 0:
               temp = cos(x) + sin(x);
               break;
            case 1:
               temp = -cos(x) + sin(x);
               break;
            case 2:
               temp = -cos(x) - sin(x);
               break;
            case 3:
               temp = cos(x) - sin(x);
               break;
         }
         b = invsqrtpi * temp / sqrt(x);
      }
      else
      {
         a = __hide_ieee754_j0(x);
         b = __hide_ieee754_j1(x);
         for(i = 1; i < n; i++)
         {
            temp = b;
            b = b * ((double)(i + i) / x) - a; /* avoid underflow */
            a = temp;
         }
      }
   }
   else
   {
      if(ix < 0x3e100000)
      {             /* x < 2**-29 */
                    /* x is tiny, return the first Taylor expansion of J(n,x)
                     * J(n,x) = 1/n!*(x/2)^n  - ...
                     */
         if(n > 33) /* underflow */
            b = zero;
         else
         {
            temp = x * 0.5;
            b = temp;
            for(a = one, i = 2; i <= n; i++)
            {
               a *= (double)i; /* a = n! */
               b *= temp;      /* b = (x/2)^n */
            }
            b = b / a;
         }
      }
      else
      {
         /* use backward recurrence */
         /* 			x      x^2      x^2
          *  J(n,x)/J(n-1,x) =  ----   ------   ------   .....
          *			2n  - 2(n+1) - 2(n+2)
          *
          * 			1      1        1
          *  (for large x)   =  ----  ------   ------   .....
          *			2n   2(n+1)   2(n+2)
          *			-- - ------ - ------ -
          *			 x     x         x
          *
          * Let w = 2n/x and h=2/x, then the above quotient
          * is equal to the continued fraction:
          *		    1
          *	= -----------------------
          *		       1
          *	   w - -----------------
          *			  1
          * 	        w+h - ---------
          *		       w+2h - ...
          *
          * To determine how many terms needed, let
          * Q(0) = w, Q(1) = w(w+h) - 1,
          * Q(k) = (w+k*h)*Q(k-1) - Q(k-2),
          * When Q(k) > 1e4	good for single
          * When Q(k) > 1e9	good for double
          * When Q(k) > 1e17	good for quadruple
          */
         /* determine k */
         double t, v;
         double q0, q1, h, tmp;
         int k, m;
         w = (n + n) / (double)x;
         h = 2.0 / (double)x;
         q0 = w;
         z = w + h;
         q1 = w * z - 1.0;
         k = 1;
         while(q1 < 1.0e9)
         {
            k += 1;
            z += h;
            tmp = z * q1 - q0;
            q0 = q1;
            q1 = tmp;
         }
         m = n + n;
         for(t = zero, i = 2 * (n + k); i >= m; i -= 2)
            t = one / (i / x - t);
         a = t;
         b = one;
         /*  estimate log((2/x)^n*n!) = n*log(2/x)+n*ln(n)
          *  Hence, if n*(log(2n/x)) > ...
          *  single 8.8722839355e+01
          *  double 7.09782712893383973096e+02
          *  long double 1.1356523406294143949491931077970765006170e+04
          *  then recurrent value may overflow and the result is
          *  likely underflow to zero
          */
         tmp = n;
         v = two / x;
         tmp = tmp * __hide_ieee754_log(fabs(v * tmp));
         if(tmp < 7.09782712893383973096e+02)
         {
            for(i = n - 1, di = (double)(i + i); i > 0; i--)
            {
               temp = b;
               b *= di;
               b = b / x - a;
               a = temp;
               di -= two;
            }
         }
         else
         {
            for(i = n - 1, di = (double)(i + i); i > 0; i--)
            {
               temp = b;
               b *= di;
               b = b / x - a;
               a = temp;
               di -= two;
               /* scale b to avoid spurious overflow */
               if(b > 1e100)
               {
                  a /= b;
                  t /= b;
                  b = one;
               }
            }
         }
         b = (t * __hide_ieee754_j0(x) / b);
      }
   }
   if(sgn == 1)
      return -b;
   else
      return b;
}

double __hide_ieee754_yn(int n, double x)
{
   int i, hx, ix, lx;
   int sign;
   double a, b, temp;

   hx = GET_HI(x);
   ix = 0x7fffffff & hx;
   lx = GET_LO(x);
   /* if Y(n,NaN) is NaN */
   if((ix | ((unsigned)(lx | -lx)) >> 31) > 0x7ff00000)
      return X_PLUS_X(x);
   if((ix | lx) == 0)
      return -one / zero;
   if(hx < 0)
      return zero / zero;
   sign = 1;
   if(n < 0)
   {
      n = -n;
      sign = 1 - ((n & 1) << 1);
   }
   if(n == 0)
      return (__hide_ieee754_y0(x));
   if(n == 1)
      return (sign * __hide_ieee754_y1(x));
   if(ix == 0x7ff00000)
      return zero;
   if(ix >= 0x52D00000)
   { /* x > 2**302 */
      /* (x >> n**2)
       *	    Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
       *	    Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
       *	    Let s=sin(x), c=cos(x),
       *		xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
       *
       *		   n	sin(xn)*sqt2	cos(xn)*sqt2
       *		----------------------------------
       *		   0	 s-c		 c+s
       *		   1	-s-c 		-c+s
       *		   2	-s+c		-c-s
       *		   3	 s+c		 c-s
       */
      switch(n & 3)
      {
         case 0:
            temp = sin(x) - cos(x);
            break;
         case 1:
            temp = -sin(x) - cos(x);
            break;
         case 2:
            temp = -sin(x) + cos(x);
            break;
         case 3:
            temp = sin(x) + cos(x);
            break;
      }
      b = invsqrtpi * temp / sqrt(x);
   }
   else
   {
      a = __hide_ieee754_y0(x);
      b = __hide_ieee754_y1(x);
      /* quit if b is -inf */
      for(i = 1; i < n && (GET_HI(b) != 0xfff00000); i++)
      {
         temp = b;
         b = ((double)(i + i) / x) * b - a;
         a = temp;
      }
   }
   if(sign > 0)
      return b;
   else
      return -b;
}
/*
 * wrapper j0(double x), y0(double x)
 */
double j0(double x) /* wrapper j0 */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_j0(x);
#else
   double z = __hide_ieee754_j0(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(fabs(x) > X_TLOSS)
   {
      return __hide_kernel_standard(x, x, 34); /* j0(|x|>X_TLOSS) */
   }
   else
      return z;
#endif
}

double y0(double x) /* wrapper y0 */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_y0(x);
#else
   double z;
   z = __hide_ieee754_y0(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(x <= 0.0)
   {
      if(x == 0.0)
         /* d= -one/(x-x); */
         return __hide_kernel_standard(x, x, 8);
      else
         /* d = zero/(x-x); */
         return __hide_kernel_standard(x, x, 9);
   }
   if(x > X_TLOSS)
   {
      return __hide_kernel_standard(x, x, 35); /* y0(x>X_TLOSS) */
   }
   else
      return z;
#endif
}

/*
 * wrapper of j1,y1
 */
double j1(double x) /* wrapper j1 */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_j1(x);
#else
   double z;
   z = __hide_ieee754_j1(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(fabs(x) > X_TLOSS)
   {
      return __hide_kernel_standard(x, x, 36); /* j1(|x|>X_TLOSS) */
   }
   else
      return z;
#endif
}

double y1(double x) /* wrapper y1 */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_y1(x);
#else
   double z;
   z = __hide_ieee754_y1(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(x <= 0.0)
   {
      if(x == 0.0)
         /* d= -one/(x-x); */
         return __hide_kernel_standard(x, x, 10);
      else
         /* d = zero/(x-x); */
         return __hide_kernel_standard(x, x, 11);
   }
   if(x > X_TLOSS)
   {
      return __hide_kernel_standard(x, x, 37); /* y1(x>X_TLOSS) */
   }
   else
      return z;
#endif
}

/*
 * wrapper jn(int n, double x), yn(int n, double x)
 * floating point Bessel's function of the 1st and 2nd kind
 * of order n
 *
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with division by zero signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *
 */
double jn(int n, double x) /* wrapper jn */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_jn(n, x);
#else
   double z;
   z = __hide_ieee754_jn(n, x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(fabs(x) > X_TLOSS)
   {
      return __hide_kernel_standard((double)n, x, 38); /* jn(|x|>X_TLOSS,n) */
   }
   else
      return z;
#endif
}

double yn(int n, double x) /* wrapper yn */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_yn(n, x);
#else
   double z;
   z = __hide_ieee754_yn(n, x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(x <= 0.0)
   {
      if(x == 0.0)
         /* d= -one/(x-x); */
         return __hide_kernel_standard((double)n, x, 12);
      else
         /* d = zero/(x-x); */
         return __hide_kernel_standard((double)n, x, 13);
   }
   if(x > X_TLOSS)
   {
      return __hide_kernel_standard((double)n, x, 39); /* yn(x>X_TLOSS,n) */
   }
   else
      return z;
#endif
}
