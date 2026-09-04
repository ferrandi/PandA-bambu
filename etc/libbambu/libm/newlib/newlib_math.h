#ifndef  _MATH_H_

#define  _MATH_H_

#include <sys/cdefs.h>

/*  ISO C++.  */

#ifdef __cplusplus
#if !(defined(_BEGIN_STD_C) && defined(_END_STD_C))
#ifdef _HAVE_STD_CXX
#define _BEGIN_STD_C namespace std { extern "C" {
#define _END_STD_C  } }
#else
#define _BEGIN_STD_C extern "C" {
#define _END_STD_C  }
#endif
#endif
#else
#define _BEGIN_STD_C
#define _END_STD_C
#endif

_BEGIN_STD_C

/* Natural log of 2 */
#define _M_LN2        0.693147180559945309417

#if __GNUC_PREREQ (3, 3)
 /* gcc >= 3.3 implicitly defines builtins for HUGE_VALx values.  */

# ifndef HUGE_VAL
#  define HUGE_VAL (__builtin_huge_val())
# endif

# ifndef HUGE_VALF
#  define HUGE_VALF (__builtin_huge_valf())
# endif

# ifndef HUGE_VALL
#  define HUGE_VALL (__builtin_huge_vall())
# endif

# ifndef INFINITY
#  define INFINITY (__builtin_inff())
# endif

# ifndef NAN
#  define NAN (__builtin_nanf(""))
# endif

#else /* !gcc >= 3.3  */

 /*      No builtins.  Use fixed defines instead.  (All 3 HUGE plus the INFINITY
  * and NAN macros are required to be constant expressions.  Using a variable--
  * even a static const--does not meet this requirement, as it cannot be
  * evaluated at translation time.)
  *      The infinities are done using numbers that are far in excess of
  * something that would be expected to be encountered in a floating-point
  * implementation.  (A more certain way uses values from float.h, but that is
  * avoided because system includes are not supposed to include each other.)
  *      This method might produce warnings from some compilers.  (It does in
  * newer GCCs, but not for ones that would hit this #else.)  If this happens,
  * please report details to the Newlib mailing list.  */

 #ifndef HUGE_VAL
  #define HUGE_VAL (1.0e999999999)
 #endif

 #ifndef HUGE_VALF
  #define HUGE_VALF (1.0e999999999F)
 #endif

 #if !defined(HUGE_VALL)  &&  defined(_HAVE_LONG_DOUBLE)
  #define HUGE_VALL (1.0e999999999L)
 #endif

 #if !defined(INFINITY)
  #define INFINITY (HUGE_VALF)
 #endif

 #if !defined(NAN)
  #if defined(__GNUC__)  &&  defined(__cplusplus)
    /* Exception:  older g++ versions warn about the divide by 0 used in the
     * normal case (even though older gccs do not).  This trick suppresses the
     * warning, but causes errors for plain gcc, so is only used in the one
     * special case.  */
    static const union { __ULong __i[1]; float __d; } __Nanf = {0x7FC00000};
    #define NAN (__Nanf.__d)
  #else
    #define NAN (0.0F/0.0F)
  #endif
 #endif

#endif /* !gcc >= 3.3  */

/* Reentrant ANSI C functions.  */

extern double atan (double);
extern double cos (double);
extern double sin (double);
extern double tan (double);
extern double tanh (double);
extern double frexp (double, int *);
extern double modf (double, double *);
extern double ceil (double);
extern double fabs (double);
extern double floor (double);

/* Non reentrant ANSI C functions.  */

extern double acos (double);
extern double asin (double);
extern double atan2 (double, double);
extern double cosh (double);
extern double sinh (double);
extern double exp (double);
extern double ldexp (double, int);
extern double log (double);
extern double log10 (double);
extern double pow (double, double);
extern double sqrt (double);
extern double fmod (double, double);

extern int finite (double);
extern int finitef (float);
extern int isinff (float);
extern int isnanf (float);
#if !defined(__cplusplus) || __cplusplus < 201103L
extern int isinf (double);
#endif
#if !defined(__cplusplus) || __cplusplus < 201103L
extern int isnan (double);
#endif

/* ISO C99 types and macros. */

/* Assume basic definitions.  */
typedef float  float_t;
typedef double double_t;

#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4

#define FP_ILOGBNAN (-__INT_MAX__ - 1)
#define FP_ILOGB0 FP_ILOGBNAN

