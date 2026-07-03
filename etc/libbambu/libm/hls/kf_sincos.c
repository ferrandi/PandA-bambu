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
#ifndef _K_SINCOS_HOTBM_C
#define _K_SINCOS_HOTBM_C
#include "hlsmath.h"
#include <bambu_config.h>
#include <bambu_macros.h>

#define CE_MACRO32(cond, a, b) \
   ((((unsigned int)((((int)(cond)) << 31) >> 31)) & (a)) | ((~((unsigned int)((((int)(cond)) << 31) >> 31))) & (b)))

#if CHECK_B_FRACT

float current_b_fract;
float current_golden_b_fract;

#endif

#ifdef DEBUG_PRINT_STATS
unsigned int max_n_leading_zeros;
unsigned int min_n_leading_zeros;
unsigned int max_start_index;
unsigned int max_offset_sum;
unsigned int max_res_offset;
#endif

static __FORCE_INLINE unsigned int __shift_left_two_words(unsigned int word1, unsigned int word2,
                                                          unsigned char lshift_value)
{
   BIT_RESIZE(lshift_value, 5);
   unsigned long long composed_ll = (((unsigned long long int)word1) << 32) | word2;
   composed_ll = lshift_value == 0 ? composed_ll : composed_ll << lshift_value;
   return composed_ll >> 32;
}

