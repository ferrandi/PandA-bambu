
/*============================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/

/*----------------------------------------------------------------------------
| The macro `FLOATX80' must be defined to enable the extended double-precision
| floating-point format `__floatx80'.  If this macro is not defined, the
| `__floatx80' type will not be defined, and none of the functions that either
| input or output the `__floatx80' type will be defined.  The same applies to
| the `FLOAT128' macro and the quadruple-precision format `__float128'.
*----------------------------------------------------------------------------*/
//#define FLOATX80
//#define FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point types.
*----------------------------------------------------------------------------*/
typedef __bits32 __float32;
typedef __bits64 __float64;
#ifdef FLOATX80
typedef struct
{
   __bits16 high;
   __bits64 low;
} __floatx80;
#endif
#ifdef FLOAT128
typedef struct
{
   __bits64 high, low;
} __float128;
#endif

/**
 * View convert expr data structures
 */
typedef float __Tfloat32;
typedef double __Tfloat64;
#ifdef FLOATX80
typedef long double __Tfloatx80;
#endif
#ifdef FLOAT128
typedef __float128 __Tfloat128;
#endif
typedef union {
   __float32 b;
   __Tfloat32 f;
} __convert32;
typedef union {
   __float64 b;
   __Tfloat64 f;
} __convert64;
#ifdef FLOATX80
typedef union {
   __floatx80 b;
   __Tfloatx80 f;
} __convertx80;
#endif
#ifdef FLOAT128
typedef union {
   __float128 b;
   __Tfloat128 f;
} __convert128;
#endif

/**
 * Floating point macro interfaces
 */
#define SF_ADAPTER1(fun_name, prec)                                 \
   __Tfloat##prec fun_name##if(__Tfloat##prec a, __Tfloat##prec b); \
   __Tfloat##prec fun_name##if(__Tfloat##prec a, __Tfloat##prec b)  \
   {                                                                \
      __convert##prec a_c, b_c, res_c;                              \
      a_c.f = a;                                                    \
      b_c.f = b;                                                    \
      res_c.b = fun_name(a_c.b, b_c.b);                             \
      return res_c.f;                                               \
   }

#define SF_ADAPTER1_ternary(fun_name, prec)                                 \
   __Tfloat##prec fun_name##if(__Tfloat##prec a, __Tfloat##prec b, __Tfloat##prec c); \
   __Tfloat##prec fun_name##if(__Tfloat##prec a, __Tfloat##prec b, __Tfloat##prec c)  \
   {                                                                \
      __convert##prec a_c, b_c, c_c, res_c;                         \
      a_c.f = a;                                                    \
      b_c.f = b;                                                    \
      c_c.f = c;                                                    \
      res_c.b = fun_name(a_c.b, b_c.b, c_c.b);                      \
      return res_c.f;                                               \
   }

#define SF_ADAPTER1_unary(fun_name, prec)         \
   __Tfloat##prec fun_name##if(__Tfloat##prec a); \
   __Tfloat##prec fun_name##if(__Tfloat##prec a)  \
   {                                              \
      __convert##prec a_c, res_c;                 \
      a_c.f = a;                                  \
      res_c.b = fun_name(a_c.b);                  \
      return res_c.f;                             \
   }
#define SF_ADAPTER2(fun_name, prec)                         \
   __flag fun_name##if(__Tfloat##prec a, __Tfloat##prec b); \
   __flag fun_name##if(__Tfloat##prec a, __Tfloat##prec b)  \
   {                                                        \
      __convert##prec a_c, b_c, res_c;                      \
      a_c.f = a;                                            \
      b_c.f = b;                                            \
      return fun_name(a_c.b, b_c.b);                        \
   }
#define SF_ADAPTER2_unary(fun_name, prec_in, prec_out) \
   __int##prec_out fun_name##if(__Tfloat##prec_in a)   \
   {                                                   \
      __convert##prec_in a_c;                          \
      a_c.f = a;                                       \
      return fun_name(a_c.b);                          \
   }
