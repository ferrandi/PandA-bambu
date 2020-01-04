/* Copyright (C) 2017-2020 Politecnico di Milano (Italy).
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

/* In this file the single precision exp function is implemented
   following the HOTBM method published by
   Jeremie Detrey and Florent de Dinechin, "Parameterized floating-point logarithm and exponential functions for FPGAs", Microprocessors and Microsystems, vol.31,n.8, 2007, pp.537-545.
   The code has been exhaustively tested and it supports subnormals.
   @author Author: Andrea Pezzutti
   @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
   Created on May 18, 2017, 6:48 PM
*/

#include "bambu_macros.h"

#include <stdint.h>

//#define DEBUG_PRINT

#ifdef CHECK_EXP_FUNCTION
#include <gmp.h>
#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#define PRECISION 200

#endif

#ifdef CHECK_EXP_FUNCTION
#define ADD_BUILTIN_PREFIX(fname) local_##fname
#else
#define ADD_BUILTIN_PREFIX(fname) fname
#endif

typedef union {
   unsigned int b;
   float f;
} float_uint_converter;

#define WE 8  // Number of exponent bit
#define WF 23 // Number of mantissa bit
#define WX 28
#define G 5
#define P 7
#define CSTINVLOG2 0x2E3
#define CSTLOG2 0x058B90BFBF

static const unsigned int nExpY1[256] = {
    81407167,  81725786,  82045651,  82366769,  82689143,  83012780,  83337682,  83663857,  83991308,  84320041,  84650060,  84981371,  85313979,  85647888,  85983104,  86319633,  86657478,  86996646,  87337141,  87678969,  88022135,  88366644,
    88712501,  89059712,  89408282,  89758216,  90109520,  90462198,  90816258,  91171702,  91528538,  91886771,  92246405,  92607448,  92969903,  93333777,  93699075,  94065803,  94433966,  94803570,  95174621,  95547124,  95921085,  96296509,
    96673403,  97051772,  97431622,  97812958,  98195787,  98580115,  98965947,  99353288,  99742146,  100132526, 100524433, 100917875, 101312856, 101709384, 102107463, 102507100, 102908302, 103311073, 103715422, 104121352, 104528872, 104937986,
    105348702, 105761025, 106174962, 106590519, 107007702, 107426519, 107846974, 108269075, 108692829, 109118240, 109545317, 109974065, 110404492, 110836602, 111270405, 111705905, 112143109, 112582025, 113022659, 113465017, 113909106, 114354934,
    114802506, 115251831, 115702914, 116155762, 116610383, 117066783, 117524969, 117984949, 118446729, 118910317, 119375718, 119842942, 120311994, 120782882, 121255612, 121730194, 122206632, 122684935, 123165111, 123647165, 124131106, 124616942,
    125104679, 125594325, 126085887, 126579373, 127074791, 127572147, 128071451, 128572708, 129075928, 129581116, 130088283, 130597434, 131108578, 131621722, 132136875, 132654044, 133173237, 133694463, 134217728, 134743041, 135270411, 135799844,
    136331350, 136864935, 137400610, 137938380, 138478256, 139020245, 139564354, 140110594, 140658971, 141209495, 141762173, 142317015, 142874028, 143433221, 143994603, 144558182, 145123966, 145691965, 146262188, 146834642, 147409336, 147986280,
    148565482, 149146951, 149730695, 150316725, 150905048, 151495674, 152088611, 152683869, 153281457, 153881383, 154483658, 155088290, 155695288, 156304662, 156916422, 157530575, 158147132, 158766103, 159387496, 160011321, 160637587, 161266305,
    161897484, 162531132, 163167261, 163805880, 164446998, 165090625, 165736772, 166385447, 167036661, 167690424, 168346746, 169005637, 169667106, 170331164, 170997822, 171667088, 172338974, 173013490, 173690645, 174370451, 175052918, 175738056,
    176425875, 177116386, 177809600, 178505528, 179204178, 179905564, 180609694, 181316581, 182026234, 182738664, 183453883, 184171901, 184892730, 185616379, 186342861, 187072187, 187804366, 188539412, 189277334, 190018145, 190761855, 191508476,
    192258019, 193010495, 193765917, 194524295, 195285642, 196049968, 196817286, 197587607, 198360943, 199137306, 199916707, 200699159, 201484674, 202273262, 203064938, 203859711, 204657596, 205458603, 206262745, 207070035, 207880484, 208694105,
    209510911, 210330913, 211154125, 211980559, 212810227, 213643143, 214479319, 215318767, 216161501, 217007533, 217856876, 218709544, 219565549, 220424904};
