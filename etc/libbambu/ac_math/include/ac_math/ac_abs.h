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
// File: ac_abs.h
//
// Description: Provides absolute value functions for AC datatypes
//              + input: argument
//              + output: absolute-value
//
//              * Integer Signed
//                void abs(ac_int<XW,true> x, ac_int<YW,false> &y)
//                void abs(ac_int<XW,true> x, ac_int<YW,true> &y)
//
//              * Fixed Point Signed
//                void abs(ac_fixed<XW,XI,true,XQ,XO> x, ac_fixed<YW,YI,false,YQ,YO> &y)
//                void abs(ac_fixed<XW,XI,true,XQ,XO> x, ac_fixed<YW,YI,true,YQ,YO> &y)
//
//              * Float
//                void abs(ac_float<XW,XI,XE,XQ> x, ac_float<YW,YI,YE,YQ> &y)
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_abs.h>
//    using namespace ac_math;
//
//    typedef ac_int<20, true> input_type;
//    typedef ac_int<24, true> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_abs(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = -3;
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
//*****************************************************************************************

#ifndef _INCLUDED_AC_ABS_H_
#define _INCLUDED_AC_ABS_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

// Include headers for data types supported by these implementations
#include <ac_fixed.h>
#include <ac_float.h>
#include <ac_int.h>

//=========================================================================
// Function: ac_abs (for ac_int)
//
// Description:
//    Calculation of absolute value of real inputs, passed as ac_int
//    variables.
//
// Usage:
//    See above example code for usage.
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <int XW, int YW>
   void ac_abs(ac_int<XW, true> x, ac_int<YW, false>& y)
   {
      ac_int<1, true> xltz = (x < 0);
      ac_int<XW, false> xabs = (xltz ^ x).template slc<XW>(0) - xltz;
      y = xabs;
   }

   template <int XW, int YW>
   void ac_abs(ac_int<XW, true> x, ac_int<YW, true>& y)
   {
      ac_int<XW, false> xabs;
      ac_abs(x, xabs);
      y = xabs;
   }

   //=========================================================================
   // Function: ac_abs (for ac_fixed)
   //
   // Description:
   //    Calculation of absolute value of real inputs, passed as ac_fixed
   //    variables.
   //
   // Usage:
   //    A sample testbench and its implementation look like
   //    this:
   //
   //    #include <ac_math/ac_abs.h>
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
   //      ac_abs(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input = -3.25;
   //      output_type output;
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int YW, int YI, ac_q_mode YQ, ac_o_mode YO>
   void ac_abs(ac_fixed<XW, XI, true, XQ, XO> x, ac_fixed<YW, YI, false, YQ, YO>& y)
   {
      ac_int<XW, true> xi = x.template slc<XW>(0);
      ac_int<XW, false> xiabs;
      ac_abs(xi, xiabs);
      ac_fixed<XW, XI, false> xabs;
      xabs.set_slc(0, xiabs.template slc<XW>(0));
      y = xabs;
   }

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int YW, int YI, ac_q_mode YQ, ac_o_mode YO>
   void ac_abs(ac_fixed<XW, XI, true, XQ, XO> x, ac_fixed<YW, YI, true, YQ, YO>& y)
   {
      ac_fixed<XW, XI, false> xabs;
      ac_abs(x, xabs);
      y = xabs;
   }

   //=========================================================================
   // Function: ac_abs (for ac_float)
   //
   // Description:
   //    Calculation of absolute value of real inputs, passed as ac_fixed
   //    variables.
   //
   // Usage:
   //    A sample testbench and its implementation look like
   //    this:
   //
   //    #include <ac_math/ac_abs.h>
   //    using namespace ac_math;
   //
   //    typedef ac_float<20, 11, 10, AC_RND> input_type;
   //    typedef ac_float<30, 15, 12, AC_RND> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_abs(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input = -3.25;
   //      output_type output;
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, int XE, ac_q_mode XQ, int YW, int YI, int YE, ac_q_mode YQ>
   void ac_abs(ac_float<XW, XI, XE, XQ> x, ac_float<YW, YI, YE, YQ>& y)
   {
      ac_fixed<XW, XI, false> xabs_m;
      ac_abs(x.m, xabs_m);
      ac_float<XW, XI, XE + 1> xabs;
      // |-2^I*2^E| = 2^I*2^E > (2^I - ulp)*2^E
      //   adjust 2^I*2^E to 2^(I-1)*2^(E+1)
      bool ismax = xabs_m[XW - 1];
      xabs.e = x.e + ismax;
      xabs.m[XW - 1] = 0;
      xabs.m.set_slc(0, xabs_m.template slc<XW - 1>(ismax));
      y = xabs;
   }
} // namespace ac_math

#endif