static __FORCE_INLINE unsigned int __bambu_range_redux(float x_par, unsigned int* global_b_fract,
                                                       unsigned char* global_e_final, unsigned int* global_mantissa,
                                                       unsigned char* octant)
{
   // constants for 1/pi
   static const unsigned int two_over_pi0 = 0x28BE60D;  // 00000010100010111110011000001101
   static const unsigned int two_over_pi1 = 0xB9391054; // 10111001001110010001000001010100
   static const unsigned int two_over_pi2 = 0xA7F09D5F; // 10100111111100001001110101011111
   static const unsigned int two_over_pi3 = 0x47D4D377; // 01000111110101001101001101110111
   static const unsigned int two_over_pi4 = 0x36D8A56;  // 00000011011011011000101001010110
   static const unsigned int two_over_pi5 = 0x64F10E41; // 01100100111100010000111001000001
   static const unsigned int two_over_pi6 = 0x7F9458E;  // 00000111111110010100010110001110
   unsigned char s, s_final;
   unsigned int x, x_final;
   unsigned char e_final, n_leading_zeros;
   short int e;
   unsigned int p = 23;
   unsigned int Middle_selection_0, Middle_selection_1, Middle_selection_2, Middle_selection_3, Middle_selection_4;
   unsigned long long int b_4, b_3, b_2, b_1, b_0, b_right;
   unsigned int b_fract, b_fract_low, res;
   unsigned long long int final_value;
   unsigned int b_right_msb, b_0_lsb;
   unsigned int two_over_pi_0, two_over_pi_1, two_over_pi_2, two_over_pi_3;
   unsigned char start_index, offset, res_offset;
   bool LSB_bit, Guard_bit, Round_bit, Sticky_bit, round;
   unsigned int ExpSig, rounded;

   float_uint_converter func_in;
   func_in.f = x_par;
   x = func_in.b;
   e = (x >> 23) & 255;
   e -= 115;
   s = x >> 31;
   s_final = s;
#ifdef DEBUG_PRINT
   printf("x=%x\n", x);
#endif
   x = (x & ((1 << 23) - 1)) | (1 << 23);
#ifdef DEBUG_PRINT
   printf("x=%x\n", x);
   printf("e=%x\n", e);
   printf("s=%x\n", s);
#endif
   start_index = e < p + 9 ? 0 : (e - p - 8) / 32;
   BIT_RESIZE(start_index, 2);
#ifdef DEBUG_PRINT
   printf("start_index=%u\n", start_index);
#endif
   if(start_index == 0)
   {
      two_over_pi_0 = two_over_pi0;
      two_over_pi_1 = two_over_pi1;
      two_over_pi_2 = two_over_pi2;
      two_over_pi_3 = two_over_pi3;
   }
   else if(start_index == 1)
   {
      two_over_pi_0 = two_over_pi1;
      two_over_pi_1 = two_over_pi2;
      two_over_pi_2 = two_over_pi3;
      two_over_pi_3 = two_over_pi4;
   }
   else if(start_index == 2)
   {
      two_over_pi_0 = two_over_pi2;
      two_over_pi_1 = two_over_pi3;
      two_over_pi_2 = two_over_pi4;
      two_over_pi_3 = two_over_pi5;
   }
   else
   {
      two_over_pi_0 = two_over_pi3;
      two_over_pi_1 = two_over_pi4;
      two_over_pi_2 = two_over_pi5;
      two_over_pi_3 = two_over_pi6;
   }

   /// normalize with respect to the offset value
   offset = e < p + 9 ? 0 : (e - p - 8) % 32;
   BIT_RESIZE(offset, 5);
#ifdef DEBUG_PRINT
   printf("offset=%u\n", offset);
#endif
   two_over_pi_0 = __shift_left_two_words(two_over_pi_0, two_over_pi_1, offset);
   two_over_pi_1 = __shift_left_two_words(two_over_pi_1, two_over_pi_2, offset);
   two_over_pi_2 = __shift_left_two_words(two_over_pi_2, two_over_pi_3, offset);
#ifdef DEBUG_PRINT
   printf("two_over_pi_0=%x\n", two_over_pi_0);
   printf("two_over_pi_1=%x\n", two_over_pi_1);
   printf("two_over_pi_2=%x\n", two_over_pi_2);
#endif

   /// decompose in five chunks of 18bits
   Middle_selection_0 = two_over_pi_0 >> 14;
   Middle_selection_1 = ((two_over_pi_0 & ((1 << 14) - 1)) << 4) | (two_over_pi_1 >> 28);
   Middle_selection_2 = ((two_over_pi_1 & ((1 << 28) - 1)) >> 10);
   Middle_selection_3 = ((two_over_pi_1 & ((1 << 10) - 1)) << 8) | (two_over_pi_2 >> 24);
   Middle_selection_4 = ((two_over_pi_2 & ((1 << 24) - 1)) >> 6);
#ifdef DEBUG_PRINT
   printf("Middle_selection_0=%x\n", Middle_selection_0);
   printf("Middle_selection_1=%x\n", Middle_selection_1);
   printf("Middle_selection_2=%x\n", Middle_selection_2);
   printf("Middle_selection_3=%x\n", Middle_selection_3);
   printf("Middle_selection_4=%x\n", Middle_selection_4);
#endif

   x = x & ((1 << 24) - 1);

   Middle_selection_4 = Middle_selection_4 & ((1 << 18) - 1);
   b_4 = ((unsigned long long)Middle_selection_4 * (unsigned long long)x);
   b_right = (b_4 & ((1 << 18) - 1)) >> 8;
   b_4 = b_4 >> 18;

   Middle_selection_3 = Middle_selection_3 & ((1 << 18) - 1);
   b_3 = ((unsigned long long)Middle_selection_3 * (unsigned long long)x) + b_4;
   b_right |= (b_3 & ((1 << 18) - 1)) << 10;
   b_3 = b_3 >> 18;

   Middle_selection_2 = Middle_selection_2 & ((1 << 18) - 1);
   b_2 = ((unsigned long long)Middle_selection_2 * (unsigned long long)x) + b_3;
   b_right |= (b_2 & ((1 << 18) - 1)) << 28;
   b_2 = b_2 >> 18;

   Middle_selection_1 = Middle_selection_1 & ((1 << 18) - 1);
   b_1 = ((unsigned long long)Middle_selection_1 * (unsigned long long)x) + b_2;
   b_right |= (b_1 & ((1 << 18) - 1)) << 46;
   b_1 = b_1 >> 18;

   Middle_selection_0 = Middle_selection_0 & ((1 << 18) - 1);
   b_0 = ((unsigned long long)Middle_selection_0 * (unsigned long long)x) + b_1;

   res_offset = e - (start_index * 32) - offset;
#ifdef DEBUG_PRINT_STATS
   max_offset_sum = max_offset_sum < offset + res_offset ? offset + res_offset : max_offset_sum;
   max_res_offset = max_res_offset < res_offset ? res_offset : max_res_offset;
#endif

   b_right = ((b_0 & ((1ULL << 17) - 1)) << (64 - 17)) | (b_right >> 17);
   b_0 = b_0 >> 17;

   res_offset &= 31;
#ifdef DEBUG_PRINT
   printf("res_offset=%u\n", res_offset);
#endif

#ifdef DEBUG_PRINT
   printf("b_right=%llx\n", b_right);
   printf("pre-res_offset_shift-b_0=%llx\n", b_0);
#endif
   b_0_lsb = __shift_left_two_words((b_0 & ((1ULL << 32) - 1)), (b_right >> 32), res_offset);
   b_right_msb = __shift_left_two_words((b_right >> 32), (b_right & ((1ULL << 32) - 1)), res_offset);
#ifdef DEBUG_PRINT
   printf("b_right_msb=%x\n", b_right_msb);
   printf("pre-and-b_0=%x\n", b_0_lsb);
#endif
#ifdef DEBUG_PRINT
   printf("b_0_lsb=%x\n", b_0_lsb);
#endif

   *octant = (b_0_lsb >> 29) & 7;

#ifdef DEBUG_PRINT
   printf("octant=%d\n", *octant);
#endif

   b_fract = (b_0_lsb << 3) | (b_right_msb >> 29);
   b_fract_low = b_right_msb << 3;

#ifdef DEBUG_PRINT
   printf("b_fract-pre=%x\n", b_fract);
   printf("b_fract_low-pre=%x\n", b_fract_low);
#endif

   if(*octant & 1)
   {
      unsigned long long int negated = -(((unsigned long long int)b_fract) << 32 | b_fract_low);
      b_fract = negated >> 32;
      b_fract_low = negated & ((1ULL << 32) - 1);
   }
   *global_b_fract = b_fract;

#ifdef DEBUG_PRINT
   printf("b_fract=%x\n", b_fract);
   printf("b_fract_low=%x\n", b_fract_low);
#endif
   // if(b_fract==0) {abort();return 0;}
   count_leading_zero_macro(32, b_fract, n_leading_zeros);
#ifdef DEBUG_PRINT_STATS
   max_n_leading_zeros = max_n_leading_zeros < n_leading_zeros ? n_leading_zeros : max_n_leading_zeros;
   min_n_leading_zeros = min_n_leading_zeros > n_leading_zeros ? n_leading_zeros : min_n_leading_zeros;
   max_start_index = max_start_index < start_index ? start_index : max_start_index;
#endif
#ifdef DEBUG_PRINT
   printf("n_leading_zeros=%d\n", n_leading_zeros);
#endif

   b_fract = __shift_left_two_words(b_fract, b_fract_low, n_leading_zeros);
   *global_mantissa = b_fract;
   *global_e_final = n_leading_zeros + 1;
#ifdef DEBUG_PRINT
   printf("b_fract=%x\n", b_fract);
#endif
#if CHECK_B_FRACT
   {
      /// rounding
      float_uint_converter func_b_fract;
      LSB_bit = SELECT_BIT(b_fract, 8);
      Guard_bit = SELECT_BIT(b_fract, 7);
      Round_bit = SELECT_BIT(b_fract, 6);
      Sticky_bit = (b_fract & 63) != 0 || (b_fract_low << n_leading_zeros);
      round = Guard_bit & (LSB_bit | Round_bit | Sticky_bit);
#ifdef DEBUG_PRINT
      printf("CHECK_B_FRACT LSB_bit=%x\n", LSB_bit);
      printf("CHECK_B_FRACT Guard_bit=%x\n", Guard_bit);
      printf("CHECK_B_FRACT Round_bit=%x\n", Round_bit);
      printf("CHECK_B_FRACT Sticky_bit=%x\n", Sticky_bit);
#endif

#ifdef DEBUG_PRINT
      if(round)
         printf("rounding\n");
#endif
      res = b_fract + (round << 8);

#ifdef DEBUG_PRINT
      printf("res rounded=%x\n", res);
#endif
      if((res & (1 << 31)) == 0)
      {
         e_final = n_leading_zeros;
         x_final = (res >> 9) & ((1 << 23) - 1);
      }
      else
      {
         e_final = n_leading_zeros + 1;
         x_final = (res >> 8) & ((1 << 23) - 1);
      }
#ifdef DEBUG_PRINT
      printf("x_final=%x\n", x_final);
#endif
      e_final = 127 - e_final;
#ifdef DEBUG_PRINT
      printf("e_final=%x\n", e_final);
#endif
      func_b_fract.b = (e_final << 23) | x_final;
#ifdef DEBUG_PRINT
      printf("b_fract_final=%x\n", func_b_fract.b);
      printf("b_fract_float=%.20f\n", func_b_fract.f);
#endif
      current_b_fract = func_b_fract.f;
   }
#endif
   res = ((unsigned long long int)b_fract * (unsigned long long int)0xC90FDAA2) >> 32;
#ifdef DEBUG_PRINT
   printf("res=%x\n", res);
#endif
   if((res & (1 << 31)) == 0)
   {
      n_leading_zeros++;
      res <<= 1;
   }
   if((res & (1 << 31)) == 0)
   {
      e_final = n_leading_zeros - 1;
      x_final = (res >> 9) & ((1 << 23) - 1);
      Guard_bit = SELECT_BIT(res, 8);
      Round_bit = SELECT_BIT(res, 7);
      Sticky_bit = (res & 127) != 0 || b_fract != 0;
   }
   else
   {
      e_final = n_leading_zeros;
      x_final = (res >> 8) & ((1 << 23) - 1);
      Guard_bit = SELECT_BIT(res, 7);
      Round_bit = SELECT_BIT(res, 6);
      Sticky_bit = (res & 63) != 0 || b_fract != 0;
   }

   e_final = 126 - e_final;
   LSB_bit = SELECT_BIT(x_final, 0);
#ifdef DEBUG_PRINT
   printf("res LSB_bit=%x\n", LSB_bit);
   printf("res Guard_bit=%x\n", Guard_bit);
   printf("res Round_bit=%x\n", Round_bit);
   printf("res Sticky_bit=%x\n", Sticky_bit);
#endif

   /// rounding
   round = Guard_bit & (LSB_bit | Round_bit | Sticky_bit);

   ExpSig = (e_final << 23) | x_final;
   rounded = ExpSig + round;

   return (((unsigned long long int)s) << 31) | (((unsigned long long int)VAL_RESIZE(rounded >> 23, 8)) << 23) |
          ((unsigned long long int)VAL_RESIZE(rounded, 23));
}

