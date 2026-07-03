//    Copyright (C) 2017-2026 Politecnico di Milano
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

#include <gmp.h>
#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#define PRECISION 30

#define TEST_PREFIX(fname) local##fname
#include "ef_sqrt.c"

static float golden_reference_sqrt(float a)
{
   mpfr_t a_mpfr, res;
   float final_value;
   mpfr_init2(a_mpfr, PRECISION);
   mpfr_init2(res, PRECISION);
   mpfr_set_d(a_mpfr, a, MPFR_RNDN);
   mpfr_sqrt(res, a_mpfr, MPFR_RNDN);
   final_value = mpfr_get_flt(res, MPFR_RNDN);
   mpfr_clear(a_mpfr);
   mpfr_clear(res);
   return final_value;
}

int main_test_sqrt()
{
   unsigned int s = 0;
   unsigned int e = 126;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;

   for(s = 0; s < 2; ++s)
   {
      printf("s=%d\n", s);
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
      for(e = 0; e < 256; ++e)
      {
         unsigned int x = 0x0;
#pragma omp critical
         printf("e=%d\n", e);
         for(x = 0; x < (1 << 23); ++x)
         {
            float_uint_converter func_in, func_out, func_golden_libm;
            func_in.b = (s << 31) | (e << 23) | x;
            func_out.f = local__ieee754_sqrtf(func_in.f);
            func_golden_libm.f = sqrtf(func_in.f);
            if((func_golden_libm.b >> 31) != (func_out.b >> 31))
            {
               float_uint_converter func_golden;
               func_golden.f = golden_reference_sqrt(func_in.f);
               printf("Opposite sign\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sqrt golden=%.40f\n", func_golden.f);
               printf("golden=%x\n", func_golden.b);
               printf("my sqrt=%.20f\n", func_out.f);
               printf("binary=%x\n", func_out.b);
               printf("sqrt libm=%.20f\n", func_golden_libm.f);
               printf("libm=%x\n", func_golden_libm.b);
               abort();
            }
            if(abs(func_golden_libm.b - func_out.b) > 1)
            {
               float_uint_converter func_golden;
               func_golden.f = golden_reference_sqrt(func_in.f);
               printf("NO PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sqrt golden=%.40f\n", func_golden.f);
               printf("golden=%x\n", func_golden.b);
               printf("my sqrt=%.20f\n", func_out.f);
               printf("binary=%x\n", func_out.b);
               printf("sqrt libm=%.20f\n", func_golden_libm.f);
               printf("libm=%x\n", func_golden_libm.b);
               abort();
            }
            else if(abs(func_golden_libm.b - func_out.b) == 1)
            {
               if(func_golden_libm.b > func_out.b)
                  n_ones_pos++;
               else
                  n_ones_neg++;
            }
         }
      }
   }
   printf("n_ones_pos=%d\n", n_ones_pos);
   printf("n_ones_neg=%d\n", n_ones_neg);
   return 0;
}

int main()
{
   printf("*** main ***\n");
   main_test_sqrt();
   return 0;
}
