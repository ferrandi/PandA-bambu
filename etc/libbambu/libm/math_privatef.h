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

#ifndef _MATH_PRIVATEF_H_
#define _MATH_PRIVATEF_H_

//There are two options in making libm at fdlibm compile time:
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
  float value;
  unsigned int u_value;
} ieee_float_shape_type;


inline
unsigned int __hide_get_uint32(float x)
{
  ieee_float_shape_type val;
  val.value = x;
  return val.u_value;
}
#define GET_FLOAT_WORD(i,d) (i) = __hide_get_uint32(d)

inline
float __hide_get_float(unsigned int x)
{
  ieee_float_shape_type val;
  val.u_value = x;
  return val.value;
}
/* Set a float from a 32 bit int.  */
#define SET_FLOAT_WORD(d,i) (d) = __hide_get_float(i)

/* Most routines need to check whether a float is finite, infinite, or not a
   number, and many need to know whether the result of an operation will
   overflow.  These conditions depend on whether the largest exponent is
   used for NaNs & infinities, or whether it's used for finite numbers.  The
   macros below wrap up that kind of information:

   FLT_UWORD_IS_FINITE(X)
	True if a positive float with bitmask X is finite.

   FLT_UWORD_IS_NAN(X)
	True if a positive float with bitmask X is not a number.

   FLT_UWORD_IS_INFINITE(X)
	True if a positive float with bitmask X is +infinity.

   FLT_UWORD_MAX
	The bitmask of FLT_MAX.

   FLT_UWORD_HALF_MAX
	The bitmask of FLT_MAX/2.

   FLT_UWORD_EXP_MAX
	The bitmask of the largest finite exponent (129 if the largest
	exponent is used for finite numbers, 128 otherwise).

   FLT_UWORD_LOG_MAX
	The bitmask of log(FLT_MAX), rounded down.  This value is the largest
	input that can be passed to exp() without producing overflow.

   FLT_UWORD_LOG_2MAX
	The bitmask of log(2*FLT_MAX), rounded down.  This value is the
	largest input than can be passed to cosh() without producing
	overflow.

   FLT_LARGEST_EXP
	The largest biased exponent that can be used for finite numbers
	(255 if the largest exponent is used for finite numbers, 254
	otherwise) */

#ifdef _FLT_LARGEST_EXPONENT_IS_NORMAL
#define FLT_UWORD_IS_FINITE(x) 1
#define FLT_UWORD_IS_NAN(x) 0
#define FLT_UWORD_IS_INFINITE(x) 0
#define FLT_UWORD_MAX 0x7fffffff
#define FLT_UWORD_EXP_MAX 0x43010000
#define FLT_UWORD_LOG_MAX 0x42b2d4fc
#define FLT_UWORD_LOG_2MAX 0x42b437e0
#define HUGE ((float)0X1.FFFFFEP128)
#else
#define FLT_UWORD_IS_FINITE(x) ((x)<0x7f800000L)
#define FLT_UWORD_IS_NAN(x) ((x)>0x7f800000L)
#define FLT_UWORD_IS_INFINITE(x) ((x)==0x7f800000L)
#define FLT_UWORD_MAX 0x7f7fffffL
#define FLT_UWORD_EXP_MAX 0x43000000
#define FLT_UWORD_LOG_MAX 0x42b17217
#define FLT_UWORD_LOG_2MAX 0x42b2d4fc
#define HUGE ((float)3.40282346638528860e+38)
#endif
#define FLT_UWORD_HALF_MAX (FLT_UWORD_MAX-(1L<<23))
#define FLT_LARGEST_EXP (FLT_UWORD_MAX>>23)

#define STRICT_ASSIGN(type, lval, rval) ((lval) = (type)(rval))

