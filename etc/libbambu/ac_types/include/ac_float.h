/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 3.7                                                 *
 *                                                                        *
 *  Release Date    : Tue May 30 14:25:58 PDT 2017                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 3.7.2                                               *
 *                                                                        *
 *  Copyright 2013-2016, Mentor Graphics Corporation,                     *
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
 *  This file was modified by PandA team from Politecnico di Milano to    *
 *  generate efficient hardware for the PandA/bambu HLS tool.             *
 *  The API remains the same as defined by Mentor Graphics.               *
 *                                                                        *
 *************************************************************************/

//  Source:          ac_float.h
//  Description:     class for floating point operation handling in C++
//  Original Author: Andres Takach, Ph.D.
//  Modified by:     Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
#ifndef __AC_FLOAT_H
#define __AC_FLOAT_H

#include <ac_fixed.h>

#ifndef __BAMBU__
#include <cmath>
#endif

#if(defined(__GNUC__) && __GNUC__ < 3 && !defined(__EDG__))
#error GCC version 3 or greater is required to include this header file
#endif

#if(defined(_MSC_VER) && _MSC_VER < 1400 && !defined(__EDG__))
#error Microsoft Visual Studio 8 or newer is required to include this header file
#endif

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(push)
#pragma warning(disable : 4003 4127 4308 4365 4514 4800)
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wbitwise-op-parentheses"
#endif

// for safety
#if(defined(E) || defined(WF) || defined(IF) || defined(SF))
#error One or more of the following is defined: E, WF, IF, SF. Definition conflicts with their usage as template parameters.
#error DO NOT use defines before including third party header files.
#endif

