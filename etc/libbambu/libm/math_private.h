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

#ifndef _MATH_PRIVATE_H_
#define _MATH_PRIVATE_H_

// There are two options in making libm at fdlibm compile time:
// 	_IEEE_LIBM 	--- IEEE libm; smaller, and somewhat faster
//	_MULTI_LIBM	--- Support multi-standard at runtime by
//			    imposing wrapper functions defined in
//			    math_private.h:
//				_IEEE_MODE 	-- IEEE
//				_XOPEN_MODE 	-- X/OPEN
//				_POSIX_MODE 	-- POSIX/ANSI
//				_SVID3_MODE 	-- SVID

// we use IEEE LIBM and _IEEE_MODE
#define _IEEE_LIBM

typedef union
{
   double dvalue;
   unsigned long long int ull_value;
} ieee_double_shape_type;

inline unsigned long long int __hide_get_uint64(double x)
{
   ieee_double_shape_type val;
   val.dvalue = x;
   return val.ull_value;
}

#define CAST_UINT64(x) ((unsigned long long int)(x))

inline double __hide_get_double(unsigned long long int x)
{
   ieee_double_shape_type val;
   val.ull_value = x;
   return val.dvalue;
}

#define MASK_LOW ((1ULL << 32) - 1)
#define MASK_HIGH (MASK_LOW << 32)

#define GET_HI(x) ((__hide_get_uint64(x) & MASK_HIGH) >> 32)
#define GET_LO(x) (__hide_get_uint64(x) & MASK_LOW)
#define MOVE_HI(x) ((x) << 32)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d, ix0, ix1) (d) = __hide_get_double(MOVE_HI(CAST_UINT64(ix0)) | CAST_UINT64(((unsigned)ix1)))

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD(d, v) (d) = __hide_get_double(MOVE_HI(CAST_UINT64(v)) | GET_LO(d))

/* Set the less significant 32 bits of a double from an int.  */
#define SET_LOW_WORD(d, v) (d) = __hide_get_double(MOVE_HI(GET_HI(d)) | CAST_UINT64(((unsigned)v)))

/* Set the less significant 32 bits of a double from an int.  */
#define RESET_LOW_WORD(d) (d) = __hide_get_double(MOVE_HI(GET_HI(d)))

/*
 * ANSI/POSIX
 */

extern int signgam;

#define MAXFLOAT ((float)3.40282346638528860e+38)

enum fdversion
{
   fdlibm_ieee = -1,
   fdlibm_svid,
   fdlibm_xopen,
   fdlibm_posix
};

#define _LIB_VERSION_TYPE enum fdversion
#define _LIB_VERSION _fdlib_version

/* if global variable _LIB_VERSION is not desirable, one may
 * change the following to be a constant by:
 *	#define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */
extern _LIB_VERSION_TYPE _LIB_VERSION;

#define _IEEE_ fdlibm_ieee
#define _SVID_ fdlibm_svid
#define _XOPEN_ fdlibm_xopen
#define _POSIX_ fdlibm_posix

struct exception
{
   int type;
   char* name;
   double arg1;
   double arg2;
   double retval;
};

#define HUGE MAXFLOAT

/*
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS 1.41484755040568800000e+16

#define DOMAIN 1
#define SING 2
#define OVERFLOW 3
#define UNDERFLOW 4
#define TLOSS 5
#define PLOSS 6

/*
 * ANSI/POSIX
 */
extern double acos(double);
extern double asin(double);
extern double atan(double);
extern double atan2(double, double);
extern double cos(double);
extern double sin(double);
extern double tan(double);

extern double cosh(double);
extern double sinh(double);
extern double tanh(double);

extern double exp(double);
extern double frexp(double, int*);
extern double ldexp(double, int);
extern double log(double);
extern double log10(double);
extern double modf(double, double*);

extern double pow(double, double);
extern double sqrt(double);