static const unsigned short int fp_exp_exp_y2_t0[64] = {2,    6,    14,   26,   42,   62,   86,   114,  145,  181,  221,  265,  313,  365,  421,  481,  545,  613,  685,  761,  841,  925,  1013, 1105, 1201, 1301, 1405, 1513, 1625, 1741, 1861, 1985,
                                                        2113, 2245, 2381, 2522, 2666, 2814, 2966, 3122, 3282, 3446, 3614, 3786, 3962, 4142, 4326, 4514, 4707, 4903, 5103, 5307, 5515, 5727, 5943, 6163, 6388, 6616, 6848, 7084, 7324, 7569, 7817, 8069};
static const unsigned short int fp_exp_exp_y2_t1_t1[64] = {2,   6,   10,  14,  18,  22,  26,  30,  34,  38,  42,  46,  50,  54,  58,  62,  66,  70,  74,  78,  82,  86,  90,  94,  98,  102, 106, 110, 114, 118, 122, 126,
                                                           130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190, 194, 198, 202, 206, 210, 214, 218, 222, 226, 230, 234, 238, 242, 246, 250, 254};

__attribute__((always_inline)) static inline unsigned short int fp_exp_exp_t1(unsigned char a, unsigned char b)
{
   unsigned char b_0, s, a_1, s_1;
   unsigned short int k_1, r0_1, r1;
   _Bool sign, sign1;

   sign = (~SELECT_BIT(b, 5)) & 0x01;
   b_0 = (SELECT_RANGE(b, 4, 0) ^ (sign * 0x1F)) & 0x1F;
   s = SELECT_RANGE(b_0, 4, 0) | (1 << 5);
   a_1 = SELECT_RANGE(a, 5, 0);
   sign1 = (~SELECT_BIT(s, 5)) & 0x01;

   s_1 = SELECT_RANGE(s, 4, 0) ^ (sign1 * 0x1F);
   k_1 = fp_exp_exp_y2_t1_t1[a_1];

   r0_1 = k_1 * ((s_1 << 1) | 1);
   r1 = (SELECT_RANGE(r0_1, 14, 7) ^ ((sign ^ sign1) * 0xFF)) & 0xFF;
   r1 |= (((sign ^ sign1) * 0x3F) << 8);

#ifdef DEBUG_PRINT
   printf("sign: %x\n", sign);
   printf("b_0: %x\n", b_0);
   printf("s: %x\n", s);
   printf("sign1: %x\n", sign1);
   printf("s_1: %x\n", s_1);
   printf("k_1: %x\n", k_1);
   printf("r0_1: %x\n", r0_1);
   printf("r1: %x\n", r1);
#endif

   return SELECT_RANGE(r1, 13, 0);
}

__attribute__((always_inline)) static inline unsigned short int fp_exp_exp_y2(unsigned short int nY2)
{
   unsigned char a_0, a_1, b_1;
   unsigned short int r_0, r_1;
   unsigned short int sum;

   a_0 = SELECT_RANGE(nY2, 11, 6);
   a_1 = SELECT_RANGE(nY2, 11, 6);
   b_1 = SELECT_RANGE(nY2, 5, 0);
   r_0 = fp_exp_exp_y2_t0[a_0] & 0x3FFF;
   r_1 = fp_exp_exp_t1(a_1, b_1) & 0x3FFF;
   sum = r_0 + r_1;

#ifdef DEBUG_PRINT
   printf("a_0: %x\n", a_0);
   printf("a_1: %x\n", a_1);
   printf("b_1: %x\n", b_1);
#endif

   return SELECT_RANGE(sum, 13, 1);
}

