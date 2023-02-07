/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "soft-fp.h"

#include "double.h"
#include "single.h"

#include "adddf3.c"
#include "addsf3.c"
#include "divdf3.c"
#include "divsf3.c"
#include "eqdf2.c"
#include "eqsf2.c"
#include "extendsfdf2.c"
#include "fixdfdi.c"
#include "fixdfsi.c"
#include "fixsfdi.c"
#include "fixsfsi.c"
#include "fixunsdfdi.c"
#include "fixunsdfsi.c"
#include "fixunssfdi.c"
#include "fixunssfsi.c"
#include "floatdidf.c"
#include "floatdisf.c"
#include "floatsidf.c"
#include "floatsisf.c"
#include "floatundidf.c"
#include "floatundisf.c"
#include "floatunsidf.c"
#include "floatunsisf.c"
#include "gedf2.c"
#include "gesf2.c"
#include "ledf2.c"
#include "lesf2.c"
#include "muldf3.c"
#include "mulsf3.c"
#include "negdf2.c"
#include "negsf2.c"
#include "subdf3.c"
#include "subsf3.c"
#include "truncdfsf2.c"
#include "unorddf2.c"
#include "unordsf2.c"

#define __float32 __bits32
#define __float64 __bits64
#define __float __bits64

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
__float32 __int32_to_float32(__int32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_S ur;
   ur.flt = __floatsisf(a);
   return ur.bits.coded;
}

__float32 __uint32_to_float32(__uint32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_S ur;
   ur.flt = __floatunsisf(a);
   return ur.bits.coded;
}

__float64 __int32_to_float64(__int32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_D ur;
   ur.flt = __floatsidf(a);
   return ur.bits.coded;
}

__float64 __uint32_to_float64(__uint32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_D ur;
   ur.flt = __floatunsidf(a);
   return ur.bits.coded;
}

__float32 __int64_to_float32(__int64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_S ur;
   ur.flt = __floatdisf(a);
   return ur.bits.coded;
}

__float64 __int64_to_float64(__int64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_D ur;
   ur.flt = __floatdidf(a);
   return ur.bits.coded;
}

__float64 __uint64_to_float64(__uint64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   union _FP_UNION_D ur;
   ur.flt = __floatundidf(a);
   return ur.bits.coded;
}

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float32_to_int32_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixsfsi(ua.flt);
}

__uint32 __float32_to_uint32_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixunssfsi(ua.flt);
}

__int64 __float32_to_int64_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixsfdi(ua.flt);
}

__uint64 __float32_to_uint64_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixunssfdi(ua.flt);
}

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
__int32 __float64_to_int32_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixdfsi(ua.flt);
}

__uint32 __float64_to_uint32_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixunsdfsi(ua.flt);
}

__int64 __float64_to_int64_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixdfdi(ua.flt);
}

__uint64 __float64_to_uint64_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           __flag __rounding, __flag __nan, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   union _FP_UNION_D ua;
   ua.bits.coded = a;
   return __fixunsdfdi(ua.flt);
}

/*----------------------------------------------------------------------------
| Software IEC/IEEE arbitrary precision conversion routines.
*----------------------------------------------------------------------------*/
__float __float_cast(__float bits, __bits8 __in_exp_bits, __bits8 __in_frac_bits, __int32 __in_exp_bias,
                     __flag __in_has_rounding, __flag __in_has_nan, __flag __in_has_one, __flag __in_has_subnorm,
                     __sbits8 __in_sign, __bits8 __out_exp_bits, __bits8 __out_frac_bits, __int32 __out_exp_bias,
                     __flag __out_has_rounding, __flag __out_has_nan, __flag __out_has_one, __flag __out_has_subnorm,
                     __sbits8 __out_sign)
{
   __bits8 __in_bw = __in_exp_bits + __in_frac_bits + (__in_sign == -1);
   __bits8 __out_bw = __out_exp_bits + __out_frac_bits + (__out_sign == -1);
   if(__in_bw == 32 && __out_bw == 64)
   {
      union _FP_UNION_S ua;
      union _FP_UNION_D ur;
      ua.bits.coded = bits;
      ur.flt = __extendsfdf2(ua.flt);
      return ur.bits.coded;
   }
   else if(__in_bw == 64 && __out_bw == 32)
   {
      union _FP_UNION_D ua;
      union _FP_UNION_S ur;
      ua.bits.coded = bits;
      ur.flt = __truncdfsf2(ua.flt);
      return ur.bits.coded;
   }
   return 0;
}

/*----------------------------------------------------------------------------
| Software IEC/IEEE arbitrary precision operations.
*----------------------------------------------------------------------------*/
__float __float_add(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __addsf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __adddf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   return 0;
}

__float __float_sub(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __subsf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __subdf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   return 0;
}

__float __float_mul(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __mulsf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __muldf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   return 0;
}

__float __float_div(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __divsf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      ur.flt = __divdf3(ua.flt, ub.flt);
      return ur.bits.coded;
   }
   return 0;
}

__flag __float_eq(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __eqsf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __eqdf2(ua.flt, ub.flt);
   }
   return 0;
}

__flag __float_le(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __lesf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __ledf2(ua.flt, ub.flt);
   }
   return 0;
}

__flag __float_lt(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__gesf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__gedf2(ua.flt, ub.flt);
   }
   return 0;
}

__flag __float_ge(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __gesf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return __gedf2(ua.flt, ub.flt);
   }
   return 0;
}

__flag __float_gt(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__lesf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__ledf2(ua.flt, ub.flt);
   }
   return 0;
}

__flag __float_ltgt_quiet(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                          __flag __rounding, __flag __nan, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 __bw = __exp_bits + __frac_bits + (__sign == -1);
   if(__bw == 32)
   {
      union _FP_UNION_S ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__eqsf2(ua.flt, ub.flt);
   }
   else if(__bw == 64)
   {
      union _FP_UNION_D ua, ub, ur;
      ua.bits.coded = a;
      ub.bits.coded = b;
      return !__eqdf2(ua.flt, ub.flt);
   }
   return 0;
}