extern double ceil(double);
extern double fabs(double);
extern double floor(double);
extern double fmod(double, double);

extern double erf(double);
extern double erfc(double);
extern double gamma(double);
extern double hypot(double, double);
extern int fpclassify(double);
extern int finite(double);
extern int isinf(double);
extern int isinf_sign(double);
extern int isnan(double);
extern int isnormal(double);
extern int signbit(double);
extern double j0(double);
extern double j1(double);
extern double jn(int, double);
extern double lgamma(double);
extern double y0(double);
extern double y1(double);
extern double yn(int, double);

extern double acosh(double);
extern double asinh(double);
extern double atanh(double);
extern double cbrt(double);
extern double logb(double);
extern double nextafter(double, double);
extern double remainder(double, double);
#ifdef _SCALB_INT
extern double scalb(double, int);
#else
extern double scalb(double, double);
#endif

extern int matherr(struct exception*);

/*
 * IEEE Test Vector
 */
extern double significand(double);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern double copysign(double, double);
extern double nan(const char*);
extern double nans(const char*);
extern double inf(void);
extern double infinity(const char*);
extern double huge_val(void);
extern int ilogb(double);
extern double rint(double);
extern double scalbn(double, int);

/*
 * BSD math library entry points
 */
extern double expm1(double);
extern double log1p(double);

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
#ifdef _REENTRANT
extern double gamma_r(double, int*);
extern double lgamma_r(double, int*);
#endif /* _REENTRANT */

/* ieee style elementary functions */
extern double __hide_kernel_cos(double x, double y);
extern double __hide_kernel_sin(double x, double y, int iy);
#ifndef HAVE_FLOPOCO
extern double __hide_ieee754_exp(double x);
extern double __hide_ieee754_log(double x);
#endif
extern double __hide_ieee754_sqrt(double);
extern double __hide_ieee754_acos(double);
extern double __hide_ieee754_acosh(double);
extern double __hide_ieee754_log(double);
extern double __hide_ieee754_atanh(double);
extern double __hide_ieee754_asin(double);
extern double __hide_ieee754_atan2(double, double);
extern double __hide_ieee754_exp(double);
extern double __hide_ieee754_cosh(double);
extern double __hide_ieee754_fmod(double, double);
extern double __hide_ieee754_pow(double, double);
extern double __hide_ieee754_log10(double);
extern double __hide_ieee754_sinh(double);
extern double __hide_ieee754_hypot(double, double);
extern double __hide_ieee754_j0(double);
extern double __hide_ieee754_j1(double);
extern double __hide_ieee754_y0(double);
extern double __hide_ieee754_y1(double);
extern double __hide_ieee754_jn(int, double);
extern double __hide_ieee754_yn(int, double);
extern double __hide_ieee754_remainder(double, double);
// extern int __hide_ieee754_rem_pio2(double, double*);
#ifdef _SCALB_INT
extern double __hide_ieee754_scalb(double, int);
#else
extern double __hide_ieee754_scalb(double, double);
#endif

#ifdef NO_RAISE_EXCEPTIONS
#define math_opt_barrier(x)  \
   ({                        \
      __typeof(x) __x = (x); \
      __x;                   \
   })
#define math_force_eval(x)
#define X_PLUS_X(x) (x)
#else
#define math_opt_barrier(x)           \
   ({                                 \
      __typeof(x) __x;                \
      __asm("" : "=f"(__x) : "0"(x)); \
      __x;                            \
   })
#define math_force_eval(x)                  \
   do                                       \
   {                                        \
      __typeof(x) __x = (x);                \
      if(sizeof(x) <= sizeof(double))       \
         __asm __volatile("" : : "m"(__x)); \
      else                                  \
         __asm __volatile("" : : "f"(__x)); \
   } while(0)
#define X_PLUS_X(x) ((x) + (x))
#endif

/* collection of costants used by libm */
#include "math_constants.h"

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

#endif /* _MATH_PRIVATE_H_ */
