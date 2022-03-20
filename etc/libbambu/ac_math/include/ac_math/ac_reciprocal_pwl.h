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
//*****************************************************************************************
// File: ac_reciprocal_pwl.h
//
// Description: Provides piece-wise linear implementations of the
//    reciprocal function for the AC (tm) Datatypes: ac_fixed, ac_float,
//    ac_complex<ac_fixed> and ac_complex<ac_float>.
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_reciprocal_pwl.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<20, 11, true, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<24, 14, true, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_reciprocal_pwl(input, output);
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
//    This file uses C++ function overloading to target implementations
//    specific to each type of data. Attempting to call the function
//    with a type that is not implemented will result in a compile error.
//
//    This library uses the ac_normalize() and ac_shift_right() function
//    from the other ac_math header files.
//
// Revision History:
//    Niramay Sanghvi : Aug 10 2017 : Added default parameters for better configurability.
//    Niramay Sanghvi : Aug 09 2017 : Added zero-input handling
//    Niramay Sanghvi : Aug 07 2017 : Used right-shift function from mgc_ac_math
//    Niramay Sanghvi : Jul 27 2017 : Added structs for checking input and output types.
//    Niramay Sanghvi : Jul 06 2017 : Updated header style.
//    Niramay Sanghvi : Jul 05 2017 : Passed output by reference.
//    Niramay Sanghvi : Jun 21 2017 : Made LUT precision configurable.
//    Niramay Sanghvi : Jun 20 2017 : Added header style format.
//    Niramay Sanghvi : Jun 19 2017 : Added support for ac_complex.
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_RECIPROCAL_PWL_H_
#define _INCLUDED_AC_RECIPROCAL_PWL_H_

// The functions use default template parameters, which are only supported by C++11 or later
// compiler standards. Hence, the user should be informed if they are not using those standards.

#if !(__cplusplus >= 201103L)
#error Please use C++11 or a later standard for compilation.
#endif

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>

// Include headers for required functions
#include <ac_math/ac_normalize.h>
#include <ac_math/ac_shift.h>

#if !defined(__BAMBU__) && defined(AC_RECIPROCAL_PWL_H_DEBUG)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Function: ac_reciprocal_pwl (for ac_fixed)
//
// Description:
//    Calculation of reciprocal of real inputs, passed as ac_fixed
//    variables.
//
//    Passes inputs for normalization to the function in ac_normalize.h,
//    which gives the exponent and output normalized between 0.5 and 1.
//    The normalized value is then subject to the piecewise linear
//    implementation to calculate the reciprocal.
//
//    This reciprocal is then up-scaled according to the exponent value
//    returned by the normalization function, and returned to the calling
//    function.
//
// Usage:
//    See above example code for usage.
//
// Notes:
//    The PWL implementation utilizes 7 elements, which has a small impact
//    on accuracy.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS,
             ac_q_mode outQ, ac_o_mode outO>
   void ac_reciprocal_pwl(const ac_fixed<W, I, S, Q, O>& input, ac_fixed<outW, outI, outS, outQ, outO>& output)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero input
      // is encountered.
      // If AC_ASSERT is not activated: the output will saturate when a zero input is encountered.
      // The functionality behind this is taken care of by other sections of the code.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!input, "Reciprocal of zero not supported.");
