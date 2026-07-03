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
#ifndef _SOFTINT_H
#define _SOFTINT_H

#include <bambu_config.h>

#include <bambu_macros.h>

#define NULL ((void*)0)

/* Prototypes of exported functions.  */
extern __int64_t __divdi3(__int64_t u, __int64_t v);
extern __int64_t __moddi3(__int64_t u, __int64_t v);
extern __uint64_t __udivdi3(__uint64_t u, __uint64_t v);
extern __uint64_t __umoddi3(__uint64_t u, __uint64_t v);

extern __int64_t __divdi36432(__int64_t u, __int32_t v);
extern __int64_t __moddi36432(__int64_t u, __int32_t v);
extern __uint64_t __udivdi36432(__uint64_t u, __uint32_t v);
extern __uint64_t __umoddi36432(__uint64_t u, __uint32_t v);

#define umul_ppmm_1(xh, xl, a, b)                                                 \
   do                                                                             \
   {                                                                              \
      /* Generate umull, under compiler control.  */                              \
      register __uint64_t __t0 = ((__uint64_t)(__uint32_t)(a)) * (__uint32_t)(b); \
      (xl) = (__uint32_t)__t0;                                                    \
      (xh) = (__uint32_t)(__t0 >> 32);                                            \
   } while(0)

#define umul_ppmm(xh, xl, a, b) umul_ppmm_1(xh, xl, a, b)

#define umul_ppmm_2(xh, xl, a, b)                        \
   do                                                    \
   {                                                     \
      register __uint32_t u0, v0, k, u1, v1, w0, w1, w2; \
      register __uint64_t t, t0, t1;                     \
      __uint32_t tlast;                                  \
      u1 = a >> 32;                                      \
      u0 = a;                                            \
      v1 = b >> 32;                                      \
      v0 = b;                                            \
      t = (__uint64_t)u0 * v0;                           \
      w0 = t;                                            \
      k = t >> 32;                                       \
      t0 = (__uint64_t)u1 * v0;                          \
      t = t0 + (__uint64_t)k;                            \
      w1 = t;                                            \
      w2 = t >> 32;                                      \
      t0 = (__uint64_t)u0 * v1;                          \
      t = t0 + (__uint64_t)w1;                           \
      (xl) = t << 32 | ((__uint64_t)w0);                 \
      k = t >> 32;                                       \
      t0 = (__uint64_t)u1 * v1;                          \
      t1 = (__uint64_t)w2 + (__uint64_t)k;               \
      (xh) = t0 + t1;                                    \
   } while(0)

/**
 * counting leading zero
 * @param 32-bit value to find the log2 of
 */
#ifdef LOG2_BASED_CLZ
static __FORCE_INLINE int clz(unsigned int v)
{
   unsigned int r; // result of log2(v) will go here
   unsigned int shift;
   r = (v > 0xFFFF) << 4;
   v >>= r;
   shift = (v > 0xFF) << 3;
   v >>= shift;
   r |= shift;
   shift = (v > 0xF) << 2;
   v >>= shift;
   r |= shift;
   shift = (v > 0x3) << 1;
   v >>= shift;
   r |= shift;
   r |= (v >> 1);

   return 31 - r;
}

int clzll(unsigned long long int v)
{
   unsigned int high = v >> 32;
   unsigned int low = v;
   return high == 0 ? 32 + clz(v) : clz(high);
}
#elif SHIFT_BASED
static __FORCE_INLINE int clz(unsigned int v)
{
   bool result_4, result_3, result_2, result_1, result_0;
   unsigned short val16;
   unsigned char val8, val4;
   result_4 = v >> 16 == 0;
   val16 = result_4 ? v : v >> 16;
   result_3 = val16 >> 8 == 0;
   val8 = result_3 ? val16 : val16 >> 8;
   result_2 = val8 >> 4 == 0;
   val4 = result_2 ? val8 & 15 : val8 >> 4;
   result_1 = val4 >> 2 == 0;
   result_0 = result_1 ? (~((val4 & 2) >> 1)) & 1 : (~((val4 & 8) >> 3)) & 1;
   return result_4 << 4 | result_3 << 3 | result_2 << 2 | result_1 << 1 | result_0;
}

static __FORCE_INLINE int clzll(unsigned long long int v)
{
   bool result_5;
   unsigned int val32;

   result_5 = v >> 32 == 0;
   val32 = result_5 ? v : v >> 32;
   return result_5 << 5 | clz(val32);
}
#else
static __FORCE_INLINE int clz(unsigned int v)
{
   unsigned char res;
   count_leading_zero_macro(32, v, res) return res;
}
static __FORCE_INLINE int clzll(unsigned long long int v)
{
   unsigned char res;
   count_leading_zero_macro(64, v, res) return res;
}
#endif

#define count_leading_zeros(count, x) ((count) = clz(x))

#define UDIV_NEEDS_NORMALIZATION 0

static __FORCE_INLINE unsigned __divlu2(unsigned u1, unsigned u0, unsigned v, unsigned* r)
{
   const unsigned b = 65536; // Number base (16 bits).
   unsigned un1, un0,        // Norm. dividend LSD's.
       vn1, vn0,             // Norm. divisor digits.
       q1, q0,               // Quotient digits.
       un32, un21, un10,     // Dividend digit pairs.
       rhat;                 // A remainder.
   int s;                    // Shift amount for norm.

   if(u1 >= v)
   {                      // If overflow, set rem.
      if(r != 0)          // to an impossible value,
         *r = 0xFFFFFFFF; // and return the largest
      return 0xFFFFFFFF;
   } // possible quotient.

   s = clz(v);       // 0 <= s <= 31.
   v = v << s;       // Normalize divisor.
   vn1 = v >> 16;    // Break divisor up into
   vn0 = v & 0xFFFF; // two 16-bit digits.

   un32 = (u1 << s) | ((u0 >> (32 - s)) & (-s >> 31));
   un10 = u0 << s; // Shift dividend left.

   un1 = un10 >> 16;    // Break right half of
   un0 = un10 & 0xFFFF; // dividend into two digits.

   q1 = un32 / vn1;        // Compute the first
   rhat = un32 - q1 * vn1; // quotient digit, q1.
again1:
   if(q1 >= b || q1 * vn0 > b * rhat + un1)
   {
      q1 = q1 - 1;
      rhat = rhat + vn1;
      if(rhat < b)
         goto again1;
   }

   un21 = un32 * b + un1 - q1 * v; // Multiply and subtract.

   q0 = un21 / vn1;        // Compute the second
   rhat = un21 - q0 * vn1; // quotient digit, q0.
again2:
   if(q0 >= b || q0 * vn0 > b * rhat + un0)
   {
      q0 = q0 - 1;
      rhat = rhat + vn1;
      if(rhat < b)
         goto again2;
   }

   if(r != 0)                              // If remainder is wanted,
      *r = (un21 * b + un0 - q0 * v) >> s; // return it.
   return q1 * b + q0;
}

#define udiv_qrnnd(q, r, nh, nl, d)    \
   do                                  \
   {                                   \
      __uint32_t __r;                  \
      (q) = __divlu2(nh, nl, d, &__r); \
      (r) = __r;                       \
   } while(0)

#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
   do                                      \
   {                                       \
      __uint32_t __x;                      \
      __x = (al) - (bl);                   \
      (sh) = (ah) - (bh) - (__x > (al));   \
      (sl) = __x;                          \
   } while(0)

#endif // _SOFTINT_H
