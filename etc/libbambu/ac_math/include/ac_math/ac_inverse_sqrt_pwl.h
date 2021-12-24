/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Math Library                                       *
 *                                                                        *
 *  Software Version: 2.0                                                 *
 *                                                                        *
 *  Release Date    : Thu Aug  2 11:19:34 PDT 2018                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2.0.10                                              *
 *                                                                        *
 *  Copyright , Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
// **************************************************************************
// File : ac_inverse_sqrt_pwl.h
//
// Created on: September 17, 2017
//
// Author: Sachchidanand Deo
//
// Description: Provides piece-wise linear implementations of the inverse
// square root function for the AC (tm) Datatypes: ac_fixed, ac_float and
// ac_complex<ac_fixed>
//
// Usage:
//    A sample testbench and its implementation looks like this:
//
//    #include <ac_math/ac_inverse_sqrt_pwl.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<16, 8, false, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<16, 8, false, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_inverse_sqrt_pwl(input,output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 1.2;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    This file uses C++ function overloading to target implementations specific
//    to each type of data. Attempting to call the function with a type that is
//    not implemented will result in a compile error.
//
//    This library uses the following functions from other files: ac_normalize()
//    , ac_shift_right() and ac_sqrt_pwl().
//
// *****************************************************************************

#ifndef _INCLUDED_AC_INVERSE_SQRT_PWL_H_
#define _INCLUDED_AC_INVERSE_SQRT_PWL_H_

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>

// Include headers for required functions
#include <ac_math/ac_normalize.h>
#include <ac_math/ac_shift.h>
#include <ac_math/ac_sqrt_pwl.h>

#if !defined(__BAMBU__) && defined(AC_INVERSE_SQRT_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

// ==============================================================================
// Function: ac_inverse_sqrt_pwl (for ac_fixed)
//
// Description:
//    Calculation of square root of positive real inputs, passed as ac_fixed
//    variables.
//
//    Passes inputs for normalization to the function in ac_normalize.h,
//    which gives the exponent and output normalized between 0.5 and 1.
//    The normalized value is then subject to the piecewise linear
//    implementation to calculate the reciprocal of square root.
//
//    For simplification square root of exponent is simply computed by
//    dividing the exponent value by 2 and square root of normalized value is
//    computed using piecewise linear mechanism.
//
// ------------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode q_mode_temp = AC_TRN, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_inverse_sqrt_pwl(const ac_fixed<W1, I1, false, q1, o1>& input, ac_fixed<W2, I2, false, q2, o2>& output)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero input
      // is encountered.
      // If AC_ASSERT is not activated: the output will saturate when a zero input is encountered.
      // The functionality behind this is taken care of by other sections of the code.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!input, "Inverse square root of zero not supported.");
