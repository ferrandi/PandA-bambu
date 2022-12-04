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
// File: ac_div.h
//
// Description:
//    - Division:
//      + inputs: dividend, divisor
//      + outputs: quotient, returns true if remainder is nonzero (for
//          fixed point, the remainder depends on the LSB precision of
//          the quotient).
//          For integer divisions, remainder output is also available.
//
//      * Integer Unsigned
//        bool ac_div(ac_int<NW,false> dividend, ac_int<DW,false> divisor,
//                 ac_int<QW,false> &quotient, ac_int<RW,false> &remainder)
//        bool ac_div(ac_int<NW,false> dividend, ac_int<DW,false> divisor,
//                 ac_int<QW,false> &quotient)
//
//      * Integer Signed
//        bool ac_div(ac_int<NW,true> dividend, ac_int<DW,true> divisor,
//                 ac_int<QW,true> &quotient, ac_int<RW,true> &remainder)
//        bool ac_div(ac_int<NW,true> dividend, ac_int<DW,true> divisor,
//                 ac_int<QW,true> &quotient)
//
//      * Fixed Point unsigned
//        bool ac_div(ac_fixed<NW,NI,false,NQ,NO> dividend,
//                 ac_fixed<DW,DI,false,DQ,DO> divisor,
//                 ac_fixed<QW,QI,false,QQ,QO> &quotient)
//
//      * Fixed Point signed
//        bool ac_div(ac_fixed<NW,NI,true,NQ,NO> dividend,
//                 ac_fixed<DW,DI,true,DQ,DO> divisor,
//                 ac_fixed<QW,QI,true,QQ,QO> &quotient)
//
//      * Floating Point
//        bool ac_div(ac_float<NW,NI,NE,NQ> dividend,
//                 ac_float<DW,DI,DE,DQ> divisor,
//                 ac_float<QW,QI,QE,QQ> &quotient)
//
//      * Complex
//        bool ac_div(ac_complex<NT> dividend,
//                 ac_complex<DT> divisor,
//                 ac_complex<QT> &quotient)
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_math/ac_div.h>
//    using namespace ac_math;
//
//    typedef ac_fixed<16, 8, true, AC_RND, AC_SAT> num_type;
//    typedef ac_fixed<17, 9, true, AC_RND, AC_SAT> den_type;
//    typedef ac_fixed<28, 16, true, AC_RND, AC_SAT> quo_type;
//
//    #pragma hls_design top
//    void project(
//      const num_type &num,
//      const den_type &den,
//      quo_type &quo
//    )
//    {
//      //to replicate: quo = num / den;
//      ac_div(num, den, quo);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      num_type num = 1.5;
//      den_type den = -2.5;
//      quo_type quo;
//      CCS_DESIGN(project)(num, den, quo);
//      CCS_RETURN (0);
//    }
//    #endif
//
//*****************************************************************************************

#ifndef _INCLUDED_AC_DIV_H_
#define _INCLUDED_AC_DIV_H_

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>
#include <ac_int.h>

// Include header file for ac_shift functions
#include <ac_math/ac_shift.h>

//=========================================================================
// Function: ac_div (for unsigned ac_int inputs and outputs)
//
// Description:
//    - Division
//        + inputs: unsigned ac_int dividend and divisor
//        + outputs: unsigned ac_int quotient and/or remainder
//
//-------------------------------------------------------------------------

namespace ac_math
{
#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int DW, int QW, int RW>
   bool ac_div(ac_int<NW, false> dividend, ac_int<DW, false> divisor, ac_int<QW, false>& quotient,
               ac_int<RW, false>& remainder)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      ac_int<QW, false> Q = 0;
      ac_int<DW + 1, true> R = 0;
      ac_int<DW + 1, true> neg_divisor = -divisor;
      for(int i = 0; i < NW; i++)
      {
         // take MSB of divd, shift it in from right to R
         R = ((R << 1) | ((dividend >> (NW - 1)) & 1)) + ((R < 0) ? (ac_int<DW + 1, true>)divisor : neg_divisor);
         Q = (Q << 1) | ((R >= 0) & 1);
         dividend <<= 1;
      }
      if(R < 0)
      {
         R += divisor;
      }

      // In case the AC_ASSERT isn't activated and the divisor is still zero, saturate the quotient output.
      if(divisor == 0)
      {
         Q.template set_val<AC_VAL_MAX>();
      }