#ifndef MATH_ERRNO
# define MATH_ERRNO 1
#endif
#ifndef MATH_ERREXCEPT
# define MATH_ERREXCEPT 2
#endif
#ifndef math_errhandling
# ifdef _IEEE_LIBM
#  define _MATH_ERRHANDLING_ERRNO 0
# else
#  define _MATH_ERRHANDLING_ERRNO MATH_ERRNO
# endif
# ifdef _SUPPORTS_ERREXCEPT
#  define _MATH_ERRHANDLING_ERREXCEPT MATH_ERREXCEPT
# else
#  define _MATH_ERRHANDLING_ERREXCEPT 0
# endif
# define math_errhandling (_MATH_ERRHANDLING_ERRNO | _MATH_ERRHANDLING_ERREXCEPT)
#endif

extern int __isinff (float);
extern int __isinfd (double);
extern int __isnanf (float);
extern int __isnand (double);
extern int __fpclassifyf (float);
extern int __fpclassifyd (double);
extern int __signbitf (float);
extern int __signbitd (double);

/* Note: isinf and isnan were once functions in newlib that took double
 *       arguments.  C99 specifies that these names are reserved for macros
 *       supporting multiple floating point types.  Thus, they are
 *       now defined as macros.  Implementations of the old functions
 *       taking double arguments still exist for compatibility purposes
 *       (prototypes for them are earlier in this header).  */

#if __GNUC_PREREQ (4, 4)
  #define fpclassify(__x) (__builtin_fpclassify (FP_NAN, FP_INFINITE, \
						 FP_NORMAL, FP_SUBNORMAL, \
						 FP_ZERO, __x))
  #ifndef isfinite
    #define isfinite(__x)	(__builtin_isfinite (__x))
  #endif
  #ifndef isinf
    #define isinf(__x) (__builtin_isinf_sign (__x))
  #endif
  #ifndef isnan
    #define isnan(__x) (__builtin_isnan (__x))
  #endif
  #define isnormal(__x) (__builtin_isnormal (__x))
#else
  #define fpclassify(__x) \
	  ((sizeof(__x) == sizeof(float))  ? __fpclassifyf(__x) : \
	  __fpclassifyd(__x))
  #ifndef isfinite
    #define isfinite(__y) \
	    (__extension__ ({int __cy = fpclassify(__y); \
			     __cy != FP_INFINITE && __cy != FP_NAN;}))
  #endif
  #ifndef isinf
    #define isinf(__x) (fpclassify(__x) == FP_INFINITE)
  #endif
  #ifndef isnan
    #define isnan(__x) (fpclassify(__x) == FP_NAN)
  #endif
  #define isnormal(__x) (fpclassify(__x) == FP_NORMAL)
#endif

#if __GNUC_PREREQ (4, 0)
  #if defined(_HAVE_LONG_DOUBLE)
    #define signbit(__x) \
	    ((sizeof(__x) == sizeof(float))  ? __builtin_signbitf(__x) : \
	     (sizeof(__x) == sizeof(double)) ? __builtin_signbit (__x) : \
					       __builtin_signbitl(__x))
  #else
    #define signbit(__x) \
	    ((sizeof(__x) == sizeof(float))  ? __builtin_signbitf(__x) : \
					       __builtin_signbit (__x))
  #endif
#else
  #define signbit(__x) \
	  ((sizeof(__x) == sizeof(float))  ?  __signbitf(__x) : \
		  __signbitd(__x))
#endif

#if __GNUC_PREREQ (2, 97)
#define isgreater(__x,__y)	(__builtin_isgreater (__x, __y))
#define isgreaterequal(__x,__y)	(__builtin_isgreaterequal (__x, __y))
#define isless(__x,__y)		(__builtin_isless (__x, __y))
#define islessequal(__x,__y)	(__builtin_islessequal (__x, __y))
#define islessgreater(__x,__y)	(__builtin_islessgreater (__x, __y))
#define isunordered(__x,__y)	(__builtin_isunordered (__x, __y))
#else
#define isgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x > __y);}))
#define isgreaterequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x >= __y);}))
#define isless(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y);}))
#define islessequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x <= __y);}))
#define islessgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y || __x > __y);}))

#define isunordered(a,b) \
          (__extension__ ({__typeof__(a) __a = (a); __typeof__(b) __b = (b); \
                           fpclassify(__a) == FP_NAN || fpclassify(__b) == FP_NAN;}))
#endif

/* Non ANSI double precision functions.  */