/* Many routines check for zero and subnormal numbers.  Such things depend
   on whether the target supports denormals or not:

   FLT_UWORD_IS_ZERO(X)
	True if a positive float with bitmask X is +0.	Without denormals,
	any float with a zero exponent is a +0 representation.	With
	denormals, the only +0 representation is a 0 bitmask.

   FLT_UWORD_IS_SUBNORMAL(X)
	True if a non-zero positive float with bitmask X is subnormal.
	(Routines should check for zeros first.)

   FLT_UWORD_MIN
	The bitmask of the smallest float above +0.  Call this number
	REAL_FLT_MIN...

   FLT_UWORD_EXP_MIN
	The bitmask of the float representation of REAL_FLT_MIN's exponent.

   FLT_UWORD_LOG_MIN
	The bitmask of |log(REAL_FLT_MIN)|, rounding down.

   FLT_SMALLEST_EXP
	REAL_FLT_MIN's exponent - EXP_BIAS (1 if denormals are not supported,
	-22 if they are).
*/

#ifdef FLT_NO_DENORMALS
#define FLT_UWORD_IS_ZERO(x) ((x)<0x00800000L)
#define FLT_UWORD_IS_SUBNORMAL(x) 0
#define FLT_UWORD_MIN 0x00800000
#define FLT_UWORD_EXP_MIN 0x42fc0000
#define FLT_UWORD_LOG_MIN 0x42aeac50
#define FLT_SMALLEST_EXP 1
#else
#define FLT_UWORD_IS_ZERO(x) ((x)==0)
#define FLT_UWORD_IS_SUBNORMAL(x) ((x)<0x00800000L)
#define FLT_UWORD_MIN 0x00000001
#define FLT_UWORD_EXP_MIN 0x43160000
#define FLT_UWORD_LOG_MIN 0x42cff1b5
#define FLT_SMALLEST_EXP -22
#endif

#ifdef NO_RAISE_EXCEPTIONS
# define math_opt_barrier(x) \
({ __typeof (x) __x = (x); __x; })
# define math_force_eval(x)
#define X_PLUS_X(x) (x)
#else
# define math_opt_barrier(x) \
({ __typeof (x) __x = (x); __asm ("" : "+m" (__x)); __x; })
# define math_force_eval(x) \
({ __typeof (x) __x = (x); __asm __volatile__ ("" : : "m" (__x)); })
#define X_PLUS_X(x) ((x)+(x))
#endif
/*
 * ANSI/POSIX
 */

enum fdversion {fdlibm_ieee = -1, fdlibm_svid, fdlibm_xopen, fdlibm_posix};

#define _LIB_VERSION_TYPE enum fdversion
#define _LIB_VERSION _fdlib_version  

/* if global variable _LIB_VERSION is not desirable, one may 
 * change the following to be a constant by: 
 *	#define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */ 
extern  _LIB_VERSION_TYPE  _LIB_VERSION;

#define _IEEE_  fdlibm_ieee
#define _SVID_  fdlibm_svid
#define _XOPEN_ fdlibm_xopen
#define _POSIX_ fdlibm_posix


/* 
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS		1.41484755040568800000e+16 

/*
 * ANSI/POSIX
 */
extern float acosf (float);
extern float asinf (float);
extern float atanf (float);
extern float atan2f (float, float);
extern float cosf (float);
extern float sinf (float);
extern float tanf (float);

extern float coshf (float);
extern float sinhf (float);
extern float tanhf (float);

extern float expf (float);
extern float frexpf (float, int *);
extern float ldexpf (float, int);
extern float logf (float);
extern float log10f (float);
extern float modff (float, float *);

extern float powf (float, float);
extern float sqrtf (float);

extern float ceilf (float);
extern float fabsf (float);
extern float floorf (float);
extern float fmodf (float, float);

extern float erff (float);
extern float erfcf (float);
extern float gammaf (float);
extern float hypotf (float, float);
extern int   isnanf (float);
extern int   __finitef (float);
extern float j0f (float);
extern float j1f (float);
extern float jnf (int, float);
extern float lgammaf (float);
extern float y0f (float);
extern float y1f (float);
extern float ynf (int, float);