#endif

      ac_fixed<outW, outI, outS, outQ, outO> output_temp;

      // Start of code outputted by ac_reciprocal_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_max_lut (upper limit of PWL domain) and
      // sc_constant_lut (scaling factor used to scale the input from 0 to n_segments_lut). However, these values aren't
      // explicitly considered in the header file because it has been optimized to work with an 8-segment PWL model that
      // covers the domain of [0.5, 1). For other PWL implementations, the user will probably have to take these values
      // into account explicitly. Guidelines for doing so are given in the comments. In addition, some of the slope
      // values here are modified slightly in order to ensure monotonicity of the PWL function as the input crosses
      // segment boundaries. The user might want to take care to ensure that for their own PWL versions.

      // Initialization for PWL LUT
      static const unsigned n_segments_lut = 8;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)) - 1. The number of
      // fractional bits hereafter used to store the LUT values is nbits + 2.
      const int nfrac_bits = 10;
      // For this particular PWL implementation, the number of fractional bits is 10.
      static const ac_fixed<nfrac_bits, 0, true> m_lut[n_segments_lut] = {
          -.22265625, -.1767578125, -.14453125, -.12109375, -.1025390625, -.087890625, -.0751953125, -.06640625};
      static const ac_fixed<nfrac_bits + 1, 1, false> c_lut[n_segments_lut] = {
          1.9970703125, 1.7744140625, 1.59765625, 1.453125, 1.33203125, 1.2294921875, 1.1416015625, 1.06640625};
      static const ac_fixed<1, 0, false> x_min_lut = 0.5;

      // End of code outputted by ac_reciprocal_pwl_lutgen.cpp

      // The absolute value of the input is taken and passed to the normalization function. Initialize variables for the
      // same.
      ac_fixed<W, I, false> input_abs_value;
      ac_fixed<W, 0, false> normalized_fixed;

      // If input is signed, take absolute value and assign to intermediate variable.
      if(S)
      {
         input_abs_value = ((input >= 0) ? (ac_fixed<W, I, false>)input : (ac_fixed<W, I, false>)(-input));
      }
      // If input is unsigned, assign value of input to intermediate variable.
      else
      {
         input_abs_value = input;
      }

      // Normalize the absolute value. expret stores the value of the returned base 2 exponential.
      int expret_temp = ac_math::ac_normalize(input_abs_value, normalized_fixed);

      const int int_bits = ac::nbits<n_segments_lut - 1>::val;
      // Compute reciprocal using pwl.
      // Scale the normalized input from 0 to n_segments_lut. Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc =
      // (normalized_fixed - x_min_lut) * sc_constant_lut where sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<int_bits + nfrac_bits, int_bits, false> x_in_sc =
          ((ac_fixed<int_bits + nfrac_bits + 4, int_bits, false>)(normalized_fixed - x_min_lut)) << 4;
      // Take out the fractional bits of the scaled input
      ac_fixed<nfrac_bits, 0, false> x_in_sc_frac;
      x_in_sc_frac.set_slc(0, x_in_sc.template slc<nfrac_bits>(0));
      // The integer part of the input is the index of the LUT table
      ac_int<int_bits, false> index = x_in_sc.to_int();
      // The output of the PWL approximation should have the same signedness as the output of the function.
      // The precision given below will ensure that there is no precision lost in the assignment to output_pwl, hence
      // rounding for the variable is switched off by default. However, if the user uses less fractional bits and turn
      // rounding on instead, they are welcome to do so by giving a different value for pwl_Q.
      typedef ac_fixed<2 * nfrac_bits + 1 + int(outS), 1 + int(outS), outS, pwl_Q> output_pwl_type;
      output_pwl_type output_pwl = m_lut[index] * x_in_sc_frac + c_lut[index];

      if(input != 0)
      { // If input is non-zero, De-normalize output by shifting right by expret_temp
         // If input and output are signed, change sign of output_pwl based on whether input is positive or negative.
         if(S && outS)
         {
            output_pwl = (input < 0) ? (output_pwl_type)(-output_pwl) : output_pwl;
         }
         // ac_shift_right function used for denormalization so as to ensure saturation and rounding.
         ac_math::ac_shift_right(output_pwl, expret_temp, output_temp);
      }
      else
      {
         // If zero input is encountered, set output to the max possible value.
         output_temp.template set_val<AC_VAL_MAX>();
      }

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_RECIPROCAL_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input                   = " << input << endl;
      cout << "input_abs_value         = " << input_abs_value << endl;
      cout << "normalized input        = " << normalized_fixed << endl;
      cout << "expret_temp             = " << expret_temp << endl;
      cout << "x_in_sc                 = " << x_in_sc << endl;
      cout << "x_in_sc_frac            = " << x_in_sc_frac << endl;
      cout << "output_temp             = " << output_temp << endl;
      cout << "output up-scaled by exp = " << output << endl;
