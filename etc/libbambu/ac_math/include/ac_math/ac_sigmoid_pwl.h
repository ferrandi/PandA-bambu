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
// Function: ac_sigmoid_pwl (for ac_fixed)
//
// Description:
//    Provides piece-wise linear approximation of the sigmoid function
//    for the ac_fixed datatype
//
// Usage:
//    A sample testbench and its implementation looks like this:
//
//    #include <ac_math/ac_sigmoid_pwl.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<10, 5, false, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<20, 2, false, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_sigmoid_pwl(input,output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 3.5;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    Attempting to call this file with a datatype that is not implemented will
//    result in a compile-time error.
//
//*************************************************************************************************

#ifndef _INCLUDED_AC_SIGMOID_PWL_H_
#define _INCLUDED_AC_SIGMOID_PWL_H_

// The below functions use default template parameters, which are only supported by C++11 or later
// compiler standards. Hence, the user should be informed if they are not using those standards.

#if !(__cplusplus >= 201103L)
#error Please use C++11 or a later standard for compilation.
#endif

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_fixed.h>

#if !defined(__BAMBU__) && defined(AC_SIGMOID_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Function: ac_sigmoid_pwl (for ac_fixed)
//
// Description:
//    Sigmoid function for real inputs, passed as ac_fixed
//    variables.
//
// Usage:
//    See above example for usage.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI,
             ac_q_mode outQ, ac_o_mode outO>
   void ac_sigmoid_pwl(const ac_fixed<W, I, S, Q, O>& input, ac_fixed<outW, outI, false, outQ, outO>& output)
   {
      // Start of code outputted by ac_sigmoid_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs a value for x_min_lut (lower limit of PWL domain). However, this
      // values isn't explicitly considered in the header file because it has been optimized to work with an 8-segment
      // PWL model that covers the domain of [0, 5). For other PWL implementations, the user will probably have to take
      // this value into account explicitly. Guidelines for doing so are given in the comments.

      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)) - 1 The number of fractional
      // bits hereafter used to store the LUT values is nbits + 2. For this particular PWL implementation, the number of
      // fractional bits is 10. Initialization for PWL LUT
      static const unsigned n_segments_lut = 8;

      const int n_frac_bits = 10;

      // slope and intercept value array
      static const ac_fixed<n_frac_bits - 1, -1, false> m_lut[n_segments_lut] = {
          .1513671875, .126953125, .08984375, .056640625, .033203125, .0185546875, .009765625, .005859375};
      static const ac_fixed<n_frac_bits, 0, false> c_lut[n_segments_lut] = {
          .5009765625, .65234375, .779296875, .869140625, .92578125, .958984375, .9775390625, .98828125};
      // Domain of PWL
      static const ac_fixed<3, 3, false> x_max_lut = 5.0;
      // Scaling constant used to scale the normalized input from 0 to n_segments_lut
      // Note that this scaling constant is optimized for 8 segments and a domain of [0, 5). For any other domain and
      // number of segments, the user should use the following formula: sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) Where x_max_lut and x_min_lut are the upper and lower limits of the PWL domain, respectively.
      static const ac_fixed<n_frac_bits, 1, false> sc_constant_lut = 1.599609375;

      // End of code outputted by ac_sigmoid_pwl_lutgen.cpp
      const int int_bits = ac::nbits<n_segments_lut - 1>::val;

      ac_fixed<W, I, false> input_pwl;

      bool is_input_negative = false;

      if(S && (input < 0))
      {
         is_input_negative = true;
      }

      if(S)
      {
         input_pwl = ((!is_input_negative) ? (ac_fixed<W, I, false>)input : (ac_fixed<W, I, false>)(-input));
      }

      else
      {
         input_pwl = input;
      }

      // Compute the sigmoid function using pwl
      // Scale the normalized input from 0 to n_segments_lut. Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc =
      // (input_frac_part - x_min_lut) * sc_constant_lut where sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<int_bits + n_frac_bits + 1, int_bits + 1, false> x_in_sc = input_pwl * sc_constant_lut;

      // Take out the fractional bits of the scaled input
      ac_fixed<n_frac_bits, 0, false> x_in_sc_frac;
      x_in_sc_frac.set_slc(0, x_in_sc.template slc<n_frac_bits>(0));
      ac_int<int_bits, false> index;
      // The integer part of the input is the index of the LUT table
      index = x_in_sc.to_int();

      // The precision given below will ensure that there is no precision lost in the assignment to output_pwl, hence
      // rounding for the variable is switched off by default. However, if the user wishes to use less fractional bits
      // and turn rounding on instead, they are welcome to do so by giving a different value for pwl_Q.
      ac_fixed<n_frac_bits * 2, 0, false, pwl_Q> output_pwl = m_lut[index] * x_in_sc_frac + c_lut[index];

      // If the input is outside the pwl domain then saturate output
      if(input_pwl >= x_max_lut)
      {
         output_pwl.template set_val<AC_VAL_MAX>();
      }

      // As the sigmoid function is symmetrical, for negative inputs from -5 to 0 (pwl domain is [0, 5) )
      // the output would be 1 - output_pwl
      if(is_input_negative)
      {
         output_pwl = 1 - output_pwl;
      }

      output = output_pwl;

#if !defined(__BAMBU__) && defined(AC_SIGMOID_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input           = " << input << endl;
      cout << "sc_constant_lut = " << sc_constant_lut << endl;
      cout << "input_pwl       = " << input_pwl << endl;
      cout << "x_in_sc         = " << x_in_sc << endl;
      cout << "output_pwl      = " << output_pwl << endl;
      cout << "output          = " << output << endl;
#endif
   }

   // The following version enables a return-by-value.
   template <class T_out, ac_q_mode pwl_Q = AC_TRN, class T_in>
   T_out ac_sigmoid_pwl(const T_in& input)
   {
      T_out output;
      ac_sigmoid_pwl<pwl_Q>(input, output);
      return output;
   }

} // namespace ac_math

#endif // _INCLUDED_AC_SIGMOID_PWL_H_
