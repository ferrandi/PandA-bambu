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
// *****************************************************************************************
// File : ac_sqrt_pwl.h
//
// Created on: Jun 14, 2017
//
// Author: Sachchidanand Deo
//
// Description: Provides piece-wise linear implementations of the
//  square root function for the AC (tm) Datatypes: ac_fixed, ac_float,
//  ac_complex<ac_fixed> and ac_complex<ac_float>.
//
// Usage:
//    A sample testbench and its implementation looks like this:
//
//    #include <ac_math/ac_sqrt_pwl.h>
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
//      ac_sqrt_pwl(input,output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 1.25;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    This file uses C++ function overloading and templates for various datatype
//    implementations. Attempting to call it with a type that is not implemented will
//    result in a compile-time error.
//
//    This file uses the ac_normalize() function from ac_normalize.h and the ac_shift_left()
//    function from ac_shift.h
//
// *****************************************************************************************

#ifndef _INCLUDED_AC_SQRT_PWL_H_
#define _INCLUDED_AC_SQRT_PWL_H_

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>

// Include headers for required functions
#include <ac_math/ac_normalize.h>
#include <ac_math/ac_shift.h>

#if !defined(__BAMBU__) && defined(AC_SQRT_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Function: ac_sqrt_pwl (for ac_fixed)
//
// Description:
//    Calculation of square root of positive real inputs, passed as ac_fixed
//    variables.
//
//    Passes inputs for normalization to the function in ac_normalize.h,
//    which gives the exponent and output normalized between 0.5 and 1.
//    The normalized value is then subject to the piecewise linear
//    implementation to calculate the square root.
//
//    For simplification square root of exponent is simply computed by
//    dividing the exponent value by 2 and square root of normalized value is
//    computed using piecewise linear mechanism.
//
// Usage:
//    Please check code snippet from above for usage.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode pwlQ = AC_TRN, int W, int I, ac_q_mode Q, ac_o_mode O, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
   void ac_sqrt_pwl(const ac_fixed<W, I, false, Q, O> input, ac_fixed<outW, outI, false, outQ, outO>& output)
   {
      // n_segments_lut are the number of pwl segments (4 segments are used in this implementation)
      static const unsigned n_segments_lut = 4;
      // Temporary variable to store output
      ac_fixed<outW, outI, false, outQ, outO> output_temp;

      // Declaring square root of 2 as constant
      static const ac_fixed<13, 1, false> root2 = 1.414306640625;

      // normalized_input is basically output of the normalization function
      ac_fixed<W, 0, false, Q, O> normalized_input;
      // Temporary variables to store exponents
      int normalized_exp, normalized_exp_temp1;
      // Call to normalization function
      normalized_exp = ac_math::ac_normalize(input, normalized_input);

      // Start of code outputted by ac_sqrt_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_max_lut (upper limit of PWL domain) and sc_constant_lut (scaling factor used to scale the input from
      // 0 to n_segments_lut). However, these values aren't explicitly considered in the header file because it has been optimized to work with an 4-segment PWL model that
      // covers the domain of [0.5, 1). For other PWL implementations, the user will probably have to take these values into account explicitly. Guidelines for doing so
      // are given in the comments.
      // In addition, some of the slope values here are modified slightly in order to ensure monotonicity of the PWL function as the input crosses segment boundaries.
      // The user might want to take care to ensure that for their own PWL versions.

      // Piece-wise linear implemenation
      static const ac_fixed<1, 0, false> x_min_lut = 0.5;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the domain of the PWL
      // when double-precision values are used for LUT values. This error will correspond to a number of fractional bits that are always
      // guaranteed to be error-free, for fixed-point PWL outputs.
      // This number of fractional bits is found out by the formula:
      // nbits = abs(ceil(log2(abs_error_max)).
      // The number of fractional bits hereafter used to store the LUT values is nbits + 2.
      // For this particular PWL implementation, the number of fractional bits is 12.
      const int n_frac_bits = 12;
      // The slope values had to be slightly modified in order to maintain montonicity between adjacent segments. As a result of the modification,
      // the 12th fractional bit is always zero. Hence, only 11 fractional bits are used for storage of slope values. For any other implementation
      // with a different number of segments and/or domain, this might not hold true.
      static const ac_fixed<n_frac_bits - 1, 0, false> m[n_segments_lut] = {.08349609375, .0751953125, .0693359375, .064453125};
      static const ac_fixed<n_frac_bits, 0, false> c[n_segments_lut] = {.707763671875, .791259765625, .866455078125, .935791015625};

      // End of code outputted by ac_sqrt_pwl_lutgen.cpp

      const int int_bits = ac::nbits<n_segments_lut - 1>::val;
      // input_sc is scaled value of input, which lies in the range of [0, 4)
      // Scaled input is computed from the normalized input value
      // Note that this equation is optimized for a domain of [0.5, 1) and 4 segments. Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc = (normalized_input - x_min_lut) * sc_constant_lut
      // where sc_constant_lut = n_segments_lut / (x_max_lut - x_min_lut)
      // (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<int_bits + n_frac_bits, int_bits, false> input_sc = ((ac_fixed<int_bits + n_frac_bits + 3, int_bits, false>)(normalized_input - x_min_lut)) << 3;
      // Take out the fractional bits of the scaled input
      ac_fixed<n_frac_bits, 0, false> input_sc_frac;
      input_sc_frac.set_slc(0, input_sc.template slc<n_frac_bits>(0));
      // index is taken as integer part of scaled value and used for selection of m and c values
      ac_int<int_bits, false> index = input_sc.to_int();
      // All the variables declared hereafter use (2*n_frac_bits - 1) fractional bits. This is because, as explained earlier, only 11 fractional bits
      // are needed earlier for storing slope values. For any other implementation with a different number of segments/domain, the user is advised to consider
      // using 2*n_frac_bitsr of fractional bits.

      // normalized output provides square root of normalized value
      ac_fixed<2 * n_frac_bits, 1, false, pwlQ> normalized_output = m[index] * input_sc_frac + c[index];

      // store the initial exponent value in temporary variable
      normalized_exp_temp1 = normalized_exp;
      // Handling of odd exponents
      ac_fixed<2 * n_frac_bits, 1, false, pwlQ> normalized_output_temp = normalized_output * root2;
      // Right shift the exponent by 1 to divide by 2
      normalized_exp = normalized_exp >> 1;
      // The precision given below will ensure that there is no precision lost in the assignment to m1, hence rounding for the variable is switched off by default.
      // However, if the user uses less fractional bits and turn rounding on instead, they are welcome to do so by giving a different value for pwlQ.
      ac_fixed<2 * n_frac_bits, 1, false, pwlQ> m1 = (normalized_exp_temp1 % 2 == 0) ? normalized_output : normalized_output_temp;

      // exponent and normalized output are combined to get the final ac_fixed value, which is written at memory location of output
      ac_math::ac_shift_left(m1, normalized_exp, output_temp);
      output = (input == 0) ? (ac_fixed<outW, outI, false, outQ, outO>)0 : output_temp;

#if !defined(__BAMBU__) && defined(AC_SQRT_PWL_H_DEBUG)
      cout << "W = " << W << endl;
      cout << "I = " << I << endl;
      cout << "outW = " << outW << endl;
      cout << "outI = " << outI << endl;
      cout << "input to normalization function = " << input << endl;
      cout << "output (fractional of normalization function = " << normalized_input << endl;
      cout << "input_sc = " << input_sc << endl;
      cout << "index of element chosen from ROM = " << index << endl;
      cout << "normalized_output = " << normalized_output << endl;
      cout << "normalized_output_temp = " << normalized_output_temp << endl;
      cout << "normalized_exp_temp1 = " << normalized_exp_temp1 << endl;
      cout << "m1 = " << m1 << endl;
      cout << "normalized_exp = " << normalized_exp << endl;
      cout << "final output" << output << endl;
#endif
   }

   // This struct provides parameterized bitwidths to ensure a lossless return type for the monotonous PWL function provided by default,
   // that operates with 4 segments and uses 12 fractional bits to store slope and intercept values.
   // n_f_b is the number of fractional bits and I is the number of integer bits in the input. The input and output are assumed to be
   // unsigned. Other PWL implementations might require different calculations for the parameterized bitwidths.
   template <int n_f_b, int I>
   struct find_rt_sqrt_pwl
   {
      enum
      {
         I1 = I % 2 == 0 ? I / 2 : (I + 1) / 2,
         n_f_b_floor = n_f_b % 2 == 0 ? n_f_b / 2 : (n_f_b - 1) / 2,
         W1 = I1 + n_f_b_floor + 22
      };
      typedef ac_fixed<W1, I1, false> rt_sqrt_pwl;
   };

   //=========================================================================
   // Function: ac_sqrt_pwl (for ac_float)
   //
   // Description:
   //    Calculation of square root of positive real inputs, passed as ac_float
   //    variables.
   //
   //    This function uses the piecewise linear implementation by separating the
   //    mantissa and exponent. Exponent is simply divided by two, if it is even or
   //    is made even and then multiplied by square root of 2, which is stored as
   //    as a constant, where as mantissa undergoes piecewise linear implementation
   //    using helper function defined above.
   //
   // Usage:
   //    A sample testbench and its implementation looks like this:
   //
   //    #include <ac_math/ac_sqrt_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_float<16, 10, 8, AC_RND> input_type;
   //    typedef ac_float<18, 20, 9, AC_RND> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_sqrt_pwl(input,output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input = 1.25;
   //      output_type output;
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   //-------------------------------------------------------------------------

   template <ac_q_mode pwlQ = AC_TRN, int W, int I, int E, ac_q_mode Q, int outW, int outI, int outE, ac_q_mode outQ>
   void ac_sqrt_pwl(const ac_float<W, I, E, Q> input, ac_float<outW, outI, outE, outQ>& output)
   {
      static const ac_fixed<16, 1, false> root2 = 1.414215087890625;

      // mantissa and exponent are separately considered
      ac_fixed<W - 1, I - 1, false, Q> mantissa = input.m;

      const int W1 = find_rt_sqrt_pwl<W - I, I - 1>::W1;
      const int I1 = find_rt_sqrt_pwl<W - I, I - 1>::I1;
      // declaring variable to store square root of mantissa
      ac_fixed<W1, I1, false> m2;

      // call to ac_fixed implementation to get square root of mantissa
      ac_sqrt_pwl<pwlQ>(mantissa, m2);

      // Multiplication by root 2 for odd exponent
      ac_fixed<W1 + 1, I1 + 1, false> m3 = (input.e % 2 == 0) ? (ac_fixed<W1 + 1, I1 + 1, false>)m2 : (ac_fixed<W1 + 1, I1 + 1, false>)(m2 * root2);

      // The mantissa without normalization will be either the square root of the original mantissa, or that square root multiplied by sqrt(2),
      // depending upon whether the input exponent is even or not.
      // The exponent without normalization will be the input exponent right-shifted by 1/divided by 2. This follows the formula:
      // sqrt(mant * (2^exp)) = sqrt(mant) * (2^(exp/2))
      // These two values are passed to an ac_float constructor that takes care of normalization.
      ac_float<outW, outI, outE, outQ> output_temp(m3, input.e >> 1, true);

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_SQRT_PWL_H_DEBUG)
      cout << "input = " << input << endl;
      cout << "W = " << W << endl;
      cout << "I = " << I << endl;
      cout << "W1 = " << W1 << endl;
      cout << "I1 = " << I1 << endl;
      cout << "output of call to ac_fixed version of sqrt_pwl = " << m2 << endl;
      cout << "m3 = " << m3 << endl;
      cout << "output_temp.m = " << output_temp.m << endl;
      cout << "output_temp.e = " << output_temp.e << endl;
      cout << "final output = " << output << endl;
