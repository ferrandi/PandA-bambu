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
// File: ac_shift.h
//
// Description:
//    - Shifts:
//        + inputs: argument, shift count
//        + output: shifted result
//
//        * Fixed Point Unsigned, signed shift value
//          void ac_shift_right(ac_fixed<XW,XI,false,XQ,XO> x, int n,
//                              ac_fixed<OW,OI,false,OQ,OO> &sr)
//          void ac_shift_left(ac_fixed<XW,XI,false,XQ,XO> x, int n,
//                             ac_fixed<OW,OI,false,OQ,OO> &sl)
//
//        * Fixed Point Unsigned, unsigned shift value
//          void ac_shift_right(ac_fixed<XW,XI,false,XQ,XO> x, unsigned int n,
//                              ac_fixed<OW,OI,false,OQ,OO> &sr)
//          void ac_shift_left(ac_fixed<XW,XI,false,XQ,XO> x, unsigned int n,
//                             ac_fixed<OW,OI,false,OQ,OO> &sl)
//
//        * Fixed Point Signed, signed shift value
//          void ac_shift_right(ac_fixed<XW,XI,true,XQ,XO> x, int n,
//                              ac_fixed<OW,OI,true,OQ,OO> &sr)
//          void ac_shift_left(ac_fixed<XW,XI,true,XQ,XO> x, int n,
//                             ac_fixed<OW,OI,true,OQ,OO> &sl)
//
//        * Fixed Point Signed, unsigned shift value
//          void ac_shift_right(ac_fixed<XW,XI,true,XQ,XO> x, unsigned int n,
//                              ac_fixed<OW,OI,true,OQ,OO> &sr)
//          void ac_shift_left(ac_fixed<XW,XI,true,XQ,XO> x, unsigned int n,
//                             ac_fixed<OW,OI,true,OQ,OO> &sl)
//
//        * Complex
//          void ac_shift_right(ac_complex<XT> x, unsigned int n,
//                              ac_complex<OT> &sr)
//          void ac_shift_left(ac_complex<XT> x, unsigned int n,
//                             ac_complex<OT> &sl)
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_shift.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<20, 11, false, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<24, 14, false, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output,
//      unsigned int &n
//    )
//    {
//      ac_shift_right(input, n, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input = 1.5;
//      output_type output;
//      unsigned int n = 5;
//      CCS_DESIGN(project)(input, output, n);
//      CCS_RETURN (0);
//    }
//    #endif
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_SHIFT_H_
#define _INCLUDED_AC_SHIFT_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>

//=========================================================================
// Function: ac_shift_right (for unsigned ac_fixed, unsigned shift value)
//
// Description:
//    - Shifts:
//        + inputs: unsigned ac_fixed argument, unsigned int shift count
//        + output: right-shifted, unsigned ac_fixed result
//
//-------------------------------------------------------------------------

namespace ac_math
{
   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_right(ac_fixed<XW, XI, false, XQ, XO> x, unsigned int n, ac_fixed<OW, OI, false, OQ, OO>& sr)
   {
      const int R_BIT = (OQ == AC_TRN || OQ == AC_TRN_ZERO) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND || OQ == AC_RND_INF) ? 0 : 1;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      // Since this is a unidirectional shift in the right direction, the intermediate type only needs the same number
      // of integer bits as in the input.
      const int TI = XI;
      const int TW = TI + TF;
      unsigned un = 0x7FFFFFFF & n;
      ac_fixed<TW, TI, false> t = ((ac_fixed<TW, TI, false>)x) >> un;

      ac_fixed<TW + R_HALF, TI, false> t2 = t;