static __FORCE_INLINE unsigned int hotbm_fsin_t1(unsigned char a, unsigned int b)
{
   static const unsigned int sin_t1_t1_hotbm_table[] = {
       10583,   31750,   52916,   74081,   95244,   116405,  137563,  158718,  179870,  201018,  222161,  243299,
       264431,  285558,  306678,  327791,  348897,  369995,  391084,  412165,  433236,  454298,  475349,  496390,
       517419,  538437,  559443,  580435,  601415,  622382,  643334,  664271,  685194,  706101,  726993,  747867,
       768725,  789566,  810389,  831193,  851979,  872746,  893493,  914219,  934925,  955610,  976274,  996915,
       1017534, 1038130, 1058703, 1079251, 1099776, 1120275, 1140750, 1161198, 1181621, 1202017, 1222385, 1242726,
       1263040, 1283324, 1303580, 1323807, 1344003, 1364170, 1384305, 1404410, 1424482, 1444523, 1464532, 1484507,
       1504449, 1524357, 1544230, 1564069, 1583873, 1603641, 1623373, 1643069, 1662728, 1682349, 1701932, 1721478,
       1740984, 1760451, 1779879, 1799267, 1818614, 1837921, 1857186, 1876410, 1895591, 1914730, 1933826, 1952879,
       1971887, 1990852, 2009772, 2028646, 2047476, 2066259, 2084996, 2103686, 2122329, 2140925, 2159472, 2177971,
       2196422, 2214823, 2233175, 2251476, 2269727, 2287928, 2306077, 2324175, 2342220, 2360214, 2378154, 2396041,
       2413875, 2431655, 2449381, 2467051, 2484667, 2502227, 2519731, 2537180};
   bool sign = !(SELECT_BIT(b, 17));
   unsigned int b0 = SELECT_RANGE(b, 16, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfffff, 0), 17);
   unsigned int s_1 = b0;
   unsigned int k_1 = sin_t1_t1_hotbm_table[a];
   BIT_RESIZE(k_1, 22);
   unsigned int r0_1 = (((unsigned long long int)k_1) * ((((unsigned long long int)s_1) << 1) | 1)) >> 18;
   BIT_RESIZE(r0_1, 22);
   unsigned int r_1 = r0_1 ^ VAL_RESIZE(CE_MACRO32(sign, 0xffffff, 0), 22);
   r_1 |= VAL_RESIZE(CE_MACRO32(sign, 0xff, 0), 8) << 22;
   return r_1;
}

static __FORCE_INLINE unsigned int hotbm_fcos_t1(unsigned char a, unsigned int b)
{
   static const unsigned int cos_t1_t1_hotbm_table[] = {
       10106,   30319,   50530,   70740,   90947,   111150,  131349,  151544,  171732,  191914,  212089,  232256,
       252414,  272563,  292701,  312829,  332944,  353047,  373137,  393213,  413274,  433319,  453348,  473360,
       493354,  513330,  533286,  553222,  573137,  593031,  612903,  632751,  652575,  672375,  692150,  711899,
       731620,  751315,  770981,  790618,  810225,  829801,  849347,  868860,  888341,  907788,  927201,  946580,
       965922,  985228,  1004497, 1023729, 1042921, 1062075, 1081188, 1100261, 1119293, 1138282, 1157228, 1176131,
       1194989, 1213803, 1232571, 1251292, 1269966, 1288593, 1307171, 1325700, 1344178, 1362607, 1380984, 1399309,
       1417581, 1435800, 1453965, 1472075, 1490129, 1508128, 1526070, 1543954, 1561780, 1579548, 1597256, 1614903,
       1632490, 1650016, 1667479, 1684880, 1702217, 1719490, 1736699, 1753842, 1770919, 1787929, 1804872, 1821747,
       1838553, 1855290, 1871958, 1888555, 1905081, 1921535, 1937916, 1954225, 1970460, 1986621, 2002707, 2018718,
       2034653, 2050511, 2066292, 2081995, 2097620, 2113166, 2128632, 2144018, 2159324, 2174548, 2189690, 2204750,
       2219727, 2234620, 2249429, 2264153, 2278792, 2293346, 2307813, 2322193};

   bool sign = !(SELECT_BIT(b, 17));
   unsigned int b0 = SELECT_RANGE(b, 16, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfffff, 0), 17);
   unsigned int s_1 = b0;
   unsigned int k_1 = cos_t1_t1_hotbm_table[a];
   BIT_RESIZE(k_1, 22);
   unsigned long int r0_1 = (((unsigned long long int)k_1) * ((((unsigned long long int)s_1) << 1) | 1)) >> 18;
   BIT_RESIZE(r0_1, 22);
   unsigned int r_1 = r0_1 ^ VAL_RESIZE(CE_MACRO32(sign, 0xffffff, 0), 22);
   r_1 |= VAL_RESIZE(CE_MACRO32(sign, 0xff, 0), 8) << 22;
   return r_1;
}