extern float acoshf (float);
extern float asinhf (float);
extern float atanhf (float);
extern float cbrtf (float);
extern float logbf (float);
extern float nextafterf (float, float);
extern float remainderf (float, float);
#ifdef _SCALB_INT
extern float scalbf (float, int);
#else
extern float scalbf (float, float);
#endif

/*
 * IEEE Test Vector
 */
extern float significandf (float);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern float copysignf (float, float);
extern int   ilogbf (float);
extern float rintf (float);
extern float scalbnf (float, int);

/*
 * BSD math library entry points
 */
extern float expm1f (float);
extern float log1pf (float);

/**
 * kernel functions
*/
extern float __hide_kernel_sinf(float x, float y, int iy);
extern float __hide_kernel_cosf(float x, float y);
extern float __hide_kernel_tanf(float x, float y, int iy);

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
#ifdef _REENTRANT
extern float gammaf_r (float, int *);
extern float lgammaf_r (float, int *);
#endif	/* _REENTRANT */

/* ieee style elementary functions */
extern float __hide_ieee754_sqrtf (float);			
extern float __hide_ieee754_acosf (float);			
extern float __hide_ieee754_acoshf (float);			
extern float __hide_ieee754_logf (float);			
extern float __hide_ieee754_atanhf (float);			
extern float __hide_ieee754_asinf (float);			
extern float __hide_ieee754_atan2f (float,float);			
extern float __hide_ieee754_expf (float);
extern float __hide_ieee754_coshf (float);
extern float __hide_ieee754_fmodf (float,float);
extern float __hide_ieee754_powf (float,float);
extern float __hide_ieee754_lgammaf_r (float,int *);
extern float __hide_ieee754_lgammaf (float);
extern float __hide_ieee754_gammaf (float);
extern float __hide_ieee754_log10f (float);
extern float __hide_ieee754_sinhf (float);
extern float __hide_ieee754_hypotf (float,float);
extern float __hide_ieee754_j0f (float);
extern float __hide_ieee754_j1f (float);
extern float __hide_ieee754_y0f (float);
extern float __hide_ieee754_y1f (float);
extern float __hide_ieee754_jnf (int,float);
extern float __hide_ieee754_ynf (int,float);
extern float __hide_ieee754_remainderf (float,float);
extern int   __hide_ieee754_rem_pio2f (float,float*);
#ifdef _SCALB_INT
extern float __hide_ieee754_scalbf (float,int);
#else
extern float __hide_ieee754_scalbf (float,float);
#endif

#ifndef FLT_EVAL_METHOD
#define FLT_EVAL_METHOD 0
typedef float float_t;
typedef double double_t;
#endif /* FLT_EVAL_METHOD */

#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4

# ifndef HUGE_VALF
#  define HUGE_VALF (__builtin_huge_valf())
# endif

#ifndef FP_ILOGB0
# define FP_ILOGB0 (-INT_MAX -1)
#endif
#ifndef FP_ILOGBNAN
# define FP_ILOGBNAN (-INT_MAX -1)
#endif

#define _M_LN2        0.693147180559945309417

/* Useful constants.  */

#define MAXFLOAT	3.40282347e+38F

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		_M_LN2
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_TWOPI         (M_PI * 2.0)
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_3PI_4		2.3561944901923448370E0
#define M_SQRTPI        1.77245385090551602792981
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#define M_LN2LO         1.9082149292705877000E-10
#define M_LN2HI         6.9314718036912381649E-1
#define M_SQRT3	1.73205080756887719000
#define M_IVLN10        0.43429448190325182765 /* 1 / log(10) */
#define M_LOG2_E        _M_LN2
#define M_INVLN2        1.4426950408889633870E0  /* 1 / log(2) */


#endif /* _MATH_PRIVATEF_H_ */
