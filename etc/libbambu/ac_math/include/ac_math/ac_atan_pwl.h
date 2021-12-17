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
//******************************************************************************************
// Function: ac_atan_pwl (for ac_fixed)
//
// Description:
//    Calculation of arctangent of real inputs, passed as ac_fixed
//    variables.
//
// Usage:
//    A sample testbench and its implementation looks like this:
//
//    #include <ac_math/ac_atan_pwl.h>
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
//      ac_atan_pwl(input,output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 0.25;
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
//    This file uses the ac_reciprocal_pwl() function from ac_reciprocal_pwl.h
//
//******************************************************************************************

#ifndef _INCLUDED_AC_ATAN_PWL_H_
#define _INCLUDED_AC_ATAN_PWL_H_

// The functions use default template parameters, which are only supported by C++11 or later
// compiler standards. Hence, the user should be informed if they are not using those standards.

#if !(__cplusplus >= 201103L)
#error Please use C++11 or a later standard for compilation.
#endif

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_fixed.h>

#if !defined(__BAMBU__) && defined(AC_ATAN_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

#include <ac_math/ac_reciprocal_pwl.h>

//=========================================================================
// Function: ac_atan_pwl (for ac_fixed)
//
// Description:
//    Calculation of arctangent of real inputs, passed as ac_fixed
//    variables.
//
// Usage:
//    See above example for usage.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, ac_q_mode Q, ac_o_mode O, int outW, int outI, ac_q_mode outQ,
             ac_o_mode outO>
   void ac_atan_pwl(const ac_fixed<W, I, false, Q, O>& input, ac_fixed<outW, outI, false, outQ, outO>& output)
   {
      // Store the approximate value of pi by 2
      static const ac_fixed<11, 1, false> pi_by_2 = 1.5703125;

      // Give the number of fraction bits to be assigned for the normalized input. 14 is chosen for this implementation,
      // because it results in a very low error.
      const int f_b_n_i = 14;
      ac_fixed<f_b_n_i, 0, false, pwl_Q, AC_SAT> normalized_input;

      // Start of code outputted by ac_atan_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_min_lut (lower limit of PWL domain), x_max_lut
      // (upper limit of PWL domain) and sc_constant_lut (scaling factor used to scale the input from 0 to
      // n_segments_lut). However, these values aren't considered in the header file because it has been optimized to
      // work with a 4-segment PWL model that covers the domain of [0, 1). For other PWL implementations, the user will
      // probably have to take these values into account explicitly. Guidelines for doing so are given in the comments.
      // In addition, some of the slope values here are modified slightly in order to ensure monotonicity of the PWL
      // function as the input crosses segment boundaries. The user might want to take care to ensure that for their own
      // PWL versions.

      // Initialization for PWL LUT
      static const unsigned n_segments_lut = 4;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)). The number of fractional
      // bits hereafter used to store the LUT values is nbits + 2. For this particular PWL implementation, the number of
      // fractional bits is 10.
      const int n_frac_bits = 10;
      // slope and intercept value array
      static const ac_fixed<n_frac_bits, 0, false> m_lut[n_segments_lut] = {.2451171875, .2197265625, .1796875,
                                                                            0.138671875};
      static const ac_fixed<n_frac_bits, 0, false> c_lut[n_segments_lut] = {.0009765625, .24609375, .4658203125,
                                                                            .646484375};

      // End of code outputted by ac_atan_pwl_lutgen.cpp

      // If the input exceeds or equals 1, we take the reciprocal of the input and find the arctangent of that
      // reciprocal. We then use the formula atan(x) = pi/2 - atan(1 / x) to find the arctangent of the original input.
      // Also, keep in mind that the input can only exceed 1 if the number of integer bits are greater than or equal
      // to 1.
      bool input_exceeds_1 = false;
      if(I >= 1)
      {
         input_exceeds_1 = input > 1 ? true : false;
      }
      if((I >= 1) && input_exceeds_1)
      {
         ac_math::ac_reciprocal_pwl<pwl_Q>(input, normalized_input);
      }
      // If input is lesser than 1, then it is within the domain of the PWL function. Hence, no reciprocal operation is
      // required.
      else
      {
         normalized_input = input;
      }

      // Compute atan using pwl.
      const int int_bits = ac::nbits<n_segments_lut - 1>::val;
      // Scale the input from 0 to 4. Note that the below expression is simplified and optimized for 4 segments and a
      // domain of [0, 1). Any other PWL implementation with a different number of segments/domain should be scaled
      // according to the formula: x_in_sc = (normalized_input - x_min_lut) * sc_constant_lut where sc_constant_lut =
      // n_segments_lut / (x_max_lut - x_min_lut) where x_min_lut and x_max_lut are the lower and upper limits of the
      // PWL Domain.
      ac_fixed<f_b_n_i, int_bits, false> x_in_sc = (ac_fixed<f_b_n_i + int_bits, int_bits, false>)normalized_input << 2;
      // Take out the fractional bits of the scaled input
      ac_fixed<f_b_n_i - int_bits, 0, false> x_in_sc_frac;
      x_in_sc_frac.set_slc(0, x_in_sc.template slc<f_b_n_i - int_bits>(0));
      ac_int<int_bits, false> index;
      // The integer part of the scaled input is the index of the LUT table
      index = x_in_sc.to_int();
      // The precision given below will ensure that there is no precision lost in the assignment to output_pwl, hence
      // rounding for the variable is switched off by default. However, if the user uses less fractional bits and wishes
      // to turn rounding on instead, they are welcome to do so by giving a different value for pwl_Q.
      ac_fixed<f_b_n_i - int_bits + n_frac_bits + 1, 1, false, pwl_Q> output_pwl =
          m_lut[index] * x_in_sc_frac + c_lut[index];

      // If the input exceeds 1, apply the previously mentioned formula.
      if((I >= 1) && input_exceeds_1)
      {
         output_pwl = pi_by_2 - output_pwl;
      }

      output = output_pwl;

#if !defined(__BAMBU__) && defined(AC_ATAN_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input            = " << input << endl;
      cout << "normalized_input = " << normalized_input << endl;
      cout << "x_in_sc          = " << x_in_sc << endl;
      cout << "output_pwl       = " << output_pwl << endl;
      cout << "output           = " << output << endl;
#endif
   }

   // The following version enables a return-by-value.
   template <class T_out, ac_q_mode pwl_Q = AC_TRN, class T_in>
   T_out ac_atan_pwl(const T_in& input)
   {
      T_out output;
      ac_atan_pwl<pwl_Q>(input, output);
      return output;
   }

} // namespace ac_math

#endif // _INCLUDED_AC_ATAN_PWL_H_