#endif

      // n_segments_lut are the number of pwl segments (8 segments are used in this implementation)
      static const unsigned n_segments_lut = 8;

      static const ac_fixed<12, 0, false> inverseroot2 = 0.70703125;
      // normalized_input is basically output of the normalized function that is to be given to the PWL.
      ac_fixed<W1, 0, false, q1, o1> normalized_input;
      int normalized_exp, normalized_exp_temp2; // Temporary variables to store exponents

      normalized_exp = ac_math::ac_normalize(input, normalized_input);

      // Start of code outputted by ac_inverse_sqrt_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_max_lut (upper limit of PWL domain) and
      // sc_constant_lut (scaling factor used to scale the input from 0 to n_segments_lut). However, these values aren't
      // explicitly considered in the header file because it has been optimized to work with an 8-segment PWL model that
      // covers the domain of [0.5, 1). For other PWL implementations, the user will probably have to take these values
      // into account explicitly. Guidelines for doing so are given in the comments. In addition, some of the slope
      // values here are modified slightly in order to ensure monotonicity of the PWL function as the input crosses
      // segment boundaries. The user might want to take care to ensure that for their own PWL versions.

      static const ac_fixed<1, 0, false> x_min_lut = 0.5;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // error-free fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)). The number of
      // fractional bits hereafter used to store the LUT values is nbits + 2. For this particular PWL implementation,
      // the number of fractional bits is 12.
      const int n_frac_bits = 12;
      // slope and intercept value array
      static const ac_fixed<n_frac_bits, 0, true> m[n_segments_lut] = {-.080810546875, -.068115234375, -.05859375,
                                                                       -.05126953125,  -.045166015625, -.040283203125,
                                                                       -.0361328125,   -.03271484375};
      static const ac_fixed<n_frac_bits + 1, 1, false> c[n_segments_lut] = {
          1.413330078125, 1.33251953125,  1.26416015625, 1.20556640625,
          1.154296875,    1.109130859375, 1.06884765625, 1.032470703125};

      // End of code outputted by ac_inverse_sqrt_pwl_lutgen.cpp

      const int int_bits = ac::nbits<n_segments_lut - 1>::val;
      // Scale the normalized input from 0 to n_segments_lut
      // Note that this equation is optimized for a domain of [0.5, 1) and 8 segments. Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc =
      // (normalized_input - x_min_lut) * sc_constant_lut where sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<n_frac_bits + int_bits, int_bits, false> input_sc =
          ((ac_fixed<n_frac_bits + int_bits + 4, int_bits, false>)(normalized_input - x_min_lut)) << 4;
      // Take out the fractional bits of the scaled input
      ac_fixed<n_frac_bits, 0, false> input_sc_frac(0);
      input_sc_frac.set_slc(0, input_sc.template slc<n_frac_bits>(0));
      // index is taken as integer part of scaled value and used for selection of m and c values
      ac_int<int_bits, false> index = input_sc.to_int();
      // normalized output provides square root of normalized value
      // The output of the PWL approximation should have the same signedness as the output of the function.
      // The precision given below will ensure that there is no precision lost in the assignment to output_pwl, hence
      // rounding for the variable is switched off by default. However, if the user uses less fractional bits and turn
      // rounding on instead, they are welcome to do so by giving a different value for q_mode_temp.
      ac_fixed<2 * n_frac_bits + 1, 1, false, q_mode_temp> normalized_output = m[index] * input_sc_frac + c[index];
      // store the initial exponent value in temporary variable
      normalized_exp_temp2 = normalized_exp;
      // Handling of odd exponents
      ac_fixed<2 * n_frac_bits, 0, false, q_mode_temp> normalized_output_temp = normalized_output * inverseroot2;
      // Right shift the exponent by 1 to divide by 2
      normalized_exp = normalized_exp >> 1;
      ac_fixed<2 * n_frac_bits + 1, 1, false> m1 =
          (normalized_exp_temp2 % 2 == 0) ?
              normalized_output :
              (ac_fixed<2 * n_frac_bits + 1, 1, false, q_mode_temp>)normalized_output_temp;
      ac_fixed<W2, I2, false, q2, o2> output_temp;
      // "De-normalize" the output by performing a right-shift and cancel out the effects of the previous normalization.
      ac_math::ac_shift_right(m1, normalized_exp, output_temp);

      // If a zero input is encountered, the output must saturate regardless of whether the assert has been activated or
      // not. Assign a variable that stores the saturated output value.
      ac_fixed<W2, I2, false, q2, o2> output_temp_max(0);
      output_temp_max.template set_val<AC_VAL_MAX>();
      // Use a ternary operator to decide whether the output should store the PWL-calculated value or the saturated
      // value, based on whether a zero was passed or not.
      output = input != 0 ? output_temp : output_temp_max;

#if !defined(__BAMBU__) && defined(AC_INVERSE_SQRT_PWL_H_DEBUG)
      cout << "input                  = " << input << endl;
      cout << "normalized_input       = " << normalized_input << endl;
      cout << "normalized_exp         = " << normalized_exp << endl;
      cout << "input_sc               = " << input_sc << endl;
      cout << "index                  = " << index << endl;
      cout << "normalized_output      = " << normalized_output << endl;
      cout << "normalized_output_temp = " << normalized_output_temp << endl;
      cout << "normalized_exp_temp2   = " << normalized_exp_temp2 << endl;
      cout << "m1                     = " << m1 << endl;
      cout << "normalized_exp         = " << normalized_exp << endl;
      cout << "output                 = " << output << endl;
