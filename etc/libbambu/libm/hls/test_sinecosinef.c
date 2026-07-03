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

#include <gmp.h>
#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT
#define DEBUG_PRINT_STATS
#endif
#define CHECK_B_FRACT 0
#define TEST_PREFIX(fname) local_##fname
#include "sf_cos.c"
#include "sf_sin.c"
#include "sf_sincos.c"

#define PRECISION 30

static float golden_reference(float a)
{
   mpfr_t pi, const4, const8, const1;
   mpfr_t b_0, b_fract, b_integral;
   mpfr_t b_res;
   mpfr_t a4;
   mpfr_t b_modulus;
   float final_value;
   unsigned long octant;

   mpfr_init2(pi, PRECISION);
   mpfr_const_pi(pi, MPFR_RNDN);
   mpfr_init2(const4, PRECISION);
   mpfr_set_d(const4, 4.0, MPFR_RNDN);
   mpfr_init2(const8, PRECISION);
   mpfr_set_d(const8, 8.0, MPFR_RNDN);
   mpfr_init2(const1, PRECISION);
   mpfr_set_d(const1, 1.0, MPFR_RNDN);
   mpfr_init2(b_0, PRECISION);
   mpfr_init2(b_fract, PRECISION);
   mpfr_init2(b_integral, PRECISION);
   mpfr_init2(b_res, PRECISION);
   mpfr_init2(a4, PRECISION);
   mpfr_mul_d(a4, const4, a, MPFR_RNDN);
   mpfr_div(b_0, a4, pi, MPFR_RNDN);
   mpfr_init2(b_modulus, PRECISION);

#ifdef DEBUG_PRINT
   printf("b_0=");
   mpfr_out_str(stdout, 10, 0, b_0, MPFR_RNDN);
   printf("\n");
#endif
   mpfr_modf(b_integral, b_fract, b_0, MPFR_RNDN);
   mpfr_fmod(b_modulus, b_integral, const8, MPFR_RNDN);
   octant = mpfr_get_ui(b_modulus, MPFR_RNDN);
#ifdef DEBUG_PRINT
   printf("octant=%ld\n", octant);
#endif
#ifdef DEBUG_PRINT
   printf("b_fract-pre=");
   mpfr_out_str(stdout, 10, 0, b_fract, MPFR_RNDN);
   printf("\n");
#endif
   if(octant & 1)
      mpfr_sub(b_fract, const1, b_fract, MPFR_RNDN);

#if CHECK_B_FRACT
   current_golden_b_fract = mpfr_get_flt(b_fract, MPFR_RNDN);
#endif
#ifdef DEBUG_PRINT
   printf("b_integral=");
   mpfr_out_str(stdout, 10, 0, b_integral, MPFR_RNDN);
   printf("\n");
   printf("b_fract=");
   mpfr_out_str(stdout, 10, 0, b_fract, MPFR_RNDN);
   printf("\n");
#endif
   mpfr_mul(b_res, b_fract, pi, MPFR_RNDN);
   mpfr_div(b_res, b_res, const4, MPFR_RNDN);

#ifdef DEBUG_PRINT
   printf("reduced=");
   mpfr_out_str(stdout, 10, 0, b_res, MPFR_RNDN);
   printf("\n");
#endif
   final_value = mpfr_get_flt(b_res, MPFR_RNDN);
   mpfr_clear(pi);
   mpfr_clear(const4);
   mpfr_clear(const8);
   mpfr_clear(const1);
   mpfr_clear(b_0);
   mpfr_clear(b_fract);
   mpfr_clear(b_integral);
   mpfr_clear(b_res);
   mpfr_clear(a4);
   mpfr_clear(b_modulus);
   return final_value;
}

static float golden_reference_sin(float a)
{
   mpfr_t a_mpfr, res;
   float final_value;
   mpfr_init2(a_mpfr, PRECISION);
   mpfr_init2(res, PRECISION);
   mpfr_set_d(a_mpfr, a, MPFR_RNDN);
   mpfr_sin(res, a_mpfr, MPFR_RNDN);
   final_value = mpfr_get_flt(res, MPFR_RNDN);
   mpfr_clear(a_mpfr);
   mpfr_clear(res);
   return final_value;
}

