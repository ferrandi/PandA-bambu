/* non-restoring division and modulus core function.
   Copyright (C) 2014-2020 Politecnico di Milano (Italy).

   author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

   The HLS-DIV Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HLS-DIV Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef DIV_NUM_BIT
#define DIV_NUM_BIT 64
#endif

#define DIV_NUM_BIT_M1 BOOST_PP_SUB(DIV_NUM_BIT, 1)

#ifndef UNROLL_FACTOR
#define UNROLL_FACTOR 1
#endif

#define LOOP_BODY(z, n, data)                                                                                                                                                       \
   divisor_select = partial_remainder_sign ^ plus_divisor_sign;                                                                                                                     \
   carry_input = !divisor_select;                                                                                                                                                   \
   w = divisor_select ? plus_divisor : minus_divisor;                                                                                                                               \
   w_sign = divisor_select ? plus_divisor_sign : minus_divisor_sign;                                                                                                                \
   partial_remainder_sign = SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1);                                                                                                         \
   partial_remainderH = (VAL_RESIZE(partial_remainderH, DIV_NUM_BIT_M1) << 1) | SELECT_BIT(partial_remainderL, DIV_NUM_BIT_M1);                                                     \
   partial_remainderL = VAL_RESIZE(partial_remainderL << 1, DIV_NUM_BIT);                                                                                                           \
   sum_result = VAL_RESIZE(w, DIV_NUM_BIT_M1) + VAL_RESIZE(partial_remainderH, DIV_NUM_BIT_M1) + carry_input;                                                                       \
   sum_result_msb = SELECT_BIT(sum_result, DIV_NUM_BIT_M1) ^ SELECT_BIT(w, DIV_NUM_BIT_M1) ^ SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1);                                        \
   sum_result_carry = (SELECT_BIT(sum_result, DIV_NUM_BIT_M1) & SELECT_BIT(w, DIV_NUM_BIT_M1)) | (SELECT_BIT(w, DIV_NUM_BIT_M1) & SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1)) | \
                      (SELECT_BIT(sum_result, DIV_NUM_BIT_M1) & SELECT_BIT(partial_remainderH, DIV_NUM_BIT_M1));                                                                    \
   sum_result_sign = w_sign ^ partial_remainder_sign ^ sum_result_carry;                                                                                                            \
   partial_remainderH = (((UDATATYPE)sum_result_msb) << DIV_NUM_BIT_M1) | VAL_RESIZE(sum_result, DIV_NUM_BIT_M1);                                                                   \
   partial_remainder_sign = sum_result_sign;                                                                                                                                        \
   res = VAL_RESIZE((res << 1) | (partial_remainder_sign ^ minus_divisor_sign), DIV_NUM_BIT);

static inline long long non_restoring_sdiv(long long a, long long b, long long* rem, _Bool signed_div)
{
   BOOLTYPE divisor_select;
   BOOLTYPE carry_input;
   UDATATYPE count;
   UDATATYPE partial_remainderL = VAL_RESIZE(a, DIV_NUM_BIT);
#if DIV_NUM_BIT < 64
   UDATATYPE partial_remainderH = VAL_RESIZE(a >> DIV_NUM_BIT, DIV_NUM_BIT);
   BOOLTYPE partial_remainder_sign = (a < 0) && signed_div;
#else
   BOOLTYPE partial_remainder_sign = SELECT_BIT(a, DIV_NUM_BIT_M1) && signed_div;
   UDATATYPE partial_remainderH = (((long long)partial_remainder_sign) << DIV_NUM_BIT_M1) >> DIV_NUM_BIT_M1;
#endif
   UDATATYPE partial_remainder;
   BOOLTYPE sign_a = partial_remainder_sign;
   BOOLTYPE plus_divisor_sign;
   UDATATYPE plus_divisor;
   BOOLTYPE minus_divisor_sign;
   UDATATYPE minus_divisor;
   BOOLTYPE w_sign;
   UDATATYPE w;
   UDATATYPE sum_result;
   BOOLTYPE sum_result_msb;
   BOOLTYPE sum_result_carry;
   BOOLTYPE sum_result_sign;
   UDATATYPE res = 0;
   BOOLTYPE plus_one;
   BOOLTYPE rem_nul;
   BOOLTYPE rem_nul_after;
   BOOLTYPE rem_correction;
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
   plus_one = (plus_divisor_sign && !sign_a) || (!partial_remainder_sign && plus_divisor_sign && rem_nul) || (!partial_remainder_sign && !plus_divisor_sign && sign_a && !rem_nul) || (!plus_divisor_sign && sign_a && !rem_nul && !rem_nul_after) ||
              (partial_remainder_sign && plus_divisor_sign && sign_a && !rem_nul && rem_nul_after);
   rem_correction = (!rem_nul && (partial_remainder_sign ^ sign_a)) || (partial_remainder_sign && sign_a && rem_nul_after);
   if(rem_correction)
   {
      partial_remainderH = partial_remainder;
   }
   res += plus_one;
   *rem = partial_remainderH;
   return res;
}
