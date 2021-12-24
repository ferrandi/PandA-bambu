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
//********************************************************************************************
// File: ac_pow_pwl.h
//
// Description:
//    Provides piece-wise linear implementations of the
//    base 2 and base e exponential functions for ac_fixed inputs.
//
// Usage:
//    A sample testbench and its implementation look like this:
//
//    #include <ac_math/ac_pow_pwl.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<20, 11, true, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<24, 14, false, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_pow2_pwl(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 2.5;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN(0);
//    }
//    #endif
//
// Notes:
//    Attempting to call the function with a type that is not implemented will result
//    in a compile error.
//
// Revision History:
//    Niramay Sanghvi : Aug 15 2017 : Default template parameters for configurability added
//    Niramay Sanghvi : Aug 07 2017 : Used ac_shift_left function for RND and SAT support.
//    Niramay Sanghvi : Jul 12 2017 : Added header style format.
//    Niramay Sanghvi : Jul 11 2017 : Added support for all possible values of integer widths.
//    Niramay Sanghvi : Jul 05 2017 : Passed output by reference.
//    Niramay Sanghvi : Jun 29 2017 : Renamed header files and functions.
//
//********************************************************************************************

#ifndef _INCLUDED_AC_POW_PWL_H_
#define _INCLUDED_AC_POW_PWL_H_

// The functions use default template parameters, which are only supported by C++11 or later
// compiler standards. Hence, the user should be informed if they are not using those standards.

#if __cplusplus < 201103L
#error Please use C++11 or a later standard for compilation.
#endif

#include <ac_fixed.h>
#include <ac_int.h>

// Include headers for required functions
#include <ac_math/ac_shift.h>