__attribute__((always_inline)) static inline unsigned int fp_exp(unsigned long long int nX, _Bool sign)
{
   unsigned int nK0, nK1, nK, nEY1, nEY2, nZ0, nZ2, nZ, fR0, fR1, fR, eR;
   unsigned long long int nKLog20, nKLog2, nY, fpR, nZ1;
   unsigned char nY1;
   _Bool sticky, round;
   unsigned short int nY2;

   nK0 = SELECT_RANGE(nX, 34, 24) * CSTINVLOG2;
   nK1 = (SELECT_BIT(nX, 35) == 1) ? (nK0 - (CSTINVLOG2 << 0x0B)) & 0x3FFFFF : nK0 & 0x3FFFFF;
   nK = SELECT_RANGE(nK1, 21, 13) + SELECT_BIT(nK1, 12);
   nKLog20 = (((unsigned long long int)nK & 0xFF) * (unsigned long long int)CSTLOG2) & 0x7FFFFFFFFFF;
   nKLog2 = (SELECT_BIT(nK, 8) == 1) ? (nKLog20 - (CSTLOG2 << 8)) & 0xFFFFFFFFFFF : nKLog20 & 0xFFFFFFFFFFF;

   nY = (nX - SELECT_RANGE(nKLog2, 42, 7)) & 0xFFFFFFFFF;
   nY1 = (((~SELECT_BIT(nY, 27)) & 0x01) << 7) | SELECT_RANGE(nY, 26, 20);
   nEY1 = nExpY1[nY1];
   nEY2 = fp_exp_exp_y2(SELECT_RANGE(nY, 19, 8));

   nZ0 = (nEY2 + (SELECT_RANGE(nY, 19, 0) << 1)) & 0x3FFFFF;
   nZ1 = ((unsigned long long int)nEY1 * (unsigned long long int)nZ0) & 0x3FFFFFFFFFFFF;
   nZ2 = SELECT_RANGE(nZ1, 49, 29);
   nZ = (nEY1 + nZ2) & 0xFFFFFFF;

   sticky = (SELECT_RANGE(nZ, 1, 0) != 0x00) ? 1 : 0;

   fR0 = (SELECT_BIT(nZ, 27) == 1) ? (SELECT_RANGE(nZ, 26, 3) << 1) | (SELECT_BIT(nZ, 2) | sticky) : (SELECT_RANGE(nZ, 25, 2) << 1) | sticky;
   round = SELECT_BIT(fR0, 1) & (SELECT_BIT(fR0, 2) | SELECT_BIT(fR0, 0));
   fR1 = SELECT_RANGE(fR0, 24, 2) + round;
   fR = SELECT_RANGE(fR1, 22, 0);

   eR = nK + ((0x3F << 1) | (SELECT_BIT(nZ, 27) | SELECT_BIT(fR1, 23)));
#ifndef NO_SUBNORMALS
   fR = (eR <= 0x200 && sign) ? ((1 << 23) | fR) >> (0x201 - eR) : fR;
   eR = (eR < 0x200 && sign) ? 0 : eR;
#endif
   fpR = (SELECT_RANGE(eR, 7, 0) << 23) | fR;

#ifdef DEBUG_PRINT
   printf("nK0: %x\n", nK0);
   printf("nK1: %x\n", nK1);
   printf("nK: %x\n", nK);
   printf("nKLog20: %lx\n", nKLog20);
   printf("nKLog2: %lx\n", nKLog2);
   printf("nY: %lx\n", nY);
   printf("nY1: %x\n", nY1);
   printf("nEY1: %x\n", nEY1);
   printf("nEY2: %x\n", nEY2);
   printf("nZ0: %x\n", nZ0);
   printf("nZ1: %lx\n", nZ1);
   printf("nZ2: %x\n", nZ2);
   printf("nZ: %x\n", nZ);
   printf("sticky: %x\n", sticky);
   printf("fR0: %x\n", fR0);
   printf("round: %x\n", round);
   printf("fR1: %x\n", fR1);
   printf("fR: %x\n", fR);
   printf("eR: %x\n", eR);
   printf("fpR: %lx\n", fpR);
#endif

   return fpR;
}