#endif
   }

   //=========================================================================
   // Function: ac_reciprocal_pwl (for ac_float)
   //
   // Description:
   //    Calculation of reciprocal of real inputs, passed as ac_float
   //    variables.
   //
   //    The mantissa of the ac_float number is passed as an ac_fixed variable
   //    to the ac_reciprocal_pwl function for ac_fixed numbers. The mantissa of
   //    the output is then set to the resultant reciprocal. The exponent
   //    of the output ac_float variable is set to the negative of the
   //    exponent of the input.
   //
   // Usage:
   //    A sample testbench and its implementation look like
   //    this:
   //
   //    #include <ac_math/ac_reciprocal_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_float<20, 11, 7, AC_RND> input_type;
   //    typedef ac_float<24, 14, 7, AC_RND> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_reciprocal_pwl(input, output);
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
   // Note:
   //    This implementation relies on the ac_fixed implementation of the
   //    reciprocal function.
   //
   //-------------------------------------------------------------------------

   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, int E, ac_q_mode Q, int outW, int outI, int outE, ac_q_mode outQ>
   void ac_reciprocal_pwl(const ac_float<W, I, E, Q>& input, ac_float<outW, outI, outE, outQ>& output)
   {
      const int W1 = W + 20 + 1;
      const int I1 = W - I + 2;

      // Find the reciprocal of the mantissa using the ac_fixed implementation.
      ac_fixed<W1, I1, true, outQ> recip_mantissa;
      ac_reciprocal_pwl<pwl_Q>(input.mantissa(), recip_mantissa);

      // Find the additive inverse of the input's exponent.
      // Pass it and recip_mantissa to an ac_float constructor that takes care of
      // normalization.
      ac_float<outW, outI, outE, outQ> output_temp(recip_mantissa, -input.exp(), true);

      // If the input is zero, set the temp output to the max. possible value.
      if(input.m == 0)
      {
         output_temp.template set_val<AC_VAL_MAX>();
      }

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_RECIPROCAL_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input                   = " << input << endl;
      cout << "input.mantissa()        = " << input.mantissa() << endl;
      cout << "input.exp()             = " << input.exp() << endl;
      cout << "recip_mantissa          = " << recip_mantissa << endl;
      cout << "output.type_name()      = " << output.type_name() << endl;
      cout << "output_temp.type_name() = " << output_temp.type_name() << endl;
      cout << "output_temp             = " << output_temp << endl;
      cout << "output                  = " << output << endl;
      cout << "output.mantissa()       = " << output.mantissa() << endl;
      cout << "output.exp()            = " << output.exp() << endl;
#endif
   }

   //=========================================================================
   // Function: ac_reciprocal_pwl (for ac_complex<ac_fixed>)
   //
   // Description:
   //    Calculation of reciprocal of complex inputs, passed as ac_complex
   //    variables with ac_fixed real and imaginary parts.
   //
   //    The reciprocal is calculated using the formula:
   //
   //    1 / (a + bi) = (a - bi) / (a^2 + b^2)
   //
   //    Where a and b are the real and imaginary parts, respectively.In
   //    order to do this, the value of a^2 + b^2 is first calculated, and the
   //    reciprocal of this real number is calculated by passing it to the
   //    ac_reciprocal_pwl function for ac_fixed numbers.
   //
   //    The resultant reciprocal is multiplied by the real and imaginary
   //    parts of the number passed, the results of which are then passed to
   //    the output.
   //
   // Usage:
   //
   //    A sample testbench and its implementation look like
   //    this:
   //
   //    #include <ac_math/ac_reciprocal_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_complex<ac_fixed<20, 11, true, AC_RND, AC_SAT> > input_type;
   //    typedef ac_complex<ac_fixed<40, 18, true, AC_RND, AC_SAT> > output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_reciprocal_pwl(input, output);
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
   // Note:
   //    This implementation relies on the ac_fixed implementation of the
   //    reciprocal function.
   //
   //-------------------------------------------------------------------------

   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS,
             ac_q_mode outQ, ac_o_mode outO>
   void ac_reciprocal_pwl(const ac_complex<ac_fixed<W, I, S, Q, O>>& input,
                          ac_complex<ac_fixed<outW, outI, outS, outQ, outO>>& output)
   {
      ac_complex<ac_fixed<outW, outI, outS, outQ, outO>> output_temp;

      const int W1 = S ? 2 * W - 1 : 2 * W + 1;
      const int I1 = S ? 2 * I - 1 : 2 * I + 1;
      const int W2 = W1 + 20 + int(!S);
      const int I2 = W1 - I1 + 1;

      ac_fixed<W2, I2, false, outQ, outO> recip_mag_sqr;
      ac_reciprocal_pwl<pwl_Q>(input.mag_sqr(), recip_mag_sqr);

      if(input.r() != 0 || input.i() != 0)
      {
         // Use the formula "1/(a+bi) = (a-bi)/(a^2+b^2)" to assign values to the output.
         output_temp.r() = input.r() * recip_mag_sqr;
         output_temp.i() = -input.i() * recip_mag_sqr;
      }
      else
      {
         // If zero input is passed, then assign the maximum possible value for output's real and imaginary part
         output_temp.r().template set_val<AC_VAL_MAX>();
         output_temp.i().template set_val<AC_VAL_MAX>();
      }

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_RECIPROCAL_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input.mag_sqr() = " << input.mag_sqr() << endl;
      cout << "recip_mag_sqr   = " << recip_mag_sqr << endl;
      cout << "output_temp     = " << output_temp << endl;
      cout << "output          = " << output << endl;
#endif
   }

   //=========================================================================
   // Function: ac_reciprocal_pwl (for ac_complex<ac_float>)
   //
   // Description:
   //    Calculation of reciprocal of complex inputs, passed as ac_complex
   //    variables with ac_float real and imaginary parts.
   //
   //    The reciprocal is calculated using the formula:
   //
   //    (a + bi) = (a - bi) / (a^2 + b^2)
   //
   //    Where a and b are the real and imaginary parts, respectively. In
   //    order to do this, the value of a^2 + b^2 is first calculated, and the
   //    reciprocal of this real number is calculated using the
   //    ac_reciprocal_pwl function for ac_float numbers.
   //
   //    The resultant reciprocal is multiplied by the real and imaginary
   //    parts of the number passed, the results of which are then passed to
   //    the output.
   //
   // Usage:
   //
   //    A sample testbench and its implementation look like
   //    this:
   //
   //    #include <ac_math/ac_reciprocal_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_complex<ac_float<20, 11, 7, AC_RND> > input_type;
   //    typedef ac_complex<ac_float<40, 18, 7, AC_RND> > output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_reciprocal_pwl(input, output);
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
   //      CCS_RETURN(0);
   //    }
   //    #endif
   //
   // Note:
   //    This implementation relies on both the ac_float and ac_fixed
   //    implementations of the reciprocal function.
   //
   //-------------------------------------------------------------------------

   template <ac_q_mode pwl_Q = AC_TRN, int W, int I, int E, ac_q_mode Q, int outW, int outI, int outE, ac_q_mode outQ>
   void ac_reciprocal_pwl(const ac_complex<ac_float<W, I, E, Q>>& input,
                          ac_complex<ac_float<outW, outI, outE, outQ>>& output)
   {
      ac_complex<ac_float<outW, outI, outE, outQ>> output_temp;

      // Calculate real^2 + imag^2
      ac_float<W, I, E, Q> input_real = input.r();
      ac_float<W, I, E, Q> input_imag = input.i();

      // Define type for input_mag_sqr.
      // E + 2 is chosen for the exponent width of input_mag_sqr
      // Because it works well with all input exponential widths,
      // as well as corner cases with very low exponential widths (as low as 1 or 2)
      typedef ac_float<2 * W + 1, 2 * I + 1, E + 2, Q> i_m_s_type;
      i_m_s_type input_mag_sqr;

      ac_float<outW, outI, E + 3, outQ> recip_mag_sqr;

      // Store value of input_mag_sqr to the variable.
      input_mag_sqr.add(input_real * input_real, input_imag * input_imag);
      ac_reciprocal_pwl<pwl_Q>(input_mag_sqr, recip_mag_sqr);

      if(input_mag_sqr.mantissa() != 0)
      {
         // 1/(a+bi) = (a-bi)/(a^2+b^2)
         output_temp.r() = input.r() * recip_mag_sqr;
         output_temp.i() = -input.i() * recip_mag_sqr;
      }
      else
      {
         // If zero input is passed, then assign the maximum possible value for output's real part,
         // and a zero value for imaginary.
         output_temp.r().template set_val<AC_VAL_MAX>();
         output_temp.i().template set_val<AC_VAL_MAX>();
      }

      output = output_temp;

#if !defined(__BAMBU__) && defined(AC_RECIPROCAL_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input                     = " << input << endl;
      cout << "input_mag_sqr             = " << input_mag_sqr << endl;
      cout << "input_mag_sqr.to_double() = " << input_mag_sqr.to_double() << endl;
      cout << "recip_mag_sqr             = " << recip_mag_sqr << endl;
      cout << "recip_mag_sqr.to_double() = " << recip_mag_sqr.to_double() << endl;
      cout << "output_temp               = " << output_temp << endl;
      cout << "output                    = " << output << endl;
      cout << "output.r().to_double()    = " << output.r().to_double() << endl;
      cout << "output.i().to_double()    = " << output.i().to_double() << endl;
#endif
   }

   //=========================================================================
   // Version that allows returning of values.
   template <class T_out, ac_q_mode pwl_Q = AC_TRN, class T_in>
   T_out ac_reciprocal_pwl(const T_in& input)
   {
      // Create an intermediate variable for output and use the pass-by-reference version
      // to evaluate it. This intermediate variable is returned as the output.
      T_out output;
      ac_reciprocal_pwl<pwl_Q>(input, output);
      return output;
   }
} // namespace ac_math

#endif
