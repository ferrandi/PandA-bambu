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
 *              Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file constdiv_magic.hpp
 * @brief LLVM-style magic-number computation for fixed-width div-by-const (<=64b).
 */
/**
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef CONSTDIV_MAGIC_HPP
#define CONSTDIV_MAGIC_HPP

#include "exceptions.hpp"

#include <cstdint>

namespace constdiv_magic
{
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
   typedef __int128 int128_t;
   typedef unsigned __int128 uint128_t;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

   struct SDivMagic64
   {
      uint64_t magic;
      unsigned shift;
      int numeratorFactor;
      int shiftMask;
   };

   struct UDivMagic64
   {
      uint64_t magic;
      unsigned preShift;
      unsigned postShift;
      bool isAdd;
   };

   inline uint64_t maskForWidth(unsigned width)
   {
      if(width >= 64)
      {
         return ~0ULL;
      }
      return (1ULL << width) - 1ULL;
   }

   inline uint64_t truncToWidth(uint64_t value, unsigned width)
   {
      return value & maskForWidth(width);
   }

   inline uint64_t addMod(uint64_t a, uint64_t b, unsigned width)
   {
      return truncToWidth(a + b, width);
   }

   inline uint64_t subMod(uint64_t a, uint64_t b, unsigned width)
   {
      return truncToWidth(a - b, width);
   }

   inline uint64_t shl1Mod(uint64_t value, unsigned width)
   {
      return truncToWidth(value << 1, width);
   }

   inline uint64_t lowBitsSet(unsigned bits)
   {
      if(bits == 0)
      {
         return 0ULL;
      }
      if(bits >= 64)
      {
         return ~0ULL;
      }
      return (1ULL << bits) - 1ULL;
   }

   inline int64_t signExtend(uint64_t value, unsigned width)
   {
      if(width >= 64)
      {
         return static_cast<int64_t>(value);
      }
      const uint64_t sign_bit = 1ULL << (width - 1);
      const uint64_t mask = (1ULL << width) - 1ULL;
      value &= mask;
      return static_cast<int64_t>((value ^ sign_bit) - sign_bit);
   }

   inline uint64_t negateMod(uint64_t value, unsigned width)
   {
      return addMod(~value, 1ULL, width);
   }

   inline void udivremU64(uint64_t num, uint64_t den, uint64_t& q, uint64_t& r)
   {
      q = num / den;
      r = num - q * den;
   }

   inline SDivMagic64 computeSDivMagic64(int64_t divisor, unsigned width)
   {
      THROW_ASSERT(width >= 3 && width <= 64, "Invalid bitwidth for signed magic computation");
      const int64_t d = signExtend(static_cast<uint64_t>(divisor), width);
      THROW_ASSERT(d != 0, "Division by zero is not supported");

      if(d == 1 || d == -1)
      {
         const int numerator_factor = d > 0 ? 1 : -1;
         return {0ULL, 0U, numerator_factor, 0};
      }

      const uint64_t signed_min = 1ULL << (width - 1);
      const uint64_t ad = d < 0 ? static_cast<uint64_t>(-static_cast<int128_t>(d)) : static_cast<uint64_t>(d);
      const uint64_t sign = d < 0 ? 1ULL : 0ULL;
      const uint64_t t = addMod(signed_min, sign, width);
      const uint64_t anc = subMod(subMod(t, 1ULL, width), t % ad, width);

      unsigned p = width - 1;
      uint64_t q1 = 0;
      uint64_t r1 = 0;
      uint64_t q2 = 0;
      uint64_t r2 = 0;
      udivremU64(signed_min, anc, q1, r1);
      udivremU64(signed_min, ad, q2, r2);

      uint64_t delta = 0;
      do
      {
         p++;
         q1 = shl1Mod(q1, width);
         r1 = shl1Mod(r1, width);
         if(r1 >= anc)
         {
            q1 = addMod(q1, 1ULL, width);
            r1 = subMod(r1, anc, width);
         }

         q2 = shl1Mod(q2, width);
         r2 = shl1Mod(r2, width);
         if(r2 >= ad)
         {
            q2 = addMod(q2, 1ULL, width);
            r2 = subMod(r2, ad, width);
         }

         delta = subMod(ad, r2, width);
      } while(q1 < delta || (q1 == delta && r1 == 0));

      uint64_t magic = addMod(q2, 1ULL, width);
      if(d < 0)
      {
         magic = negateMod(magic, width);
      }
      const unsigned shift = p - width;

      const int64_t magic_signed = signExtend(magic, width);
      int numerator_factor = 0;
      if(d > 0 && magic_signed < 0)
      {
         numerator_factor = 1;
      }
      else if(d < 0 && magic_signed > 0)
      {
         numerator_factor = -1;
      }

      const int shift_mask = -1;
      return {magic, shift, numerator_factor, shift_mask};
   }

   inline UDivMagic64 computeUDivMagic64(uint64_t divisor, unsigned width, unsigned leading_zeros,
                                         bool allow_even_divisor_optimization)
   {
      THROW_ASSERT(width > 1 && width <= 64, "Invalid bitwidth for unsigned magic computation");
      THROW_ASSERT(leading_zeros < width, "Invalid leading zeros for unsigned magic computation");

      const uint64_t d = divisor & maskForWidth(width);
      THROW_ASSERT(d != 0 && d != 1, "Division by zero/one is not supported");

      const uint64_t all_ones = lowBitsSet(width - leading_zeros);
      const uint64_t signed_min = 1ULL << (width - 1);
      const uint64_t signed_max = signed_min - 1ULL;

      const uint64_t tmp = subMod(addMod(all_ones, 1ULL, width), d, width);
      const uint64_t rem = tmp % d;
      const uint64_t nc = subMod(all_ones, rem, width);

      bool is_add = false;
      unsigned p = width - 1;
      uint64_t q1 = 0;
      uint64_t r1 = 0;
      uint64_t q2 = 0;
      uint64_t r2 = 0;
      udivremU64(signed_min, nc, q1, r1);
      udivremU64(signed_max, d, q2, r2);

      uint64_t delta = 0;
      do
      {
         p++;
         const uint64_t nc_minus_r1 = subMod(nc, r1, width);
         if(r1 >= nc_minus_r1)
         {
            q1 = addMod(shl1Mod(q1, width), 1ULL, width);
            r1 = subMod(shl1Mod(r1, width), nc, width);
         }
         else
         {
            q1 = shl1Mod(q1, width);
            r1 = shl1Mod(r1, width);
         }

         const uint64_t d_minus_r2 = subMod(d, r2, width);
         if(addMod(r2, 1ULL, width) >= d_minus_r2)
         {
            if(q2 >= signed_max)
            {
               is_add = true;
            }
            q2 = addMod(shl1Mod(q2, width), 1ULL, width);
            r2 = subMod(addMod(shl1Mod(r2, width), 1ULL, width), d, width);
         }
         else
         {
            if(q2 >= signed_min)
            {
               is_add = true;
            }
            q2 = shl1Mod(q2, width);
            r2 = addMod(shl1Mod(r2, width), 1ULL, width);
         }

         delta = subMod(subMod(d, 1ULL, width), r2, width);
      } while(p < width * 2 && (q1 < delta || (q1 == delta && r1 == 0)));

      if(is_add && ((d & 1ULL) == 0) && allow_even_divisor_optimization)
      {
         const unsigned pre_shift = static_cast<unsigned>(__builtin_ctzll(d));
         const uint64_t shifted_d = d >> pre_shift;
         UDivMagic64 inner = computeUDivMagic64(shifted_d, width, leading_zeros + pre_shift, true);
         THROW_ASSERT(!inner.isAdd && inner.preShift == 0, "Unexpected even-divisor optimization state");
         inner.preShift = pre_shift;
         return inner;
      }

      const uint64_t magic = addMod(q2, 1ULL, width);
      unsigned post_shift = p - width;
      if(is_add)
      {
         THROW_ASSERT(post_shift > 0, "Unexpected shift");
         post_shift -= 1;
      }

      UDivMagic64 retval;
      retval.magic = magic;
      retval.preShift = 0;
      retval.postShift = post_shift;
      retval.isAdd = is_add;
      return retval;
   }

} // namespace constdiv_magic

#endif
