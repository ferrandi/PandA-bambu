/* Specific functions for bambu architecture.
   Copyright (C) 2014-2026 Politecnico di Milano (Italy).
   SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
   This file is part of the HLS-DIV Library.

   author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "softint.h"

static __FORCE_INLINE __uint32_t lsl(__uint32_t a, __uint32_t b)
{
   return a << b;
}

static __FORCE_INLINE __uint32_t lsr(__uint32_t a, __uint32_t b)
{
   return a >> b;
}

static __FORCE_INLINE __uint32_t mul(__uint32_t a, __uint32_t b)
{
   return a * b;
}

#define USI__WORDSIZE 32

static __FORCE_INLINE __uint32_t __udivmodsi4(__uint32_t x, __uint32_t y, __uint32_t* res)
{
   static unsigned char unrt[256] = {
       -2,   -4,   -6,   -8,   -10,  -12,  -14,  -16,  -18,  -20,  -22,  -23,  -25,  -27,  -29,  -31,  -32,  -34,  -36,
       -38,  -39,  -41,  -43,  -44,  -46,  -48,  -49,  -51,  -53,  -54,  -56,  -57,  -59,  -61,  -62,  -64,  -65,  -67,
       -68,  -70,  -71,  -73,  -74,  -76,  -77,  -78,  -80,  -81,  -83,  -84,  -86,  -87,  -88,  -90,  -91,  -92,  -94,
       -95,  -96,  -98,  -99,  -100, -102, -103, -104, -105, -107, -108, -109, -110, -112, -113, -114, -115, -117, -118,
       -119, -120, -121, -122, -124, -125, -126, -127, -128, 127,  126,  125,  123,  122,  121,  120,  119,  118,  117,
       116,  115,  114,  113,  112,  111,  110,  109,  108,  107,  106,  105,  104,  103,  102,  101,  100,  99,   98,
       97,   96,   95,   94,   93,   92,   91,   90,   89,   88,   88,   87,   86,   85,   84,   83,   82,   81,   80,
       80,   79,   78,   77,   76,   75,   74,   74,   73,   72,   71,   70,   70,   69,   68,   67,   66,   66,   65,
       64,   63,   62,   62,   61,   60,   59,   59,   58,   57,   56,   56,   55,   54,   53,   53,   52,   51,   50,
       50,   49,   48,   48,   47,   46,   46,   45,   44,   43,   43,   42,   41,   41,   40,   39,   39,   38,   37,
       37,   36,   35,   35,   34,   33,   33,   32,   32,   31,   30,   30,   29,   28,   28,   27,   27,   26,   25,
       25,   24,   24,   23,   22,   22,   21,   21,   20,   19,   19,   18,   18,   17,   17,   16,   15,   15,   14,
       14,   13,   13,   12,   12,   11,   10,   10,   9,    9,    8,    8,    7,    7,    6,    6,    5,    5,    4,
       4,    3,    3,    2,    2,    1,    1,    0,    0};
   // table lookup start
   unsigned char k = clz(y);
   // prescaling
   __uint32_t lshifted_y = lsl(y, k);
   /// remove the leading 1
   lshifted_y = lshifted_y << 1;
   __uint32_t ty = lsr(lshifted_y, USI__WORDSIZE - 8);
   // table lookup
   __uint32_t t = unrt[ty] | 256;
   // postscaling
   __uint32_t z = lsr(lsl(t, USI__WORDSIZE - 9), USI__WORDSIZE - k - 1);
   // z recurrence, 2 iterations
   __uint32_t my = 0 - y;
   __uint32_t z0, d0, q0, q1;
   __uint32_t q;
   z0 = mul(my, z);
   umul_ppmm(d0, q0, z, z0);
   z = z + d0;
   z0 = mul(my, z);
   umul_ppmm(d0, q0, z, z0);
   z = z + d0;

   // q estimate
   umul_ppmm(q, q1, x, z);
   __uint32_t r = x - mul(y, q);
   // q refinement
   if(r >= y)
   {
      r = r - y;
      q = q + 1;
      if(r >= y)
      {
         r = r - y;
         q = q + 1;
      }
   }

   if(res != 0)
      *res = r;
   return q;
}

__int32_t __divsi3(__int32_t u, __int32_t v)
{
   bool c = 0;
   __int32_t w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
   {
      c = !c;
      v = -v;
   }
   w = __udivmodsi4(u, v, NULL);
   if(c)
      w = -w;
   return w;
}

__int32_t __modsi3(__int32_t u, __int32_t v)
{
   bool c = 0;
   __int32_t w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmodsi4(u, v, (__uint32_t*)&w);
   if(c)
      w = -w;
   return w;
}

__uint32_t __udivsi3(__uint32_t u, __uint32_t v)
{
   return __udivmodsi4(u, v, NULL);
}

__uint32_t __umodsi3(__uint32_t u, __uint32_t v)
{
   __uint32_t w;

   __udivmodsi4(u, v, &w);
   return w;
}