extern double infinity (void);
extern double nan (const char *);
extern double copysign (double, double);
extern double logb (double);
extern int ilogb (double);

extern double asinh (double);
extern double cbrt (double);
extern double nextafter (double, double);
extern double rint (double);
extern double scalbn (double, int);

extern double exp2 (double);
extern double scalbln (double, long int);
extern double tgamma (double);
extern double nearbyint (double);
extern long int lrint (double);
extern long long int llrint (double);
extern double round (double);
extern long int lround (double);
extern long long int llround (double);
extern double trunc (double);
extern double remquo (double, double, int *);
extern double fdim (double, double);
extern double fmax (double, double);
extern double fmin (double, double);
extern double fma (double, double, double);

extern double log1p (double);
extern double expm1 (double);

extern double acosh (double);
extern double atanh (double);
extern double remainder (double, double);
// extern double gamma (double);
// extern double lgamma (double);
extern double erf (double);
extern double erfc (double);
extern double log2 (double);
#if !defined(__cplusplus)
#define log2(x) (log (x) / _M_LN2)
#endif

extern double hypot (double, double);


/* Single precision versions of ANSI functions.  */

extern float atanf (float);
extern float cosf (float);
extern float sinf (float);
extern float tanf (float);
extern float tanhf (float);
extern float frexpf (float, int *);
extern float modff (float, float *);
extern float ceilf (float);
extern float fabsf (float);
extern float floorf (float);

extern float acosf (float);
extern float asinf (float);
extern float atan2f (float, float);
extern float coshf (float);
extern float sinhf (float);
extern float expf (float);
extern float ldexpf (float, int);
extern float logf (float);
extern float log10f (float);
extern float powf (float, float);
extern float sqrtf (float);
extern float fmodf (float, float);

/* Other single precision functions.  */

extern float exp2f (float);
extern float scalblnf (float, long int);
extern float tgammaf (float);
extern float nearbyintf (float);
extern long int lrintf (float);
extern long long int llrintf (float);
extern float roundf (float);
extern long int lroundf (float);
extern long long int llroundf (float);
extern float truncf (float);
extern float remquof (float, float, int *);
extern float fdimf (float, float);
extern float fmaxf (float, float);
extern float fminf (float, float);
extern float fmaf (float, float, float);

extern float infinityf (void);
extern float nanf (const char *);
extern float copysignf (float, float);
extern float logbf (float);
extern int ilogbf (float);

extern float asinhf (float);
extern float cbrtf (float);
extern float nextafterf (float, float);
extern float rintf (float);
extern float scalbnf (float, int);
extern float log1pf (float);
extern float expm1f (float);

extern float acoshf (float);
extern float atanhf (float);
extern float remainderf (float, float);
extern float gammaf (float);
extern float lgammaf (float);
extern float erff (float);
extern float erfcf (float);
extern float log2f (float);
extern float hypotf (float, float);


extern double drem (double, double);
extern float dremf (float, float);
extern double gamma_r (double, int *);
extern double lgamma_r (double, int *);
extern float gammaf_r (float, int *);
extern float lgammaf_r (float, int *);

extern double y0 (double);
extern double y1 (double);
extern double yn (int, double);
extern double j0 (double);
extern double j1 (double);
extern double jn (int, double);

extern float y0f (float);
extern float y1f (float);
extern float ynf (int, float);
extern float j0f (float);
extern float j1f (float);
extern float jnf (int, float);

/* GNU extensions */
extern void sincos (double, double *, double *);
extern void sincosf (float, float *, float *);
# ifndef exp10
extern double exp10 (double);
# endif
# ifndef pow10
extern double pow10 (double);
# endif
# ifndef exp10f
extern float exp10f (float);
# endif
# ifndef pow10f
extern float pow10f (float);
# endif

/* Useful constants.  */

#define MAXFLOAT	3.40282347e+38F

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		_M_LN2
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440

#define M_TWOPI         (M_PI * 2.0)
#define M_3PI_4		2.3561944901923448370E0
#define M_SQRTPI        1.77245385090551602792981
#define M_LN2LO         1.9082149292705877000E-10
#define M_LN2HI         6.9314718036912381649E-1
#define M_SQRT3	1.73205080756887719000
#define M_IVLN10        0.43429448190325182765 /* 1 / log(10) */
#define M_LOG2_E        _M_LN2
#define M_INVLN2        1.4426950408889633870E0  /* 1 / log(2) */

_END_STD_C

#endif /* _MATH_H_ */
