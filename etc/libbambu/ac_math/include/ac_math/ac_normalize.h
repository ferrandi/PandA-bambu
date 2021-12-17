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
//************************************************************************************
// File: ac_normalize.h
//
// Description: Provides normalization for the AC (tm) Datatypes: ac_fixed and
//    ac_complex<ac_fixed>
//
// Usage:
//    A sample testbench and its implementation look like this:
//
//    #include <ac_math/ac_normalize.h>
//    using namespace ac_math;
//
//    const int  W = 20;
//    const int  I = 10;
//    const bool S = true;
//
//    typedef ac_fixed<W, I, S, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<W, int(S), S, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output,
//      int &expret
//    ) {
//      expret = ac_normalize(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      int expret;
//      input_type input = 1.2;
//      output_type output;
//      CCS_DESIGN(project)(input, output, expret);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    This file uses C++ function overloading to target implementations
//    specific to each type of data. Attempting to call the function
//    with a type that is not implemented will result in a compile error.
//
// Revision History:
//    Niramay Sanghvi : Aug 18 2017 : Used OR logic instead of comparison.
//    Niramay Sanghvi : Aug 10 2017 : Added support for signed ac_complex<ac_fixed>.
//    Niramay Sanghvi : Aug 07 2017 : Added support for unsigned ac_complex<ac_fixed>.
//
//************************************************************************************

#ifndef _INCLUDED_AC_NORMALIZE_H_
#define _INCLUDED_AC_NORMALIZE_H_

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>

#if !defined(__BAMBU__)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Function: ac_normalize (for ac_fixed)
//
// Description:
//    Normalization of real inputs, passed as ac_fixed variables.
//
//    Normalizes the ac_fixed input such that the MSB coincides
//    with the leading 1. Returns a base-2 exponential that reflects the
//    relation of the normalized value with the original value. The
//    normalized value is an ac_fixed variable that is passed by reference
//    as an output, while the output for the base 2 exponent is passed by
//    value as an int variable.
//
// Usage:
//    See above example code for usage.
//
// Notes:
//    The normalized_fixed value always lies in the range of:
//    (a) [0.5, 1) for unsigned ac_complex<ac_fixed> numbers
//    (b) [-1, -0.5) or [0.5, 1) for signed ac_fixed numbers.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   int ac_normalize(const ac_fixed<W, I, S, Q, O>& input, ac_fixed<W, int(S), S, Q, O>& normalized_fixed)
   {
      // Temporary variable to hold the normalized value.
      ac_fixed<W, int(S), S> normalized_fixed_temp(0);

      // Slice out all the bits, place them in the normalized_fixed_temp variable, then
      // find the position of the leading 1 there.
      normalized_fixed_temp.set_slc(0, input.template slc<W>(0));

      // the leading_1 variable tells us where the leading 1 is.
      int leading_1 = normalized_fixed_temp.leading_sign();

      // Left-shift accordingly to make sure that the leading 1 coincides with the MSB.
      normalized_fixed_temp <<= leading_1;
      normalized_fixed = normalized_fixed_temp;
      // If input is zero, return 0 as output for base 2 exponential. Else return a value
      // that corresponds to leading_1.
      int expret = input != 0 ? I - int(S) - leading_1 : 0;

#if !defined(__BAMBU__) && defined(AC_NORMALIZE_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input to normalize = " << input << endl;
      cout << "leading_1          = " << leading_1 << endl;
      cout << "normalized_fixed   = " << normalized_fixed << endl;
      cout << "expret             = " << expret << endl;
#endif

      return expret;
   }

   //===============================================================================
   // Function: ac_normalize (for ac_complex<ac_fixed>)
   //
   // Description:
   //    Normalization of real inputs, passed as ac_complex<ac_fixed> variables.
   //
   //    Normalizes the ac_complex<ac_fixed> input such that the MSB of the
   //    normalization of the real/imaginary part coincides with the leading 1
   //    of the real/imaginary part, depending upon which has the greater
   //    absolute value. The other part is left-shifted and normalized in
   //    accordance to the normalization for the greater part. The base 2
   //    exponent that is returned hence corresponds to the normalization for
   //    both the real and imaginary part.
   //
   // Usage:
   //    A sample testbench and its implementation look like this:
   //
   //    #include <ac_math/ac_normalize.h>
   //    using namespace ac_math;
   //
   //    const int  W = 20;
   //    const int  I = 10;
   //    const bool S = true;
   //
   //    typedef ac_complex<ac_fixed<W, I, S, AC_RND, AC_SAT> > input_type;
   //    typedef ac_complex<ac_fixed<W, int(S), S, AC_RND, AC_SAT> > output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output,
   //      int &expret
   //    ) {
   //      expret = ac_normalize(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      int expret;
   //      input_type input(1.25, 3);
   //      output_type output;
   //      CCS_DESIGN(project)(input, output, expret);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    The real/imaginary part(whichever has greater absolute value) of the
   //    ac_complex output always lies in the range of:
   //    (a) [0.5, 1) for unsigned ac_fixed numbers.
   //    (b) [-1, -0.5] or [0.5, 1) for signed ac_fixed numbers.
   //
   //-------------------------------------------------------------------------------

   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   int ac_normalize(const ac_complex<ac_fixed<W, I, S, Q, O>>& input,
                    ac_complex<ac_fixed<W, int(S), S, Q, O>>& normalized_complex)
   {
      ac_complex<ac_fixed<W, int(S), S>> normalized_complex_temp;
      typedef ac_fixed<W, I, S> ac_fixed_type;
      ac_fixed_type input_l1_ind;

      // If input is signed, store the bit content or the complement of the bit content of the real and imaginary
      // parts (depending upon whether the real/imaginary part is negative or not) in two separate variables to
      // give an indication of where the leading 1 is.
      if(S)
      {
         ac_fixed_type input_r_l1_ind = input.r() >= 0 ? (ac_fixed_type)input.r() : (ac_fixed_type)~input.r();
         ac_fixed_type input_i_l1_ind = input.i() >= 0 ? (ac_fixed_type)input.i() : (ac_fixed_type)~input.i();
         // Bitwise OR takes into account the leading 1 of the larger value.
         input_l1_ind = (input_r_l1_ind | input_i_l1_ind);
      }
      // Bitwise OR takes into account the leading 1 of the larger value for unsigned values. No complement
      // operation is needed as the input is always positive or 0.
      else
      {
         input_l1_ind = input.r() | input.i();
      }

      // Find leading 1 and left-shift both real and imaginary parts accordingly.
      int leading_1 = input_l1_ind.leading_sign();
      normalized_complex_temp.r().set_slc(0, input.r().template slc<W>(0));
      normalized_complex_temp.i().set_slc(0, input.i().template slc<W>(0));
      normalized_complex_temp.r() <<= leading_1;
      normalized_complex_temp.i() <<= leading_1;
      normalized_complex = normalized_complex_temp;

      // If input is zero, return 0 as output for base 2 exponential. Else return a value
      // that corresponds to leading_1.
      int expret = input != 0 ? I - int(S) - leading_1 : 0;

#if !defined(__BAMBU__) && defined(AC_NORMALIZE_H_DEBUG)
      cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
      cout << "input to normalize = " << input << endl;
      cout << "leading_1          = " << leading_1 << endl;
      cout << "normalized_complex = " << normalized_complex << endl;
      cout << "expret             = " << expret << endl;
#endif

      return expret;
   }
} // namespace ac_math

#endif