#define AC_FL(v) ac_float<W##v, I##v, E##v, Q##v>
#define AC_FL0(v) ac_float<W##v, I##v, E##v>
#define AC_FL_T(v) int W##v, int I##v, int E##v, ac_q_mode Q##v
#define AC_FL_TV(v) W##v, I##v, E##v, Q##v
#define AC_FL_T0(v) int W##v, int I##v, int E##v
#define AC_FL_TV0(v) W##v, I##v, E##v

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

   template <int W, int I, int E, ac_q_mode Q = AC_TRN>
   class ac_float;

   namespace ac_private
   {
      typedef ac_float<54, 2, 11> ac_float_cdouble_t;
      typedef ac_float<25, 2, 8> ac_float_cfloat_t;

      template <typename T>
      struct rt_ac_float_T
      {
         template <AC_FL_T0()>
         struct op1
         {
            typedef AC_FL0() fl_t;
            typedef typename T::template rt_T<fl_t>::mult mult;
            typedef typename T::template rt_T<fl_t>::plus plus;
            typedef typename T::template rt_T<fl_t>::minus2 minus;
            typedef typename T::template rt_T<fl_t>::minus minus2;
            typedef typename T::template rt_T<fl_t>::logic logic;
            typedef typename T::template rt_T<fl_t>::div2 div;
            typedef typename T::template rt_T<fl_t>::div div2;
         };
      };
      // specializations after definition of ac_float

      __FORCE_INLINE ac_float_cdouble_t double_to_ac_float(double d);
      __FORCE_INLINE ac_float_cfloat_t float_to_ac_float(float f);
   } // namespace ac_private

   //////////////////////////////////////////////////////////////////////////////
   //  ac_float
   //////////////////////////////////////////////////////////////////////////////

   template <AC_FL_T()>
   class ac_float
   {
      enum
      {
         NO_UN = true,
         S = true,
         S2 = true,
         SR = true
      };

    public:
      typedef ac_fixed<W, I, S> mant_t;
      typedef ac_int<E, true> exp_t;
      mant_t m;
      exp_t e;

      void set_mantissa(const ac_fixed<W, I, S>& man)
      {
         m = man;
      }
      void set_exp(const ac_int<E, true>& exp)
      {
         if(E)
            e = exp;
      }

    private:
      __FORCE_INLINE bool is_neg() const
      {
         return m < 0; // is_neg would be more efficient
      }

      enum
      {
         NZ_E = !!E,
         MIN_EXP = static_cast<int>(static_cast<unsigned>(-NZ_E) << (E - NZ_E)),
         MAX_EXP = (1 << (E - NZ_E)) - 1
      };

    public:
      static const int width = W;
      static const int i_width = I;
      static const int e_width = E;
      static const bool sign = S;
      static const ac_q_mode q_mode = Q;
      static const ac_o_mode o_mode = AC_SAT;

      template <AC_FL_T0(2)>
      struct rt
      {
         enum
         {
            // need to validate
            F = W - I,
            F2 = W2 - I2,
            mult_w = W + W2,
            mult_i = I + I2,
            mult_e = AC_MAX(E, E2) + 1,
            mult_s = S || S2,
            plus_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1 + AC_MAX(F, F2),
            plus_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1,
            plus_e = AC_MAX(E, E2),
            plus_s = S || S2,
            minus_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1 + AC_MAX(F, F2),
            minus_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1,
            minus_e = AC_MAX(E, E2),
            minus_s = true,
            div_w = W + AC_MAX(W2 - I2, 0) + S2,
            div_i = I + (W2 - I2) + S2,
            div_e = AC_MAX(E, E2) + 1,
            div_s = S || S2,
            logic_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + AC_MAX(F, F2),
            logic_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)),
            logic_s = S || S2,
            logic_e = AC_MAX(E, E2)
         };
         typedef ac_float<mult_w, mult_i, mult_e> mult;
         typedef ac_float<plus_w, plus_i, plus_e> plus;
         typedef ac_float<minus_w, minus_i, minus_e> minus;
         typedef ac_float<logic_w, logic_i, logic_e> logic;
         typedef ac_float<div_w, div_i, div_e> div;
         typedef ac_float arg1;
      };

      template <int WI, bool SI>
      struct rt_i
      {
         enum
         {
            lshift_w = W,
            lshift_i = I,
            lshift_s = S,
            lshift_e_0 = exp_t::template rt<WI, SI>::plus::width,
            lshift_e = AC_MIN(lshift_e_0, 24),
            rshift_w = W,
            rshift_i = I,
            rshift_s = S,
            rshift_e_0 = exp_t::template rt<WI, SI>::minus::width,
            rshift_e = AC_MIN(rshift_e_0, 24)
         };
         typedef ac_float<lshift_w, lshift_i, lshift_e> lshift;
         typedef ac_float<rshift_w, rshift_i, rshift_e> rshift;
      };

      template <typename T>
      struct rt_T
      {
         typedef typename ac_private::map<T>::t map_T;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::mult mult;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::plus plus;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::minus minus;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::minus2 minus2;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::logic logic;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::div div;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::div2 div2;
         typedef ac_float arg1;
      };

      template <typename T>
      struct rt_T2
      {
         typedef typename ac_private::map<T>::t map_T;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::mult mult;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::plus plus;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::minus2 minus;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::minus minus2;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::logic logic;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::div2 div;
         typedef typename ac_private::rt_ac_float_T<map_T>::template op1<AC_FL_TV0()>::div div2;
         typedef ac_float arg1;
      };

      struct rt_unary
      {
         enum
         {
            neg_w = W + 1,
            neg_i = I + 1,
            neg_e = E,
            neg_s = true,
            mag_sqr_w = 2 * W - S + NO_UN,
            mag_sqr_i = 2 * I - S + NO_UN,
            mag_sqr_e = E,
            mag_sqr_s = false | NO_UN,
            mag_w = W + S + NO_UN,
            mag_i = I + S + NO_UN,
            mag_e = E,
            mag_s = false | NO_UN,
            to_fx_i = I + MAX_EXP,
            to_fx_w = W + MAX_EXP - MIN_EXP,
            to_fx_s = S,
            to_i_w = AC_MAX(to_fx_i, 1),
            to_i_s = S
         };
         typedef ac_float<neg_w, neg_i, neg_e> neg;
         typedef ac_float<mag_sqr_w, mag_sqr_i, mag_sqr_e> mag_sqr;
         typedef ac_float<mag_w, mag_i, mag_e> mag;
         template <unsigned N>
         struct set
         {
            enum
            {
               sum_w = W + ac::log2_ceil<N>::val,
               sum_i = (sum_w - W) + I,
               sum_e = E,
               sum_s = S
            };
            typedef ac_float<sum_w, sum_i, sum_e> sum;
         };
         typedef ac_fixed<to_fx_w, to_fx_i, to_fx_s> to_ac_fixed_t;
         typedef ac_int<to_i_w, to_i_s> to_ac_int_t;
      };

      template <AC_FL_T(2)>
      friend class ac_float;

      ac_float()
      {
#if defined(AC_DEFAULT_IN_RANGE)
#endif
      }
      ac_float(const ac_float& op)
      {
         m = op.m;
         e = op.e;
      }

      template <AC_FL_T(2)>
      ac_float(const AC_FL(2) & op, bool assert_on_overflow = false, bool assert_on_rounding = false)
      {
         typedef AC_FL(2) fl2_t;
         enum
         {
            ST2 = S | S2,
            WT2 = W2 + (S2 == 0 && S != 0 ? 1 : 0),
            IT2 = I2 + (S2 == 0 && S != 0 ? 1 : 0),
            RND = (Q != AC_TRN) && (Q != AC_TRN_ZERO) && (WT2 > W),
            MAX_EXP2 = fl2_t::MAX_EXP,
            MIN_EXP2 = fl2_t::MIN_EXP,
            I_DIFF = IT2 - I,
            MAX_EXP_T = MAX_EXP2 + I_DIFF + RND,
            MIN_EXP_T = MIN_EXP2 + I_DIFF,
            MAX_EXP_T_P = MAX_EXP_T < 0 ? ~MAX_EXP_T : MAX_EXP_T,
            MIN_EXP_T_P = MIN_EXP_T < 0 ? ~MIN_EXP_T : MIN_EXP_T,
            WC_EXP_T = AC_MAX(MAX_EXP_T_P, MIN_EXP_T_P),
            ET = ac::template nbits<WC_EXP_T>::val + 1
         };

         typedef ac_fixed<WT2, IT2, ST2> fx2_t;
         typedef ac_fixed<WT2, I, S> fx_t;

         fx2_t m2 = op.m;
         fx_t t;
         t.set_slc(0, m2.template slc<fx_t::width>(0));
         ac_fixed<W + RND, I + RND, S, Q> m_1 = t;
         bool rnd_ovfl = false;
         if(RND)
         {
            rnd_ovfl = !m_1[W] && m_1[W - 1];
            m_1[W - 1] = m_1[W - 1] && !rnd_ovfl;
            m_1[W - 2] = m_1[W - 2] || rnd_ovfl;
         }
         m.set_slc(0, m_1.template slc<W>(0));
         if(fx_t::width > W && assert_on_rounding)
            AC_ASSERT(m == t, "Loss of precision due to Rounding in ac_float constructor");
         ac_int<ET, true> exp = !m ? ac_int<ET, true>(0) : ac_int<ET, true>(op.e + I_DIFF + (RND & rnd_ovfl));
         adjust(exp, false, assert_on_overflow);
      }

      ac_float(const ac_fixed<W, I, S>& m2, const ac_int<E, true>& e2, bool normalize = true)
      {
         m = m2;
         if(E)
         {
            if(!m)
               e = 0;
            else
            {
               e = e2;
               if(normalize)
                  m.normalize(e);
            }
         }
      }

      template <int WFX, int IFX, bool SFX, int E2>
      ac_float(const ac_fixed<WFX, IFX, SFX>& m2, const ac_int<E2, true>& e2, bool normalize = true)
      {
         enum
         {
            WF2 = WFX + !SFX,
            IF2 = IFX + !SFX
         };
         ac_float<WF2, IF2, E2> f(ac_fixed<WF2, IF2, true>(m2), e2, normalize);
         *this = f;
      }

      template <int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
      ac_float(const ac_fixed<WFX, IFX, SFX, QFX, OFX>& op, bool normalize = true)
      {
         enum
         {
            U2S = !SFX,
            WT2 = WFX + U2S,
            IT2 = IFX + U2S,
            RND = (QFX != AC_TRN) && (QFX != AC_TRN_ZERO) && (WT2 > W),
            I_DIFF = IT2 - I,
            MAX_EXP_T = -(I_DIFF + RND),
            MIN_EXP_T = AC_MAX(MIN_EXP - I_DIFF, -WT2 + 1),
            MAX_EXP_T_P = MAX_EXP_T < 0 ? ~MAX_EXP_T : MAX_EXP_T,
            MIN_EXP_T_P = MIN_EXP_T < 0 ? ~MIN_EXP_T : MIN_EXP_T,
            WC_EXP_T = AC_MAX(MAX_EXP_T_P, MIN_EXP_T_P),
            ET = ac::template nbits<WC_EXP_T>::val + 1
         };
         ac_float<WT2, IT2, ET> f(op, ac_int<ET, true>(0), normalize);
         *this = f;
      }

      template <int WI, bool SI>
      ac_float(const ac_int<WI, SI>& op)
      {
         *this = ac_fixed<WI, WI, SI>(op);
      }

      __FORCE_INLINE ac_float(bool b)
      {
         *this = (ac_int<1, false>)b;
      }
      __FORCE_INLINE ac_float(char b)
      {
         *this = (ac_int<8, true>)b;
      }
      __FORCE_INLINE ac_float(signed char b)
      {
         *this = (ac_int<8, true>)b;
      }
      __FORCE_INLINE ac_float(unsigned char b)
      {
         *this = (ac_int<8, false>)b;
      }
      __FORCE_INLINE ac_float(signed short b)
      {
         *this = (ac_int<16, true>)b;
      }
      __FORCE_INLINE ac_float(unsigned short b)
      {
         *this = (ac_int<16, false>)b;
      }
      __FORCE_INLINE ac_float(signed int b)
      {
         *this = (ac_int<32, true>)b;
      }
      __FORCE_INLINE ac_float(unsigned int b)
      {
         *this = (ac_int<32, false>)b;
      }
      __FORCE_INLINE ac_float(signed long b)
      {
         *this = (ac_int<ac_private::long_w, true>)b;
      }
      __FORCE_INLINE ac_float(unsigned long b)
      {
         *this = (ac_int<ac_private::long_w, false>)b;
      }
      __FORCE_INLINE ac_float(Slong b)
      {
         *this = (ac_int<64, true>)b;
      }
      __FORCE_INLINE ac_float(Ulong b)
      {
         *this = (ac_int<64, false>)b;
      }

      // Explicit conversion functions to ac_int and ac_fixed
      __FORCE_INLINE typename rt_unary::to_ac_fixed_t to_ac_fixed() const
      {
         typename rt_unary::to_ac_fixed_t r = m;
         r <<= e;
         return r;
      }
      __FORCE_INLINE typename rt_unary::to_ac_int_t to_ac_int() const
      {
         return to_ac_fixed().to_ac_int();
      }

      // Explicit conversion functions to C built-in types -------------
      __FORCE_INLINE int to_int() const
      {
         return to_ac_int().to_int();
      }
      __FORCE_INLINE unsigned to_uint() const
      {
         return to_ac_int().to_uint();
      }
      __FORCE_INLINE long to_long() const
      {
         return (signed long)to_ac_int().to_int64();
      }
      __FORCE_INLINE unsigned long to_ulong() const
      {
         return (unsigned long)to_ac_int().to_uint64();
      }
      __FORCE_INLINE Slong to_int64() const
      {
         return to_ac_int().to_int64();
      }
      __FORCE_INLINE Ulong to_uint64() const
      {
         return to_ac_int().to_uint64();
      }
      __FORCE_INLINE float to_float() const
      {
         return ldexpf(m.to_double(), exp());
      }
      __FORCE_INLINE double to_double() const
      {
         return ldexp(m.to_double(), exp());
      }

      const ac_fixed<W, I, S> mantissa() const
      {
         return m;
      }
      const ac_int<E, true> exp() const
      {
         return e;
      }
      bool normalize()
      {
         bool normalized = operator!() || !S && m[W - 1] || S && (m[W - 1] ^ m[W - 2]);
         if(E && !normalized)
            normalized = m.normalize(e);
         return normalized;
      }

      template <int ET>
      void adjust(ac_int<ET, true> new_e, bool normalize, bool assert_on_overflow)
      {
         if(E >= ET)
         {
            e = new_e;
            if(E && normalize)
               m.normalize(e);
         }
         else
         {
            ac_int<E, false> offset = 0;
            offset[E - 1] = 1;
            ac_int<ET + 1, true> e_s = new_e + offset;
            if(e_s < 0)
            {
               m >>= (MIN_EXP - new_e); // can a full barrel-shifter be avoided ???
               e = MIN_EXP;
            }
            else
            {
               // break down:  bits( (1<<E) -1 + W-1 ) + 1 is max bit for normalization
               // other bits can be tested separetely
               ac_int<ET, false> e_u = e_s;
               if(ET && normalize)
                  m.normalize(e_u);
               e_u -= offset;
               if(e_u[ET - 1] | !(e_u >> (E - 1))) // what about E == 0 or ET == 0 ???
                  e = e_u;
               else
               {
                  e = MAX_EXP;
                  m = m < 0 ? value<AC_VAL_MIN>(m) : value<AC_VAL_MAX>(m);
                  if(assert_on_overflow)
                     AC_ASSERT(0, "OVERFLOW ON ASSIGNMENT TO AC_FLOAT");
               }
            }
         }
      }

      ac_float(double d, bool assert_on_overflow = false, bool assert_on_rounding = false)
      {
         enum
         {
            I_EXT = AC_MAX(I, 1),
            W_EXT = ac_private::ac_float_cdouble_t::width + I_EXT - 1,
         };
         ac_private::ac_float_cdouble_t t = ac_private::double_to_ac_float(d);
         ac_float r(t, assert_on_overflow, assert_on_rounding);
         *this = r;
      }

      ac_float(float f, bool assert_on_overflow = false, bool assert_on_rounding = false)
      {
         enum
         {
            I_EXT = AC_MAX(I, 1),
            W_EXT = ac_private::ac_float_cfloat_t::width + I_EXT - 1,
         };
         ac_private::ac_float_cfloat_t t = ac_private::float_to_ac_float(f);
         ac_float r(t, assert_on_overflow, assert_on_rounding);
         *this = r;
      }

      template <AC_FL_T(2)>
      typename rt<AC_FL_TV0(2)>::mult operator*(const AC_FL(2) & op2) const
      {
         typename rt<AC_FL_TV0(2)>::mult r(m * op2.m, exp() + op2.exp(), false);
         return r;
      }

      template <AC_FL_T(2)>
      bool compare(const AC_FL(2) & op2, bool* gt) const
      {
         typedef ac_fixed<W2, I, S2> fx2_t;
         typedef typename ac_fixed<W, I, S>::template rt_T<fx2_t>::logic fx_t;
         typedef ac_fixed<fx_t::width, fx_t::i_width, false> fxu_t;

         fx2_t op2_m_0;
         op2_m_0.set_slc(0, op2.m.template slc<W2>(0));

         fx_t op1_m = m;
         fx_t op2_m = op2_m_0;
         int e_dif = exp() - op2.exp() + I - I2;
         bool op2_m_neg = op2_m[fx_t::width - 1];
         fx_t out_bits = op2_m ^ ((op2_m_neg & e_dif < 0) ? ~fx_t(0) : fx_t(0));
         out_bits &= ~(fxu_t(~fxu_t(0)) << e_dif);
         op2_m >>= e_dif;
         bool overflow = e_dif < 0 & !!out_bits | op2_m_neg ^ op2_m[fx_t::width - 1];

         *gt = overflow & op2_m_neg | !overflow & op1_m > op2_m;
         bool eq = op1_m == op2_m & !overflow & !out_bits;
         return eq;
      }

      template <AC_FL_T(2), AC_FL_T(R)>
      void plus_minus(const AC_FL(2) & op2, AC_FL(R) & r, bool sub = false) const
      {
         typedef AC_FL(R) r_type;
         enum
         {
            IT = AC_MAX(I, I2)
         };
         typedef ac_fixed<W, IT, S> fx1_t;
         typedef ac_fixed<W2, IT, S2> fx2_t;
         typedef typename fx1_t::template rt_T<ac_fixed<WR, IT, SR>>::logic fx1t_t;
         typedef typename fx2_t::template rt_T<ac_fixed<WR, IT, SR>>::logic fx2t_t;
         typedef typename fx1t_t::template rt_T<fx2t_t>::plus mt_t;
         typedef ac_fixed<mt_t::width + 1, mt_t::i_width, SR> t1_t;
         typedef ac_fixed<mt_t::width + 2, mt_t::i_width, SR> t2_t;
         typedef ac_fixed<t2_t::width, IR, SR> r1_t;

         enum
         {
            MAX_EXP2 = AC_FL(2)::MAX_EXP,
            MIN_EXP2 = AC_FL(2)::MIN_EXP,
            I_DIFF = mt_t::i_width - IR,
            MAX_EXP_T = AC_MAX(MAX_EXP, MAX_EXP2) + I_DIFF,
            MIN_EXP_T = AC_MIN(MIN_EXP, MIN_EXP2) + I_DIFF,
            MAX_EXP_T_P = MAX_EXP_T < 0 ? ~MAX_EXP_T : MAX_EXP_T,
            MIN_EXP_T_P = MIN_EXP_T < 0 ? ~MIN_EXP_T : MIN_EXP_T,
            WC_EXP_T = AC_MAX(MAX_EXP_T_P, MIN_EXP_T_P),
            ET = ac::template nbits<WC_EXP_T>::val + 1
         };

         ac_fixed<mt_t::width, I + 1, mt_t::sign> op1_m_0 = m;
         mt_t op1_m = 0;
         op1_m.set_slc(0, op1_m_0.template slc<mt_t::width>(0));
         int op1_e = exp() + I - IT + (SR & !S);

         ac_fixed<mt_t::width, I2 + 1, mt_t::sign> op2_m_0 = op2.m;
         mt_t op2_m = 0;
         op2_m.set_slc(0, op2_m_0.template slc<mt_t::width>(0));
         if(sub)
            op2_m = -op2_m;
         int op2_e = op2.exp() + I2 - IT + (SR & !S2);

         bool op1_zero = operator!();
         bool op2_zero = !op2;
         int e_dif = op1_e - op2_e;
         bool e1_lt_e2 = e_dif < 0;
         e_dif = (op1_zero | op2_zero) ? 0 : e1_lt_e2 ? -e_dif : e_dif;
         mt_t op_sl = e1_lt_e2 ? op1_m : op2_m;
         // Sticky bits are bits shifted out, leaving out the MSB
         mt_t sticky_bits = op_sl;
         sticky_bits &= ~((~t1_t(0)) << e_dif);
         bool sticky_bit = !!sticky_bits;
         bool msb_shifted_out = (bool)t1_t(op_sl).template slc<1>(e_dif);
         op_sl >>= e_dif;
         op1_m = e1_lt_e2 ? op_sl : op1_m;
         op2_m = e1_lt_e2 ? op2_m : op_sl;

         t1_t t1 = op1_m;
         t1 += op2_m;
         t1[0] = msb_shifted_out;

         bool shift_r_1 = false; // t1[t1_t::width-1] != t1[t1_t::width-2];
         sticky_bit |= shift_r_1 & t1[0];
         t1 >>= shift_r_1;
         t2_t t2 = t1;
         t2[0] = sticky_bit;
         r1_t r1;
         r1.set_slc(0, t2.template slc<t2_t::width>(0));
         r_type r_t;
         r_t.m = (ac_fixed<WR, IR, SR, QR>)r1; // t2;  This could overflow if !SR&ST
         ac_int<ET, true> r_e = op1_zero ? op2_e : (op2_zero ? op1_e : AC_MAX(op1_e, op2_e));
         r_e = ac_int<1, true>(!op1_zero | !op2_zero) & (r_e + shift_r_1 + I_DIFF);
         r_t.adjust(r_e, true, false);
         r.m = r_t.m;
         r.e = r_t.e;
      }

      template <AC_FL_T(1), AC_FL_T(2)>
      ac_float add(const AC_FL(1) & op1, const AC_FL(2) & op2)
      {
         op1.plus_minus(op2, *this);
         return *this;
      }

      template <AC_FL_T(1), AC_FL_T(2)>
      ac_float sub(const AC_FL(1) & op1, const AC_FL(2) & op2)
      {
         op1.plus_minus(op2, *this, true);
         return *this;
      }

      typename rt_unary::neg abs() const
      {
         typedef typename rt_unary::neg r_t;
         r_t r;
         r.m = is_neg() ? -m : r_t::mant_t(m);
         r.e = e;
         return r;
      }