#endif
   }

   // ===========================================================================
   // Function: ac_inverse_sqrt_pwl (for ac_complex <ac_fixed>)
   //
   // Description:
   //    Calculation of square root of fixed point complex inputs,
   //    passed as ac_complex <ac_fixed> variables.
   //
   //    Uses the following formula to compute inverse square root of
   //    complex number:
   //
   //    Formula : 1 / sqrt (a+bi) = inverse_sqrt(a^2+b^2) * sqrt (a-bi)
   //
   //    Note that, function accepts true as input sign and hence can accept
   //    negative/positive real and imaginary signed complex numbers.
   //
   //    This function relies on the fixed point implementation of ac_inverse_sqrt_pwl
   //    and complex fixed point implementation of ac_sqrt_pwl function.
   //
   // ---------------------------------------------------------------------------

   template <ac_q_mode q_mode_temp = AC_TRN, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_inverse_sqrt_pwl(const ac_complex<ac_fixed<W1, I1, true, q1, o1>>& input,
                            ac_complex<ac_fixed<W2, I2, true, q2, o2>>& output)
   {
      // Calculate bitwidths for storing the square root of the conjugated input.
      const int W_1 = find_rt_sqrt_pwl<(2 * (W1 - I1)), (2 * I1 - 1)>::W1;
      const int I_1 = find_rt_sqrt_pwl<(2 * (W1 - I1)), (2 * I1 - 1)>::I1;
      const int n_f_b_1 = W_1 - I_1;
      const int t_I = I1 + 1;
      const int t_n_f_b = (W1 - I1) > n_f_b_1 ? W1 - I1 : n_f_b_1;
      const int t_W = t_I + t_n_f_b;
      const int sqrt_W = find_rt_sqrt_pwl<t_W, t_I - 1>::W1 + 1;
      const int sqrt_I = find_rt_sqrt_pwl<t_W, t_I - 1>::I1 + 1;

      ac_complex<ac_fixed<sqrt_W, sqrt_I, true, q_mode_temp>> sqrt_conj(0);
      // Calculate square root of conjugate of input
      ac_sqrt_pwl<q_mode_temp>(input.conj(), sqrt_conj);

      ac_fixed<W1 + 23, W1 - I1, false, q_mode_temp> inverse_sqrt;
      // compute ac_inverse_sqrt_pwl (a^2+b^2)
      ac_inverse_sqrt_pwl<q_mode_temp>(input.mag_sqr(), inverse_sqrt);

      typedef ac_fixed<sqrt_W + inverse_sqrt.width, sqrt_I + inverse_sqrt.i_width, true, q_mode_temp> output_temp_type;
      ac_complex<output_temp_type> output_temp;
      // compute final result
      output_temp.i() = sqrt_conj.i() * inverse_sqrt;
      output_temp.r() = sqrt_conj.r() * inverse_sqrt;
      // One corner case isn't covered by the above formula. This case happens when the real part of the input is
      // negative and the imaginary part is zero. In such a case, the output imaginary part won't have the correct sign.
      // The line below corrects this.
      output_temp.i() = (input.r() < 0 && input.i() == 0) ? (output_temp_type)-output_temp.i() : output_temp.i();

      // If a zero input is encountered, the real part of the output must saturate regardless of whether the assert has
      // been activated or not. Assign a variable that stores the saturated output value.
      ac_fixed<W2, I2, true, q2, o2> output_temp_max;
      output_temp_max.template set_val<AC_VAL_MAX>();
      bool non_zero_input = input.r() != 0 || input.i() != 0;
      // Use a ternary operator to decide whether the output real part should store the PWL-calculated value or the
      // saturated value, based on whether a zero was passed or not at the input.
      output.r() = non_zero_input ? (ac_fixed<W2, I2, true, q2, o2>)output_temp.r() : output_temp_max;
      // Use a ternary operator to decide whether the output imaginary part should store the PWL-calculated value or a
      // zero value, based on whether a zero was passed or not at the input.
      output.i() = non_zero_input ? (ac_fixed<W2, I2, true, q2, o2>)output_temp.i() : output_temp_max;

#if !defined(__BAMBU__) && defined(AC_INVERSE_SQRT_PWL_H_DEBUG)
      cout << "W1              = " << W1 << endl;
      cout << "I1              = " << I1 << endl;
      cout << "input           = " << input << endl;
      cout << "input.mag_sqr   = " << input.mag_sqr() << endl;
      cout << "sqrt_conj       = " << sqrt_conj << endl;
      cout << "inverse_sqrt    = " << inverse_sqrt << endl;
      cout << "output_temp_max = " << output_temp_max << endl;
      cout << "output_temp     = " << output_temp << endl;
      cout << "output          = " << output << endl;
#endif
   }

   // This struct provides parameterized bitwidths to ensure a lossless return type for the monotonous PWL function
   // provided by default, that operates with 8 segments and uses 12 fractional bits to store slope and intercept
   // values. n_f_b is the number of fractional bits and I is the number of integer bits in the input. The input and
   // output are assumed to be unsigned. Other PWL implementations might require different calculations for the
   // parameterized bitwidths.
   template <int n_f_b, int I>
   struct find_rt_inv_sqrt_pwl
   {
      enum
      {
         I1 = n_f_b % 2 == 0 ? n_f_b / 2 : (n_f_b + 1) / 2,
         n_f_b_floor = I % 2 == 0 ? I / 2 : (I - 1) / 2,
         W1 = I1 + n_f_b_floor + 24
      };
      typedef ac_fixed<W1, I1, false> rt_inv_sqrt_pwl;
   };

   // =============================================================================
   // Function: ac_inverse_sqrt_pwl (for ac_float)
   //
   // Description:
   //    Calculation of square root of floating point inputs,
   //    passed as ac_float variables.
   //
   //    Separates mantissa and exponent of floating point number.
   //    Gives mantissa to the fixed point implementation of
   //    ac_inverse_sqrt_pwl, and halves the exponent. Based on if input exponent
   //    is even or odd, final result is multiplied by inverse of root (2) or
   //    not.
   //
   //    The function accepts only positive real floating point numbers.
   //
   //    This function uses the fixed point implementation of ac_inverse_sqrt_pwl.
   //
   // ----------------------------------------------------------------------------

   template <ac_q_mode q_mode_temp = AC_TRN, int W1, int I1, int E1, ac_q_mode q1, int W2, int I2, int E2, ac_q_mode q2>
   void ac_inverse_sqrt_pwl(const ac_float<W1, I1, E1, q1>& input, ac_float<W2, I2, E2, q2>& output)
   {
      // Calculate bitwidths for mantissa of square root.
      const int W_2 = find_rt_inv_sqrt_pwl<W1 - I1, I1 - 1>::W1;
      const int I_2 = find_rt_inv_sqrt_pwl<W1 - I1, I1 - 1>::I1;
      static const ac_fixed<12, 0, false> inverseroot2 = 0.70703125;
      ac_fixed<W_2, I_2, false, q_mode_temp> output2;
      ac_fixed<W_2 + 1, I_2 + 1, true, q_mode_temp> output2_mant;
      ac_fixed<W1, I1, false> m1 = input.m;
      ac_int<AC_MAX(E1, 2), true> e1 = input.e;
      // The exponent without normalization will be the additive inverse of the input exponent right-shifted by
      // 1/divided by 2. This follows the formula: 1 / sqrt(mant * (2^exp)) = (1 / sqrt(mant)) * (2^(-exp/2))
      ac_int<AC_MAX(E1, 2), true> e2 = -(e1 >> 1);
      ac_inverse_sqrt_pwl<q_mode_temp>(m1, output2);

      output2_mant = (input.e % 2 == 0) ? (ac_fixed<W_2 + 1, I_2 + 1, true, q2>)output2 :
                                          (ac_fixed<W_2 + 1, I_2 + 1, true, q2>)(output2 * inverseroot2);
      // The mantissa without normalization will be either the inverse square root of the original mantissa, or that
      // inverse square root multiplied by 1/sqrt(2), depending upon whether the input exponent is even or not. These
      // two values are passed to an ac_float constructor that takes care of normalization.
      ac_float<W2, I2, E2, q2> output_temp(output2_mant, e2, true);

      // If input is zero, set output to max possible value.
      if(input.m == 0)
      {
         output_temp.template set_val<AC_VAL_MAX>();
      }

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_INVERSE_SQRT_PWL_H_DEBUG)
      cout << "m1          = " << m1 << endl;
      cout << "e1          = " << e1 << endl;
      cout << "e2          = " << e2 << endl;
      cout << "output2     = " << output2 << endl;
      cout << "output_temp = " << output_temp << endl;
      cout << "output      = " << output << endl;
#endif
   }

   // Function definition to enable return by value.

   // =========================================================================
   // Version that allows returning of values
   template <class T_out, ac_q_mode q_mode_temp = AC_TRN, class T_in>
   T_out ac_inverse_sqrt_pwl(const T_in& input)
   {
      // Initializing the final output value that is to be returned
      T_out output;
      // Call the function by referencing the output variable. This is call to one of above implementations
      ac_inverse_sqrt_pwl<q_mode_temp>(input, output);
      // Return the final computed output
      return output;
   }

} // namespace ac_math

#endif // _INCLUDED_AC_INVERSE_SQRT_PWL_H_