static float golden_reference_cos(float a)
{
   mpfr_t a_mpfr, res;
   float final_value;
   mpfr_init2(a_mpfr, PRECISION);
   mpfr_init2(res, PRECISION);
   mpfr_set_d(a_mpfr, a, MPFR_RNDN);
   mpfr_cos(res, a_mpfr, MPFR_RNDN);
   final_value = mpfr_get_flt(res, MPFR_RNDN);
   mpfr_clear(a_mpfr);
   mpfr_clear(res);
   return final_value;
}

int main_test_range_redux()
{
   unsigned int e = 127;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;

#ifdef DEBUG_PRINT_STATS
   max_n_leading_zeros = 0;
   min_n_leading_zeros = 32;
   max_start_index = 0;
   max_offset_sum = 0;
   max_res_offset = 0;
#endif
#if !CHECK_B_FRACT
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
#endif
   for(e = 115; e < 255; ++e)
   {
      unsigned int x = 0; // x16cbe4;
#pragma omp critical
      printf("e=%d\n", e);
      for(x = 0; x < (1 << 23); ++x)
      {
         float_uint_converter func_in, func_out, func_golden;
         func_in.b = (e << 23) | x;
         unsigned int global_b_fract;
         unsigned char global_e_final;
         unsigned int global_mantissa;
         unsigned char n;
         unsigned long long int res_rr;
         res_rr = __bambu_range_redux(func_in.f, &global_b_fract, &global_e_final, &global_mantissa, &n);
         func_out.b = res_rr;
         float golden_ref;
         golden_ref = golden_reference(func_in.f);
         func_golden.f = golden_ref;
         if(
#if CHECK_B_FRACT
             current_b_fract != current_golden_b_fract ||
#endif
             abs(func_golden.b - func_out.b) > 1)
         {
#if CHECK_B_FRACT
            if(current_b_fract != current_golden_b_fract)
            {
               printf("NO PASS b_fract\n");
               printf("current_b_fract=%.20f\n", current_b_fract);
               printf("current_golden_b_fract=%.20f\n", current_golden_b_fract);
               {
                  float_uint_converter bfract_conv;
                  bfract_conv.f = current_b_fract;
                  printf("actual=%x\n", bfract_conv.b);
                  bfract_conv.f = current_golden_b_fract;
                  printf("golden=%x\n", bfract_conv.b);
               }
            }
#endif
#if !CHECK_B_FRACT
#pragma omp critical
#endif
            {
               printf("NO PASS\n");
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("quadrant=%llx\n", res_rr >> 32);
               printf("reduced=%.20f\n", func_out.f);
               printf("golden=%x\n", func_golden.b);
               printf("actual=%x\n", func_out.b);
#ifdef DEBUG_PRINT_STATS
               printf("max_n_leading_zeros=%d\n", max_n_leading_zeros);
               printf("min_n_leading_zeros=%d\n", min_n_leading_zeros);
               printf("max_start_index=%d\n", max_start_index);
               printf("max_offset_sum=%d\n", max_offset_sum);
               printf("max_res_offset=%d\n", max_res_offset);
#endif
               abort();
            }
         }
         else if(abs(func_golden.b - func_out.b) == 1)
         {
            if(func_golden.b > func_out.b)
               n_ones_pos++;
            else
               n_ones_neg++;
#ifdef DEBUG_PRINT
            printf("quadrant=%llx\n", res_rr >> 32);
            printf("reduced=%.20f\n", func_out.f);
            printf("golden=%x\n", func_golden.b);
            printf("actual=%x\n", func_out.b);
#endif

#if CHECK_B_FRACT
            if(current_b_fract != current_golden_b_fract)
            {
               printf("NO PASS b_fract\n");
               printf("current_b_fract=%.20f\n", current_b_fract);
               printf("current_golden_b_fract=%.20f\n", current_golden_b_fract);
               {
                  float_uint_converter bfract_conv;
                  bfract_conv.f = current_b_fract;
                  printf("actual=%x\n", bfract_conv.b);
                  bfract_conv.f = current_golden_b_fract;
                  printf("golden=%x\n", bfract_conv.b);
               }
            }
#endif
         }
      }
#ifdef DEBUG_PRINT_STATS
      printf("max_n_leading_zeros=%d\n", max_n_leading_zeros);
      printf("min_n_leading_zeros=%d\n", min_n_leading_zeros);
      printf("max_start_index=%d\n", max_start_index);
      printf("max_offset_sum=%d\n", max_offset_sum);
      printf("max_res_offset=%d\n", max_res_offset);
#endif
   }
#ifdef DEBUG_PRINT_STATS
   printf("max_n_leading_zeros=%d\n", max_n_leading_zeros);
   printf("min_n_leading_zeros=%d\n", min_n_leading_zeros);
   printf("max_start_index=%d\n", max_start_index);
   printf("max_offset_sum=%d\n", max_offset_sum);
   printf("max_res_offset=%d\n", max_res_offset);
#endif
   printf("n_ones_pos=%d\n", n_ones_pos);
   printf("n_ones_neg=%d\n", n_ones_neg);
   return 0;
}

