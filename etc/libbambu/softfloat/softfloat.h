/**
 * SoftFloat package modified by:
 *    Fabrizio Ferrandi - Politecnico di Milano
 *    Michele Fiorito - Politecnico di Milano
 * Changes made are mainly oriented to improve the results of a generic high
 * level synthesis framework and to add support for custom floating-point
 * data types.
 */
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

#define __FORCE_INLINE __attribute__((always_inline)) inline

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
#define __float32 __bits32
#define __float64 __bits64
#define __float __bits64
#define FLOAT_RND_TYPE __bits8
#define FLOAT_EXC_TYPE __bits8
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
void __float_raise(__int8);

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
__float32 __int32_to_float32(__int32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float32 __int16_to_float32(__int16, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float32 __int8_to_float32(__int8, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                            __sbits8);
__float32 __uint32_to_float32(__uint32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                              __sbits8);
__float32 __uint16_to_float32(__uint16, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                              __sbits8);
__float32 __uint8_to_float32(__uint8, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float64 __int32_to_float64(__int32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float64 __uint32_to_float64(__uint32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                              __sbits8);
#ifdef FLOATX80
__floatx80 __int32_to_floatx80_ieee(__int32);
#endif
#ifdef FLOAT128
__float128 __int32_to_float128_ieee(__int32);
#endif
__float32 __int64_to_float32(__int64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float32 __uint64_to_float32(__uint64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                              __sbits8);
__float64 __int64_to_float64(__int64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                             __sbits8);
__float64 __uint64_to_float64(__uint64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                              __sbits8);
#ifdef FLOATX80
__floatx80 __int64_to_floatx80_ieee(__int64);
#endif
#ifdef FLOAT128
__float128 __int64_to_float128_ieee(__int64);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float32_to_int32(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                           __sbits8);
__int32 __float32_to_int32_round_to_zero(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                         __flag, __sbits8);
__uint32 __float32_to_uint32_round_to_zero(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                           __flag, __sbits8);
__int64 __float32_to_int64(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                           __sbits8);
__int64 __float32_to_int64_round_to_zero(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                         __flag, __sbits8);
__uint64 __float32_to_uint64_round_to_zero(__float32, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                           __flag, __sbits8);
__float64 __float32_to_float64_ieee(__float32, FLOAT_EXC_TYPE, __flag);
#ifdef FLOATX80
__floatx80 __float32_to_floatx80_ieee(__float32);
#endif
#ifdef FLOAT128
__float128 __float32_to_float128_ieee(__float32);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision operations.
*----------------------------------------------------------------------------*/
__float32 __float32_round_to_int_ieee(__float32);

// __float32 __float32_muladd_ieee(__float32 uiA, __float32 uiB, __float32 uiC);

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float64_to_int32(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                           __sbits8);
__int32 __float64_to_int32_round_to_zero(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                         __flag, __sbits8);
__uint32 __float64_to_uint32_round_to_zero(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                           __flag, __sbits8);
__int64 __float64_to_int64(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                           __sbits8);
__int64 __float64_to_int64_round_to_zero(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                         __flag, __sbits8);
__uint64 __float64_to_uint64_round_to_zero(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag,
                                           __flag, __sbits8);
__float32 __float64_to_float32_ieee(__float64, FLOAT_EXC_TYPE, __flag);
#ifdef FLOATX80
__floatx80 __float64_to_floatx80_ieee(__float64);
#endif
#ifdef FLOAT128
__float128 __float64_to_float128_ieee(__float64);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE arbitrary precision conversion routines.
*----------------------------------------------------------------------------*/
__float __float_cast(__float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag, __sbits8,
                     __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag, __sbits8);

/*----------------------------------------------------------------------------
| Software IEC/IEEE arbitrary precision operations.
*----------------------------------------------------------------------------*/
__float __float_add(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                    __sbits8);
__float __float_sub(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                    __sbits8);
__float __float_mul(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                    __sbits8);
__float __float_divSRT4(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                        __sbits8);
__float __float_divG(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                     __sbits8);
__flag __float_eq(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                  __sbits8);
__flag __float_le(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                  __sbits8);
__flag __float_lt(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                  __sbits8);
__flag __float_ge(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                  __sbits8);
__flag __float_gt(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                  __sbits8);
__flag __float_is_signaling_nan(__float64, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                                __sbits8);
__flag __float_ltgt_quiet(__float, __float, __bits8, __bits8, __int32, FLOAT_RND_TYPE, FLOAT_EXC_TYPE, __flag, __flag,
                          __sbits8);

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __floatx80_to_int32_ieee(__floatx80);
__int32 __floatx80_to_int32_round_to_zero_ieee(__floatx80);
__int64 __floatx80_to_int64_ieee(__floatx80);
__int64 __floatx80_to_int64_round_to_zero_ieee(__floatx80);
__float32 __floatx80_to_float32_ieee(__floatx80);
__float64 __floatx80_to_float64_ieee(__floatx80);
#ifdef FLOAT128
__float128 __floatx80_to_float128_ieee(__floatx80);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision rounding precision.  Valid
| values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern __int8 __floatx80_rounding_precision;

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/
__floatx80 __floatx80_round_to_int_ieee(__floatx80);
__floatx80 floatx80_add_ieee(__floatx80, __floatx80);
__floatx80 __floatx80_sub_ieee(__floatx80, __floatx80);
__floatx80 __floatx80_mul_ieee(__floatx80, __floatx80);
__floatx80 __floatx80_div_ieee(__floatx80, __floatx80);
__floatx80 __floatx80_rem_ieee(__floatx80, __floatx80);
__floatx80 __floatx80_sqrt(__floatx80);
__flag __floatx80_eq_ieee(__floatx80, __floatx80);
__flag __floatx80_le_ieee(__floatx80, __floatx80);
__flag __floatx80_lt_ieee(__floatx80, __floatx80);
__flag __floatx80_ge_ieee(__floatx80 a, __floatx80 b);
__flag __floatx80_gt_ieee(__floatx80 a, __floatx80 b);
__flag __floatx80_eq_signaling_ieee(__floatx80, __floatx80);
__flag __floatx80_le_quiet_ieee(__floatx80, __floatx80);
__flag __floatx80_lt_quiet_ieee(__floatx80, __floatx80);
__flag __floatx80_is_signaling_nan_ieee(__floatx80);

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float128_to_int32_ieee(__float128);
__int32 __float128_to_int32_round_to_zero_ieee(__float128);
__int64 __float128_to_int64_ieee(__float128);
__int64 __float128_to_int64_round_to_zero_ieee(__float128);
__float32 __float128_to_float32_ieee(__float128);
__float64 __float128_to_float64_ieee(__float128);
#ifdef FLOATX80
__floatx80 __float128_to_floatx80_ieee(__float128);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision operations.
*----------------------------------------------------------------------------*/
__float128 __float128_round_to_int_ieee(__float128);
__float128 __float128_add_ieee(__float128, __float128);
__float128 __float128_sub_ieee(__float128, __float128);
__float128 __float128_mul_ieee(__float128, __float128);
__float128 __float128_div_ieee(__float128, __float128);
__float128 __float128_rem_ieee(__float128, __float128);
__float128 __float128_sqrt(__float128);
__flag __float128_eq_ieee(__float128, __float128);
__flag __float128_le_ieee(__float128, __float128);
__flag __float128_lt_ieee(__float128, __float128);
__flag __float128_ge_ieee(__float128 a, __float128 b);
__flag __float128_gt_ieee(__float128 a, __float128 b);
__flag __float128_eq_signaling_ieee(__float128, __float128);
__flag __float128_le_quiet_ieee(__float128, __float128);
__flag __float128_lt_quiet_ieee(__float128, __float128);
__flag __float128_is_signaling_nan_ieee(__float128);

#endif
