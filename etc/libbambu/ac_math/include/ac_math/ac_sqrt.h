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
// File: ac_sqrt.h
//
// Description: Provides square root functions for AC datatypes
//    + input: argument
//    + output: square root
//
//    * Integer Unsigned
//      void ac_sqrt(ac_int<XW,false> x, ac_int<OW,OS> &sqrt)
//
//    * Fixed Point Unsigned
//      void ac_sqrt(ac_fixed<XW,XI,false,XQ,XO> x, ac_fixed<OW,OI,false,OQ,OO> &sqrt)
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_sqrt.h>
//    using namespace ac_math;
//
//    typedef ac_int<20, false> input_type;
//    typedef ac_int<24, false> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_sqrt(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 4;
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
//    This header file also uses the ac_shift_right() and ac_shift_left() functions from
//    the ac_shift header file.
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_SQRT_H_
#define _INCLUDED_AC_SQRT_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

// Include headers for data types supported by these implementations
#include <ac_fixed.h>
#include <ac_int.h>

// Include header for required functions
#include <ac_math/ac_shift.h>

namespace ac_math
{
   //=========================================================================
   // Function: ac_sqrt (for ac_int)
   //
   // Description:
   //    Calculation of square root of real inputs, passed as ac_int
   //    variables.
   //    + input: unsigned ac_int argument
   //    + output: ac_int square root
   //
   //-------------------------------------------------------------------------

   template <int XW, int OW, bool OS>
   void ac_sqrt(ac_int<XW, false> x, ac_int<OW, OS>& sqrt)
   {
      const int RW = (XW + 1) / 2;
      // masks used only to hint synthesis on precision
      ac_int<RW + 2, false> mask_d = 0;

      ac_int<RW + 2, false> d = 0;
      ac_int<RW, false> r = 0;
      ac_int<2 * RW, false> z = x;

      // needs to pick 2 bits of z for each iteration starting from
      // the 2 MSB bits. Inside loop, z will be shifted left by 2 each
      // iteration. The bits of interest are always on the same
      // position (z_shift+1 downto z_shift)
      unsigned int z_shift = (RW - 1) * 2;

      for(int i = RW - 1; i >= 0; i--)
      {
         r <<= 1;

         mask_d = (mask_d << 2) | 0x3;
         d = mask_d & (d << 2) | ((z >> z_shift) & 0x3);

         ac_int<RW + 2, false> t = d - ((((ac_int<RW + 1, false>)r) << 1) | 0x1);
         if(!t[RW + 1])
         { // since t is unsigned, look at MSB
            r |= 0x1;
            d = mask_d & t;
         }
         z <<= 2;
      }
      sqrt = r;
   }

   //=========================================================================
   // Function: ac_sqrt (for ac_fixed)
   //
   // Description:
   //    Calculation of square root of real inputs, passed as ac_fixed
   //    variables.
   //    + input: unsigned ac_fixed argument
   //    + output: ac_fixed square root
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_sqrt(ac_fixed<XW, XI, false, XQ, XO> x, ac_fixed<OW, OI, false, OQ, OO>& sqrt)
   {
      const int RBIT = (OQ == AC_TRN || OQ == AC_TRN_ZERO) ? 0 : 1;
      const int OF = (OW - OI) + RBIT;

      const int RI = (XI + 1) / 2;
      if(RI - 1 < -OF)
      {
         // MSB of result is smaller than LSB of requested output
         sqrt = 0;
         return;
      }

      // max is used to avoid compilation problems with non pos bitwidth
      const int RF = AC_MAX(OF, -RI + 1); // OF may be negative
      const int RW = RI + RF;

      // store relevant bits of x in z
      const int ZF = 2 * AC_MIN((XW - XI + 1) / 2, RF);
      const int ZW = 2 * RI + ZF;
      ac_fixed<ZW, ZW, false> z_fx;
      ac_math::ac_shift_left(x, ZF, z_fx);
      ac_int<ZW, false> z = z_fx.template slc<ZW>(0);

      // masks used only to hint synthesis on precision
      ac_int<RW + 2, false> mask_d = 0;

      ac_int<RW + 2, false> d = 0;
      ac_int<RW, false> r = 0;

      // needs to pick 2 bits of z for each iteration starting from
      // the 2 MSB bits. Inside loop, z will be shifted left by 2 each
      // iteration. The bits of interest are always on the same
      // position (z_shift+1 downto z_shift)
      unsigned int z_shift = ZW - 2;

      for(int i = RW - 1; i >= 0; i--)
      {
         r <<= 1;

         mask_d = (mask_d << 2) | 0x3;
         d = mask_d & (d << 2) | ((z >> z_shift) & 0x3);

         ac_int<RW + 2, false> t = d - ((((ac_int<RW + 1, false>)r) << 1) | 0x1);
         if(!t[RW + 1])
         { // since t is unsigned, look at MSB
            r |= 0x1;
            d = mask_d & t;
         }
         z <<= 2;
      }

      auto r2 = (ac_fixed<RW + 1, RW, false>)r;
      if(OQ == AC_RND_ZERO || OQ == AC_RND_MIN_INF || OQ == AC_RND_CONV || OQ == AC_RND_CONV_ODD)
      {
         bool rem = (d != 0) || ((z >> 2 * RW) != 0);
         if(ZF < (XW - XI))
         {
            // max is to used to avoid compilation problems with non pos bitwidth
            const int rbits = AC_MAX((XW - XI) - ZF, 1);
            ac_fixed<rbits, -ZF, false> zr = x;
            rem |= !!zr;
         }
         r2[0] = rem ? 1 : 0;
      }
      ac_math::ac_shift_right(r2, RF, sqrt);
   }

} // namespace ac_math

#endif
