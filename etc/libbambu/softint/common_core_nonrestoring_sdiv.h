//    Copyright (C) 2014-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libsoftint IP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef _COMMON_CORE_NONRESTORING_H
#define _COMMON_CORE_NONRESTORING_H

#include <bambu_config.h>
#include <bambu_macros.h>

#ifndef DIV_NUM_BIT
#define DIV_NUM_BIT 64
#endif

#define DIV_NUM_BIT_M1 BOOST_PP_SUB(DIV_NUM_BIT, 1)

#ifndef UNROLL_FACTOR
#define UNROLL_FACTOR 1
#endif

#define LOOP_BODY(z, n, data)                                                                                      \
   divisor_select = partial_remainder_sign ^ plus_divisor_sign;                                                    \
   carry_input = !divisor_select;                                                                                  \
   w = divisor_select ? plus_divisor : minus_divisor;                                                              \
   w_sign = divisor_select ? plus_divisor_sign : minus_divisor_sign;                                               \
   partial_remainder_sign = SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1);                                        \
   partial_remainderH =                                                                                            \
       (VAL_RESIZE(partial_remainderH, DIV_NUM_BIT_M1) << 1) | SELECT_BIT(partial_remainderL, DIV_NUM_BIT_M1);     \
   partial_remainderL = VAL_RESIZE(partial_remainderL << 1, DIV_NUM_BIT);                                          \
   sum_result = VAL_RESIZE(w, DIV_NUM_BIT_M1) + VAL_RESIZE(partial_remainderH, DIV_NUM_BIT_M1) + carry_input;      \
   sum_result_msb = SELECT_BIT(sum_result, DIV_NUM_BIT_M1) ^ SELECT_BIT(w, DIV_NUM_BIT_M1) ^                       \
                    SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1);                                                \
   sum_result_carry = (SELECT_BIT(sum_result, DIV_NUM_BIT_M1) & SELECT_BIT(w, DIV_NUM_BIT_M1)) |                   \
                      (SELECT_BIT(w, DIV_NUM_BIT_M1) & SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1)) |           \
                      (SELECT_BIT(sum_result, DIV_NUM_BIT_M1) & SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1));   \
   sum_result_sign = w_sign ^ partial_remainder_sign ^ sum_result_carry;                                           \
   partial_remainderH = (((__uint64_t)sum_result_msb) << DIV_NUM_BIT_M1) | VAL_RESIZE(sum_result, DIV_NUM_BIT_M1); \
   partial_remainder_sign = sum_result_sign;                                                                       \
   res = VAL_RESIZE((res << 1) | (partial_remainder_sign ^ minus_divisor_sign), DIV_NUM_BIT);

static __FORCE_INLINE __int64_t non_restoring_sdiv(__int64_t a, __int64_t b, __int64_t* rem, bool signed_div)
{
   bool divisor_select, carry_input;
   __uint64_t count;
   __uint64_t partial_remainder;
   bool plus_divisor_sign, minus_divisor_sign;
   __uint64_t plus_divisor, minus_divisor;
   bool w_sign;
   __uint64_t w;
   __uint64_t sum_result;
   bool sum_result_msb, sum_result_carry, sum_result_sign;
   bool plus_one;
   bool rem_nul, rem_nul_after, rem_correction;
   __uint64_t partial_remainderL = VAL_RESIZE(a, DIV_NUM_BIT);
#if DIV_NUM_BIT < 64
   __uint64_t partial_remainderH = VAL_RESIZE(a >> DIV_NUM_BIT, DIV_NUM_BIT);
   bool partial_remainder_sign = (a < 0) && signed_div;
#else
   bool partial_remainder_sign = SELECT_BIT(a, DIV_NUM_BIT_M1) && signed_div;
   __uint64_t partial_remainderH = (((__int64_t)partial_remainder_sign) << DIV_NUM_BIT_M1) >> DIV_NUM_BIT_M1;
#endif
   bool sign_a = partial_remainder_sign;
   __uint64_t res = 0;
   plus_divisor = VAL_RESIZE(b, DIV_NUM_BIT);
   plus_divisor_sign = SELECT_BIT(plus_divisor, DIV_NUM_BIT_M1) && signed_div;
   minus_divisor = VAL_RESIZE(~b, DIV_NUM_BIT);
   minus_divisor_sign = SELECT_BIT(minus_divisor, DIV_NUM_BIT_M1) || !signed_div;
   for(count = 0; count < (DIV_NUM_BIT / UNROLL_FACTOR); ++count)
   {
      BOOST_PP_REPEAT(UNROLL_FACTOR, LOOP_BODY, count);
   }
   divisor_select = (sign_a && partial_remainder_sign) ? minus_divisor_sign : (minus_divisor_sign ^ sign_a);
   carry_input = !divisor_select;
   w = divisor_select ? plus_divisor : minus_divisor;
   partial_remainder = VAL_RESIZE(partial_remainderH + w + carry_input, DIV_NUM_BIT);
   rem_nul = partial_remainderH == 0;
   rem_nul_after = partial_remainder == 0;
   plus_one = (plus_divisor_sign && !sign_a) || (!partial_remainder_sign && plus_divisor_sign && rem_nul) ||
              (!partial_remainder_sign && !plus_divisor_sign && sign_a && !rem_nul) ||
              (!plus_divisor_sign && sign_a && !rem_nul && !rem_nul_after) ||
              (partial_remainder_sign && plus_divisor_sign && sign_a && !rem_nul && rem_nul_after);
   rem_correction =
       (!rem_nul && (partial_remainder_sign ^ sign_a)) || (partial_remainder_sign && sign_a && rem_nul_after);
   if(rem_correction)
   {
      partial_remainderH = partial_remainder;
   }
   res += plus_one;
   *rem = partial_remainderH;
   return res;
}

#endif // _COMMON_CORE_NONRESTORING_H