#endif
   }

   //=====================================================================================
   // Function: ac_sqrt_pwl (for ac_complex <ac_fixed>)
   //
   // Description:
   //    Calculation of square root of positive complex inputs, passed as ac_complex
   //    data with real and imaginary part as ac_fixed type of data.
   //
   //    This function uses following mathematical formula for computation of square root:
   //
   //    output_real_part = sqrt((input_real_part + sqrt(input.mag_sqr()))/2.0)
   //    output_imaginary_part = sqrt((-input_real_part + sqrt(input.mag_sqr()))/2.0)
   //    Then square root is calculated by using the PWL model for ac_fixed values.
   //
   // Usage:
   //    A sample testbench and its implementation looks like this:
   //
   //    #include <ac_math/ac_sqrt_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_complex<ac_fixed<16, 10, 8, AC_RND> > input_type;
   //    typedef ac_complex<ac_fixed<18, 20, 9, AC_RND> > output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_sqrt_pwl(input,output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input(1.0, 2.0);
   //      output_type output;
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   //-------------------------------------------------------------------------------------

   template <ac_q_mode pwlQ = AC_TRN, int W, int I, ac_q_mode Q, ac_o_mode O, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
   void ac_sqrt_pwl(const ac_complex<ac_fixed<W, I, true, Q, O>> input, ac_complex<ac_fixed<outW, outI, true, outQ, outO>>& output)
   {
      // Calculate parameterized bitwidths for all intermediate types.
      typedef typename find_rt_sqrt_pwl<(2 * (W - I)), (2 * I - 1)>::rt_sqrt_pwl sqrt_mod_type;
      const int W1 = sqrt_mod_type::width;
      const int I1 = sqrt_mod_type::i_width;
      const int n_f_b_1 = W1 - I1;
      const int t_I = I + 1;
      const int t_n_f_b = (W - I) > n_f_b_1 ? W - I : n_f_b_1;
      const int t_W = t_I + t_n_f_b;
      typedef typename find_rt_sqrt_pwl<t_W, t_I - 1>::rt_sqrt_pwl x_y_type;
      const int W2 = x_y_type::width;
      const int I2 = x_y_type::i_width;

      sqrt_mod_type sqrt_mod;
      ac_sqrt_pwl<pwlQ>(input.mag_sqr(), sqrt_mod); // computation of square root of mag_sqr
      ac_fixed<t_W, t_I, false, AC_TRN, AC_SAT> temp_real = sqrt_mod + input.r();
      ac_fixed<t_W, t_I, false, AC_TRN, AC_SAT> temp_imag = sqrt_mod - input.r();
      ac_fixed<t_W, t_I - 1, false> sqr_real = ((ac_fixed<t_W + 1, t_I, false>)temp_real) >> 1; // calculating square of the output's real part
      ac_fixed<t_W, t_I - 1, false> sqr_imag = ((ac_fixed<t_W + 1, t_I, false>)temp_imag) >> 1; // calculating square of the output's imaginary part
      x_y_type x;
      x_y_type y;
      ac_sqrt_pwl<pwlQ>(sqr_real, x); // calculating output's real part
      ac_sqrt_pwl<pwlQ>(sqr_imag, y); // calculating output's imaginary part
      output.r() = x;
      output.i() = (input.i() < 0) ? -y : (ac_fixed<W2 + 1, I2 + 1, true>)y; // if imaginary part is less than zero, assign output value as negative otherwise positive

#if !defined(__BAMBU__) && defined(AC_SQRT_PWL_H_DEBUG)
      cout << "initial input = " << input << endl;
      cout << "W1 = " << W1 << endl;
      cout << "I1 = " << I1 << endl;
      cout << "Value of square root of mag_sqr = " << sqrt_mod << endl;
      cout << "Type of square root of mag_sqr = " << sqrt_mod.type_name() << endl;
      cout << "Result of addition = " << temp_real << endl;
      cout << "Result of subtraction = " << temp_imag << endl;
      cout << "Type of sqr_real and sqr_imag = " << sqr_real.type_name() << endl;
      cout << "Result of square of output real part = " << sqr_real << endl;
      cout << "Result of square of output imaginary part = " << sqr_imag << endl;
      cout << "Type of real and imaginary part of answer = " << x.type_name() << endl;
      cout << "Absolute value of real part = " << x << endl;
      cout << "Absolute value of imaginary part = " << y << endl;
      cout << "Final value of square root of complex number = " << output << endl;
#endif
   }

   //=========================================================================
   // Version that allows returning of values
   template <class T_out, ac_q_mode pwlQ = AC_TRN, class T_in>
   T_out ac_sqrt_pwl(const T_in& input)
   {
      // Initializing the final output value that is to be returned
      T_out output;
      // Call the function by referencing the output variable. This is call to one of above implementations
      ac_sqrt_pwl<pwlQ>(input, output);
      // Return the final computed output
      return output;
   }
} // namespace ac_math

#endif
