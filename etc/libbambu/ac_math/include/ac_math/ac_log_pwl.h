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
//***********************************************************************************
// File: ac_log_pwl.h
//
// Description: Provides piece-wise linear implementations of the
//    log function for the AC (tm) Datatypes: ac_fixed.
//    Two different functions compute values with bases as 2 and e.
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_log_pwl.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<16, 8, false, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<16, 8, true, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      // Use an 'ifdef' directive to choose between these two functions.
//      // It is assumed that the relevant macro has been defined earlier.
//      #ifdef TEST_LOG2
//      ac_log2_pwl(input,output);
//      #endif
//      #ifdef TEST_LOGE
//      ac_log_pwl(input, output)
//      #endif
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
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
//    This file contains two functions:
//     1. ac_log2_pwl : It takes ac_fixed type value as input outputs log2 value
//     2. ac_log_pwl : It takes ac_fixed type value as input and outputs loge value
//    This file uses the normalization function from ac_normalize.h
//
//***********************************************************************************

#ifndef _INCLUDED_AC_LOG_PWL_H_
#define _INCLUDED_AC_LOG_PWL_H_

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_fixed.h>

// Include headers for required functions
#include <ac_math/ac_normalize.h>

#if !defined(__BAMBU__) && defined(AC_LOG_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

//=================================================================================
// Function: ac_log2_pwl (for ac_fixed, returns log base 2 value of input provided)
//
// Description:
//    Calculation of log base 2 of real inputs, passed as ac_fixed
//    variables.
//
//    Passes inputs for normalization to the function in ac_normalize.h,
//    which gives the exponent and output normalized between 0.5 and 1.
//    The normalized output is then subjected to piecewise linear implementation
//    of log2 value and returns log2 of normalized value.
//
// Usage:
//    See above example code for usage.
//
//---------------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode q_mode_temp = AC_TRN, int W1, int I1, ac_q_mode q_mode_in, ac_o_mode o_mode_in, int W2, int I2,
             bool S2, ac_q_mode q_mode_out, ac_o_mode o_mode_out>
   void ac_log2_pwl(const ac_fixed<W1, I1, false, q_mode_in, o_mode_in> input,
                    ac_fixed<W2, I2, S2, q_mode_out, o_mode_out>& result)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero input
      // is encountered.
      // If AC_ASSERT is not activated: the output will saturate when a zero input is encountered.
      // The functionality behind this is taken care of by other sections of the code.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!input, "Log of zero not supported.");
#endif

      ac_fixed<W1, 0, false, q_mode_in, o_mode_in> input_normalized;
      // exp is used to store the final exponent value
      int exp = ac_math::ac_normalize(input, input_normalized);

      // Start of code outputted by ac_log_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_max_lut (upper limit of PWL domain) and
      // sc_constant_lut (scaling factor used to scale the input from 0 to n_segments_lut). However, these values aren't
      // explicitly considered in the header file because it has been optimized to work with an 8-segment PWL model that
      // covers the domain of [0.5, 1). For other PWL implementations, the user will probably have to take these values
      // into account explicitly. Guidelines for doing so are given in the comments. In addition, some of the slope
      // values here are modified slightly in order to ensure monotonicity of the PWL function as the input crosses
      // segment boundaries. The user might want to take care to ensure that for their own PWL versions.

      // Define lower limit of PWL
      static const ac_fixed<1, 0, false> x_min_lut = 0.5;
      // No. of PWL segments
      static const unsigned n_segments_lut = 8;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)) - 1. The number of
      // fractional bits hereafter used to store the LUT values is nbits + 2. For this particular PWL implementation,
      // the number of fractional bits is 11.
      const int n_frac_bits = 11;
      const int int_bits = ac::nbits<n_segments_lut - 1>::val;
      // Store values for slope and intercept of line segments.
      static const ac_fixed<n_frac_bits, 0, false> m[n_segments_lut] = {
          0.169921875, 0.1513671875, 0.13720703125, 0.125, 0.115234375, 0.1064453125, 0.099609375, 0.09326171875};
      static const ac_fixed<n_frac_bits + 1, 1, true> c[n_segments_lut] = {-.99853515625, -.82861328125, -.67724609375,
                                                                           -.53955078125, -.41455078125, -.298828125,
                                                                           -.1923828125,  -.0927734375};

      // End of code outputted by ac_log_pwl_lutgen.cpp

      // Scale the normalized input from 0 to n_segments_lut
      // Note that this equation is optimized for a domain of [0.5, 1) and 8 segments.  Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc =
      // (input_normalized - x_min_lut) * sc_constant_lut where sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<n_frac_bits + int_bits, int_bits, false> input_sc =
          ((ac_fixed<n_frac_bits + int_bits + 4, int_bits, false>)(input_normalized - x_min_lut)) << 4;
      // Take out the fractional bits of the scaled input
      ac_fixed<n_frac_bits, 0, false> input_sc_frac;
      input_sc_frac.set_slc(0, input_sc.template slc<n_frac_bits>(0));
      // Integer part of scaled input is index
      ac_int<int_bits, false> index = input_sc.to_int();
      ac_fixed<W2, I2, S2, q_mode_out, o_mode_out> result_min;
      // If 0 is supplied as the function input, maximum negative value is returned at the output
      result_min.template set_val<AC_VAL_MIN>();
      // computation of the pwl output
      // The precision given below will ensure that there is no precision lost in the assignment to t, hence rounding
      // for the variable is switched off by default. However, if the user uses less fractional bits and turn rounding
      // on instead, they are welcome to do so by changing giving a different value for q_mode_temp.
      ac_fixed<2 * n_frac_bits + 1, 1, true, q_mode_temp> t = m[index] * input_sc_frac + c[index];
      // Add the exponent to get the final function output
      ac_fixed<W2, I2, S2, q_mode_out, o_mode_out> t2 = t + exp;
      // assignment to the final output
      result = (input == 0) ? result_min : t2;

