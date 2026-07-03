//    Copyright (C) 2020-2026 Politecnico di Milano
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// #define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define DEBUG_PRINT_STATS
#endif

#define TEST_PREFIX(fname) local##fname
#include "e_sqrt.c"

int main_test_sqrt()
{
   int nT = 4;
   unsigned long long s = 0;
   unsigned long long e = 0;
#ifdef DEBUG_PRINT_STATS
   unsigned long long n_ones_pos = 0;
   unsigned long long n_ones_neg = 0;
#endif

   unsigned eindex, start_eindex, end_eindex;
   const unsigned startE[] = {0, 1023 - nT / 2, 2048 - nT};
   const unsigned endE[] = {nT, 1023 + nT / 2, 2048};

   for(s = 0; s < 2; ++s)
   {
      for(eindex = 0; eindex < 3; ++eindex)
      {
         unsigned mindex;
         start_eindex = startE[eindex];
         end_eindex = endE[eindex];
#ifdef DEBUG_PRINT_STATS
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) private(e) private(mindex) collapse(2)
#else
#pragma omp parallel for private(e) private(mindex) collapse(2)
#endif
         for(e = start_eindex; e < end_eindex; ++e)
         {
            for(mindex = 0; mindex < 2; ++mindex)
            {
               const unsigned long long startM[] = {0, 0xFFFFF00000000LL};
               const unsigned long long endM[] = {0x100000000LL, 0x10000000000000LL};
               unsigned long long x;
#ifdef DEBUG_PRINT
#pragma omp critical
               printf("e=%lld\n", e);
#endif
               for(x = startM[mindex]; x < endM[mindex]; ++x)
               {
                  double_uint_converter func_in, func_out, func_golden_libm;
                  func_in.b = (s << 63) | (e << 52) | x;
                  func_out.f = local__ieee754_sqrt(func_in.f);
                  func_golden_libm.f = sqrt(func_in.f);
                  if((func_golden_libm.b >> 63) != (func_out.b >> 63))
                  {
                     printf("Opposite sign\n");
                     printf("s=%llu\n", s);
                     printf("e=%llu\n", e);
                     printf("x=%llx\n", x);
                     printf("my sqrt=%.60f\n", func_out.f);
                     printf("binary=%llx\n", func_out.b);
                     printf("sqrt libm=%.60f\n", func_golden_libm.f);
                     printf("libm=%llx\n", func_golden_libm.b);
                     abort();
                  }
                  if(llabs((long long)(func_golden_libm.b - func_out.b)) > 1)
                  {
                     double_uint_converter func_golden;
                     printf("NO PASS\n");
                     printf("NO PASS\n");
                     printf("s=%llu\n", s);
                     printf("e=%llu\n", e);
                     printf("x=%llx\n", x);
                     printf("my sqrt=%.60f\n", func_out.f);
                     printf("binary=%llx\n", func_out.b);
                     printf("sqrt libm=%.60f\n", func_golden_libm.f);
                     printf("libm=%llx\n", func_golden_libm.b);
                     abort();
                  }
#ifdef DEBUG_PRINT_STATS
                  else if(llabs((long long)(func_golden_libm.b - func_out.b)) == 1)
                  {
                     if(func_golden_libm.b > func_out.b)
                        n_ones_pos++;
                     else
                        n_ones_neg++;
                  }
#endif
               }
            }
         }
      }
   }
#ifdef DEBUG_PRINT_STATS
   printf("n_ones_pos=%lld\n", n_ones_pos);
   printf("n_ones_neg=%lld\n", n_ones_neg);
#endif
   return 0;
}

int main()
{
   printf("*** main ***\n");
   main_test_sqrt();
   return 0;
}