static __FORCE_INLINE unsigned int hotbm_fsin_or_cos_t1(unsigned char a, unsigned int b, bool do_cosine)
{
   static const unsigned int sin_or_cos_t1_t1_hotbm_table[] = {
       10583,   31750,   52916,   74081,   95244,   116405,  137563,  158718,  179870,  201018,  222161,  243299,
       264431,  285558,  306678,  327791,  348897,  369995,  391084,  412165,  433236,  454298,  475349,  496390,
       517419,  538437,  559443,  580435,  601415,  622382,  643334,  664271,  685194,  706101,  726993,  747867,
       768725,  789566,  810389,  831193,  851979,  872746,  893493,  914219,  934925,  955610,  976274,  996915,
       1017534, 1038130, 1058703, 1079251, 1099776, 1120275, 1140750, 1161198, 1181621, 1202017, 1222385, 1242726,
       1263040, 1283324, 1303580, 1323807, 1344003, 1364170, 1384305, 1404410, 1424482, 1444523, 1464532, 1484507,
       1504449, 1524357, 1544230, 1564069, 1583873, 1603641, 1623373, 1643069, 1662728, 1682349, 1701932, 1721478,
       1740984, 1760451, 1779879, 1799267, 1818614, 1837921, 1857186, 1876410, 1895591, 1914730, 1933826, 1952879,
       1971887, 1990852, 2009772, 2028646, 2047476, 2066259, 2084996, 2103686, 2122329, 2140925, 2159472, 2177971,
       2196422, 2214823, 2233175, 2251476, 2269727, 2287928, 2306077, 2324175, 2342220, 2360214, 2378154, 2396041,
       2413875, 2431655, 2449381, 2467051, 2484667, 2502227, 2519731, 2537180, 10106,   30319,   50530,   70740,
       90947,   111150,  131349,  151544,  171732,  191914,  212089,  232256,  252414,  272563,  292701,  312829,
       332944,  353047,  373137,  393213,  413274,  433319,  453348,  473360,  493354,  513330,  533286,  553222,
       573137,  593031,  612903,  632751,  652575,  672375,  692150,  711899,  731620,  751315,  770981,  790618,
       810225,  829801,  849347,  868860,  888341,  907788,  927201,  946580,  965922,  985228,  1004497, 1023729,
       1042921, 1062075, 1081188, 1100261, 1119293, 1138282, 1157228, 1176131, 1194989, 1213803, 1232571, 1251292,
       1269966, 1288593, 1307171, 1325700, 1344178, 1362607, 1380984, 1399309, 1417581, 1435800, 1453965, 1472075,
       1490129, 1508128, 1526070, 1543954, 1561780, 1579548, 1597256, 1614903, 1632490, 1650016, 1667479, 1684880,
       1702217, 1719490, 1736699, 1753842, 1770919, 1787929, 1804872, 1821747, 1838553, 1855290, 1871958, 1888555,
       1905081, 1921535, 1937916, 1954225, 1970460, 1986621, 2002707, 2018718, 2034653, 2050511, 2066292, 2081995,
       2097620, 2113166, 2128632, 2144018, 2159324, 2174548, 2189690, 2204750, 2219727, 2234620, 2249429, 2264153,
       2278792, 2293346, 2307813, 2322193};
   bool sign = !(SELECT_BIT(b, 17));
   unsigned int b0 = SELECT_RANGE(b, 16, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfffff, 0), 17);
   unsigned int s_1 = b0;
   unsigned int k_1 = sin_or_cos_t1_t1_hotbm_table[(do_cosine << 7) | a];
   BIT_RESIZE(k_1, 22);
   unsigned int r0_1 = (((unsigned long long int)k_1) * ((((unsigned long long int)s_1) << 1) | 1)) >> 18;
   BIT_RESIZE(r0_1, 22);
   unsigned int r_1 = r0_1 ^ VAL_RESIZE(CE_MACRO32(sign, 0xffffff, 0), 22);
   r_1 |= VAL_RESIZE(CE_MACRO32(sign, 0xff, 0), 8) << 22;
   return r_1;
}

static __FORCE_INLINE unsigned short int hotbm_t2_pow(unsigned short int x)
{
   unsigned int pp0 = 0, pp1 = 0, pp2 = 0, pp3 = 0, pp4 = 0, pp5 = 0, pp6 = 0, r0;
   SET_BIT(pp0, 11, SELECT_BIT(x, 10) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 11, SELECT_BIT(x, 11));

   SET_BIT(pp0, 10, SELECT_BIT(x, 9) & SELECT_BIT(x, 11));

   SET_BIT(pp0, 9, SELECT_BIT(x, 8) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 9, SELECT_BIT(x, 9) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 9, SELECT_BIT(x, 10));

   SET_BIT(pp0, 8, SELECT_BIT(x, 7) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 8, SELECT_BIT(x, 8) & SELECT_BIT(x, 10));

   SET_BIT(pp0, 7, SELECT_BIT(x, 6) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 7, SELECT_BIT(x, 7) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 7, SELECT_BIT(x, 8) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 7, SELECT_BIT(x, 9));

   SET_BIT(pp0, 6, SELECT_BIT(x, 5) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 6, SELECT_BIT(x, 6) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 6, SELECT_BIT(x, 7) & SELECT_BIT(x, 9));

   SET_BIT(pp0, 5, SELECT_BIT(x, 4) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 5, SELECT_BIT(x, 5) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 5, SELECT_BIT(x, 6) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 5, SELECT_BIT(x, 7) & SELECT_BIT(x, 8));
   SET_BIT(pp4, 5, SELECT_BIT(x, 8));

   SET_BIT(pp0, 4, SELECT_BIT(x, 3) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 4, SELECT_BIT(x, 4) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 4, SELECT_BIT(x, 5) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 4, SELECT_BIT(x, 6) & SELECT_BIT(x, 8));

   SET_BIT(pp0, 3, SELECT_BIT(x, 2) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 3, SELECT_BIT(x, 3) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 3, SELECT_BIT(x, 4) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 3, SELECT_BIT(x, 5) & SELECT_BIT(x, 8));
   SET_BIT(pp4, 3, SELECT_BIT(x, 6) & SELECT_BIT(x, 7));
   SET_BIT(pp5, 3, SELECT_BIT(x, 7));

   SET_BIT(pp0, 2, SELECT_BIT(x, 1) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 2, SELECT_BIT(x, 2) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 2, SELECT_BIT(x, 3) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 2, SELECT_BIT(x, 4) & SELECT_BIT(x, 8));
   SET_BIT(pp4, 2, SELECT_BIT(x, 5) & SELECT_BIT(x, 7));

   SET_BIT(pp0, 1, SELECT_BIT(x, 0) & SELECT_BIT(x, 11));
   SET_BIT(pp1, 1, SELECT_BIT(x, 1) & SELECT_BIT(x, 10));
   SET_BIT(pp2, 1, SELECT_BIT(x, 2) & SELECT_BIT(x, 9));
   SET_BIT(pp3, 1, SELECT_BIT(x, 3) & SELECT_BIT(x, 8));
   SET_BIT(pp4, 1, SELECT_BIT(x, 4) & SELECT_BIT(x, 7));
   SET_BIT(pp5, 1, SELECT_BIT(x, 5) & SELECT_BIT(x, 6));
   SET_BIT(pp6, 1, SELECT_BIT(x, 6));

   SET_BIT(pp0, 0, SELECT_BIT(x, 0) & SELECT_BIT(x, 10));
   SET_BIT(pp1, 0, SELECT_BIT(x, 1) & SELECT_BIT(x, 9));
   SET_BIT(pp2, 0, SELECT_BIT(x, 2) & SELECT_BIT(x, 8));
   SET_BIT(pp3, 0, SELECT_BIT(x, 3) & SELECT_BIT(x, 7));
   SET_BIT(pp4, 0, SELECT_BIT(x, 4) & SELECT_BIT(x, 6));
   SET_BIT(pp5, 0, SELECT_BIT(x, 11));

   r0 = pp0 + pp1 + pp2 + pp3 + pp4 + pp5 + pp6;
   return SELECT_RANGE(r0, 12, 2);
}