#ifdef __AC_FLOAT_ENABLE_ALPHA
      // These will be changed!!! For now only enable to explore integration with
      // ac_complex
      template <AC_FL_T(2)>
      typename rt<AC_FL_TV0(2)>::plus operator+(const AC_FL(2) & op2) const
      {
         typename rt<AC_FL_TV0(2)>::plus r;
         plus_minus(op2, r);
         return r;
      }
      template <AC_FL_T(2)>
      typename rt<AC_FL_TV0(2)>::minus operator-(const AC_FL(2) & op2) const
      {
         typename rt<AC_FL_TV0(2)>::minus r;
         plus_minus(op2, r, true);
         return r;
      }
#endif

      template <AC_FL_T(2)>
      typename rt<AC_FL_TV0(2)>::div operator/(const AC_FL(2) & op2) const
      {
         typename rt<AC_FL_TV0(2)>::div r(m / op2.m, exp() - op2.exp());
         return r;
      }
      template <AC_FL_T(2)>
      ac_float operator+=(const AC_FL(2) & op2)
      {
         ac_float r;
         plus_minus(op2, r);
         *this = r;
      }
      template <AC_FL_T(2)>
      ac_float operator-=(const AC_FL(2) & op2)
      {
         ac_float r;
         plus_minus(op2, r, true);
         *this = r;
      }
      template <AC_FL_T(2)>
      ac_float operator*=(const AC_FL(2) & op2)
      {
         *this = *this * op2;
      }
      template <AC_FL_T(2)>
      ac_float operator/=(const AC_FL(2) & op2)
      {
         *this = *this / op2;
      }
      ac_float operator+() const
      {
         return *this;
      }
      typename rt_unary::neg operator-() const
      {
         typename rt_unary::neg r;
         r.m = -m;
         r.e = e;
         return r;
      }
      bool operator!() const
      {
         return !m;
      }

      // Shift --------------------------------------------------------------------
      template <int WI, bool SI>
      typename rt_i<WI, SI>::lshift operator<<(const ac_int<WI, SI>& op2) const
      {
         typename rt_i<WI, SI>::lshift r;
         r.m = m;
         r.e = e + op2;
         return r;
      }
      template <int WI, bool SI>
      typename rt_i<WI, SI>::rshift operator>>(const ac_int<WI, SI>& op2) const
      {
         typename rt_i<WI, SI>::rshift r;
         r.m = m;
         r.e = e - op2;
         return r;
      }
      // Shift assign -------------------------------------------------------------
      template <int WI, bool SI>
      ac_float& operator<<=(const ac_int<WI, SI>& op2)
      {
         *this = operator<<(op2);
         return *this;
      }
      template <int WI, bool SI>
      ac_float& operator>>=(const ac_int<WI, SI>& op2)
      {
         *this = operator>>(op2);
         return *this;
      }

      template <AC_FL_T(2)>
      bool operator==(const AC_FL(2) & f) const
      {
         bool gt;
         return compare(f, &gt);
      }
      template <AC_FL_T(2)>
      bool operator!=(const AC_FL(2) & f) const
      {
         return !operator==(f);
      }
      template <AC_FL_T(2)>
      bool operator<(const AC_FL(2) & f) const
      {
         bool gt;
         bool eq = compare(f, &gt);
         return !(eq | gt);
      }
      template <AC_FL_T(2)>
      bool operator>=(const AC_FL(2) & f) const
      {
         return !operator<(f);
      }
      template <AC_FL_T(2)>
      bool operator>(const AC_FL(2) & f) const
      {
         bool gt;
         compare(f, &gt);
         return gt;
      }
      template <AC_FL_T(2)>
      bool operator<=(const AC_FL(2) & f) const
      {
         return !operator>(f);
      }

      __FORCE_INLINE std::string to_string(ac_base_mode base_rep, bool sign_mag = false, bool hw = true) const
      {
         // TODO: printing decimal with exponent
         if(!hw)
         {
            ac_fixed<W, 0, S> mantissa;
            mantissa.set_slc(0, m.template slc<W>(0));
            std::string r = mantissa.to_string(base_rep, sign_mag);
            r += "e2";
            r += (e + I).to_string(base_rep, sign_mag | base_rep == AC_DEC);
            return r;
         }
         else
         {
            std::string r = m.to_string(base_rep, sign_mag);
            if(base_rep != AC_DEC)
               r += "_";
            r += "e2";
            if(base_rep != AC_DEC)
               r += "_";
            if(E)
               r += e.to_string(base_rep, sign_mag | base_rep == AC_DEC);
            else
               r += "0";
            return r;
         }
      }

      __FORCE_INLINE static std::string type_name()
      {
         const char* tf[] = {"false", "true"};
         const char* q[] = {"AC_TRN",     "AC_RND",         "AC_TRN_ZERO", "AC_RND_ZERO",
                            "AC_RND_INF", "AC_RND_MIN_INF", "AC_RND_CONV"};
         std::string r = "ac_float<";
         r += ac_int<32, true>(W).to_string(AC_DEC) + ',';
         r += ac_int<32, true>(I).to_string(AC_DEC) + ',';
         r += ac_int<32, true>(E).to_string(AC_DEC) + ',';
         r += tf[S];
         r += ',';
         r += q[Q];
         r += '>';
         return r;
      }

      template <ac_special_val V>
      __FORCE_INLINE ac_float& set_val()
      {
         m.template set_val<V>();
         if(V == AC_VAL_MIN)
            e.template set_val<AC_VAL_MAX>();
         else if(V == AC_VAL_QUANTUM)
            e.template set_val<AC_VAL_MIN>();
         else
            e.template set_val<V>();
         return *this;
      }
   };

   namespace ac_private
   {
      template <typename T>
      bool ac_fpclassify(T x, bool& inf)
      {
         bool nan = !(x == x);
         if(!nan)
         {
            T d = x - x;
            inf = !(d == d);
         }
         return nan;
      }

      __FORCE_INLINE ac_float_cdouble_t double_to_ac_float(double d)
      {
         typedef ac_float_cdouble_t r_t;
#ifndef __BAMBU__
         bool inf;
         bool nan = ac_fpclassify(d, inf);
         if(nan)
            AC_ASSERT(0, "In conversion from double to ac_float: double is NaN");
         else if(inf)
            AC_ASSERT(0, "In conversion from double to ac_float: double is Infinite");
#endif
         r_t::exp_t exp;
         r_t::mant_t mant = ac::frexp_d(d, exp);
         return r_t(mant, exp, false);
      }

      __FORCE_INLINE ac_float_cfloat_t float_to_ac_float(float f)
      {
         typedef ac_float_cfloat_t r_t;
#ifndef __BAMBU__
         bool inf;
         bool nan = ac_fpclassify(f, inf);
         if(nan)
            AC_ASSERT(0, "In conversion from float to ac_float: float is NaN");
         else if(inf)
            AC_ASSERT(0, "In conversion from float to ac_float: float is Infinite");
#endif
         r_t::exp_t exp;
         r_t::mant_t mant = ac::frexp_f(f, exp);
         return r_t(mant, exp, false);
      }
   }; // namespace ac_private

   namespace ac
   {
      template <typename T>
      struct ac_float_represent
      {
         typedef typename ac_fixed_represent<T>::type fx_t;
         typedef ac_float<fx_t::width + !fx_t::sign, fx_t::i_width + !fx_t::sign, 1, fx_t::q_mode> type;
      };
      template <>
      struct ac_float_represent<float>
      {
         typedef ac_private::ac_float_cfloat_t type;
      };
      template <>
      struct ac_float_represent<double>
      {
         typedef ac_private::ac_float_cdouble_t type;
      };
   } // namespace ac

   namespace ac_private
   {
      // with T == ac_float
      template <AC_FL_T0(2)>
      struct rt_ac_float_T<AC_FL0(2)>
      {
         typedef AC_FL0(2) fl2_t;
         template <AC_FL_T0()>
         struct op1
         {
            typedef AC_FL0() fl_t;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::mult mult;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::plus plus;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::minus minus;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::minus minus2;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::logic logic;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::div div;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::div div2;
         };
      };
      // with T == ac_fixed
      template <int WFX, int IFX, bool SFX>
      struct rt_ac_float_T<ac_fixed<WFX, IFX, SFX>>
      {
         // For now E2 > 0
         enum
         {
            E2 = 1,
            S2 = true,
            W2 = WFX + !SFX,
            I2 = IFX + !SFX
         };
         typedef AC_FL0(2) fl2_t;
         template <AC_FL_T0()>
         struct op1
         {
            typedef AC_FL0() fl_t;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::mult mult;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::plus plus;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::minus minus;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::minus minus2;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::logic logic;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::div div;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::div div2;
         };
      };
      // with T == ac_int
      template <int WI, bool SI>
      struct rt_ac_float_T<ac_int<WI, SI>>
      {
         // For now E2 > 0
         enum
         {
            E2 = 1,
            S2 = true,
            I2 = WI + !SI,
            W2 = I2
         };
         typedef AC_FL0(2) fl2_t;
         template <AC_FL_T0()>
         struct op1
         {
            typedef AC_FL0() fl_t;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::mult mult;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::plus plus;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::minus minus;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::minus minus2;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::logic logic;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::div div;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::div div2;
         };
      };

      // Multiplication is optimizable, general operator +/- is not yet supported
      template <typename T>
      struct rt_ac_float_T<c_type<T>>
      {
         // For now E2 > 0
         enum
         {
            SCT = c_type_params<T>::S,
            S2 = true,
            W2 = c_type_params<T>::W + !SCT,
            I2 = c_type_params<T>::I + !SCT,
            E2 = AC_MAX(1, c_type_params<T>::E)
         };
         typedef AC_FL0(2) fl2_t;
         template <AC_FL_T0()>
         struct op1
         {
            typedef AC_FL0() fl_t;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::mult mult;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::plus plus;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::minus minus;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::minus minus2;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::logic logic;
            typedef typename fl_t::template rt<AC_FL_TV0(2)>::div div;
            typedef typename fl2_t::template rt<AC_FL_TV0()>::div div2;
         };
      };
   } // namespace ac_private

   // Stream --------------------------------------------------------------------

