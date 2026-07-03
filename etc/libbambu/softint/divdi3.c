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

#include "softint.h"

static __uint64_t mul64(__uint64_t u, __uint64_t v)
{
#if 1
   __uint64_t t;
   __uint32_t u0, u1, v0, v1, k;
   __uint32_t w0, w1;
   __uint32_t tlast;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (__uint64_t)u0 * v0;
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((__uint64_t)tlast) << 32) | ((__uint64_t)w0);
#else
   return u * v;
#endif
}

static __uint64_t umulh64(__uint64_t u, __uint64_t v)
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

static __FORCE_INLINE __uint64_t lsl64(__uint64_t a, __uint64_t b)
{
   return a << b;
}

static __FORCE_INLINE __uint64_t lsr64(__uint64_t a, __uint64_t b)
{
   return a >> b;
}

static __FORCE_INLINE __uint64_t __udivmoddi4(__uint64_t x, __uint64_t y, __uint64_t* res)
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
#define W 64
   unsigned char k = clzll(y);
   __uint64_t lshifted_y = lsl64(y, k);
   /// remove the leading 1
   lshifted_y = lshifted_y << 1;
   __uint64_t ty = lsr64(lshifted_y, W - 8);         // prescaling
   __uint64_t t = unrt[ty] | 256;                    // table lookup
   __uint64_t z = lsr64(lsl64(t, W - 9), W - k - 1); // postscaling
                                                     // z recurrence
   __uint64_t my = 0 - y;
//#define NR_UNROLLED
#ifdef NR_UNROLLED
   z = z + umulh64(z, mul64(my, z));
   z = z + umulh64(z, mul64(my, z));
   z = z + umulh64(z, mul64(my, z));
#else
   unsigned int index;
   for(index = 0; index < 3; ++index)
   {
      __uint64_t zd = umulh64(z, mul64(my, z));
      // if (zd == 0) break;
      z = z + zd;
   }
#endif
   // q estimate
   __uint64_t q = umulh64(x, z);
   __uint64_t r = x - mul64(y, q);
   // q refinement
   if(r >= y)
   {
      r = r - y;
      q = q + 1;
      if(r >= y)
      {
         r = r - y;
         q = q + 1;
         if(r >= y)
         { // add this in case of three iterations
            r = r - y;
            q = q + 1;
         }
      }
   }
   if(res != 0)
      *res = r;
   return q;
}

__int64_t __divdi3(__int64_t u, __int64_t v)
{
   bool c = 0;
   __int64_t w;

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
   w = __udivmoddi4(u, v, NULL);
   if(c)
      w = -w;
   return w;
}

__int64_t __moddi3(__int64_t u, __int64_t v)
{
   bool c = 0;
   __int64_t w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmoddi4(u, v, (__uint64_t*)&w);
   if(c)
      w = -w;
   return w;
}

__uint64_t __udivdi3(__uint64_t u, __uint64_t v)
{
   return __udivmoddi4(u, v, NULL);
}

__uint64_t __umoddi3(__uint64_t u, __uint64_t v)
{
   __uint64_t w;

   __udivmoddi4(u, v, &w);
   return w;
}