static __FORCE_INLINE unsigned short int hotbm_fsin_t2(unsigned char a, unsigned short int b)
{
   static const unsigned short int sin_t2_t1_hotbm_table[] = {
       5292, 5292, 5291, 5291, 5291, 5290, 5289, 5288, 5287, 5286, 5285, 5284, 5282, 5281, 5279, 5277, 5275, 5273, 5271,
       5269, 5267, 5264, 5262, 5259, 5256, 5253, 5250, 5247, 5243, 5240, 5236, 5233, 5229, 5225, 5221, 5217, 5212, 5208,
       5203, 5199, 5194, 5189, 5184, 5179, 5174, 5169, 5163, 5158, 5152, 5146, 5140, 5134, 5128, 5122, 5115, 5109, 5102,
       5096, 5089, 5082, 5075, 5068, 5060, 5053, 5045, 5038, 5030, 5022, 5014, 5006, 4998, 4990, 4981, 4973, 4964, 4955,
       4947, 4938, 4928, 4919, 4910, 4901, 4891, 4881, 4872, 4862, 4852, 4842, 4832, 4821, 4811, 4801, 4790, 4779, 4769,
       4758, 4747, 4736, 4724, 4713, 4702, 4690, 4678, 4667, 4655, 4643, 4631, 4619, 4606, 4594, 4582, 4569, 4556, 4544,
       4531, 4518, 4505, 4492, 4478, 4465, 4452, 4438, 4425, 4411, 4397, 4383, 4369, 4355};
   bool sign = !(SELECT_BIT(b, 12));
   unsigned short int b0 = SELECT_RANGE(b, 11, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfff, 0), 12);
   unsigned short int s = hotbm_t2_pow(b0);
   unsigned short int s_1 = SELECT_RANGE(s, 10, 0);
   unsigned short int k_1 = sin_t2_t1_hotbm_table[a];
   BIT_RESIZE(k_1, 13);
   return (((unsigned int)k_1) * ((((unsigned int)s_1) << 1) | 1)) >> 12;
}

static __FORCE_INLINE unsigned int hotbm_fcos_t2(unsigned char a, unsigned short int b)
{
   static const unsigned short int cos_t2_t1_hotbm_table[] = {
       5053, 5053, 5053, 5052, 5051, 5050, 5049, 5048, 5046, 5045, 5043, 5041, 5038, 5036, 5033, 5030, 5027, 5024, 5021,
       5017, 5013, 5009, 5005, 5001, 4996, 4992, 4987, 4981, 4976, 4971, 4965, 4959, 4953, 4947, 4940, 4934, 4927, 4920,
       4913, 4906, 4898, 4890, 4882, 4874, 4866, 4858, 4849, 4840, 4831, 4822, 4813, 4803, 4793, 4783, 4773, 4763, 4753,
       4742, 4731, 4720, 4709, 4698, 4686, 4674, 4663, 4651, 4638, 4626, 4613, 4601, 4588, 4575, 4561, 4548, 4534, 4521,
       4507, 4493, 4478, 4464, 4449, 4434, 4419, 4404, 4389, 4374, 4358, 4342, 4326, 4310, 4294, 4278, 4261, 4244, 4227,
       4210, 4193, 4176, 4158, 4140, 4123, 4104, 4086, 4068, 4050, 4031, 4012, 3993, 3974, 3955, 3936, 3916, 3896, 3877,
       3857, 3836, 3816, 3796, 3775, 3755, 3734, 3713, 3692, 3670, 3649, 3628, 3606, 3584};
   bool sign = !(SELECT_BIT(b, 12));
   unsigned short int b0 = SELECT_RANGE(b, 11, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfff, 0), 12);
   unsigned short int s_0 = hotbm_t2_pow(b0);
   unsigned short int s_1 = SELECT_RANGE(s_0, 10, 0);
   unsigned short int k_1 = cos_t2_t1_hotbm_table[a];
   BIT_RESIZE(k_1, 13);
   return (((unsigned int)k_1) * ((((unsigned int)s_1) << 1) | 1)) >> 12;
}

static __FORCE_INLINE unsigned short int hotbm_fsin_or_cos_t2(unsigned char a, unsigned short int b, bool do_cosine)
{
   static const unsigned short int sin_or_cos_t2_t1_hotbm_table[] = {
       5292, 5292, 5291, 5291, 5291, 5290, 5289, 5288, 5287, 5286, 5285, 5284, 5282, 5281, 5279, 5277, 5275, 5273, 5271,
       5269, 5267, 5264, 5262, 5259, 5256, 5253, 5250, 5247, 5243, 5240, 5236, 5233, 5229, 5225, 5221, 5217, 5212, 5208,
       5203, 5199, 5194, 5189, 5184, 5179, 5174, 5169, 5163, 5158, 5152, 5146, 5140, 5134, 5128, 5122, 5115, 5109, 5102,
       5096, 5089, 5082, 5075, 5068, 5060, 5053, 5045, 5038, 5030, 5022, 5014, 5006, 4998, 4990, 4981, 4973, 4964, 4955,
       4947, 4938, 4928, 4919, 4910, 4901, 4891, 4881, 4872, 4862, 4852, 4842, 4832, 4821, 4811, 4801, 4790, 4779, 4769,
       4758, 4747, 4736, 4724, 4713, 4702, 4690, 4678, 4667, 4655, 4643, 4631, 4619, 4606, 4594, 4582, 4569, 4556, 4544,
       4531, 4518, 4505, 4492, 4478, 4465, 4452, 4438, 4425, 4411, 4397, 4383, 4369, 4355, 5053, 5053, 5053, 5052, 5051,
       5050, 5049, 5048, 5046, 5045, 5043, 5041, 5038, 5036, 5033, 5030, 5027, 5024, 5021, 5017, 5013, 5009, 5005, 5001,
       4996, 4992, 4987, 4981, 4976, 4971, 4965, 4959, 4953, 4947, 4940, 4934, 4927, 4920, 4913, 4906, 4898, 4890, 4882,
       4874, 4866, 4858, 4849, 4840, 4831, 4822, 4813, 4803, 4793, 4783, 4773, 4763, 4753, 4742, 4731, 4720, 4709, 4698,
       4686, 4674, 4663, 4651, 4638, 4626, 4613, 4601, 4588, 4575, 4561, 4548, 4534, 4521, 4507, 4493, 4478, 4464, 4449,
       4434, 4419, 4404, 4389, 4374, 4358, 4342, 4326, 4310, 4294, 4278, 4261, 4244, 4227, 4210, 4193, 4176, 4158, 4140,
       4123, 4104, 4086, 4068, 4050, 4031, 4012, 3993, 3974, 3955, 3936, 3916, 3896, 3877, 3857, 3836, 3816, 3796, 3775,
       3755, 3734, 3713, 3692, 3670, 3649, 3628, 3606, 3584};
   bool sign = !(SELECT_BIT(b, 12));
   unsigned short int b0 = SELECT_RANGE(b, 11, 0) ^ VAL_RESIZE(CE_MACRO32(sign, 0xfff, 0), 12);
   unsigned short int s = hotbm_t2_pow(b0);
   unsigned short int s_1 = SELECT_RANGE(s, 10, 0);
   unsigned short int k_1 = sin_or_cos_t2_t1_hotbm_table[(do_cosine << 7) | a];
   BIT_RESIZE(k_1, 13);
   return (((unsigned int)k_1) * ((((unsigned int)s_1) << 1) | 1)) >> 12;
}