#ifndef __BAMBU__
   template <AC_FL_T()>
   __FORCE_INLINE std::ostream& operator<<(std::ostream& os, const AC_FL() & x)
   {
      os << x.to_string(AC_DEC);
      return os;
   }
#endif

#define FL_BIN_OP_WITH_CTYPE(BIN_OP, C_TYPE, RTYPE)                                                                \
   template <AC_FL_T()>                                                                                            \
   __FORCE_INLINE typename AC_FL()::template rt_T2<C_TYPE>::RTYPE operator BIN_OP(C_TYPE c_op, const AC_FL() & op) \
   {                                                                                                               \
      typedef typename ac::template ac_float_represent<C_TYPE>::type fl2_t;                                        \
      return fl2_t(c_op).operator BIN_OP(op);                                                                      \
   }                                                                                                               \
   template <AC_FL_T()>                                                                                            \
   __FORCE_INLINE typename AC_FL()::template rt_T<C_TYPE>::RTYPE operator BIN_OP(const AC_FL() & op, C_TYPE c_op)  \
   {                                                                                                               \
      typedef typename ac::template ac_float_represent<C_TYPE>::type fl2_t;                                        \
      return op.operator BIN_OP(fl2_t(c_op));                                                                      \
   }

#define FL_REL_OP_WITH_CTYPE(REL_OP, C_TYPE)                                \
   template <AC_FL_T()>                                                     \
   __FORCE_INLINE bool operator REL_OP(const AC_FL() & op, C_TYPE op2)      \
   {                                                                        \
      typedef typename ac::template ac_float_represent<C_TYPE>::type fl2_t; \
      return op.operator REL_OP(fl2_t(op2));                                \
   }                                                                        \
   template <AC_FL_T()>                                                     \
   __FORCE_INLINE bool operator REL_OP(C_TYPE op2, const AC_FL() & op)      \
   {                                                                        \
      typedef typename ac::template ac_float_represent<C_TYPE>::type fl2_t; \
      return fl2_t(op2).operator REL_OP(op);                                \
   }

