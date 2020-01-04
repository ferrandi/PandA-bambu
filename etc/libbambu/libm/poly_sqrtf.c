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

/* In this file the single precision sqrt function is implemented
   following the method published by:
   Florent de Dinechin, Mioara Joldes, Bogdan Pasca, Guillaume Revy: Multiplicative Square Root Algorithms for FPGAs. FPL 2010: 574-577
   The code has been exhaustively tested and it does support subnormals.
   @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*/
#include "bambu_macros.h"

#ifdef CHECK_SQRT_FUNCTION
#include <gmp.h>
#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#define PRECISION 200
#endif

#ifdef CHECK_SQRT_FUNCTION
#define ADD_BUILTIN_PREFIX(fname) local_##fname
#else
#define ADD_BUILTIN_PREFIX(fname) fname
#endif

typedef union {
   unsigned int b;
   float f;
} float_uint_converter;

float ADD_BUILTIN_PREFIX(sqrtf)(float x)
{
   static const unsigned int a0[] = {4,        261638,   522260,   781881,   1040514,  1298168,  1554856,  1810587,  2065374,  2319225,  2572151,  2824163,  3075270,  3325482,  3574807,  3823257,  4070839,  4317563,  4563438,  4808472,  5052675,  5296053,
                                     5538616,  5780372,  6021329,  6261495,  6500876,  6739482,  6977320,  7214396,  7450719,  7686295,  7921131,  8155234,  8388612,  8621270,  8853216,  9084456,  9314996,  9544843,  9774002,  10002481, 10230284, 10457419,
                                     10683890, 10909704, 11134867, 11359383, 11583258, 11806499, 12029110, 12251096, 12472463, 12693216, 12913360, 13132900, 13351841, 13570188, 13787946, 14005119, 14221712, 14437730, 14653177, 14868058, 15082377, 15296139,
                                     15509347, 15722007, 15934122, 16145697, 16356736, 16567242, 16777220, 16986674, 17195607, 17404024, 17611928, 17819323, 18026213, 18232601, 18438492, 18643888, 18848793, 19053211, 19257145, 19460599, 19663576, 19866079,
                                     20068112, 20269677, 20470779, 20671420, 20871603, 21071332, 21270609, 21469439, 21667822, 21865764, 22063266, 22260332, 22456964, 22653165, 22848939, 23044287, 23239213, 23433719, 23627809, 23821484, 24014747, 24207602,
                                     24400050, 24592094, 24783737, 24974980, 25165828, 25356282, 25546344, 25736017, 25925303, 26114205, 26302725, 26490866, 26678629, 26866016, 27053031, 27239675, 27425951, 27611860, 27797405, 28167411, 28535987, 28903148,
                                     29268908, 29633286, 29996298, 30357956, 30718279, 31077279, 31434971, 31791369, 32146489, 32500341, 32852941, 33204301, 33554436, 33903356, 34251075, 34597608, 34942961, 35287150, 35630186, 35972081, 36312846, 36652491,
                                     36991027, 37328468, 37664821, 38000096, 38334307, 38667462, 38999571, 39330644, 39660689, 39989718, 40317738, 40644761, 40970794, 41295846, 41619927, 41943043, 42265207, 42586423, 42906703, 43226051, 43544480, 43861993,
                                     44178602, 44494310, 44809130, 45123066, 45436126, 45748318, 46059649, 46370126, 46679756, 46988544, 47296501, 47603630, 47909938, 48215434, 48520121, 48824010, 49127102, 49429406, 49730929, 50031676, 50331652, 50630865,
                                     50929318, 51227019, 51523972, 51820184, 52115662, 52410407, 52704428, 52997728, 53290315, 53582193, 53873366, 54163839, 54453618, 54742709, 55031116, 55318843, 55605895, 55892279, 56177996, 56463053, 56747453, 57031203,
                                     57314304, 57596764, 57878584, 58159772, 58440328, 58720259, 58999571, 59278263, 59556342, 59833814, 60110679, 60386943, 60662611, 60937684, 61212169, 61486065, 61759382, 62032118, 62304281, 62575872, 62846896, 63117356,
                                     63387254, 63656598, 63925385, 64193624, 64461315, 64728463, 64995070, 65261142, 65526678, 65791684, 66056163, 66320119, 66583552, 66846468};

   static const unsigned int a1[] = {131071, 130562, 130059, 129563, 129070, 128584, 128103, 127629, 127157, 126693, 126234, 125779, 125329, 124883, 124443, 124007, 123576, 123149, 122727, 122309, 121893, 121485, 121080, 120678, 120279, 119885,
                                     119497, 119111, 118727, 118349, 117973, 117601, 117234, 116870, 116509, 116151, 115796, 115444, 115096, 114750, 114409, 114070, 113734, 113401, 113071, 112744, 112418, 112097, 111779, 111462, 111148, 110838,
                                     110530, 110224, 109921, 109620, 109322, 109026, 108732, 108441, 108152, 107866, 107581, 107299, 107019, 106742, 106467, 106194, 105923, 105653, 105385, 105121, 104857, 104596, 104337, 104079, 103824, 103571,
                                     103319, 103070, 102821, 102575, 102331, 102088, 101847, 101607, 101369, 101133, 100898, 100666, 100434, 100205, 99977,  99751,  99527,  99302,  99082,  98861,  98642,  98424,  98208,  97994,  97780,  97569,
                                     97358,  97149,  96940,  96734,  96530,  96325,  96122,  95921,  95720,  95523,  95325,  95128,  94933,  94740,  94548,  94356,  94165,  93975,  93787,  93601,  93415,  93231,  93047,  92864,  92682,  92322,
                                     91966,  91613,  91266,  90924,  90583,  90248,  89915,  89586,  89260,  88939,  88620,  88306,  87995,  87686,  87381,  87080,  86781,  86484,  86192,  85903,  85615,  85332,  85050,  84772,  84497,  84223,
                                     83952,  83686,  83420,  83158,  82897,  82639,  82384,  82130,  81880,  81631,  81385,  81141,  80899,  80660,  80422,  80187,  79953,  79722,  79492,  79265,  79039,  78816,  78594,  78374,  78157,  77940,
                                     77726,  77513,  77301,  77093,  76884,  76679,  76475,  76272,  76072,  75871,  75675,  75478,  75283,  75090,  74898,  74707,  74519,  74331,  74146,  73961,  73776,  73595,  73414,  73235,  73058,  72880,
                                     72705,  72531,  72359,  72187,  72016,  71848,  71679,  71512,  71346,  71181,  71019,  70856,  70694,  70534,  70376,  70217,  70061,  69906,  69750,  69597,  69444,  69292,  69141,  68991,  68842,  68694,
                                     68546,  68402,  68256,  68113,  67969,  67827,  67686,  67544,  67406,  67265,  67129,  66991,  66855,  66719,  66585,  66450,  66318,  66186,  66054,  65923,  65793,  65664};

   static const unsigned short int a2[] = {508, 502, 497, 494, 486, 480, 475, 471, 463, 460, 456, 451, 447, 441, 436, 432, 428, 423, 420, 416, 408, 407, 404, 399, 392, 389, 388, 384, 378, 375, 370, 366, 365, 362, 360, 356, 352,
                                           348, 345, 341, 339, 337, 333, 331, 328, 326, 320, 319, 317, 313, 310, 309, 307, 304, 302, 299, 297, 294, 291, 289, 286, 285, 281, 279, 276, 276, 274, 273, 271, 267, 264, 264, 260, 259,
                                           257, 254, 253, 252, 250, 249, 246, 245, 244, 242, 240, 237, 235, 233, 231, 230, 227, 227, 225, 225, 225, 220, 222, 220, 218, 216, 215, 214, 212, 212, 210, 208, 205, 205, 205, 202, 200,
                                           199, 196, 198, 196, 194, 193, 194, 194, 192, 189, 187, 187, 187, 186, 186, 185, 182, 180, 178, 176, 173, 171, 171, 168, 167, 165, 163, 160, 159, 157, 156, 155, 152, 151, 150, 148, 146,
                                           145, 144, 141, 141, 139, 138, 137, 135, 133, 133, 131, 131, 129, 128, 127, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 109, 107, 107, 106, 104,
                                           104, 102, 102, 101, 100, 100, 98,  99,  97,  96,  96,  95,  94,  94,  93,  93,  92,  90,  90,  89,  88,  89,  87,  87,  86,  86,  85,  84,  85,  83,  83,  82,  81,  82,  81,  79,  79,
                                           79,  78,  78,  78,  77,  77,  76,  76,  75,  74,  74,  73,  72,  73,  72,  72,  71,  71,  71,  69,  70,  68,  69,  68,  68,  67,  67,  66,  66,  66,  65,  65,  64,  64};

   _Bool s;
   unsigned short e;
   unsigned int m;
   float_uint_converter func_in;
   unsigned int fpX;
   func_in.f = x;
   fpX = func_in.b;

   s = (fpX >> 31) & 1;
   e = SELECT_RANGE(fpX, 30, 23);
   m = SELECT_RANGE(fpX, 22, 0);
   float_uint_converter res_fp;

#ifndef NO_SUBNORMALS
   if((fpX & 0x7fffffff) == 0)
#else
   if(e == 0)
#endif
   {
      res_fp.b = ((unsigned int)(s)) << 31;
      return res_fp.f;
   }
   if(e == 0xFF)
   {
      if(m | s)
         func_in.b |= (0x7FC << 20);
      return func_in.f;
   }
   if(s)
   {
      res_fp.b = 0x7FC00000 | (s << 31);
      return res_fp.f;
   }

   _Bool isOddExp = !SELECT_BIT(e, 0);
#ifndef NO_SUBNORMALS
   if(e == 0)
   {
      unsigned int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(23, m, subnormal_lz, mshifted);
      e = -subnormal_lz;
      BIT_RESIZE(e, 9);
      isOddExp = !SELECT_BIT(e, 0);
      m = SELECT_RANGE(mshifted, 21, 0) << 1;
   }
#endif
   unsigned char zExp = VAL_RESIZE(e + 0x7E + SELECT_BIT(e, 0), 9) >> 1;
   unsigned char coef_sqrt_row = (((unsigned int)(isOddExp)) << 7) | SELECT_RANGE(m, 22, 16);
   unsigned int lowMantissa = VAL_RESIZE(((SELECT_RANGE(m, 15, 0)) << isOddExp), 17);
   unsigned short int poly_a2 = VAL_RESIZE(((lowMantissa * a2[coef_sqrt_row]) >> 17), 9);
   unsigned int poly_a1 = a1[coef_sqrt_row] - poly_a2;
   VAL_RESIZE(poly_a1, 17);
   unsigned int poly_a0 = VAL_RESIZE(((((unsigned long long)lowMantissa) * ((unsigned long long)poly_a1)) >> 15), 19);
   unsigned int zFrac = VAL_RESIZE(((a0[coef_sqrt_row] + poly_a0) >> 3), 24);
   zFrac = SELECT_BIT(zFrac, 23) ? 0x7FFFFF : zFrac;

   res_fp.b = (((unsigned int)(s)) << 31) | (zExp << 23) | zFrac;

   return res_fp.f;
}

float __hide_ieee754_sqrtf(float x)
{
   return ADD_BUILTIN_PREFIX(sqrtf)(x);
}

#ifdef CHECK_SQRT_FUNCTION
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
            func_out.f = ADD_BUILTIN_PREFIX(sqrtf)(func_in.f);
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
#endif
