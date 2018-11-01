/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Math Library                                       *
 *                                                                        *
 *  Software Version: 1.0                                                 *
 *                                                                        *
 *  Release Date    : Thu Mar  8 11:17:22 PST 2018                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 1.0.0                                               *
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
// File: ac_ns.h
//
// Description: Defines the ac_math namespace with helper structs
//    for compile-time computations
//
// Usage: N/A
//
// Notes:
//
// Revision History:
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_MATH_NS_H_
#define _INCLUDED_AC_MATH_NS_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

// Include headers for data types supported by these implementations
#include <ac_fixed.h>

template <bool b>
struct ac_math_static_assert
{ /* Compile error, no 'test' symbol. */
};
template <>
struct ac_math_static_assert<true>
{
   enum
   {
      test
   };
};

namespace ac_math
{
   //==========================================================================
   // Helper structs for compile-time computations

   //--------------------------------------------------------------------------
   // Statically computed log2(x)
   template <int W>
   struct slog2
   {
      enum
      {
         floor = 1 + slog2<(W >> 1)>::floor,
         pow2 = (1 << floor) == W,
         ceil = pow2 ? floor : floor + 1
      };
   };
   template <>
   struct slog2<1>
   {
      enum
      {
         floor = 0,
         pow2 = 1,
         ceil = 0
      };
   };

   //--------------------------------------------------------------------------
   // Statically computed Factorial(x)
   template <unsigned M>
   struct Factorial
   {
      enum
      {
         value = M * Factorial<M - 1>::value
      };
   };

   template <>
   struct Factorial<0>
   {
      enum
      {
         value = 1
      };
   };

   //--------------------------------------------------------------------------
   // This struct computes the precision of input_inter variable ("pii") for
   // base e exponent. It also makes sure that there are a set minimum number
   // of fractional bits to represent the multiplication of x with log2(e)
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

} // namespace ac_math

#endif
