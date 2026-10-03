
/*============================================================================

This C source file is part of TestFloat, Release 3e, a package of programs for
testing the correctness of floating-point arithmetic complying with the IEEE
Standard for Floating-Point, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2017 The Regents of the University of
California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include "platform.h"

#include "subjfloat.h"
#include "subjfloat_config.h"
#include <stdbool.h>
#include <stdint.h>

#include <softfloat/softfloat.h>

#define IEEE_SUBJ_EXC FLOAT_EXC_STD
#ifndef IEEE_SUBJ_SUBNORM
#define IEEE_SUBJ_SUBNORM 1
#endif

#define IEEE16_SUBJ_SPEC \
   IEEE16_EXP_BITS, IEEE16_FRAC_BITS, IEEE16_EXP_BIAS, IEEE_RND, IEEE_SUBJ_EXC, IEEE_ONE, IEEE_SUBJ_SUBNORM, IEEE_SIGN
#define IEEE32_SUBJ_SPEC \
   IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE32_EXP_BIAS, IEEE_RND, IEEE_SUBJ_EXC, IEEE_ONE, IEEE_SUBJ_SUBNORM, IEEE_SIGN
#define IEEE64_SUBJ_SPEC \
   IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE64_EXP_BIAS, IEEE_RND, IEEE_SUBJ_EXC, IEEE_ONE, IEEE_SUBJ_SUBNORM, IEEE_SIGN

#pragma STDC FENV_ACCESS ON

void subjfloat_setRoundingMode(uint_fast8_t roundingMode)
{
#ifndef NO_PARAMETRIC
   __float_rounding_mode = roundingMode;
#endif
}

void subjfloat_setExtF80RoundingPrecision(uint_fast8_t roundingPrecision)
{
}

uint_fast8_t subjfloat_clearExceptionFlags(void)
{
#ifndef NO_PARAMETRIC
   uint_fast8_t prevFlags;
   prevFlags = __float_exception_flags;
   __float_exception_flags = 0;
   return prevFlags;
#else
   return 0;
#endif
}

/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/

#if defined(FLOAT16)

float16_t subj_ui32_to_f16(uint32_t a)
{
   float16_t f16;
   f16.v = __uint32_to_float(a, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_ui64_to_f16(uint64_t a)
{
   float16_t f16;
   f16.v = __uint64_to_float(a, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_i32_to_f16(int32_t a)
{
   float16_t f16;
   f16.v = __int32_to_float(a, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_i64_to_f16(int64_t a)
{
   float16_t f16;
   f16.v = __int64_to_float(a, IEEE16_SUBJ_SPEC);
   return f16;
}

#ifndef NO_PARAMETRIC
uint_fast32_t subj_f16_to_ui32_rx_near_even(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint(a.v, 32, IEEE16_SUBJ_SPEC);
}

uint_fast64_t subj_f16_to_ui64_rx_near_even(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint(a.v, 64, IEEE16_SUBJ_SPEC);
}

int_fast32_t subj_f16_to_i32_rx_near_even(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   int res = __float_to_int(a.v, 32, IEEE16_SUBJ_SPEC);
   return res;
}

int_fast64_t subj_f16_to_i64_rx_near_even(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_int(a.v, 64, IEEE16_SUBJ_SPEC);
}
#endif

uint_fast32_t subj_f16_to_ui32_rx_minMag(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint(a.v, 32, IEEE16_SUBJ_SPEC);
}

uint_fast64_t subj_f16_to_ui64_rx_minMag(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint(a.v, 64, IEEE16_SUBJ_SPEC);
}

int_fast32_t subj_f16_to_i32_rx_minMag(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   int res = __float_to_int(a.v, 32, IEEE16_SUBJ_SPEC);
   return res;
}

int_fast64_t subj_f16_to_i64_rx_minMag(float16_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_int(a.v, 64, IEEE16_SUBJ_SPEC);
}

float16_t subj_f16_add(float16_t a, float16_t b)
{
   float16_t f16;
   f16.v = __float_add(a.v, b.v, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_f16_sub(float16_t a, float16_t b)
{
   float16_t f16;
   f16.v = __float_sub(a.v, b.v, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_f16_mul(float16_t a, float16_t b)
{
   float16_t f16;
   f16.v = __float_mul(a.v, b.v, IEEE16_SUBJ_SPEC);
   return f16;
}

float16_t subj_f16_div(float16_t a, float16_t b)
{
   float16_t f16;
   f16.v = __float_divSRT4(a.v, b.v, IEEE16_SUBJ_SPEC);
   return f16;
}

bool subj_f16_eq(float16_t a, float16_t b)
{
   return __float_eq(a.v, b.v, IEEE16_SUBJ_SPEC);
}

bool subj_f16_le(float16_t a, float16_t b)
{
   return __float_le(a.v, b.v, IEEE16_SUBJ_SPEC);
}

bool subj_f16_lt(float16_t a, float16_t b)
{
   return __float_lt(a.v, b.v, IEEE16_SUBJ_SPEC);
}

#endif

/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/

float32_t subj_ui32_to_f32(uint32_t a)
{
   float32_t f32;
   f32.v = __uint32_to_float(a, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_ui64_to_f32(uint64_t a)
{
   float32_t f32;
   f32.v = __uint64_to_float(a, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_i32_to_f32(int32_t a)
{
   float32_t f32;
   f32.v = __int32_to_float(a, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_i64_to_f32(int64_t a)
{
   float32_t f32;
   f32.v = __int64_to_float(a, IEEE32_SUBJ_SPEC);
   return f32;
}

#ifndef NO_PARAMETRIC
int_fast32_t subj_f32_to_i32_rx_near_even(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   int res = __float_to_int32(a.v, IEEE32_SUBJ_SPEC);
   return res;
}

uint_fast32_t subj_f32_to_ui32_rx_near_even(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint32(a.v, IEEE32_SUBJ_SPEC);
}

int_fast64_t subj_f32_to_i64_rx_near_even(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_int64(a.v, IEEE32_SUBJ_SPEC);
}

uint_fast64_t subj_f32_to_ui64_rx_near_even(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint64(a.v, IEEE32_SUBJ_SPEC);
}
#endif

uint_fast32_t subj_f32_to_ui32_rx_minMag(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint32(a.v, IEEE32_SUBJ_SPEC);
}

uint_fast64_t subj_f32_to_ui64_rx_minMag(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint64(a.v, IEEE32_SUBJ_SPEC);
}

int_fast32_t subj_f32_to_i32_rx_minMag(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   int res = __float_to_int32(a.v, IEEE32_SUBJ_SPEC);
   return res;
}

int_fast64_t subj_f32_to_i64_rx_minMag(float32_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_int64(a.v, IEEE32_SUBJ_SPEC);
}

float32_t subj_f32_add(float32_t a, float32_t b)
{
   float32_t f32;
   f32.v = __float_add(a.v, b.v, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_f32_sub(float32_t a, float32_t b)
{
   float32_t f32;
   f32.v = __float_sub(a.v, b.v, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_f32_mul(float32_t a, float32_t b)
{
   float32_t f32;
   f32.v = __float_mul(a.v, b.v, IEEE32_SUBJ_SPEC);
   return f32;
}

float32_t subj_f32_div(float32_t a, float32_t b)
{
   float32_t f32;
   f32.v = __float_divSRT4(a.v, b.v, IEEE32_SUBJ_SPEC);
   return f32;
}

bool subj_f32_eq(float32_t a, float32_t b)
{
   return __float_eq(a.v, b.v, IEEE32_SUBJ_SPEC);
}

bool subj_f32_le(float32_t a, float32_t b)
{
   return __float_le(a.v, b.v, IEEE32_SUBJ_SPEC);
}

bool subj_f32_lt(float32_t a, float32_t b)
{
   return __float_lt(a.v, b.v, IEEE32_SUBJ_SPEC);
}

/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/

#ifdef FLOAT64

float64_t subj_ui32_to_f64(uint32_t a)
{
   float64_t f64;
   f64.v = __uint32_to_float(a, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_ui64_to_f64(uint64_t a)
{
   float64_t f64;
   f64.v = __uint64_to_float(a, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_i32_to_f64(int32_t a)
{
   float64_t f64;
   f64.v = __int32_to_float(a, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_i64_to_f64(int64_t a)
{
   float64_t f64;
   f64.v = __int64_to_float(a, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_f32_to_f64(float32_t a)
{
   float64_t f64;
   f64.v = __float_cast(a.v, IEEE32_SUBJ_SPEC, IEEE64_SUBJ_SPEC);
   return f64;
}

#ifndef NO_PARAMETRIC
int_fast32_t subj_f64_to_i32_rx_near_even(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   int res = __float_to_int32(a.v, IEEE64_SUBJ_SPEC);
   return res;
}

uint_fast32_t subj_f64_to_ui32_rx_near_even(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint32(a.v, IEEE64_SUBJ_SPEC);
}

int_fast64_t subj_f64_to_i64_rx_near_even(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_int64(a.v, IEEE64_SUBJ_SPEC);
}

uint_fast64_t subj_f64_to_ui64_rx_near_even(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_near_even);
   return __float_to_uint64(a.v, IEEE64_SUBJ_SPEC);
}
#endif

uint_fast32_t subj_f64_to_ui32_rx_minMag(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint32(a.v, IEEE64_SUBJ_SPEC);
}

uint_fast64_t subj_f64_to_ui64_rx_minMag(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_uint64(a.v, IEEE64_SUBJ_SPEC);
}

int_fast32_t subj_f64_to_i32_rx_minMag(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   int res = __float_to_int32(a.v, IEEE64_SUBJ_SPEC);
   return res;
}

int_fast64_t subj_f64_to_i64_rx_minMag(float64_t a)
{
   subjfloat_setRoundingMode(softfloat_round_minMag);
   return __float_to_int64(a.v, IEEE64_SUBJ_SPEC);
}

float32_t subj_f64_to_f32(float64_t a)
{
   float32_t f32;
   f32.v = __float64_to_float32_ieee(a.v, IEEE_SUBJ_EXC, IEEE_SUBJ_SUBNORM);
   return f32;
}

float64_t subj_f64_add(float64_t a, float64_t b)
{
   float64_t f64;
   f64.v = __float_add(a.v, b.v, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_f64_sub(float64_t a, float64_t b)
{
   float64_t f64;
   f64.v = __float_sub(a.v, b.v, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_f64_mul(float64_t a, float64_t b)
{
   float64_t f64;
   f64.v = __float_mul(a.v, b.v, IEEE64_SUBJ_SPEC);
   return f64;
}

float64_t subj_f64_div(float64_t a, float64_t b)
{
   float64_t f64;
   f64.v = __float_divSRT4(a.v, b.v, IEEE64_SUBJ_SPEC);
   return f64;
}

bool subj_f64_eq(float64_t a, float64_t b)
{
   return __float_eq(a.v, b.v, IEEE64_SUBJ_SPEC);
}

bool subj_f64_le(float64_t a, float64_t b)
{
   return __float_le(a.v, b.v, IEEE64_SUBJ_SPEC);
}

bool subj_f64_lt(float64_t a, float64_t b)
{
   return __float_lt(a.v, b.v, IEEE64_SUBJ_SPEC);
}

#endif