static __FORCE_INLINE unsigned int hotbm_fsin_t3(bool b)
{
   unsigned int r0 = VAL_RESIZE(CE_MACRO32(b, 0xffffffff, 0), 30);
   return r0;
}

static __FORCE_INLINE unsigned int hotbm_fcos_t3(bool a, bool b)
{
   bool sign = !b;
   unsigned int r0 = ((!a) ^ sign) & 1;
   r0 |= (VAL_RESIZE(CE_MACRO32(!sign, 0xffffffff, 0), 29)) << 1;
   return r0;
}

static __FORCE_INLINE unsigned int hotbm_fsin(unsigned int x)
{
   static const unsigned int sin_t0_hotbm_table[] = {
       5302,      47636,     132302,    259300,    428625,    640274,    894243,    1190525,   1529114,   1910003,
       2333182,   2798642,   3306373,   3856363,   4448600,   5083070,   5759759,   6478652,   7239732,   8042983,
       8888386,   9775922,   10705570,  11677311,  12691122,  13746981,  14844862,  15984742,  17166595,  18390394,
       19656112,  20963720,  22313188,  23704485,  25137582,  26612445,  28129040,  29687335,  31287293,  32928878,
       34612054,  36336782,  38103024,  39910739,  41759887,  43650426,  45582314,  47555507,  49569960,  51625628,
       53722465,  55860423,  58039454,  60259510,  62520539,  64822491,  67165315,  69548957,  71973363,  74438479,
       76944250,  79490619,  82077528,  84704920,  87372735,  90080913,  92829392,  95618112,  98447010,  101316021,
       104225081, 107174125, 110163086, 113191897, 116260490, 119368796, 122516744, 125704265, 128931286, 132197735,
       135503537, 138848620, 142232908, 145656325, 149118793, 152620235, 156160572, 159739725, 163357614, 167014156,
       170709270, 174442873, 178214882, 182025210, 185873774, 189760486, 193685259, 197648006, 201648637, 205687062,
       209763192, 213876934, 218028196, 222216886, 226442909, 230706170, 235006575, 239344027, 243718428, 248129681,
       252577687, 257062347, 261583559, 266141223, 270735236, 275365497, 280031901, 284734343, 289472720, 294246925,
       299056850, 303902390, 308783434, 313699875, 318651603, 323638506, 328660474, 333717395};
   unsigned char a_0 = SELECT_RANGE(x, 24, 18);
   unsigned int r_0 = sin_t0_hotbm_table[a_0];
   BIT_RESIZE(r_0, 29);
   unsigned char a_1 = SELECT_RANGE(x, 24, 18);
   unsigned int b_1 = SELECT_RANGE(x, 17, 0);
   unsigned int r_1 = hotbm_fsin_t1(a_1, b_1);
   unsigned char a_2 = SELECT_RANGE(x, 24, 18);
   unsigned short int b_2 = SELECT_RANGE(x, 17, 5);
   unsigned short int r_2 = hotbm_fsin_t2(a_2, b_2);
   bool b_3 = SELECT_RANGE(x, 17, 17);
   unsigned int r_3 = hotbm_fsin_t3(b_3);
   unsigned int sum = r_0 + r_1 + r_2 + r_3;
   return SELECT_RANGE(sum, 29, 4);
}

static __FORCE_INLINE unsigned int hotbm_fcos(unsigned int x)
{
   static const unsigned int cos_t0_hotbm_table[] = {
       5064,      45489,     126339,    247609,    409296,    611394,    853894,    1136788,   1460064,   1823712,
       2227717,   2672063,   3156735,   3681714,   4246979,   4852511,   5498286,   6184279,   6910466,   7676818,
       8483307,   9329902,   10216572,  11143283,  12110000,  13116687,  14163305,  15249817,  16376180,  17542352,
       18748290,  19993947,  21279277,  22604233,  23968762,  25372816,  26816339,  28299279,  29821579,  31383182,
       32984030,  34624061,  36303215,  38021427,  39778634,  41574769,  43409764,  45283551,  47196059,  49147215,
       51136947,  53165180,  55231836,  57336839,  59480109,  61661566,  63881126,  66138708,  68434225,  70767591,
       73138719,  75547519,  77993900,  80477771,  82999037,  85557604,  88153376,  90786255,  93456142,  96162935,
       98906534,  101686835, 104503734, 107357123, 110246897, 113172945, 116135159, 119133425, 122167633, 125237666,
       128343410, 131484748, 134661561, 137873730, 141121134, 144403651, 147721156, 151073526, 154460634, 157882352,
       161338552, 164829103, 168353874, 171912733, 175505545, 179132175, 182792487, 186486342, 190213602, 193974126,
       197767773, 201594401, 205453863, 209346017, 213270714, 217227808, 221217149, 225238587, 229291971, 233377148,
       237493964, 241642264, 245821892, 250032692, 254274503, 258547167, 262850522, 267184407, 271548658, 275943112,
       280367602, 284821962, 289306025, 293819621, 298362581, 302934734, 307535907, 312165927};
   unsigned char a_0 = SELECT_RANGE(x, 24, 18);
   unsigned int r_0 = cos_t0_hotbm_table[a_0];
   BIT_RESIZE(r_0, 29);
   unsigned char a_1 = SELECT_RANGE(x, 24, 18);
   unsigned int b_1 = SELECT_RANGE(x, 17, 0);
   unsigned int r_1 = hotbm_fcos_t1(a_1, b_1);
   unsigned char a_2 = SELECT_RANGE(x, 24, 18);
   unsigned short int b_2 = SELECT_RANGE(x, 17, 5);
   unsigned int r_2 = hotbm_fcos_t2(a_2, b_2);
   bool a_3 = SELECT_RANGE(x, 24, 24);
   bool b_3 = SELECT_RANGE(x, 17, 17);
   unsigned int r_3 = hotbm_fcos_t3(a_3, b_3);
   unsigned int sum = r_0 + r_1 + r_2 + r_3;
   return SELECT_RANGE(sum, 29, 4);
}