      if(R_HALF)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 <<= un;
         ac_fixed<XW, XI, false> mask = ~(ac_fixed<XW, XI, false>)m1;
         t2[0] = !!(x & mask);
      }
      sr = t2;
   }

   //=========================================================================
   // Function: ac_shift_right (for unsigned ac_fixed, signed shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: unsigned ac_fixed argument, signed int shift count
   //        + output: right-shifted, unsigned ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_right(ac_fixed<XW, XI, false, XQ, XO> x, int n, ac_fixed<OW, OI, false, OQ, OO>& sr)
   {
      const int R_BIT = (OQ == AC_TRN || OQ == AC_TRN_ZERO) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND || OQ == AC_RND_INF) ? 0 : 1;
      const int S_OVER = (OO == AC_WRAP) ? 0 : 1;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, false> t = ((ac_fixed<TW, TI, false>)x) >> n;

      ac_fixed<TW + R_HALF + S_OVER, TI + S_OVER, false> t2 = t;

      if(R_HALF || S_OVER)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 <<= n;
         ac_fixed<XW, XI, false> mask = ~(ac_fixed<XW, XI, false>)m1;
         if(n >= 0)
         {
            if(R_HALF)
            {
               t2[0] = !!(x & mask);
            }
         }
         else
         {
            if(S_OVER)
            {
               t2[TW + R_HALF + S_OVER - 1] = !!(x & mask);
            }
         }
      }
      sr = t2;
   }

   //=========================================================================
   // Function: ac_shift_right (for signed ac_fixed, unsigned shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: signed ac_fixed argument, unsigned int shift count
   //        + output: right-shifted, signed ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_right(ac_fixed<XW, XI, true, XQ, XO> x, unsigned int n, ac_fixed<OW, OI, true, OQ, OO>& sr)
   {
      const int R_BIT = (OQ == AC_TRN) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND) ? 0 : 1;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      // Since this is a unidirectional shift in the right direction, the intermediate type only needs the same number
      // of integer bits as in the input.
      const int TI = XI;
      const int TW = TI + TF;
      ac_fixed<TW, TI, true> t = ((ac_fixed<TW, TI, true>)x) >> n;
      unsigned un = 0x7FFFFFFF & n;

      ac_fixed<TW + R_HALF, TI, true> t2 = t;

      if(R_HALF)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 <<= un;
         ac_fixed<XW, XI, true> mask = ~(ac_fixed<XW, XI, true>)m1;
         t2[0] = !!(x & mask);
      }
      sr = t2;
   }

   //=========================================================================
   // Function: ac_shift_right (for signed ac_fixed, signed shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: signed ac_fixed argument, signed int shift count
   //        + output: right-shifted, signed ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_right(ac_fixed<XW, XI, true, XQ, XO> x, int n, ac_fixed<OW, OI, true, OQ, OO>& sr)
   {
      const int R_BIT = (OQ == AC_TRN) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND) ? 0 : 1;
      const int S_OVER = (OO == AC_WRAP) ? 1 : 2;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, true> t = ((ac_fixed<TW, TI, true>)x) >> n;

      ac_fixed<TW + R_HALF + S_OVER, TI + S_OVER, true> t2 = t;

      if(R_HALF || S_OVER == 2)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, true>)0;
         m1 <<= n;
         ac_fixed<XW, XI, true> mask = ~(ac_fixed<XW, XI, true>)m1;
         if(n >= 0)
         {
            if(R_HALF)
            {
               t2[0] = !!(x & mask);
            }
         }
         else
         {
            t2[TW + R_HALF + S_OVER - 1] = x[XW - 1];
            if(S_OVER == 2 && !!mask)
            {
               t2[TW + R_HALF + S_OVER - 2] = !!(x & mask) && !(!!(~x & mask) && x[XW - 1]);
            }
         }
      }
      sr = t2;
   }

   //=========================================================================
   // Function: ac_shift_left (for unsigned ac_fixed, unsigned shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: unsigned ac_fixed argument, unsigned int shift count
   //        + output: left-shifted, unsigned ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_left(ac_fixed<XW, XI, false, XQ, XO> x, unsigned int n, ac_fixed<OW, OI, false, OQ, OO>& sl)
   {
      const int S_OVER = (OO == AC_WRAP) ? 0 : 1;
      // Since this is a unidirectional shift in the left direction, the intermediate type only needs the same number of
      // fractional bits as in the input.
      const int TF = XW - XI;
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, false> t = ((ac_fixed<TW, TI, false>)x) << n;

      ac_fixed<TW + S_OVER, TI + S_OVER, false> t2 = t;

      if(S_OVER)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 >>= n;
         ac_fixed<XW, XI, false> mask = ~(ac_fixed<XW, XI, false>)m1;
         t2[TW + S_OVER - 1] = !!(x & mask);
      }
      sl = t2;
   }

   //=========================================================================
   // Function: ac_shift_left (for unsigned ac_fixed, signed shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: unsigned ac_fixed argument, signed int shift count
   //        + output: left-shifted, unsigned ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_left(ac_fixed<XW, XI, false, XQ, XO> x, int n, ac_fixed<OW, OI, false, OQ, OO>& sl)
   {
      const int R_BIT = (OQ == AC_TRN || OQ == AC_TRN_ZERO) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND || OQ == AC_RND_INF) ? 0 : 1;
      const int S_OVER = (OO == AC_WRAP) ? 0 : 1;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, false> t = ((ac_fixed<TW, TI, false>)x) << n;

      ac_fixed<TW + R_HALF + S_OVER, TI + S_OVER, false> t2 = t;

      if(R_HALF || S_OVER)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 >>= n;
         ac_fixed<XW, XI, false> mask = ~(ac_fixed<XW, XI, false>)m1;
         if(n < 0)
         {
            if(R_HALF)
            {
               t2[0] = !!(x & mask);
            }
         }
         else
         {
            if(S_OVER)
            {
               t2[TW + R_HALF + S_OVER - 1] = !!(x & mask);
            }
         }
      }
      sl = t2;
   }

   //=========================================================================
   // Function: ac_shift_left (for signed ac_fixed, unsigned shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: signed ac_fixed argument, unsigned int shift count
   //        + output: left-shifted, signed ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_left(ac_fixed<XW, XI, true, XQ, XO> x, unsigned int n, ac_fixed<OW, OI, true, OQ, OO>& sl)
   {
      const int S_OVER = (OO == AC_WRAP) ? 1 : 2;
      // Since this is a unidirectional shift in the left direction, the intermediate type only needs the same number of
      // fractional bits as in the input.
      const int TF = XW - XI;
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, true> t = ((ac_fixed<TW, TI, true>)x) << n;

      ac_fixed<TW + S_OVER, TI + S_OVER, true> t2 = t;

      if(S_OVER == 2)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 >>= n;
         ac_fixed<XW, XI, true> mask = ~(ac_fixed<XW, XI, true>)m1;
         t2[TW + S_OVER - 1] = x[XW - 1];
         if(mask != 0)
         {
            t2[TW + S_OVER - 2] = !!(x & mask) && !(!!(~x & mask) && x[XW - 1]);
         }
      }
      sl = t2;
   }

   //=========================================================================
   // Function: ac_shift_left (for signed ac_fixed, signed shift value)
   //
   // Description:
   //    - Shifts:
   //        + inputs: signed ac_fixed argument, signed int shift count
   //        + output: left-shifted, signed ac_fixed result
   //
   //-------------------------------------------------------------------------

   template <int XW, int XI, ac_q_mode XQ, ac_o_mode XO, int OW, int OI, ac_q_mode OQ, ac_o_mode OO>
   void ac_shift_left(ac_fixed<XW, XI, true, XQ, XO> x, int n, ac_fixed<OW, OI, true, OQ, OO>& sl)
   {
      const int R_BIT = (OQ == AC_TRN) ? 0 : 1;
      const int R_HALF = (R_BIT == 0 || OQ == AC_RND) ? 0 : 1;
      const int S_OVER = (OO == AC_WRAP) ? 1 : 2;
      const int TF = AC_MAX(XW - XI, OW - OI + R_BIT);
      const int TI = AC_MAX(XI, OI);
      const int TW = TI + TF;
      ac_fixed<TW, TI, true> t = ((ac_fixed<TW, TI, true>)x) << n;

      ac_fixed<TW + R_HALF + S_OVER, TI + S_OVER, true> t2 = t;

      if(R_HALF || S_OVER == 2)
      {
         ac_fixed<TW, TI, false> m1 = ~(ac_fixed<TW, TI, false>)0;
         m1 >>= n;
         ac_fixed<XW, XI, true> mask = ~(ac_fixed<XW, XI, true>)m1;
         if(n < 0)
         {
            if(R_HALF)
            {
               t2[0] = !!(x & mask);
            }
         }
         else
         {
            t2[TW + R_HALF + S_OVER - 1] = x[XW - 1];
            if(S_OVER == 2 && !!mask)
            {
               t2[TW + R_HALF + S_OVER - 2] = !!(x & mask) && !(!!(~x & mask) && x[XW - 1]);
            }
         }
      }
      sl = t2;
   }

   //=========================================================================
   // Function: ac_shift_right (for ac_complex)
   //
   // Description:
   //    - Shifts:
   //        + inputs: ac_complex argument, unsigned int shift count
   //        + output: right-shifted ac_complex result
   //
   //-------------------------------------------------------------------------

   template <typename XT, typename OT>
   void ac_shift_right(ac_complex<XT> x, unsigned int n, ac_complex<OT>& sr)
   {
      ac_shift_right(x.r(), n, sr._r);
      ac_shift_right(x.i(), n, sr._i);
   }

   //=========================================================================
   // Function: ac_shift_left (for ac_complex)
   //
   // Description:
   //    - Shifts:
   //        + inputs: ac_complex argument, unsigned int shift count
   //        + output: left-shifted ac_complex result
   //
   //-------------------------------------------------------------------------

   template <typename XT, typename OT>
   void ac_shift_left(ac_complex<XT> x, unsigned int n, ac_complex<OT>& sl)
   {
      ac_shift_left(x.r(), n, sl._r);
      ac_shift_left(x.i(), n, sl._i);
   }
} // namespace ac_math

#endif