      quotient = Q;
      remainder = R;
      return !!R;
   }

#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int DW, int QW>
   bool ac_div(ac_int<NW, false> dividend, ac_int<DW, false> divisor, ac_int<QW, false>& quotient)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      ac_int<DW, false> remainder;
      ac_div(dividend, divisor, quotient, remainder);

      // The quotient output will have already been saturated from the above ac_div function call.
      // Hence, there's no need to separately saturate it here.

      return !!remainder;
   }

   //=========================================================================
   // Function: ac_div (for signed ac_int inputs and outputs)
   //
   // Description:
   //    - Division:
   //        + inputs: signed ac_int dividend and divisor
   //        + outputs: signed ac_int quotient and/or remainder
   //
   //-------------------------------------------------------------------------

#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int DW, int QW, int RW>
   bool ac_div(ac_int<NW, true> dividend, ac_int<DW, true> divisor, ac_int<QW, true>& quotient,
               ac_int<RW, true>& remainder)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      bool neg_dividend = dividend < 0;
      ac_int<NW, false> uN = neg_dividend ? (ac_int<NW, false>)-dividend : (ac_int<NW, false>)dividend;
      bool neg_divisor = divisor < 0;
      ac_int<DW, false> uD = neg_divisor ? (ac_int<DW, false>)-divisor : (ac_int<DW, false>)divisor;
      ac_int<QW, false> uQ;
      ac_int<DW, false> uR;
      ac_div(uN, uD, uQ, uR);
      ac_int<QW, true> quotient_temp = neg_dividend == neg_divisor ? (ac_int<QW, true>)uQ : (ac_int<QW, true>)-uQ;

      // If the AC_ASSERT wasn't activated and the divisor is still zero, saturate to the min or max val. based
      // on whether the dividend is negative or positive.
      if(divisor == 0)
      {
         if(neg_dividend)
         {
            quotient_temp.template set_val<AC_VAL_MIN>();
         }
         else
         {
            quotient_temp.template set_val<AC_VAL_MAX>();
         }
      }

      quotient = quotient_temp;

      ac_int<RW, true> rem = neg_dividend ? (ac_int<DW, true>)-uR : (ac_int<DW, true>)uR;
      remainder = rem;
      return !!rem;
   }

#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int DW, int QW>
   bool ac_div(ac_int<NW, true> dividend, ac_int<DW, true> divisor, ac_int<QW, true>& quotient)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      ac_int<DW, true> remainder;
      ac_div(dividend, divisor, quotient, remainder);

      // The quotient output will have already been saturated from the above ac_div function call.
      // Hence, there's no need to separately saturate it here.

      return !!remainder;
   }

   //=========================================================================
   // Function: ac_div (for unsigned ac_fixed inputs and outputs)
   //
   // Description:
   //    - Division:
   //        + inputs: unsigned ac_fixed dividend and divisor
   //        + output: unsigned ac_fixed quotient
   //
   //-------------------------------------------------------------------------