#define FL_ASSIGN_OP_WITH_CTYPE_2(ASSIGN_OP, C_TYPE)                        \
   template <AC_FL_T()>                                                     \
   __FORCE_INLINE AC_FL()& operator ASSIGN_OP(AC_FL() & op, C_TYPE op2)     \
   {                                                                        \
      typedef typename ac::template ac_float_represent<C_TYPE>::type fl2_t; \
      return op.operator ASSIGN_OP(fl2_t(op2));                             \
   }

#define FL_OPS_WITH_CTYPE(C_TYPE)         \
   FL_BIN_OP_WITH_CTYPE(*, C_TYPE, mult)  \
   FL_BIN_OP_WITH_CTYPE(+, C_TYPE, plus)  \
   FL_BIN_OP_WITH_CTYPE(-, C_TYPE, minus) \
   FL_BIN_OP_WITH_CTYPE(/, C_TYPE, div)   \
                                          \
   FL_REL_OP_WITH_CTYPE(==, C_TYPE)       \
   FL_REL_OP_WITH_CTYPE(!=, C_TYPE)       \
   FL_REL_OP_WITH_CTYPE(>, C_TYPE)        \
   FL_REL_OP_WITH_CTYPE(>=, C_TYPE)       \
   FL_REL_OP_WITH_CTYPE(<, C_TYPE)        \
   FL_REL_OP_WITH_CTYPE(<=, C_TYPE)       \
                                          \
   FL_ASSIGN_OP_WITH_CTYPE_2(+=, C_TYPE)  \
   FL_ASSIGN_OP_WITH_CTYPE_2(-=, C_TYPE)  \
   FL_ASSIGN_OP_WITH_CTYPE_2(*=, C_TYPE)  \
   FL_ASSIGN_OP_WITH_CTYPE_2(/=, C_TYPE)