static __FORCE_INLINE unsigned int hotbm_fsin_or_cos(unsigned int x, bool do_cosine)
{
   static const unsigned int sin_or_cos_t0_hotbm_table[] = {
       5302,      47636,     132302,    259300,    428625,    640274,    894243,    1190525,   1529114,   1910003,
       2333182,   2798642,   3306373,   3856363,   4448600,   5083070,   5759759,   6478652,   7239732,   8042983,
       8888386,   9775922,   10705570,  11677311,  12691122,  13746981,  14844862,  15984742,  17166595,  18390394,
       19656112,  20963720,  22313188,  23704485,  25137582,  26612445,  28129040,  29687335,  31287293,  32928878,
       34612054,  36336782,  38103024,  39910739,  41759887,  43650426,  45582314,  47555507,  49569960,  51625628,
       53722465,  55860423,  58039454,  60259510,  62520539,  64822491,  67165315,  69548957,  71973363,  74438479,
       76944250,  79490619,  82077528,  84704920,  87372735,  90080913,  92829392,  95618112,  98447010,  101316021,
       104225081, 107174125, 110163086, 113191897, 116260490, 119368796, 122516744, 125704265, 128931286, 132197735,
       135503537, 138848620, 142232908, 145656325, 149118793, 152620235, 156160572, 159739725, 163357614, 167014156,
       170709270, 174442873, 178214882, 182025210, 185873774, 189760486, 193685259, 197648006, 201648637, 205687062,
       209763192, 213876934, 218028196, 222216886, 226442909, 230706170, 235006575, 239344027, 243718428, 248129681,
       252577687, 257062347, 261583559, 266141223, 270735236, 275365497, 280031901, 284734343, 289472720, 294246925,
       299056850, 303902390, 308783434, 313699875, 318651603, 323638506, 328660474, 333717395, 5064,      45489,
       126339,    247609,    409296,    611394,    853894,    1136788,   1460064,   1823712,   2227717,   2672063,
       3156735,   3681714,   4246979,   4852511,   5498286,   6184279,   6910466,   7676818,   8483307,   9329902,
       10216572,  11143283,  12110000,  13116687,  14163305,  15249817,  16376180,  17542352,  18748290,  19993947,
       21279277,  22604233,  23968762,  25372816,  26816339,  28299279,  29821579,  31383182,  32984030,  34624061,
       36303215,  38021427,  39778634,  41574769,  43409764,  45283551,  47196059,  49147215,  51136947,  53165180,
       55231836,  57336839,  59480109,  61661566,  63881126,  66138708,  68434225,  70767591,  73138719,  75547519,
       77993900,  80477771,  82999037,  85557604,  88153376,  90786255,  93456142,  96162935,  98906534,  101686835,
       104503734, 107357123, 110246897, 113172945, 116135159, 119133425, 122167633, 125237666, 128343410, 131484748,
       134661561, 137873730, 141121134, 144403651, 147721156, 151073526, 154460634, 157882352, 161338552, 164829103,
       168353874, 171912733, 175505545, 179132175, 182792487, 186486342, 190213602, 193974126, 197767773, 201594401,
       205453863, 209346017, 213270714, 217227808, 221217149, 225238587, 229291971, 233377148, 237493964, 241642264,
       245821892, 250032692, 254274503, 258547167, 262850522, 267184407, 271548658, 275943112, 280367602, 284821962,
       289306025, 293819621, 298362581, 302934734, 307535907, 312165927};
   unsigned char a_0 = SELECT_RANGE(x, 24, 18);
   unsigned int r_0 = sin_or_cos_t0_hotbm_table[(do_cosine << 7) | a_0];
   BIT_RESIZE(r_0, 29);
   unsigned char a_1 = SELECT_RANGE(x, 24, 18);
   unsigned int b_1 = SELECT_RANGE(x, 17, 0);
   unsigned int r_1 = hotbm_fsin_or_cos_t1(a_1, b_1, do_cosine);
   unsigned char a_2 = SELECT_RANGE(x, 24, 18);
   unsigned short int b_2 = SELECT_RANGE(x, 17, 5);
   unsigned short int r_2 = hotbm_fsin_or_cos_t2(a_2, b_2, do_cosine);
   bool a_3 = SELECT_RANGE(x, 24, 24);
   bool b_3 = SELECT_RANGE(x, 17, 17);
   unsigned int r_3 = do_cosine ? hotbm_fcos_t3(a_3, b_3) : hotbm_fsin_t3(b_3);
   unsigned int sum = r_0 + r_1 + r_2 + r_3;
   return SELECT_RANGE(sum, 29, 4);
}

__attribute__((noinline)) static float __fsin(unsigned int b_fract, unsigned char exponent,
                                              unsigned int b_fract_shifted)
{
   unsigned int mantissa = b_fract_shifted >> 4;
   unsigned int x = b_fract >> 7;
   unsigned int nS0 = hotbm_fsin(x);
   BIT_RESIZE(nS0, 26);
   unsigned int nS1 = 0xC90FDAA - nS0;
   unsigned long long int mS2 =
       (((unsigned long long int)nS1) * ((unsigned long long int)SELECT_RANGE(mantissa, 27, 0)));
   unsigned int mS3 = ((mS2 >> 30) << 1) | (0 != SELECT_RANGE(mS2, 29, 0));
   unsigned int mS4 =
       CE_MACRO32(SELECT_BIT(mS3, 26), ((SELECT_RANGE(mS3, 26, 2) << 1) | (SELECT_BIT(mS3, 1) | SELECT_BIT(mS3, 0))),
                  SELECT_RANGE(mS3, 25, 0));
   unsigned char eS4 = (VAL_RESIZE((~1u), 7) | (SELECT_BIT(mS3, 26))) - exponent;
   bool LSB_bit, Guard_bit, RS_bit, round;
   LSB_bit = SELECT_BIT(mS4, 2);
   Guard_bit = SELECT_BIT(mS4, 1);
   RS_bit = SELECT_BIT(mS4, 0);
   round = Guard_bit & (LSB_bit | RS_bit);
   unsigned int ExpSig = (eS4 << 23) | SELECT_RANGE(mS4, 24, 2);
   unsigned int rounded = ExpSig + round;
   float_uint_converter func_out;
   func_out.b = SELECT_RANGE(rounded, 30, 0);
   return func_out.f;
}