#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int NI, ac_q_mode NQ, ac_o_mode NO, int DW, int DI, ac_q_mode DQ, ac_o_mode DO, int QW, int QI,
             ac_q_mode QQ, ac_o_mode QO>
   bool ac_div(ac_fixed<NW, NI, false, NQ, NO> dividend, ac_fixed<DW, DI, false, DQ, DO> divisor,
               ac_fixed<QW, QI, false, QQ, QO>& quotient)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      // relevant bits for Q

      const int RBIT = (QQ == AC_TRN || QQ == AC_TRN_ZERO) ? 0 : 1;
      const int QF = (QW - QI) + RBIT;
      const int ZI = NI + (DW - DI);

      ac_fixed<QW, QI, false, QQ, QO> quotient_temp;

      if(ZI - 1 < -QF)
      {
         // MSB of result is smaller than LSB of requested output
         quotient_temp = 0;
         return !!dividend;
      }
      // max is to used to avoid compilation problems with non pos bitwidth
      const int ZF = AC_MAX(QF, -ZI + 1);
      const int ZW = ZI + ZF;

      ac_fixed<NW, NW, false> N_fx;
      ac_math::ac_shift_left(dividend, NW - NI, N_fx);
      ac_int<NW, false> N = N_fx.template slc<NW>(0);
      ac_fixed<DW, DW, false> D_fx;
      ac_math::ac_shift_left(divisor, DW - DI, D_fx);
      ac_int<DW, false> D = D_fx.template slc<DW>(0);

      ac_int<ZW, false> Q = 0;
      ac_int<DW + 1, true> R = 0;
      ac_int<DW + 1, true> neg_D = -D;
      for(int i = 0; i < ZW; i++)
      {
         // take MSB of N, shift it in from right to R
         R = ((R << 1) | ((N >> (NW - 1)) & 1)) + ((R < 0) ? (ac_int<DW + 1, true>)D : neg_D);
         Q = (Q << 1) | ((R >= 0) & 1);
         N <<= 1;
      }
      if(R < 0)
      {
         R += D;
      }

      bool rem = (R != 0) || ((N >> ZW) != 0);

      auto Q_fx = (ac_fixed<ZW + 1, ZW, false>)Q;
      if(QQ == AC_RND_ZERO || QQ == AC_RND_MIN_INF || QQ == AC_RND_CONV || QQ == AC_RND_CONV_ODD)
      {
         Q_fx[0] = rem;
      }

      ac_math::ac_shift_right(Q_fx, (NW - NI) - (DW - DI) + (ZW - NW), quotient_temp);

      // In case the AC_ASSERT isn't activated and the divisor is still zero, saturate the quotient output.
      if(divisor == 0)
      {
         quotient_temp.template set_val<AC_VAL_MAX>();
      }

      quotient = quotient_temp;

      return rem || (RBIT && Q[0]);
   }

   //=========================================================================
   // Function: ac_div (for signed ac_fixed inputs and outputs)
   //
   // Description:
   //    - Division:
   //        + inputs: signed ac_fixed dividend and divisor
   //        + output: signed ac_fixed quotient
   //
   //-------------------------------------------------------------------------