#define SF_UADAPTER2_unary(fun_name, prec_in, prec_out) \
   __uint##prec_out fun_name##if(__Tfloat##prec_in a)   \
   {                                                    \
      __convert##prec_in a_c;                           \
      a_c.f = a;                                        \
      return fun_name(a_c.b);                           \
   }
#define SF_ADAPTER3_unary(fun_name, prec_in, prec_out) \
   __Tfloat##prec_out fun_name##if(__int##prec_in a)   \
   {                                                   \
      __convert##prec_out res_c;                       \
      res_c.b = fun_name(a);                           \
      return res_c.f;                                  \
   }
#define SF_UADAPTER3_unary(fun_name, prec_in, prec_out) \
   __Tfloat##prec_out fun_name##if(__uint##prec_in a)   \
   {                                                    \
      __convert##prec_out res_c;                        \
      res_c.b = fun_name(a);                            \
      return res_c.f;                                   \
   }
#define SF_ADAPTER4_unary(fun_name, prec_in, prec_out)  \
   __Tfloat##prec_out fun_name##if(__Tfloat##prec_in a) \
   {                                                    \
      __convert##prec_in a_c;                           \
      __convert##prec_out res_c;                        \
      a_c.f = a;                                        \
      res_c.b = fun_name(a_c.b);                        \
      return res_c.f;                                   \
   }

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
static const __int8 __float_detect_tininess = 0;
#else
extern __int8 __float_detect_tininess;
#endif
enum
{
   float_tininess_after_rounding = 0,
   float_tininess_before_rounding = 1
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point rounding mode.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
static const __int8 __float_rounding_mode = 0;
#else
extern __int8 __float_rounding_mode;
#endif
enum
{
   float_round_nearest_even = 0,
   float_round_to_zero = 1,
   float_round_down = 2,
   float_round_up = 3
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point exception flags.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
static const __int8 __float_exception_flags = 0;
#else
extern __int8 __float_exception_flags;
#endif
enum
{
   float_flag_inexact = 1,
   float_flag_underflow = 2,
   float_flag_overflow = 4,
   float_flag_divbyzero = 8,
   float_flag_invalid = 16
};

/*----------------------------------------------------------------------------
| Routine to raise any or all of the software IEC/IEEE floating-point
| exception flags.
*----------------------------------------------------------------------------*/
static void __float_raise(__int8);

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
static __float32 __int32_to_float32(__int32);
SF_ADAPTER3_unary(__int32_to_float32, 32, 32);
static inline __float32 __int8_to_float32(__int8 a)
{
   return __int32_to_float32(a);
}
SF_ADAPTER3_unary(__int8_to_float32, 8, 32);
static inline __float32 __int16_to_float32(__int16 a)
{
   return __int32_to_float32(a);
}
SF_ADAPTER3_unary(__int16_to_float32, 16, 32);
static __float32 __uint32_to_float32(__uint32);
SF_UADAPTER3_unary(__uint32_to_float32, 32, 32);
static inline __float32 __uint8_to_float32(__uint8 a)
{
   return __uint32_to_float32(a);
}
SF_UADAPTER3_unary(__uint8_to_float32, 8, 32);
static inline __float32 __uint16_to_float32(__uint16 a)
{
   return __uint32_to_float32(a);
}
SF_UADAPTER3_unary(__uint16_to_float32, 16, 32);
static __float64 __int32_to_float64(__int32);
SF_ADAPTER3_unary(__int32_to_float64, 32, 64);
static __float64 __uint32_to_float64(__uint32);
SF_UADAPTER3_unary(__uint32_to_float64, 32, 64);
#ifdef FLOATX80
static __floatx80 __int32_to_floatx80(__int32);
#endif
#ifdef FLOAT128
static __float128 __int32_to_float128(__int32);
#endif
static __float32 __int64_to_float32(__int64);
SF_ADAPTER3_unary(__int64_to_float32, 64, 32);
static __float32 __uint64_to_float32(__uint64);
SF_UADAPTER3_unary(__uint64_to_float32, 64, 32);
static __float64 __int64_to_float64(__int64);
SF_ADAPTER3_unary(__int64_to_float64, 64, 64);
static __float64 __uint64_to_float64(__uint64 a);
SF_UADAPTER3_unary(__uint64_to_float64, 64, 64);
#ifdef FLOATX80
static __floatx80 __int64_to_floatx80(__int64);
#endif
#ifdef FLOAT128
static __float128 __int64_to_float128(__int64);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
static __int32 __float32_to_int32(__float32);
SF_ADAPTER2_unary(__float32_to_int32, 32, 32);
static __int32 __float32_to_int32_round_to_zero(__float32);
SF_ADAPTER2_unary(__float32_to_int32_round_to_zero, 32, 32);
static __uint32 __float32_to_uint32_round_to_zero(__float32 a);
SF_UADAPTER2_unary(__float32_to_uint32_round_to_zero, 32, 32);
static __int64 __float32_to_int64(__float32);
SF_ADAPTER2_unary(__float32_to_int64, 32, 64);
static __int64 __float32_to_int64_round_to_zero(__float32);
SF_ADAPTER2_unary(__float32_to_int64_round_to_zero, 32, 64);
static __uint64 __float32_to_uint64_round_to_zero(__float32);
SF_UADAPTER2_unary(__float32_to_uint64_round_to_zero, 32, 64);
static __float64 __float32_to_float64(__float32);
SF_ADAPTER4_unary(__float32_to_float64, 32, 64);
#ifdef FLOATX80
static __floatx80 __float32_to_floatx80(__float32);
SF_ADAPTER4_unary(__float32_to_floatx80, 32, x80);
#endif
#ifdef FLOAT128
static __float128 __float32_to_float128(__float32);
SF_ADAPTER4_unary(__float32_to_float128, 32, 128);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision operations.
*----------------------------------------------------------------------------*/
static __float32 __float32_round_to_int(__float32);
static __float32 __float32_add(__float32, __float32);
SF_ADAPTER1(__float32_add, 32);
static __float32 __float32_sub(__float32, __float32);
SF_ADAPTER1(__float32_sub, 32);
static __float32 __float32_mul(__float32, __float32);
SF_ADAPTER1(__float32_mul, 32);
static __float32 __float32_divG(__float32, __float32);
SF_ADAPTER1(__float32_divG, 32);
static __float32 __float32_divSRT4(__float32, __float32);
SF_ADAPTER1(__float32_divSRT4, 32);
static __float32 __float32_rem(__float32, __float32);
static __float32 __float32_sqrt(__float32);
SF_ADAPTER1_unary(__float32_sqrt, 32); // inline float sqrtf(float x) {return float32_sqrtif(x);}
static __flag __float32_eq(__float32, __float32);
static __flag __float32_le(__float32, __float32);
SF_ADAPTER2(__float32_le, 32);
static __flag __float32_lt(__float32, __float32);
SF_ADAPTER2(__float32_lt, 32);
static inline __flag __float32_ge(__float32 a, __float32 b)
{
   return __float32_le(b, a);
}
SF_ADAPTER2(__float32_ge, 32);
static inline __flag __float32_gt(__float32 a, __float32 b)
{
   return __float32_lt(b, a);
}
SF_ADAPTER2(__float32_gt, 32);
static __flag __float32_eq_signaling(__float32, __float32);
static __flag __float32_le_quiet(__float32, __float32);
static __flag __float32_lt_quiet(__float32, __float32);
static __flag __float32_is_signaling_nan(__float32);
// static __flag __float32_ltgt_quiet( __float32 a, __float32 b) {return !__float32_eq(b,a);}SF_ADAPTER2(__float32_ltgt_quiet,32);

static __float32 __float32_muladd(__float32 uiA, __float32 uiB, __float32 uiC);
SF_ADAPTER1_ternary(__float32_muladd, 32);

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
static __int32 __float64_to_int32(__float64);
SF_ADAPTER2_unary(__float64_to_int32, 64, 32);
static __int32 __float64_to_int32_round_to_zero(__float64);
SF_ADAPTER2_unary(__float64_to_int32_round_to_zero, 64, 32);
static __uint32 __float64_to_uint32_round_to_zero(__float64 a);
SF_UADAPTER2_unary(__float64_to_uint32_round_to_zero, 64, 32);
static __int64 __float64_to_int64(__float64);
SF_ADAPTER2_unary(__float64_to_int64, 64, 64);
static __int64 __float64_to_int64_round_to_zero(__float64);
SF_ADAPTER2_unary(__float64_to_int64_round_to_zero, 64, 64);
static __uint64 __float64_to_uint64_round_to_zero(__float64 a);
SF_UADAPTER2_unary(__float64_to_uint64_round_to_zero, 64, 64);
static __float32 __float64_to_float32(__float64);
SF_ADAPTER4_unary(__float64_to_float32, 64, 32);
#ifdef FLOATX80
static __floatx80 __float64_to_floatx80(__float64);
SF_ADAPTER4_unary(__float64_to_floatx80, 64, x80);
#endif
#ifdef FLOAT128
static __float128 __float64_to_float128(__float64);
SF_ADAPTER4_unary(__float64_to_float128, 64, 128);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision operations.
*----------------------------------------------------------------------------*/
static __float64 __float64_round_to_int(__float64);
static __float64 __float64_add(__float64, __float64);
SF_ADAPTER1(__float64_add, 64);
static __float64 __float64_sub(__float64, __float64);
SF_ADAPTER1(__float64_sub, 64);
static __float64 __float64_mul(__float64, __float64);
SF_ADAPTER1(__float64_mul, 64);
static __float64 __float64_divSRT4(__float64, __float64);
SF_ADAPTER1(__float64_divSRT4, 64);
static __float64 __float64_divG(__float64, __float64);
SF_ADAPTER1(__float64_divG, 64);
static __float64 __float64_rem(__float64, __float64);
static __float64 __float64_sqrt(__float64);
SF_ADAPTER1_unary(__float64_sqrt, 64); // inline double sqrt(double x) {return float64_sqrtif(x);}
static __flag __float64_eq(__float64, __float64);
static __flag __float64_le(__float64, __float64);
SF_ADAPTER2(__float64_le, 64);
static __flag __float64_lt(__float64, __float64);
SF_ADAPTER2(__float64_lt, 64);
static inline __flag __float64_ge(__float64 a, __float64 b)
{
   return __float64_le(b, a);
}
SF_ADAPTER2(__float64_ge, 64);
static inline __flag __float64_gt(__float64 a, __float64 b)
{
   return __float64_lt(b, a);
}
SF_ADAPTER2(__float64_gt, 64);
static __flag __float64_eq_signaling(__float64, __float64);
static __flag __float64_le_quiet(__float64, __float64);
static __flag __float64_lt_quiet(__float64, __float64);
static __flag __float64_is_signaling_nan(__float64);
// static __flag __float64_ltgt_quiet( __float64 a, __float64 b) {return !__float64_eq(b,a);}SF_ADAPTER2(__float64_ltgt_quiet,64);

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision conversion routines.
*----------------------------------------------------------------------------*/
static __int32 __floatx80_to_int32(__floatx80);
static __int32 __floatx80_to_int32_round_to_zero(__floatx80);
static __int64 __floatx80_to_int64(__floatx80);
static __int64 __floatx80_to_int64_round_to_zero(__floatx80);
static __float32 __floatx80_to_float32(__floatx80);
static __float64 __floatx80_to_float64(__floatx80);
#ifdef FLOAT128
static __float128 __floatx80_to_float128(__floatx80);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision rounding precision.  Valid
| values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern __int8 __floatx80_rounding_precision;

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/
static __floatx80 __floatx80_round_to_int(__floatx80);
static __floatx80 floatx80_add(__floatx80, __floatx80);
SF_ADAPTER1(floatx80_add, x80);
static __floatx80 __floatx80_sub(__floatx80, __floatx80);
SF_ADAPTER1(__floatx80_sub, x80);
static __floatx80 __floatx80_mul(__floatx80, __floatx80);
SF_ADAPTER1(__floatx80_mul, x80);
static __floatx80 __floatx80_div(__floatx80, __floatx80);
SF_ADAPTER1(__floatx80_div, x80);
static __floatx80 __floatx80_rem(__floatx80, __floatx80);
static __floatx80 __floatx80_sqrt(__floatx80);
SF_ADAPTER1_unary(__floatx80_sqrt, x80); // inline long double sqrtl(long double x) {return floatx80_sqrtif(x);}
static __flag __floatx80_eq(__floatx80, __floatx80);
static __flag __floatx80_le(__floatx80, __floatx80);
SF_ADAPTER2(__floatx80_le, x80);
static __flag __floatx80_lt(__floatx80, __floatx80);
SF_ADAPTER2(__floatx80_lt, x80);
static inline __flag __floatx80_ge(__floatx80 a, __floatx80 b)
{
   return __floatx80_le(b, a);
}
SF_ADAPTER2(__floatx80_ge, x80);
static inline __flag __floatx80_gt(__floatx80 a, __floatx80 b)
{
   return __floatx80_lt(b, a);
}
SF_ADAPTER2(__floatx80_gt, x80);
static __flag __floatx80_eq_signaling(__floatx80, __floatx80);
static __flag __floatx80_le_quiet(__floatx80, __floatx80);
static __flag __floatx80_lt_quiet(__floatx80, __floatx80);
static __flag __floatx80_is_signaling_nan(__floatx80);

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision conversion routines.
*----------------------------------------------------------------------------*/
static __int32 __float128_to_int32(__float128);
static __int32 __float128_to_int32_round_to_zero(__float128);
static __int64 __float128_to_int64(__float128);
static __int64 __float128_to_int64_round_to_zero(__float128);
static __float32 __float128_to_float32(__float128);
static __float64 __float128_to_float64(__float128);
#ifdef FLOATX80
static __floatx80 __float128_to_floatx80(__float128);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision operations.
*----------------------------------------------------------------------------*/
static __float128 __float128_round_to_int(__float128);
static __float128 __float128_add(__float128, __float128);
SF_ADAPTER1(__float128_add, 128);
static __float128 __float128_sub(__float128, __float128);
SF_ADAPTER1(__float128_sub, 128);
static __float128 __float128_mul(__float128, __float128);
SF_ADAPTER1(__float128_mul, 128);
static __float128 __float128_div(__float128, __float128);
SF_ADAPTER1(__float128_div, 128);
static __float128 __float128_rem(__float128, __float128);
static __float128 __float128_sqrt(__float128);
SF_ADAPTER1_unary(__float128_sqrt, 128); // inline __float128 sqrtl(__float128 x) {return float128_sqrtif(x);}
static __flag __float128_eq(__float128, __float128);
static __flag __float128_le(__float128, __float128);
SF_ADAPTER2(__float128_le, 128);
static __flag __float128_lt(__float128, __float128);
SF_ADAPTER2(__float128_lt, 128);
static inline __flag __float128_ge(__float128 a, __float128 b)
{
   return __float128_le(b, a);
}
SF_ADAPTER2(__float128_ge, 128);
static inline __flag __float128_gt(__float128 a, __float128 b)
{
   return __float128_lt(b, a);
}
SF_ADAPTER2(__float128_gt, 128);
static __flag __float128_eq_signaling(__float128, __float128);
static __flag __float128_le_quiet(__float128, __float128);
static __flag __float128_lt_quiet(__float128, __float128);
static __flag __float128_is_signaling_nan(__float128);

#endif