#define FL_SHIFT_OP_WITH_INT_CTYPE(BIN_OP, C_TYPE, RTYPE)                                      \
   template <AC_FL_T()>                                                                        \
   __FORCE_INLINE typename AC_FL()::template rt_i<ac_private::c_type_params<C_TYPE>::W,        \
                                                  ac_private::c_type_params<C_TYPE>::S>::RTYPE \
   operator BIN_OP(const AC_FL() & op, C_TYPE i_op)                                            \
   {                                                                                           \
      typedef typename ac::template ac_int_represent<C_TYPE>::type i_t;                        \
      return op.operator BIN_OP(i_t(i_op));                                                    \
   }

#define FL_SHIFT_ASSIGN_OP_WITH_INT_CTYPE(ASSIGN_OP, C_TYPE)             \
   template <AC_FL_T()>                                                  \
   __FORCE_INLINE AC_FL()& operator ASSIGN_OP(AC_FL() & op, C_TYPE i_op) \
   {                                                                     \
      typedef typename ac::template ac_int_represent<C_TYPE>::type i_t;  \
      return op.operator ASSIGN_OP(i_t(i_op));                           \
   }

#define FL_SHIFT_OPS_WITH_INT_CTYPE(C_TYPE)       \
   FL_SHIFT_OP_WITH_INT_CTYPE(>>, C_TYPE, rshift) \
   FL_SHIFT_OP_WITH_INT_CTYPE(<<, C_TYPE, lshift) \
   FL_SHIFT_ASSIGN_OP_WITH_INT_CTYPE(>>=, C_TYPE) \
   FL_SHIFT_ASSIGN_OP_WITH_INT_CTYPE(<<=, C_TYPE)

