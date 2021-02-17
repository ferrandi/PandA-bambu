
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
__float32 __int32_to_float32(__int32);
__float32 __int8_to_float32(__int8 a);
__float32 __int16_to_float32(__int16 a);
__float32 __uint32_to_float32(__uint32);
__float32 __uint8_to_float32(__uint8 a);
__float32 __uint16_to_float32(__uint16 a);
__float64 __int32_to_float64(__int32 a);
__float64 __uint32_to_float64(__uint32 a);
#ifdef FLOATX80
__floatx80 __int32_to_floatx80(__int32);
#endif
#ifdef FLOAT128
__float128 __int32_to_float128(__int32);
#endif
__float32 __int64_to_float32(__int64 a);
__float32 __uint64_to_float32(__uint64 a);
__float64 __int64_to_float64(__int64 a);
__float64 __uint64_to_float64(__uint64 a);
#ifdef FLOATX80
__floatx80 __int64_to_floatx80(__int64);
#endif
#ifdef FLOAT128
__float128 __int64_to_float128(__int64);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float32_to_int32(__float32);
__int32 __float32_to_int32_round_to_zero(__float32);
__uint32 __float32_to_uint32_round_to_zero(__float32 a);
__int64 __float32_to_int64(__float32);
__int64 __float32_to_int64_round_to_zero(__float32);
__uint64 __float32_to_uint64_round_to_zero(__float32);
__float64 __float32_to_float64(__float32);
#ifdef FLOATX80
__floatx80 __float32_to_floatx80(__float32);
#endif
#ifdef FLOAT128
__float128 __float32_to_float128(__float32);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision operations.
*----------------------------------------------------------------------------*/
__float32 __float32_round_to_int(__float32);
__float32 __float32_add(__float32, __float32);
__float32 __float32_sub(__float32, __float32);
__float32 __float32_mul(__float32, __float32);
__float32 __float32_divG(__float32, __float32);
__float32 __float32_divSRT4(__float32, __float32);
__float32 __float32_rem(__float32, __float32);
__float32 __float32_sqrt(__float32);
__flag __float32_eq(__float32, __float32);
__flag __float32_le(__float32, __float32);
__flag __float32_lt(__float32, __float32);
__flag __float32_ge(__float32 a, __float32 b);
__flag __float32_gt(__float32 a, __float32 b);
__flag __float32_eq_signaling(__float32, __float32);
__flag __float32_le_quiet(__float32, __float32);
__flag __float32_lt_quiet(__float32, __float32);
__flag __float32_is_signaling_nan(__float32);
__flag __float32_ltgt_quiet(__float32 a, __float32 b);

__float32 __float32_muladd(__float32 uiA, __float32 uiB, __float32 uiC);

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float64_to_int32(__float64);
__int32 __float64_to_int32_round_to_zero(__float64);
__uint32 __float64_to_uint32_round_to_zero(__float64 a);
__int64 __float64_to_int64(__float64);
__int64 __float64_to_int64_round_to_zero(__float64);
__uint64 __float64_to_uint64_round_to_zero(__float64 a);
__float32 __float64_to_float32(__float64);
#ifdef FLOATX80
__floatx80 __float64_to_floatx80(__float64);
#endif
#ifdef FLOAT128
__float128 __float64_to_float128(__float64);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision operations.
*----------------------------------------------------------------------------*/
__float64 __float64_round_to_int(__float64);
__float64 __float64_add(__float64, __float64);
__float64 __float64_sub(__float64, __float64);
__float64 __float64_mul(__float64, __float64);
__float64 __float64_divSRT4(__float64, __float64);
__float64 __float64_divG(__float64, __float64);
__float64 __float64_rem(__float64, __float64);
__float64 __float64_sqrt(__float64);
__flag __float64_eq(__float64, __float64);
__flag __float64_le(__float64, __float64);
__flag __float64_lt(__float64, __float64);
__flag __float64_ge(__float64 a, __float64 b);
__flag __float64_gt(__float64 a, __float64 b);
__flag __float64_eq_signaling(__float64, __float64);
__flag __float64_le_quiet(__float64, __float64);
__flag __float64_lt_quiet(__float64, __float64);
__flag __float64_is_signaling_nan(__float64);
__flag __float64_ltgt_quiet(__float64 a, __float64 b);

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __floatx80_to_int32(__floatx80);
__int32 __floatx80_to_int32_round_to_zero(__floatx80);
__int64 __floatx80_to_int64(__floatx80);
__int64 __floatx80_to_int64_round_to_zero(__floatx80);
__float32 __floatx80_to_float32(__floatx80);
__float64 __floatx80_to_float64(__floatx80);
#ifdef FLOAT128
__float128 __floatx80_to_float128(__floatx80);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision rounding precision.  Valid
| values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern __int8 __floatx80_rounding_precision;

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/
__floatx80 __floatx80_round_to_int(__floatx80);
__floatx80 floatx80_add(__floatx80, __floatx80);
__floatx80 __floatx80_sub(__floatx80, __floatx80);
__floatx80 __floatx80_mul(__floatx80, __floatx80);
__floatx80 __floatx80_div(__floatx80, __floatx80);
__floatx80 __floatx80_rem(__floatx80, __floatx80);
__floatx80 __floatx80_sqrt(__floatx80);
__flag __floatx80_eq(__floatx80, __floatx80);
__flag __floatx80_le(__floatx80, __floatx80);
__flag __floatx80_lt(__floatx80, __floatx80);
__flag __floatx80_ge(__floatx80 a, __floatx80 b);
__flag __floatx80_gt(__floatx80 a, __floatx80 b);
__flag __floatx80_eq_signaling(__floatx80, __floatx80);
__flag __floatx80_le_quiet(__floatx80, __floatx80);
__flag __floatx80_lt_quiet(__floatx80, __floatx80);
__flag __floatx80_is_signaling_nan(__floatx80);

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float128_to_int32(__float128);
__int32 __float128_to_int32_round_to_zero(__float128);
__int64 __float128_to_int64(__float128);
__int64 __float128_to_int64_round_to_zero(__float128);
__float32 __float128_to_float32(__float128);
__float64 __float128_to_float64(__float128);
#ifdef FLOATX80
__floatx80 __float128_to_floatx80(__float128);
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision operations.
*----------------------------------------------------------------------------*/
__float128 __float128_round_to_int(__float128);
__float128 __float128_add(__float128, __float128);
__float128 __float128_sub(__float128, __float128);
__float128 __float128_mul(__float128, __float128);
__float128 __float128_div(__float128, __float128);
__float128 __float128_rem(__float128, __float128);
__float128 __float128_sqrt(__float128);
__flag __float128_eq(__float128, __float128);
__flag __float128_le(__float128, __float128);
__flag __float128_lt(__float128, __float128);
__flag __float128_ge(__float128 a, __float128 b);
__flag __float128_gt(__float128 a, __float128 b);
__flag __float128_eq_signaling(__float128, __float128);
__flag __float128_le_quiet(__float128, __float128);
__flag __float128_lt_quiet(__float128, __float128);
__flag __float128_is_signaling_nan(__float128);

#endif
