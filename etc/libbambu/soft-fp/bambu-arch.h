/* Specific functions for bambu architecture.
   Copyright (C) 2014-2020 Politecnico di Milano (Italy).
   This file is part of the HLS-FP Library.

   The HLS-FP Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HLS-FP Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
#ifndef BAMBU_ARCH_H
#define BAMBU_ARCH_H
#include "bambu_macros.h"

#define __WORDSIZE 32
#define __BYTE_ORDER __LITTLE_ENDIAN
#define __bambu__

#define USE_NEWTON_RAPHSON

#define umul_ppmm_1(xh, xl, a, b)                                     \
   do                                                                 \
   {                                                                  \
      /* Generate umull, under compiler control.  */                  \
      register UDItype __t0 = ((UDItype)(USItype)(a)) * (USItype)(b); \
      (xl) = (USItype)__t0;                                           \
      (xh) = (USItype)(__t0 >> 32);                                   \
   } while(0)

#define umul_ppmm(xh, xl, a, b) umul_ppmm_1(xh, xl, a, b)

#define umul_ppmm_2(xh, xl, a, b)                     \
   do                                                 \
   {                                                  \
      register USItype u0, v0, k, u1, v1, w0, w1, w2; \
      register UDItype t, t0, t1;                     \
      USItype tlast;                                  \
      u1 = a >> 32;                                   \
      u0 = a;                                         \
      v1 = b >> 32;                                   \
      v0 = b;                                         \
      t = (UDItype)u0 * v0;                           \
      w0 = t;                                         \
      k = t >> 32;                                    \
      t0 = (UDItype)u1 * v0;                          \
      t = t0 + (UDItype)k;                            \
      w1 = t;                                         \
      w2 = t >> 32;                                   \
      t0 = (UDItype)u0 * v1;                          \
      t = t0 + (UDItype)w1;                           \
      (xl) = t << 32 | ((UDItype)w0);                 \
      k = t >> 32;                                    \
      t0 = (UDItype)u1 * v1;                          \
      t1 = (UDItype)w2 + (UDItype)k;                  \
      (xh) = t0 + t1;                                 \
   } while(0)

/**
 * counting leading zero
 * @param 32-bit value to find the log2 of
 */
#ifdef LOG2_BASED_CLZ
inline int clz(unsigned int v)
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
inline int clz(unsigned int v)
{
   _Bool result_4, result_3, result_2, result_1, result_0;
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

inline int clzll(unsigned long long int v)
{
   _Bool result_5;
   unsigned int val32;

   result_5 = v >> 32 == 0;
   val32 = result_5 ? v : v >> 32;
   return result_5 << 5 | clz(val32);
}
#else
inline int clz(unsigned int v)
{
   unsigned char res;
   count_leading_zero_macro(32, v, res) return res;
}
inline int clzll(unsigned long long int v)
{
   unsigned char res;
   count_leading_zero_macro(64, v, res) return res;
}

#endif

#define count_leading_zeros(count, x) ((count) = clz(x))

#define UDIV_NEEDS_NORMALIZATION 0

inline unsigned __divlu2(unsigned u1, unsigned u0, unsigned v, unsigned* r)
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
      USItype __r;                     \
      (q) = __divlu2(nh, nl, d, &__r); \
      (r) = __r;                       \
   } while(0)

#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
   do                                      \
   {                                       \
      UWtype __x;                          \
      __x = (al) - (bl);                   \
      (sh) = (ah) - (bh) - (__x > (al));   \
      (sl) = __x;                          \
   } while(0)

#endif