#define FL_OPS_WITH_INT_CTYPE(C_TYPE) \
   FL_OPS_WITH_CTYPE(C_TYPE)          \
   FL_SHIFT_OPS_WITH_INT_CTYPE(C_TYPE)

   // --------------------------------------- End of Macros for Binary Operators
   // with C Floats

   // Binary Operators with C Floats --------------------------------------------
   FL_OPS_WITH_CTYPE(float)
   FL_OPS_WITH_CTYPE(double)
   FL_OPS_WITH_INT_CTYPE(bool)
   FL_OPS_WITH_INT_CTYPE(char)
   FL_OPS_WITH_INT_CTYPE(signed char)
   FL_OPS_WITH_INT_CTYPE(unsigned char)
   FL_OPS_WITH_INT_CTYPE(short)
   FL_OPS_WITH_INT_CTYPE(unsigned short)
   FL_OPS_WITH_INT_CTYPE(int)
   FL_OPS_WITH_INT_CTYPE(unsigned int)
   FL_OPS_WITH_INT_CTYPE(long)
   FL_OPS_WITH_INT_CTYPE(unsigned long)
   FL_OPS_WITH_INT_CTYPE(Slong)
   FL_OPS_WITH_INT_CTYPE(Ulong)
   // -------------------------------------- End of Binary Operators with C Floats

   // Macros for Binary Operators with ac_int
   // --------------------------------------------

#define FL_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE)                                                                        \
   template <AC_FL_T(), int WI, bool SI>                                                                              \
   __FORCE_INLINE typename AC_FL()::template rt_T2<ac_int<WI, SI>>::RTYPE operator BIN_OP(const ac_int<WI, SI>& i_op, \
                                                                                          const AC_FL() & op)         \
   {                                                                                                                  \
      typedef typename ac::template ac_float_represent<ac_int<WI, SI>>::type fl2_t;                                   \
      return fl2_t(i_op).operator BIN_OP(op);                                                                         \
   }

#define FL_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)                                                                        \
   template <AC_FL_T(), int WI, bool SI>                                                                              \
   __FORCE_INLINE typename AC_FL()::template rt_T2<ac_int<WI, SI>>::RTYPE operator BIN_OP(const AC_FL() & op,         \
                                                                                          const ac_int<WI, SI>& i_op) \
   {                                                                                                                  \
      typedef typename ac::template ac_float_represent<ac_int<WI, SI>>::type fl2_t;                                   \
      return op.operator BIN_OP(fl2_t(i_op));                                                                         \
   }

#define FL_BIN_OP_WITH_AC_INT(BIN_OP, RTYPE) \
   FL_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE)    \
   FL_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)

#define FL_REL_OP_WITH_AC_INT(REL_OP)                                                 \
   template <AC_FL_T(), int WI, bool SI>                                              \
   __FORCE_INLINE bool operator REL_OP(const AC_FL() & op, const ac_int<WI, SI>& op2) \
   {                                                                                  \
      typedef typename ac::template ac_float_represent<ac_int<WI, SI>>::type fl2_t;   \
      return op.operator REL_OP(fl2_t(op2));                                          \
   }                                                                                  \
   template <AC_FL_T(), int WI, bool SI>                                              \
   __FORCE_INLINE bool operator REL_OP(ac_int<WI, SI>& op2, const AC_FL() & op)       \
   {                                                                                  \
      typedef typename ac::template ac_float_represent<ac_int<WI, SI>>::type fl2_t;   \
      return fl2_t(op2).operator REL_OP(op);                                          \
   }

