/* Configuration for math routines.
   Copyright (c) 2017-2018 Arm Ltd.  All rights reserved.

   SPDX-License-Identifier: BSD-3-Clause

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. The name of the company may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

   THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef _MATH_CONFIG_H
#define _MATH_CONFIG_H

#ifdef NO_RAISE_EXCEPTIONS
#define _IEEE_LIBM
#endif

#include "newlib_math.h"
#include <stdint.h>

#ifndef WANT_ROUNDING
/* Correct special case results in non-nearest rounding modes.  */
# define WANT_ROUNDING 1
#endif
#ifdef _IEEE_LIBM
# define WANT_ERRNO 0
# define _LIB_VERSION _IEEE_
#else
/* Set errno according to ISO C with (math_errhandling & MATH_ERRNO) != 0.  */
# define WANT_ERRNO 1
# define _LIB_VERSION _POSIX_
#endif
#ifndef WANT_ERRNO_UFLOW
/* Set errno to ERANGE if result underflows to 0 (in all rounding modes).  */
# define WANT_ERRNO_UFLOW (WANT_ROUNDING && WANT_ERRNO)
#endif

#define _IEEE_  -1
#define _POSIX_ 0

/* Compiler can inline round as a single instruction.  */
#define HAVE_FAST_ROUND 0

/* Compiler can inline lround, but not (long)round(x).  */
#define HAVE_FAST_LROUND 0

/* Compiler can inline fma as a single instruction.  */
#define HAVE_FAST_FMA 0

#define HAVE_FAST_FMAF 0

#ifndef TOINT_INTRINSICS
# define TOINT_INTRINSICS 0
#endif

#define asuint64(f) ((union{double _f; uint64_t _i;}){f})._i

#define asdouble(i) ((union{uint64_t _i; double _f;}){i})._f

#define asuint(f) ((union{float _f; uint32_t _i;}){f})._i

#define asfloat(i) ((union{uint32_t _i; float _f;}){i})._f

#ifdef NO_RAISE_EXCEPTIONS
#define issignalingf_inline(x) (0)
#define issignaling_inline(x) (0)
#else
#ifndef IEEE_754_2008_SNAN
# define IEEE_754_2008_SNAN 1
#endif
static inline int
issignalingf_inline (float x)
{
  uint32_t ix = asuint (x);
  if (!IEEE_754_2008_SNAN)
    return (ix & 0x7fc00000) == 0x7fc00000;
  return 2 * (ix ^ 0x00400000) > 0xFF800000u;
}

static inline int
issignaling_inline (double x)
{
  uint64_t ix = asuint64 (x);
  if (!IEEE_754_2008_SNAN)
    return (ix & 0x7ff8000000000000) == 0x7ff8000000000000;
  return 2 * (ix ^ 0x0008000000000000) > 2 * 0x7ff8000000000000ULL;
}
#endif

#ifdef NO_RAISE_EXCEPTIONS
#define opt_barrier_float(x) ((float)x)
#define opt_barrier_double(x) ((double)x)
#define force_eval_float(x) ((float)x)
#define force_eval_double(x) ((double)x)
#define eval_as_float(x) ((float)x)
#define eval_as_double(x) ((double)x)
#else
static inline float
opt_barrier_float (float x)
{
  volatile float y = x;
  return y;
}
static inline double
opt_barrier_double (double x)
{
  volatile double y = x;
  return y;
}
// #pragma GCC diagnostic ignored "-Wunused-variable"
static inline void
force_eval_float (float x)
{
  volatile float y = x;
}
static inline void
force_eval_double (double x)
{
  volatile double y = x;
}
// #pragma GCC diagnostic pop

/* Evaluate an expression as the specified type, normally a type
   cast should be enough, but compilers implement non-standard
   excess-precision handling, so when FLT_EVAL_METHOD != 0 then
   these functions may need to be customized.  */
static inline float
eval_as_float (float x)
{
  return x;
}
static inline double
eval_as_double (double x)
{
  return x;
}
#endif

#ifdef __GNUC__
# define NOINLINE __attribute__ ((noinline))
# define likely(x) __builtin_expect (!!(x), 1)
# define unlikely(x) __builtin_expect (x, 0)
#else
# define NOINLINE
# define likely(x) (x)
# define unlikely(x) (x)
#endif

/* Error handling tail calls for special cases, with a sign argument.
   The sign of the return value is set if the argument is non-zero.  */
#if NO_RAISE_EXCEPTIONS
#define __math_oflowf(sign) ((sign) ? (-__builtin_inff()) : __builtin_inff())
#define __math_uflowf(sign) ((sign) ? -0.0f : 0.0f)
#define __math_divzerof(sign) __math_oflowf(sign)

#define __math_oflow(sign) ((sign) ? (-__builtin_inf()) : __builtin_inf())
#define __math_uflow(sign) ((sign) ? -0.0f : 0.0f)
#define __math_divzero(sign) __math_oflow(sign)

#define __math_invalidf(x) (__builtin_nanf(""))
#define __math_invalid(x) (__builtin_nan(""))
#define __math_raise(raise_expr, retval) (retval)

#define RAISE_VOLATILE
#else
#define __math_xflowf(sign, y) (((sign) ? -y : y) * y)
/* The result overflows.  */
#define __math_oflowf(sign) __math_xflowf(sign, 0x1p97f)
/* The result underflows to 0 in nearest rounding mode.  */
#define __math_uflowf(sign) __math_xflowf(sign, 0x1p-95f)
/* Division by zero.  */
#define __math_divzerof(sign) (((sign) ? -1.0f : 1.0f) / 0.0f)


#define __math_xflow(sign, y) (((sign) ? -y : y) * y)
/* The result overflows.  */
#define __math_oflow(sign) __math_xflow(sign, 0x1p769)
/* The result underflows to 0 in nearest rounding mode.  */
#define __math_uflow(sign) __math_xflow(sign, 0x1p-767)
/* Division by zero.  */
#define __math_divzero(sign) ((sign ? -1.0 : 1.0) / 0.0)

/* Error handling using input checking.  */

/* Invalid input unless it is a quiet NaN.  */
#define __math_invalidf(x) ((x - x) / (x - x))
/* Invalid input unless it is a quiet NaN.  */
#define __math_invalid(x) ((x - x) / (x - x))
/* Flag raising expression.  */
#define __math_raise(raise_expr, retval) (raise_expr)

#define RAISE_VOLATILE volatile
#endif

#endif