#if !defined(__BAMBU__) && defined(AC_LOG_PWL_H_DEBUG)
      cout << __FILE__ << __LINE__ << endl;
      cout << "input_width" << input_width << endl;
      cout << "input_int" << input_int << endl;
      cout << "input = " << input << endl;
      cout << "input to normalization function" << input << endl;
      cout << "output (fractional of normalization function" << input_normalized << endl;
      cout << "normalized exp" << exp << endl;
      cout << "index of element chosen from ROM" << index << endl;
      cout << "final output = " << result << endl;
#endif
   }

   //=========================================================================
   // Function: ac_log_pwl (for ac_fixed, returns ln(input) )
   //
   // Description:
   //    Calculation of ln (natural log) of real inputs, passed as ac_fixed
   //    variables.
   //
   //    This implementation uses change of base method to compute log base e of
   //    input. The value of ln of input is given by,
   //    ln(x) = log2(x)/log2(e)
   //    which makes, ln(x) = log2(x)*constant
   //    log2(x) is computed using above piecewise linear implementation.
   //    Note that, 1/log2(e) = loge(2), which is declared as a constant.
   //
   // Usage:
   //    See above example code for usage.
   //
   //-------------------------------------------------------------------------
   template <ac_q_mode q_mode_temp = AC_TRN, int W1, int I1, ac_q_mode q_mode_in, ac_o_mode o_mode_in, int W2, int I2,
             bool S2, ac_q_mode q_mode_out, ac_o_mode o_mode_out>
   void ac_log_pwl(const ac_fixed<W1, I1, false, q_mode_in, o_mode_in> input,
                   ac_fixed<W2, I2, S2, q_mode_out, o_mode_out>& result)
   {
      // Store ln(2) as a constant ac_fixed value.
      static const ac_fixed<12, 0, false, AC_RND> log_constant = 0.69314718056;
      // Find the number of integer bits required to represent the minimum and maximum values expressable for the input
      // type. The number of integer bits used for the temporary variable is whichever is larger.
      const int t_I_frac = ac::nbits<AC_MAX(W1 - I1, 0)>::val;
      const int t_I_int = ac::nbits<AC_MAX(I1, 0)>::val;
      const int t_I = (t_I_frac > t_I_int ? t_I_frac : t_I_int) + 1;
      // Store the number of fractional bits in the PWL output. This can change based on PWL implementations, hence, the
      // user must handle these changes appropriately.
      const int n_f_b_pwl_out = 22;
      // The above precision will ensure that the assignment to result_temp is lossless, hence rounding is turned off by
      // default. However, the user is free to use less bits and turn rounding on instead if they wish, by changing the
      // q_mode_temp variable.
      ac_fixed<n_f_b_pwl_out + t_I, t_I, true, q_mode_temp> result_temp;
      ac_fixed<W2, I2, true, q_mode_out, o_mode_out> result_min;
      result_min.template set_val<AC_VAL_MIN>();
      // call to the log base 2 pwl function
      ac_log2_pwl<q_mode_temp>(input, result_temp);

      // If input is non-zero, multiply the output of log2(input) by ln(2) to get the final result
      // If input is zero, then assign minimum value to output.
      result = input != 0 ? (ac_fixed<W2, I2, true, q_mode_out, o_mode_out>)(result_temp * log_constant) : result_min;

#if !defined(__BAMBU__) && defined(AC_LOG_PWL_H_DEBUG)
      cout << "Input to the log base e function = " << input << endl;
      cout << "constant_width = " << constant_width << endl;
      cout << "result_temp = " << result_temp << endl;
      cout << "Final output =" << result << endl;
#endif
   }

   //=========================================================================
   // Version that allows returning of values for log2.
   template <class T_out, ac_q_mode q_mode_temp = AC_TRN, class T_in>
   T_out ac_log2_pwl(const T_in& input)
   {
      // create a variable that is to be returned
      T_out output;
      // call above implementation of log base 2
      ac_log2_pwl<q_mode_temp>(input, output);
      // return the final output
      return output;
   }

   //=========================================================================
   // Version that allows returning of values for ln.
   template <class T_out, ac_q_mode q_mode_temp = AC_TRN, class T_in>
   T_out ac_log_pwl(const T_in& input)
   {
      // create a variable that is to be returned
      T_out output;
      // call above implementation of log base e
      ac_log_pwl<q_mode_temp>(input, output);
      // return the final output
      return output;
   }
} // namespace ac_math
#endif
