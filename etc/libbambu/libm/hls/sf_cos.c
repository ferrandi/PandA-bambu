//    Copyright (C) 2016-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libm_hls IP Library.
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

/* In this file the single precision sine and cosine functions are implemented
   following the HOTBM method published by
   Jeremie Detrey and Florent de Dinechin, "Floating-point Trigonometric Functions for FPGAs" FPL 2007.
*/

#include "kf_sincos.c"

#ifndef TEST_PREFIX
#define TEST_PREFIX
#endif

float TEST_PREFIX(cosf)(float x)
{
   unsigned int y;
   bool s;
   unsigned char e;
   unsigned int m;
   float_uint_converter func_in;
   func_in.f = x;
   y = func_in.b;
   e = (y >> 23) & 255;
   s = y >> 31;
   m = y & 0x007fffff;
   if(e == 255 && m == 0)
      return -__builtin_nanf("");
   else if(e == 255)
   {
      func_in.b |= (0x7FC << 20);
      return func_in.f;
   }
   else if(e < 115) // if x is less than 2^-12 return x
      return 1.0;
   else
   {
      unsigned int global_b_fract;
      unsigned char global_e_final;
      unsigned int global_mantissa;
      unsigned char n;
      extern float fabsf(float x);
      unsigned int res_rr = __bambu_range_redux(fabsf(x), &global_b_fract, &global_e_final, &global_mantissa, &n);
      float_uint_converter func_out;
      func_out.b = res_rr;
      float y = func_out.f;
#ifdef DEBUG_PRINT
      printf("y=%.20f\n", y);
      printf("n=%d\n", n);
#endif
      bool do_cosine = 1;
      bool do_negate = 0;
      switch(n & 7)
      {
         case 0:
            break;
         case 1:
            do_cosine = 0;
            break;
         case 2:
            do_cosine = 0;
            do_negate = 1;
            break;
         case 3:
            do_negate = 1;
            break;
         case 4:
            do_negate = 1;
            break;
         case 5:
            do_cosine = 0;
            do_negate = 1;
            break;
         case 6:
            do_cosine = 0;
            break;
         case 7:
         default:
            break;
      }
      return __fsin_or_cos(global_b_fract, global_e_final, global_mantissa, do_cosine, do_negate);
   }
}