#pragma calypto_flag DIV0_CHECK DIVISOR = 2
   template <int NW, int NI, ac_q_mode NQ, ac_o_mode NO, int DW, int DI, ac_q_mode DQ, ac_o_mode DO, int QW, int QI,
             ac_q_mode QQ, ac_o_mode QO>
   bool ac_div(ac_fixed<NW, NI, true, NQ, NO> dividend, ac_fixed<DW, DI, true, DQ, DO> divisor,
               ac_fixed<QW, QI, true, QQ, QO>& quotient)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      const int ZI = (QO == AC_WRAP) ? QI : NI + (DW - DI);
      const int ZW = ZI + (QW - QI);

      ac_fixed<NW, NI, true> N = dividend;
      ac_fixed<DW, DI, true> D = divisor;

      bool neg_N = N < 0;
      ac_fixed<NW, NI, false> uN = neg_N ? (ac_fixed<NW, NI, false>)-N : (ac_fixed<NW, NI, false>)N;
      bool neg_D = D < 0;
      ac_fixed<DW, DI, false> uD = neg_D ? (ac_fixed<DW, DI, false>)-D : (ac_fixed<DW, DI, false>)D;

      bool has_rem;
      if(QQ == AC_RND_ZERO || QQ == AC_RND_INF || QQ == AC_RND_CONV || QQ == AC_RND_CONV_ODD || QQ == AC_TRN_ZERO)
      {
         ac_fixed<ZW, ZI, false, QQ> uQ;
         ac_fixed<ZW + 1, ZI + 1, true, QQ> Q;
         has_rem = ac_div(uN, uD, uQ);
         if(neg_N == neg_D)
         {
            Q = uQ;
         }
         else
         {
            Q = -uQ;
         }
         quotient = Q;
      }
      else
      {
         const int RBIT = (QQ == AC_TRN) ? 0 : 1;
         ac_fixed<ZW + RBIT, ZI, false> uQ;
         ac_fixed<ZW + RBIT + 2, ZI + 1, true> Q;
         has_rem = ac_div(uN, uD, uQ);
         if(neg_N == neg_D)
         {
            Q = uQ;
            if(QQ == AC_RND_MIN_INF)
            {
               Q[0] = has_rem ? 1 : 0;
            }
         }
         else
         {
            ac_fixed<ZW + RBIT, ZI, false> lsb = 0;
            lsb[0] = has_rem && QQ != AC_RND_MIN_INF ? 1 : 0;
            Q = -(uQ + lsb);
         }
         quotient = Q;
         has_rem |= RBIT && uQ[0];
      }

      // If the AC_ASSERT wasn't activated and the divisor is still zero, saturate to the min or max val. based
      // on whether the dividend is negative or positive.
      if(divisor == 0)
      {
         if(neg_N)
         {
            quotient.template set_val<AC_VAL_MIN>();
         }
         else
         {
            quotient.template set_val<AC_VAL_MAX>();
         }
      }

      return has_rem;
   }

   //=========================================================================
   // Function: ac_div (for ac_float inputs and outputs)
   //
   // Description:
   //    - Division:
   //        + inputs: ac_float dividend and divisor
   //        + output: ac_float quotient
   //
   //-------------------------------------------------------------------------

   template <int NW, int NI, int NE, ac_q_mode NQ, int DW, int DI, int DE, ac_q_mode DQ, int QW, int QI, int QE,
             ac_q_mode QQ>
   bool ac_div(ac_float<NW, NI, NE, NQ> dividend, ac_float<DW, DI, DE, DQ> divisor, ac_float<QW, QI, QE, QQ>& quotient)
   {
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!divisor, "Division by zero.");
#endif
      const int STI = NI + (DW - DI);
      const int STW = STI + (QW - QI);
      const int STE = AC_MAX(NE, DE) + 1;
      ac_fixed<STW, STI, true> tm;
      bool has_rem = ac_div(dividend.m, divisor.m, tm);
      ac_int<STE, true> te = dividend.exp() - divisor.exp();
      ac_float<STW, STI, STE> qt(tm, te, true);
      quotient = qt;

      // If the AC_ASSERT wasn't activated and the divisor is still zero, saturate to the min or max val. based
      // on whether the dividend is negative or positive.
      if(divisor.m == 0)
      {
         if(dividend.m < 0)
         {
            quotient.template set_val<AC_VAL_MIN>();
         }
         else
         {
            quotient.template set_val<AC_VAL_MAX>();
         }
      }

      return has_rem;
   }

   //=========================================================================
   // Function: ac_div (for ac_complex inputs and outputs)
   //
   // Description:
   //    - Division:
   //        + inputs: ac_complex dividend and divisor
   //        + output: ac_complex quotient
   //
   //-------------------------------------------------------------------------

   // Complex division with ac_complex::operator*.
   template <typename NT, typename DT, typename QT>
   bool ac_div(ac_complex<NT> dividend, ac_complex<DT> divisor, ac_complex<QT>& quotient)
   {
      // Assuming notation, X = Re(X) + Im(X)j = x + x'j
      // X/D = (x + x'j)/(d + d'j) = (x + x'j)(d - d'j)/(d^2 + d'^2)
      ac_complex<NT>& X = dividend;
      ac_complex<DT>& D = divisor;
      ac_complex<QT> Q = quotient;
      enum
      {
         DTW = DT::width,
         DTI = DT::i_width,
         DTS = DT::sign
      };
      // Use a macro to activate the AC_ASSERT
      // If AC_ASSERT is activated: the program will stop running as soon as a zero divisor
      // is encountered.
#ifdef ASSERT_ON_INVALID_INPUT
      AC_ASSERT(!!D.r() || !!D.i(), "Division by zero.");
#endif
      // Create signed sum-of-squares result for the following divs.
      ac_fixed<2 * DTW + 2, 2 * DTI + 2, true> s = D.r() * D.r() + D.i() * D.i();
      typedef typename ac_complex<DT>::rt_unary::neg DCT;
      DCT D_conj(D.r(), -D.i());
      typename ac_complex<QT>::template rt_T<DCT>::mult prod = X * D_conj;
      bool has_rem = ac_div(prod.r(), s, Q._r);
      has_rem |= ac_div(prod.i(), s, Q._i);

      // If the AC_ASSERT wasn't activated and the divisor is still zero, saturate the real
      // and imaginary parts of the output to the min or max val. based on
      // whether the real/imaginary parts of "prod" are negative or positive.
      if(D.r() == 0 && D.i() == 0)
      {
         if(prod.r() < 0)
         {
            Q.r().template set_val<AC_VAL_MIN>();
         }
         else
         {
            Q.r().template set_val<AC_VAL_MAX>();
         }
         if(prod.i() < 0)
         {
            Q.i().template set_val<AC_VAL_MIN>();
         }
         else
         {
            Q.i().template set_val<AC_VAL_MAX>();
         }
      }

      quotient = Q;

      return has_rem;
   }

} // namespace ac_math

#endif