float ADD_BUILTIN_PREFIX(expf)(float x)
{
   float_uint_converter fpX, result;
   unsigned int check;
   unsigned long long int mXu, mXs, nX;
   short int eX, e0;
   _Bool ufl0;
   unsigned int ffpX;
   unsigned char e;

   fpX.f = x;
   check = fpX.b & 0x7FFFFFFF;
   _Bool sign = SELECT_BIT(fpX.b, 31);

   if(check >= 0x42b17218)
   {
      if(check > 0x7f800000)
      {
         fpX.b |= (0x7FC << 20);
         return fpX.f;
      }
      if(!sign)
      {
         return __builtin_inff();
      }
      if(check == 0x7f800000)
         return 0;
      if(check >= 0x42cff1b5)
         return 0;
   }

   e = SELECT_RANGE(fpX.b, 30, 23);
   eX = (short int)e - (127 - WX);

   ufl0 = SELECT_BIT(eX, 9);
   if(ufl0)
      return 1.0f;

   mXu = SELECT_RANGE(fpX.b, 22, 0) | (1 << 23);
   mXu <<= eX;
   BIT_RESIZE(mXu, 58);
   mXu >>= 23;
   mXs = sign ? -mXu : mXu;
   nX = mXs;

#ifdef DEBUG_PRINT
   printf("eX:  %x\n", eX);
   printf("ufl0:  %x\n", ufl0);
   printf("ofl0:  %x\n", ofl0);
   printf("mXu: %x\n", mXu);
   printf("mXs: %x\n", mXs);
   printf("nX: %lx\n", nX);
#endif
   result.b = fp_exp(nX, sign);
   return result.f;
}

float __hide_ieee754_expf(float x)
{
   return ADD_BUILTIN_PREFIX(expf)(x);
}

#ifdef CHECK_EXP_FUNCTION

int test_print_bin(unsigned int n)
{
   unsigned int i;
   for(i = 1 << 31; i > 0; i = i / 2)
   {
      (n & i) ? printf("1") : printf("0");
   }
   printf("\n");
}

static float golden_reference_exp(float a)
{
   mpfr_t a_mpfr, res;
   float final_value;
   mpfr_init2(a_mpfr, PRECISION);
   mpfr_init2(res, PRECISION);
   mpfr_set_d(a_mpfr, a, MPFR_RNDN);
   mpfr_exp(res, a_mpfr, MPFR_RNDN);
   final_value = mpfr_get_flt(res, MPFR_RNDN);
   mpfr_clear(a_mpfr);
   mpfr_clear(res);
   return final_value;
}

int main_test_exp()
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
            float_uint_converter func_in, func_out, func_golden_libm;
            func_in.b = (s << 31) | (e << 23) | x;
            func_out.f = ADD_BUILTIN_PREFIX(expf)(func_in.f);
            func_golden_libm.f = expf(func_in.f);
            if((func_golden_libm.b >> 31) != (func_out.b >> 31))
            {
               float_uint_converter func_golden;
               func_golden.f = golden_reference_exp(func_in.f);
               printf("Opposite sign\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("exp golden=%.40f\n", func_golden.f);
               printf("golden=%x\n", func_golden.b);
               printf("my exp=%.20f\n", func_out.f);
               printf("binary=%x\n", func_out.b);
               printf("exp libm=%.20f\n", func_golden_libm.f);
               printf("libm=%x\n", func_golden_libm.b);
               abort();
            }
            if(abs(func_golden_libm.b - func_out.b) > 1)
            {
               float_uint_converter func_golden;
               func_golden.f = golden_reference_exp(func_in.f);
               printf("NO PASS\n");
               printf("s=%d\n", s);
               printf("e=%d\n", e);
               printf("x=%x\n", x);
               printf("exp golden=%.40f\n", func_golden.f);
               printf("golden=%x\n", func_golden.b);
               printf("my exp=%.20f\n", func_out.f);
               printf("binary=%x\n", func_out.b);
               printf("exp libm=%.20f\n", func_golden_libm.f);
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
   main_test_exp();
   return 0;
}
#endif