#if !defined(__BAMBU__)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Function: ac_pow2_pwl (for ac_fixed)
//
// Description:
//    Calculation of base 2 exponential of real inputs, passed as ac_fixed
//    variables.
//
//    Separates input into integer and fractional part, the fractional part
//    is passed to the PWL approximation. The output is then left-shifted
//    by the value of the integer part, in order to de-normalize.
//
// Usage:
//    See above example code for usage.
//
// Notes:
//    The PWL implementation utilizes 3 elements, which has a small impact
//    on accuracy.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <ac_q_mode pwl_Q = AC_RND, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI,
             ac_q_mode outQ, ac_o_mode outO>
   void ac_pow2_pwl(const ac_fixed<W, I, S, Q, O>& input, ac_fixed<outW, outI, false, outQ, outO>& output)
   {
      // Stores the fractional part of the input. By default it is set to 0
      ac_fixed<AC_MAX(W - I, 1), 0, false> input_frac_part = 0;

      // Take out the fractional part of the input
      // This serves as a sort of normalization, with the fractional part being
      // the normalized data (can only vary from 0 to 0.9999...)

      // Only carry out slicing if the input has a fractional component.
      // If the input doesn't have a fractional part, the default value of input_frac_part, i.e. 0,
      // is suitable to be used in later calculations.
      if(W > I)
      {
         input_frac_part.set_slc(0, input.template slc<AC_MAX(W - I, 1)>(0));
      }

      // Start of code outputted by ac_pow_pwl_lutgen.cpp
      // Note that the LUT generator file also outputs values for x_min_lut (lower limit of PWL domain), x_max_lut
      // (upper limit of PWL domain) and sc_constant_lut (scaling factor used to scale the input from 0 to
      // n_segments_lut). However, these values aren't considered in the header file because it has been optimized to
      // work with a 4-segment PWL model that covers the domain of [0, 1). For other PWL implementations, the user will
      // probably have to take these values into account explicitly. Guidelines for doing so are given in the comments.
      // In addition, some of the slope values here are modified slightly in order to ensure monotonicity of the PWL
      // function as the input crosses segment boundaries. The user might want to take care to ensure that for their own
      // PWL versions.

      // Initialization for PWL LUT
      const unsigned n_segments_lut = 4;
      // The number of fractional bits for the LUT values is chosen by first finding the maximum absolute error over the
      // domain of the PWL when double-precision values are used for LUT values. This error will correspond to a number
      // of fractional bits that are always guaranteed to be error-free, for fixed-point PWL outputs. This number of
      // fractional bits is found out by the formula: nbits = abs(ceil(log2(abs_error_max)). The number of fractional
      // bits hereafter used to store the LUT values is nbits + 2. For this particular PWL implementation, the number of
      // fractional bits is 9. Initializing the LUT arrays
      static const ac_fixed<10, 0, false> m_lut[n_segments_lut] = {.189453125, .224609375, 0.2666015625, .3173828125};
      static const ac_fixed<11, 1, false> c_lut[n_segments_lut] = {.998046875, 1.1875, 1.412109375, 1.6787109375};

      // End of code outputted by ac_pow_pwl_lutgen.cpp

      // Compute power of two using pwl
      // Scale the normalized input from 0 to n_segments_lut. Any other PWL implementation
      // with a different number of segments/domain should be scaled according to the formula: x_in_sc =
      // (input_frac_part - x_min_lut) * sc_constant_lut where sc_constant_lut = n_segments_lut / (x_max_lut -
      // x_min_lut) (x_min_lut and and x_max_lut are the lower and upper limits of the domain)
      ac_fixed<12, 2, false> x_in_sc = ((ac_fixed<14, 2, false>)input_frac_part) << 2;
      ac_fixed<12 - 2, 0, false> x_in_sc_frac;
      // Slice out the fractional part from the scaled input, store it in another variable.
      x_in_sc_frac.set_slc(0, x_in_sc.template slc<12 - 2>(0));
      // The integer part of the scaled input is the index of the LUT table
      ac_int<2, false> index = x_in_sc.to_int();
      typedef ac_fixed<21, 1, false, pwl_Q> output_pwl_type;
      output_pwl_type output_pwl = m_lut[index] * x_in_sc_frac + c_lut[index];

      // Shift left by the integer part of the input to cancel out the previous normalization.
      ac_math::ac_shift_left(output_pwl, input.to_int(), output);

#if !defined(__BAMBU__) && defined(AC_POW_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "Actual input              = " << input << endl;
      cout << "normalized input          = " << input_frac_part << endl;
      cout << "output up-scaled by exp   = " << output << endl;
      cout << "index                     = " << index << endl;
#endif
   }

   //=============================================================================
   // Version that allows the return of values.
   template <class T_out, ac_q_mode pwl_Q = AC_RND, class T_in>
   T_out ac_pow2_pwl(const T_in& input)
   {
      // Create a temporary variable for output and use the pass-by-reference version
      // to evaluate it. This temporary variable is returned as the output.
      T_out output;
      ac_pow2_pwl<pwl_Q>(input, output);
      return output;
   }

   //=============================================================================
   // Function: ac_exp_pwl (for ac_fixed)
   //
   // Description:
   //    Calculation of base e exponential of real inputs, passed as ac_fixed
   //    variables.
   //
   //    Separates input into integer and fractional part, the fractional part
   //    is passed to the PWL approximation. The output is then left-shifted
   //    by the value of the integer part, in order to de-normalize.
   //
   // Usage:
   //    A sample testbench and its implementation look like this:
   //
   //    #include <ac_math/ac_pow_pwl.h>
   //    using namespace ac_math;
   //
   //    typedef ac_fixed<20, 11, true, AC_RND, AC_SAT> input_type;
   //    typedef ac_fixed<24, 14, false, AC_RND, AC_SAT> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_exp_pwl(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input = 2.5;
   //      output_type output;
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN(0);
   //    }
   //    #endif
   //
   // Notes:
   //    This function relies on the ac_pow2_pwl function for its computation. It
   //    does this by multiplying the input with log2(e), then passing it to
   //    the ac_pow2_pwl function. In doing so, we also make sure that the
   //    product variable has enough precision to store the result of
   //    input*log2(e).
   //
   //-----------------------------------------------------------------------------

   // This struct computes precision of input_inter variable ("pii") for base e exponent. It also makes sure
   // that there are a set minimum no. of fractional bits to represent the multiplication of x with log2(e)
   // (this is decided by the n_f_b variable).
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int n_f_b>
   struct comp_pii_exp
   {
      enum
      {
         pit_i = I + 1,
         pit_w_inter = W + 1,
         pit_w = (W - I) > n_f_b ? pit_w_inter : pit_i + n_f_b
      };
      typedef ac_fixed<pit_w, pit_i, S, Q, O> pit_t;
   };

   // n_f_b = minimum no of fractional bits used in storing the result of multiplication by log2(e)
   template <int n_f_b = 9, ac_q_mode pwl_Q = AC_RND, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW,
             int outI, ac_q_mode outQ, ac_o_mode outO>
   void ac_exp_pwl(const ac_fixed<W, I, S, Q, O>& input, ac_fixed<outW, outI, false, outQ, outO>& output)
   {
      static const ac_fixed<17, 3, true> log2e = 1.44269504089;
      // Find type of intermediate variable used to store output of x*log2(e)
      typedef typename comp_pii_exp<W, I, S, Q, O, n_f_b>::pit_t input_inter_type;
      input_inter_type input_inter;
      // e^x = 2^(x*log2(e))
      input_inter = input * log2e;
      ac_pow2_pwl<pwl_Q>(input_inter, output);

#if !defined(__BAMBU__) && defined(AC_POW_PWL_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input_inter.width       = " << input_inter.width << endl;
      cout << "input_inter.i_width     = " << input_inter.i_width << endl;
      cout << "input (power_exp)       = " << input << endl;
      cout << "log2e (power_exp)       = " << log2e << endl;
      cout << "input_inter (power_exp) = " << input_inter << endl;
      cout << "output (power_exp)      = " << output << endl;
#endif
   }

   //=============================================================================
   // Version that allows the return of values.
   template <class T_out, int n_f_b = 9, ac_q_mode pwl_Q = AC_RND, class T_in>
   T_out ac_exp_pwl(const T_in& input)
   {
      // Create a temporary variable for output and use the pass-by-reference version
      // to evaluate it. This temporary variable is returned as the output.
      T_out output;
      ac_exp_pwl<n_f_b, pwl_Q>(input, output);
      return output;
   }
} // namespace ac_math

#endif
