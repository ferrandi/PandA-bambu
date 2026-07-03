//    Copyright (C) 2014-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu IP Library.
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
/**
 * @file div_utilities
 * @brief function used to speedup the single precision floating point division.
 * The excerpt of code has been derived elaborating concepts and solutions described in:
 * - Eric Rice and Richard Hughey Multiprecision division on small-word parallel processors: Expanded Version:
 * http://www.cse.ucsc.edu/research/kestrel/papers/new2.pdf
 * - S.-K. Raina. FLIP: a Floating-point Library for Integer Processors. PhD thesis, ENS Lyon, France, 2006.
 *
 */
#ifndef _DIV_UTILITIES_H
#define _DIV_UTILITIES_H

#include <bambu_config.h>

static __FORCE_INLINE __uint64_t __builtin_umulh64(__uint64_t u, __uint64_t v)
{
   register __uint32_t u0, v0, k, u1, v1, w1, w2;
   register __uint64_t t, t0, t1;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (__uint64_t)u0 * v0;
   k = t >> 32;
   t0 = (__uint64_t)u1 * v0;
   t = t0 + (__uint64_t)k;
   w1 = t;
   w2 = t >> 32;
   t0 = (__uint64_t)u0 * v1;
   t = t0 + (__uint64_t)w1;
   k = t >> 32;
   t0 = (__uint64_t)u1 * v1;
   t1 = (__uint64_t)w2 + (__uint64_t)k;
   return t0 + t1;
}

#define GOLDSCHMIDT_MANTISSA_DIVISION_64()                                                                      \
   zExp = aExp - bExp - __exp_bias - 2;                                                                         \
   aSig = (aSig | (1ULL << __frac_bits)) << (62 - __frac_bits);                                                 \
   bSig = (bSig | (1ULL << __frac_bits)) << (63 - __frac_bits);                                                 \
   if(bSig <= (aSig << 1))                                                                                      \
   {                                                                                                            \
      aSig >>= 1;                                                                                               \
      ++zExp;                                                                                                   \
   }                                                                                                            \
   {                                                                                                            \
      static const __uint16_t softfloat_approxRecip_1k0s[16] = {0xFFC4, 0xF0BE, 0xE363, 0xD76F, 0xCCAD, 0xC2F0, \
                                                                0xBA16, 0xB201, 0xAA97, 0xA3C6, 0x9D7A, 0x97A6, \
                                                                0x923C, 0x8D32, 0x887E, 0x8417};                \
      static const __uint16_t softfloat_approxRecip_1k1s[16] = {0xF0F1, 0xD62C, 0xBFA1, 0xAC77, 0x9C0A, 0x8DDB, \
                                                                0x8185, 0x76BA, 0x6D3B, 0x64D4, 0x5D5C, 0x56B1, \
                                                                0x50B6, 0x4B55, 0x4679, 0x4211};                \
      __uint8_t index = bSig >> (27 + 32) & 0xF;                                                                \
      __uint16_t eps = (__uint16_t)(bSig >> (11 + 32));                                                         \
      __uint16_t r0 =                                                                                           \
          softfloat_approxRecip_1k0s[index] - ((softfloat_approxRecip_1k1s[index] * (__uint32_t)eps) >> 20);    \
      __uint32_t sigma0 = ~(__uint32_t)((r0 * (bSig >> 32)) >> 7);                                              \
      __uint32_t r = ((__uint32_t)r0 << 16) + ((r0 * (__uint64_t)sigma0) >> 24);                                \
      __uint32_t sqrSigma0 = ((__uint64_t)sigma0 * sigma0) >> 32;                                               \
      r += ((__uint32_t)r * (__uint64_t)sqrSigma0) >> 48;                                                       \
      __uint64_t p = ((__uint64_t)r) << (15 + 16);                                                              \
      ga0 = __builtin_umulh64(aSig, p) << 2;                                                                    \
      gb0 = __builtin_umulh64(bSig, p);                                                                         \
      gb1 = 0x8000000000000000U - gb0;                                                                          \
      ga1 = __builtin_umulh64(ga0, gb1) << 1;                                                                   \
      zSig = (ga1 + 0x10) >> 8;                                                                                 \
      __uint64_t zSigminus1 = zSig - 1;                                                                         \
      zSig <<= 9;                                                                                               \
      zSigminus1 <<= 9;                                                                                         \
      rem0 = __builtin_umulh64(bSig, zSig);                                                                     \
      rem = aSig - rem0;                                                                                        \
      zSig = ((__int64_t)rem) <= 0 ? zSigminus1 : zSig;                                                         \
      zSig = zSig | (0x1 << 8);                                                                                 \
   }

#endif // _DIV_UTILITIES_H