#define FL_ASSIGN_OP_WITH_AC_INT(ASSIGN_OP)                                            \
   template <AC_FL_T(), int WI, bool SI>                                               \
   __FORCE_INLINE AC_FL()& operator ASSIGN_OP(AC_FL() & op, const ac_int<WI, SI>& op2) \
   {                                                                                   \
      typedef typename ac::template ac_float_represent<ac_int<WI, SI>>::type fl2_t;    \
      return op.operator ASSIGN_OP(fl2_t(op2));                                        \
   }

   // -------------------------------------------- End of Macros for Binary
   // Operators with ac_int

   // Binary Operators with ac_int --------------------------------------------
   FL_BIN_OP_WITH_AC_INT(*, mult)
   FL_BIN_OP_WITH_AC_INT(+, plus)
   FL_BIN_OP_WITH_AC_INT(-, minus)
   FL_BIN_OP_WITH_AC_INT(/, div)

   FL_REL_OP_WITH_AC_INT(==)
   FL_REL_OP_WITH_AC_INT(!=)
   FL_REL_OP_WITH_AC_INT(>)
   FL_REL_OP_WITH_AC_INT(>=)
   FL_REL_OP_WITH_AC_INT(<)
   FL_REL_OP_WITH_AC_INT(<=)

   FL_ASSIGN_OP_WITH_AC_INT(+=)
   FL_ASSIGN_OP_WITH_AC_INT(-=)
   FL_ASSIGN_OP_WITH_AC_INT(*=)
   FL_ASSIGN_OP_WITH_AC_INT(/=)
   FL_ASSIGN_OP_WITH_AC_INT(%=)
   // -------------------------------------- End of Binary Operators with ac_int

   // Macros for Binary Operators with ac_fixed
   // --------------------------------------------

#define FL_BIN_OP_WITH_AC_FIXED_1(BIN_OP, RTYPE)                                                 \
   template <AC_FL_T(), int WF, int IF, bool SF, ac_q_mode QF, ac_o_mode OF>                     \
   __FORCE_INLINE typename AC_FL()::template rt_T2<ac_fixed<WF, IF, SF>>::RTYPE operator BIN_OP( \
       const ac_fixed<WF, IF, SF, QF, OF>& f_op, const AC_FL() & op)                             \
   {                                                                                             \
      typedef typename ac::template ac_float_represent<ac_fixed<WF, IF, SF>>::type fl2_t;        \
      return fl2_t(f_op).operator BIN_OP(op);                                                    \
   }

#define FL_BIN_OP_WITH_AC_FIXED_2(BIN_OP, RTYPE)                                                 \
   template <AC_FL_T(), int WF, int IF, bool SF, ac_q_mode QF, ac_o_mode OF>                     \
   __FORCE_INLINE typename AC_FL()::template rt_T2<ac_fixed<WF, IF, SF>>::RTYPE operator BIN_OP( \
       const AC_FL() & op, const ac_fixed<WF, IF, SF, QF, OF>& f_op)                             \
   {                                                                                             \
      typedef typename ac::template ac_float_represent<ac_fixed<WF, IF, SF>>::type fl2_t;        \
      return op.operator BIN_OP(fl2_t(f_op));                                                    \
   }

#define FL_BIN_OP_WITH_AC_FIXED(BIN_OP, RTYPE) \
   FL_BIN_OP_WITH_AC_FIXED_1(BIN_OP, RTYPE)    \
   FL_BIN_OP_WITH_AC_FIXED_2(BIN_OP, RTYPE)

#define FL_REL_OP_WITH_AC_FIXED(REL_OP)                                                             \
   template <AC_FL_T(), int WF, int IF, bool SF, ac_q_mode QF, ac_o_mode OF>                        \
   __FORCE_INLINE bool operator REL_OP(const AC_FL() & op, const ac_fixed<WF, IF, SF, QF, OF>& op2) \
   {                                                                                                \
      typedef typename ac::template ac_float_represent<ac_fixed<WF, IF, SF>>::type fl2_t;           \
      return op.operator REL_OP(fl2_t(op2));                                                        \
   }                                                                                                \
   template <AC_FL_T(), int WF, int IF, bool SF, ac_q_mode QF, ac_o_mode OF>                        \
   __FORCE_INLINE bool operator REL_OP(ac_fixed<WF, IF, SF, QF, OF>& op2, const AC_FL() & op)       \
   {                                                                                                \
      typedef typename ac::template ac_float_represent<ac_fixed<WF, IF, SF>>::type fl2_t;           \
      return fl2_t(op2).operator REL_OP(op);                                                        \
   }

#define FL_ASSIGN_OP_WITH_AC_FIXED(ASSIGN_OP)                                                        \
   template <AC_FL_T(), int WF, int IF, bool SF, ac_q_mode QF, ac_o_mode OF>                         \
   __FORCE_INLINE AC_FL()& operator ASSIGN_OP(AC_FL() & op, const ac_fixed<WF, IF, SF, QF, OF>& op2) \
   {                                                                                                 \
      typedef typename ac::template ac_float_represent<ac_fixed<WF, IF, SF>>::type fl2_t;            \
      return op.operator ASSIGN_OP(fl2_t(op2));                                                      \
   }

   // -------------------------------------------- End of Macros for Binary
   // Operators with ac_fixed

   // Binary Operators with ac_fixed --------------------------------------------
   FL_BIN_OP_WITH_AC_FIXED(*, mult)
   FL_BIN_OP_WITH_AC_FIXED(+, plus)
   FL_BIN_OP_WITH_AC_FIXED(-, minus)
   FL_BIN_OP_WITH_AC_FIXED(/, div)

   FL_REL_OP_WITH_AC_FIXED(==)
   FL_REL_OP_WITH_AC_FIXED(!=)
   FL_REL_OP_WITH_AC_FIXED(>)
   FL_REL_OP_WITH_AC_FIXED(>=)
   FL_REL_OP_WITH_AC_FIXED(<)
   FL_REL_OP_WITH_AC_FIXED(<=)

   FL_ASSIGN_OP_WITH_AC_FIXED(+=)
   FL_ASSIGN_OP_WITH_AC_FIXED(-=)
   FL_ASSIGN_OP_WITH_AC_FIXED(*=)
   FL_ASSIGN_OP_WITH_AC_FIXED(/=)
   // -------------------------------------- End of Binary Operators with ac_fixed

   // Global templatized functions for easy initialization to special values
   template <ac_special_val V, AC_FL_T()>
   __FORCE_INLINE AC_FL() value(AC_FL())
   {
      AC_FL() r;
      return r.template set_val<V>();
   }

   namespace ac
   {
      // function to initialize (or uninitialize) arrays
      template <ac_special_val V, AC_FL_T()>
      __FORCE_INLINE bool init_array(AC_FL() * a, int n)
      {
         AC_FL0() t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
   } // namespace ac

   ///////////////////////////////////////////////////////////////////////////////

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(pop)
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef __AC_NAMESPACE
}
#endif

#endif // __AC_FLOAT_H