__attribute__((noinline)) static float __fcos(unsigned int b_fract)
{
   unsigned int x = b_fract >> 7;
   unsigned int nC0 = hotbm_fcos(x);
   BIT_RESIZE(nC0, 26);
   unsigned int mC1 = -nC0;
   unsigned int mC2 = SELECT_RANGE(mC1, 25, 0);
   unsigned int mC3 = SELECT_RANGE(mC2, 25, 2) + ((SELECT_BIT(mC2, 1) & (SELECT_BIT(mC2, 2) | SELECT_BIT(mC2, 0))));
   unsigned char eC = VAL_RESIZE((~1u), 7) | (1 & ~(SELECT_BIT(mC3, 23)));
   unsigned fC = SELECT_RANGE(mC3, 22, 0);
   float_uint_converter func_out;
   func_out.b = (eC << 23) | fC;
   return func_out.f;
}

__attribute__((noinline)) static float __fsin_or_cos(unsigned int b_fract, unsigned char exponent,
                                                     unsigned int b_fract_shifted, bool do_cosine, bool do_negate)
{
   float_uint_converter func_out;
   unsigned int mantissa = b_fract_shifted >> 4;
   unsigned int x = b_fract >> 7;
   unsigned int nSC0 = hotbm_fsin_or_cos(x, do_cosine);
   BIT_RESIZE(nSC0, 26);
   unsigned int mC1 = (1 << 26) - nSC0;

   bool LSB_bit_cosine, Guard_bit_cosine, RS_bit_cosine, round_cosine;
   LSB_bit_cosine = SELECT_BIT(mC1, 2);
   Guard_bit_cosine = SELECT_BIT(mC1, 1);
   RS_bit_cosine = SELECT_BIT(mC1, 0);
   round_cosine = (Guard_bit_cosine & (LSB_bit_cosine | RS_bit_cosine));
   unsigned char eC = VAL_RESIZE((~1u), 7) | (SELECT_BIT(mC1, 26));
   unsigned int ExpSig_cosine = ((eC << 23) | SELECT_RANGE(mC1, 24, 2));

   unsigned int nS1 = 0xC90FDAA - nSC0;
   unsigned long long int mS2 =
       (((unsigned long long int)nS1) * ((unsigned long long int)SELECT_RANGE(mantissa, 27, 0)));
   unsigned int mS3 = ((mS2 >> 30) << 1) | (0 != SELECT_RANGE(mS2, 29, 0));
   unsigned int mS4 =
       CE_MACRO32(SELECT_BIT(mS3, 26), ((SELECT_RANGE(mS3, 26, 2) << 1) | (SELECT_BIT(mS3, 1) | SELECT_BIT(mS3, 0))),
                  SELECT_RANGE(mS3, 25, 0));
   unsigned char eS4 = (VAL_RESIZE((~1u), 7) | (SELECT_BIT(mS3, 26))) - exponent;

   bool LSB_bit_sine, Guard_bit_sine, RS_bit_sine, round_sine;
   LSB_bit_sine = SELECT_BIT(mS4, 2);
   Guard_bit_sine = SELECT_BIT(mS4, 1);
   RS_bit_sine = SELECT_BIT(mS4, 0);
   round_sine = Guard_bit_sine & (LSB_bit_sine | RS_bit_sine);
   unsigned int ExpSig_sine = (eS4 << 23) | SELECT_RANGE(mS4, 24, 2);

   bool round = (do_cosine && round_cosine) || ((!do_cosine) && round_sine);
   unsigned int ExpSig = CE_MACRO32(do_cosine, ExpSig_cosine, ExpSig_sine);

   unsigned int rounded = ExpSig + round;
   unsigned res = (do_negate << 31) | SELECT_RANGE(rounded, 30, 0);
   func_out.b = res;
   return func_out.f;
}

static __FORCE_INLINE float _Complex __cexpif(float x)
{
   extern float fabsf(float x);
   unsigned int y;
   float _Complex Res;
   bool s;
   unsigned char e;
   unsigned int m;
   bool do_negate_sine;
   float_uint_converter func_in;
   func_in.f = x;
   y = func_in.b;
   e = (y >> 23) & 255;
   s = y >> 31;
   m = y & 0x007fffff;
   do_negate_sine = s;
   if(e == 255 && m == 0)
   {
      __imag__ Res = -__builtin_nanf("");
      __real__ Res = -__builtin_nanf("");
      return Res;
   }
   if(e == 255)
   {
      func_in.b |= (0x7FC << 20);
      __imag__ Res = func_in.f;
      __real__ Res = func_in.f;
      return Res;
   }
   if(e < 115) // if x is less than 2^-12 return x
   {
      __imag__ Res = fabsf(x);
      __real__ Res = 1.0;
   }
   else
   {
      unsigned int global_b_fract;
      unsigned char global_e_final;
      unsigned int global_mantissa;
      unsigned char n;
      unsigned int res_rr = __bambu_range_redux(fabsf(x), &global_b_fract, &global_e_final, &global_mantissa, &n);
      float_uint_converter func_out;
      func_out.b = res_rr;
      float y = func_out.f;
#ifdef DEBUG_PRINT
      printf("y=%.20f\n", y);
      printf("n=%d\n", n);
#endif
      bool do_sine_sine = 0;
      bool do_sine_cosine = 0;
      bool do_negate_cosine = 0;

      float res_sine = __fsin(global_b_fract, global_e_final, global_mantissa);
      float res_cosine = __fcos(global_b_fract);
      switch(n & 7)
      {
         case 0:
            do_sine_sine = 1;
            break;
         case 1:
            do_sine_cosine = 1;
            break;
         case 2:
            do_sine_cosine = 1;
            do_negate_cosine = 1;
            break;
         case 3:
            do_sine_sine = 1;
            do_negate_cosine = 1;
            break;
         case 4:
            do_sine_sine = 1;
            do_negate_sine = !do_negate_sine;
            do_negate_cosine = 1;
            break;
         case 5:
            do_sine_cosine = 1;
            do_negate_sine = !do_negate_sine;
            do_negate_cosine = 1;
            break;
         case 6:
            do_sine_cosine = 1;
            do_negate_sine = !do_negate_sine;
            break;
         case 7:
         default:
            do_negate_sine = !do_negate_sine;
            do_sine_sine = 1;
            break;
      }
      if(do_sine_sine)
         __imag__ Res = res_sine;
      else
         __imag__ Res = res_cosine;
      if(do_sine_cosine)
         __real__ Res = res_sine;
      else
         __real__ Res = res_cosine;
      if(do_negate_cosine)
         __real__ Res = -__real__ Res;
   }
   if(do_negate_sine)
      __imag__ Res = -__imag__ Res;
   return Res;
}

#endif // _K_SINCOS_HOTBM_C
