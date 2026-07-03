//    Copyright (C) 2017-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libm_hls_ext IP Library.
//
//    author Serena Curzel <serena.curzel@polimi.it>
//    author Iris Cusini
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WE 8
#define WF 23
#define WX 26
#define G 3
#define P 7

#define TEST_PREFIX(fname) local##fname
#include "ef_pow.c"

void test_print_bin_long(unsigned long long int n)
{
   unsigned long long int i;
   for(i = 1ULL << 63; i > 0; i = i / 2)
   {
      (n & i) ? printf("1") : printf("0");
   }
   printf("\n");
}
void test_print_bin_short(unsigned short int n)
{
   unsigned short int i;
   for(i = 1 << 15; i > 0; i = i / 2)
   {
      (n & i) ? printf("1") : printf("0");
   }
   printf("\n");
}
void test_print_bin(unsigned int n)
{
   unsigned int i;
   for(i = 1 << 31; i > 0; i = i / 2)
   {
      (n & i) ? printf("1") : printf("0");
   }
   printf("\n");
}

int main_test_pow(unsigned int eyMin, unsigned int eyMax, unsigned int myMin, unsigned int myMax, unsigned int exMin,
                  unsigned int exMax, unsigned int mxMin, unsigned int mxMax)
{
   unsigned int sy;
   unsigned int ey;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;

   for(sy = 0; sy < 2; ++sy)
   {
      for(ey = eyMin; ey < eyMax; ++ey)
      {
         unsigned int my;
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
         for(my = myMin; my < myMax; ++my)
         {
            unsigned int sx;
            float_uint_converter func_in_y;
            func_in_y.b = (sy << 31) | (ey << 23) | my;
            for(sx = 0; sx < 2; ++sx)
            {
               unsigned int ex;
               for(ex = exMin; ex < exMax; ++ex)
               {
                  unsigned int mx;
                  for(mx = mxMin; mx < mxMax; ++mx)
                  {
                     float_uint_converter func_in_x;
                     float_uint_converter func_golden_libm;
                     float_uint_converter func_out;
                     func_in_x.b = (sx << 31) | (ex << 23) | mx;
                     func_out.f = local__ieee754_powf(func_in_x.f, func_in_y.f);
                     func_golden_libm.f = powf(func_in_x.f, func_in_y.f);

                     if((func_golden_libm.b >> 31) != (func_out.b >> 31))
                     {
                        printf("Opposite sign\n");
                        printf("sy=%d\n", sy);
                        printf("ey=%d\n", ey);
                        printf("my=%x\n", my);
                        printf("sx=%d\n", sx);
                        printf("ex=%d\n", ex);
                        printf("mx=%x\n", mx);
                        printf("my pow=%.20f\n", func_out.f);
                        printf("binary=%x\n", func_out.b);
                        printf("pow libm=%.20f\n", func_golden_libm.f);
                        printf("libm=%x\n", func_golden_libm.b);
                        printf("y=");
                        test_print_bin(func_in_y.b);
                        printf("x=");
                        test_print_bin(func_in_x.b);

                        abort();
                     }
                     if(abs((int)func_golden_libm.b - (int)func_out.b) > 1)
                     {
                        printf("NO PASS\n");
                        printf("sy=%d\n", sy);
                        printf("ey=%d\n", ey);
                        printf("my=%x\n", my);
                        printf("y=");
                        test_print_bin(func_in_y.b);
                        printf("sx=%d\n", sx);
                        printf("ex=%d\n", ex);
                        printf("mx=%x\n", mx);
                        printf("x=");
                        test_print_bin(func_in_x.b);
                        printf("my pow=%.20f\n", func_out.f);
                        printf("binary=%x\n", func_out.b);
                        printf("pow libm=%.20f\n", func_golden_libm.f);
                        printf("libm=%x\n", func_golden_libm.b);
                        abort();
                     }
                     else if(abs((int)func_golden_libm.b - (int)func_out.b) == 1)
                     {
                        if(func_golden_libm.b > func_out.b)
                           n_ones_pos++;
                        else
                           n_ones_neg++;
                     }
                  }
               }
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
   unsigned int test = 0;

   main_test_pow(127, 128, 2097152, 2097153, 126, 127, 4194304, 4194305);
   ++test;
   printf("*** Test %d passed ***\n", test);

   main_test_pow(125, 129, 0, (1 << 3), 125, 129, 0, (1 << 23));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(125, 129, 0, (1 << 23), 125, 129, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(254, 256, 0, (1 << 3), 0, 1, 0, 1);
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(254, 256, 0, (1 << 3), 254, 256, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(254, 256, 0, (1 << 3), 1, 2, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(0, 1, 0, 1, 255, 256, 4194304, 4194304 + (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(254, 255, 0, (1 << 3), 254, 256, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(1, 2, 0, (1 << 3), 254, 256, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   main_test_pow(1, 2, 0, (1 << 3), 1, 2, 0, (1 << 3));
   ++test;
   printf("*** Test %d passed ***\n", test);
   return 0;
}