int main_test_sine()
{
   unsigned int s = 0;
   unsigned int e = 127;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;
   for(s = 0; s < 2; ++s)
   {
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
      for(e = 0; e < 256; ++e)
      {
         unsigned int x = 0x0;
#pragma omp critical
         printf("e=%d\n", e);
         for(x = 0; x < (1 << 23); ++x)
         {
            float_uint_converter func_in, func_out_sine, func_golden, func_golden_sine_libm;
            func_in.b = (s << 31) | (e << 23) | x;
            func_golden_sine_libm.f = sinf(func_in.f);
            func_out_sine.f = TEST_PREFIX(sinf)(func_in.f);
            if((func_golden_sine_libm.b >> 31) != (func_out_sine.b >> 31))
            {
               float_uint_converter func_golden_sine;
               func_golden_sine.f = golden_reference_sin(func_in.f);
               printf("Opposite sign\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sine golden=%.40f\n", func_golden_sine.f);
               printf("golden=%x\n", func_golden_sine.b);
               printf("sine my=%.40f\n", func_out_sine.f);
               printf("binary=%x\n", func_out_sine.b);
               printf("libm sine=%.40f\n", func_golden_sine_libm.f);
               printf("sine=%x\n", func_golden_sine_libm.b);
               abort();
            }
            if(abs(func_golden_sine_libm.b - func_out_sine.b) > 1)
            {
               float_uint_converter func_golden_sine;
               func_golden_sine.f = golden_reference_sin(func_in.f);
               printf("SINE NO PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sine golden=%.40f\n", func_golden_sine.f);
               printf("golden=%x\n", func_golden_sine.b);
               printf("sine my=%.40f\n", func_out_sine.f);
               printf("binary=%x\n", func_out_sine.b);
               printf("libm sine=%.40f\n", func_golden_sine_libm.f);
               printf("sine=%x\n", func_golden_sine_libm.b);
               abort();
            }
            else if(abs(func_golden_sine_libm.b - func_out_sine.b) == 1)
            {
               if(func_golden_sine_libm.b > func_out_sine.b)
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

int main_test_cosine()
{
   unsigned int s = 0;
   unsigned int e = 126;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;
   for(s = 0; s < 2; ++s)
   {
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
      for(e = 0; e < 256; ++e)
      {
         unsigned int x = 0x0;
#pragma omp critical
         printf("e=%d\n", e);
         for(x = 0; x < (1 << 23); ++x)
         {
            float_uint_converter func_in, func_out_cosine, func_golden_cosine_libm;
            func_in.b = (s << 31) | (e << 23) | x;
            func_golden_cosine_libm.f = cosf(func_in.f);
            func_out_cosine.f = TEST_PREFIX(cosf)(func_in.f);
            if((func_golden_cosine_libm.b >> 31) != (func_out_cosine.b >> 31))
            {
               float_uint_converter func_golden_cosine;
               func_golden_cosine.f = golden_reference_cos(func_in.f);
               printf("COSINE NO PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("cosine golden=%.40f\n", func_golden_cosine.f);
               printf("golden=%x\n", func_golden_cosine.b);
               printf("cosine my=%.40f\n", func_out_cosine.f);
               printf("binary=%x\n", func_out_cosine.b);
               printf("libm cosine=%.40f\n", func_golden_cosine_libm.f);
               printf("cosine=%x\n", func_golden_cosine_libm.b);
               abort();
            }
            if(abs(func_golden_cosine_libm.b - func_out_cosine.b) > 1)
            {
               float_uint_converter func_golden_cosine;
               func_golden_cosine.f = golden_reference_cos(func_in.f);
               printf("COSINE NO PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("cosine golden=%.40f\n", func_golden_cosine.f);
               printf("golden=%x\n", func_golden_cosine.b);
               printf("cosine my=%.40f\n", func_out_cosine.f);
               printf("binary=%x\n", func_out_cosine.b);
               printf("libm cosine=%.40f\n", func_golden_cosine_libm.f);
               printf("cosine=%x\n", func_golden_cosine_libm.b);
               abort();
            }
            else if(abs(func_golden_cosine_libm.b - func_out_cosine.b) == 1)
            {
               if(func_golden_cosine_libm.b > func_out_cosine.b)
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

int main_test_sine_and_cosine()
{
   unsigned int s = 0;
   unsigned int e = 126;
   unsigned int n_ones_pos = 0;
   unsigned int n_ones_neg = 0;
   for(s = 0; s < 2; ++s)
   {
#pragma omp parallel for reduction(+ : n_ones_pos, n_ones_neg) schedule(dynamic)
      for(e = 0; e < 256; ++e)
      {
         unsigned int x = 0x0;
#pragma omp critical
         printf("e=%d\n", e);
         for(x = 0; x < (1 << 23); ++x)
         {
            float_uint_converter func_in, func_out_sine, func_out_cosine, func_golden_sine_libm,
                func_golden_cosine_libm;
            func_in.b = (s << 31) | (e << 23) | x;
            func_golden_cosine_libm.f = cosf(func_in.f);
            func_golden_sine_libm.f = sinf(func_in.f);
            TEST_PREFIX(sincosf)(func_in.f, &func_out_sine.f, &func_out_cosine.f);
            if(((func_golden_sine_libm.b >> 31) != (func_out_sine.b >> 31)) ||
               ((func_golden_cosine_libm.b >> 31) != (func_out_cosine.b >> 31)))
            {
               float_uint_converter func_golden_cosine, func_golden_sine;
               func_golden_cosine.f = golden_reference_cos(func_in.f);
               func_golden_sine.f = golden_reference_sin(func_in.f);
               printf("Opposite sign\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sine golden=%.40f\n", func_golden_sine.f);
               printf("golden=%x\n", func_golden_sine.b);
               printf("cosine golden=%.40f\n", func_golden_cosine.f);
               printf("golden=%x\n", func_golden_cosine.b);
               printf("sine my=%.40f\n", func_out_sine.f);
               printf("binary=%x\n", func_out_sine.b);
               printf("cosine my=%.40f\n", func_out_cosine.f);
               printf("binary=%x\n", func_out_cosine.b);
               printf("libm sine=%.40f\n", func_golden_sine_libm.f);
               printf("sine=%x\n", func_golden_sine_libm.b);
               printf("libm cosine=%.40f\n", func_golden_cosine_libm.f);
               printf("cosine=%x\n", func_golden_cosine_libm.b);
               abort();
            }
            if((abs(func_golden_sine_libm.b - func_out_sine.b) > 1) ||
               (abs(func_golden_cosine_libm.b - func_out_cosine.b) > 1))
            {
               float_uint_converter func_golden_cosine, func_golden_sine;
               func_golden_cosine.f = golden_reference_cos(func_in.f);
               func_golden_sine.f = golden_reference_sin(func_in.f);
               printf("SINE AND COSINE DO NOT PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("sine golden=%.40f\n", func_golden_sine.f);
               printf("golden=%x\n", func_golden_sine.b);
               printf("cosine golden=%.40f\n", func_golden_cosine.f);
               printf("golden=%x\n", func_golden_cosine.b);
               printf("sine my=%.40f\n", func_out_sine.f);
               printf("binary=%x\n", func_out_sine.b);
               printf("cosine my=%.40f\n", func_out_cosine.f);
               printf("binary=%x\n", func_out_cosine.b);
               printf("libm sine=%.40f\n", func_golden_sine_libm.f);
               printf("sine=%x\n", func_golden_sine_libm.b);
               printf("libm cosine=%.40f\n", func_golden_cosine_libm.f);
               printf("cosine=%x\n", func_golden_cosine_libm.b);
               abort();
            }
            else if(abs(func_golden_sine_libm.b - func_out_sine.b) == 1)
            {
               if(func_golden_sine_libm.b > func_out_sine.b)
                  n_ones_pos++;
               else
                  n_ones_neg++;
            }
            else if(abs(func_golden_cosine_libm.b - func_out_cosine.b) == 1)
            {
               if(func_golden_cosine_libm.b > func_out_cosine.b)
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
   printf("Testing sine\n");
   main_test_sine();
   printf("Testing cosine\n");
   main_test_cosine();
   printf("Testing sine+cosine\n");
   main_test_sine_and_cosine();
#ifdef TEST_RANGE_REDUX
   printf("Testing range reduction\n");
   main_test_range_redux();
#endif
   return 0;
}
