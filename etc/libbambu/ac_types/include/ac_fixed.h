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
 *  Copyright 2005-2017, Mentor Graphics Corporation,                     *
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

/*
//  Source:           ac_fixed.h
//  Description:      class for fixed point operation handling in C++
//  Original Author:  Andres Takach, Ph.D.
//  Modified by:      Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*/

#ifndef __AC_FIXED_H
#define __AC_FIXED_H

#include "ac_int.h"

#if(defined(__GNUC__) && __GNUC__ < 3 && !defined(__EDG__))
#error GCC version 3 or greater is required to include this header file
#endif

#if(defined(_MSC_VER) && _MSC_VER < 1400 && !defined(__EDG__))
#error Microsoft Visual Studio 8 or newer is required to include this header file
#endif

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(push)
#pragma warning(disable : 4127 4308 4365 4514 4800)
#endif

#ifndef __BAMBU__
#ifndef __AC_FIXED_UTILITY_BASE
#define __AC_FIXED_UTILITY_BASE
#endif

#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

   namespace ac_private
   {
      template <typename T>
      struct rt_ac_fixed_T
      {
         template <int W, int I, bool S>
         struct op1
         {
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::mult mult;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::plus plus;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::minus2 minus;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::minus minus2;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::logic logic;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::div2 div;
            typedef typename T::template rt_T<ac_fixed<W, I, S, AC_TRN, AC_WRAP>>::div div2;
         };
      };
      // specializations after definition of ac_fixed
   } // namespace ac_private

   //////////////////////////////////////////////////////////////////////////////
   //  ac_fixed
   //////////////////////////////////////////////////////////////////////////////

   // enum ac_q_mode { AC_TRN, AC_RND, AC_TRN_ZERO, AC_RND_ZERO, AC_RND_INF,
   // AC_RND_MIN_INF, AC_RND_CONV, AC_RND_CONV_ODD }; enum ac_o_mode { AC_WRAP,
   // AC_SAT, AC_SAT_ZERO, AC_SAT_SYM };

   template <int W, int I, bool S = true, ac_q_mode Q = AC_TRN, ac_o_mode O = AC_WRAP>
   class ac_fixed : private ac_private::iv<(W + 31 + !S) / 32, false>
#ifndef __BAMBU__
                        __AC_FIXED_UTILITY_BASE
#endif
   {
      enum
      {
         N = (W + 31 + !S) / 32
      };

      template <int W2>
      struct rt_priv
      {
         enum
         {
            w_shiftl = AC_MAX(W + W2, 1)
         };
         typedef ac_fixed<w_shiftl, I, S> shiftl;
      };

      typedef ac_private::iv<N, false> Base;

      __FORCE_INLINE constexpr void bit_adjust()
      {
         Base::v.template bit_adjust<W, S>();
      }
      __FORCE_INLINE constexpr Base& base()
      {
         return *this;
      }
      __FORCE_INLINE constexpr const Base& base() const
      {
         return *this;
      }

      __FORCE_INLINE void overflow_adjust(bool underflow, bool overflow)
      {
         if(O == AC_WRAP)
         {
            bit_adjust();
            return;
         }
         else if(O == AC_SAT_ZERO)
         {
            if((overflow || underflow))
               ac_private::iv_extend<0>(Base::v, 0);
            else
               bit_adjust();
         }
         else if(S)
         {
            if(overflow)
            {
               LOOP(int, idx, 0, exclude, N - 1, { Base::v.set(idx, ~0); });
               Base::v.set(N - 1, (~((unsigned)~0 << ((W - 1) & 31))));
            }
            else if(underflow)
            {
               LOOP(int, idx, 0, exclude, N - 1, { Base::v.set(idx, 0); });
               Base::v.set(N - 1, ((unsigned)~0 << ((W - 1) & 31)));
               if(O == AC_SAT_SYM)
                  Base::v.set(0, Base::v[0] | 1);
            }
            else
               bit_adjust();
         }
         else
         {
            if(overflow)
            {
               LOOP(int, idx, 0, exclude, N - 1, { Base::v.set(idx, ~0); });
               Base::v.set(N - 1, ~((unsigned)~0 << (W & 31)));
            }
            else if(underflow)
               ac_private::iv_extend<0>(Base::v, 0);
            else
               bit_adjust();
         }
      }

      constexpr __FORCE_INLINE bool quantization_adjust(bool qb, bool r, bool s)
      {
         if(Q == AC_TRN)
            return false;
         if(Q == AC_RND_ZERO)
            qb &= s || r;
         else if(Q == AC_RND_MIN_INF)
            qb &= r;
         else if(Q == AC_RND_INF)
            qb &= !s || r;
         else if(Q == AC_RND_CONV)
            qb &= (Base::v[0] & 1) || r;
         else if(Q == AC_RND_CONV_ODD)
            qb &= (!(Base::v[0] & 1)) || r;
         else if(Q == AC_TRN_ZERO)
            qb = s && (qb || r);
         return ac_private::iv_uadd_carry(Base::v, qb, Base::v);
      }

      __FORCE_INLINE bool is_neg() const
      {
         return S && Base::v[N - 1] < 0;
      }

    public:
      static const int width = W;
      static const int i_width = I;
      static const int iwidth = I;
      static const bool sign = S;
      static const ac_o_mode o_mode = O;
      static const ac_q_mode q_mode = Q;
      static const ac_q_mode qmode = Q;
      static const ac_o_mode omode = O;
      static const int e_width = 0;

      template <int W2, int I2, bool S2>
      struct rt
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2,
            mult_w = W + W2,
            mult_i = I + I2,
            mult_s = S || S2,
            plus_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1 + AC_MAX(F, F2),
            plus_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1,
            plus_s = S || S2,
            minus_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1 + AC_MAX(F, F2),
            minus_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + 1,
            minus_s = true,
            div_w = W + AC_MAX(W2 - I2, 0) + S2,
            div_i = I + (W2 - I2) + S2,
            div_s = S || S2,
            logic_w = AC_MAX(I + (S2 && !S), I2 + (S && !S2)) + AC_MAX(F, F2),
            logic_i = AC_MAX(I + (S2 && !S), I2 + (S && !S2)),
            logic_s = S || S2
         };
         typedef ac_fixed<mult_w, mult_i, mult_s> mult;
         typedef ac_fixed<plus_w, plus_i, plus_s> plus;
         typedef ac_fixed<minus_w, minus_i, minus_s> minus;
         typedef ac_fixed<logic_w, logic_i, logic_s> logic;
         typedef ac_fixed<div_w, div_i, div_s> div;
         typedef ac_fixed<W, I, S> arg1;
      };

      template <typename T>
      struct rt_T
      {
         typedef typename ac_private::map<T>::t map_T;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::mult mult;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::plus plus;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::minus minus;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::minus2 minus2;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::logic logic;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::div div;
         typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W, I, S>::div2 div2;
         typedef ac_fixed<W, I, S> arg1;
      };

      struct rt_unary
      {
         enum
         {
            neg_w = W + 1,
            neg_i = I + 1,
            neg_s = true,
            mag_sqr_w = 2 * W - S,
            mag_sqr_i = 2 * I - S,
            mag_sqr_s = false,
            mag_w = W + S,
            mag_i = I + S,
            mag_s = false,
            leading_sign_w = ac::log2_ceil<W + !S>::val,
            leading_sign_s = false
         };
         typedef ac_int<leading_sign_w, leading_sign_s> leading_sign;
         typedef ac_fixed<neg_w, neg_i, neg_s> neg;
         typedef ac_fixed<mag_sqr_w, mag_sqr_i, mag_sqr_s> mag_sqr;
         typedef ac_fixed<mag_w, mag_i, mag_s> mag;
         template <unsigned N>
         struct set
         {
            enum
            {
               sum_w = W + ac::log2_ceil<N>::val,
               sum_i = (sum_w - W) + I,
               sum_s = S
            };
            typedef ac_fixed<sum_w, sum_i, sum_s> sum;
         };
      };

      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      friend class ac_fixed;
      constexpr ac_fixed()
      {
#if !defined(__BAMBU__) && defined(AC_DEFAULT_IN_RANGE)
         bit_adjust();
         if(O == AC_SAT_SYM && S && Base::v[N - 1] < 0 &&
            (W > 1 ? ac_private::iv_equal_zeros_to<W - 1, N>(Base::v) : true))
            Base::v.set(0, (Base::v[0] | 1));
#endif
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE constexpr ac_fixed(const ac_fixed<W2, I2, S2, Q2, O2>& op)
      {
         enum
         {
            N2 = (W2 + 31 + !S2) / 32,
            F = W - I,
            F2 = W2 - I2,
            QUAN_INC = F2 > F && !(Q == AC_TRN || (Q == AC_TRN_ZERO && !S2))
         };
         bool carry = false;
         // handle quantization
         if(F2 == F)
            Base::operator=(op);
         else if(F2 > F)
         {
            op.template const_shift_r<F2 - F>(*this);
            //      ac_private::iv_const_shift_r<N2,N,F2-F>(op.v, Base::v);
            if(Q != AC_TRN && !(Q == AC_TRN_ZERO && !S2))
            {
               bool qb = (F2 - F > W2) ? (op.v[N2 - 1] < 0) : (bool)op[F2 - F - 1];
               bool r =
                   (F2 > F + 1) ? !ac_private::iv_equal_zeros_to<((F2 > F + 1) ? F2 - F - 1 : 1), N2>(op.v) : false;
               carry = quantization_adjust(qb, r, S2 && op.v[N2 - 1] < 0);
            }
         }
         else // no quantization
            op.template const_shift_l<F - F2>(*this);
         //      ac_private::iv_const_shift_l<N2,N,F-F2>(op.v, Base::v);
         // handle overflow/underflow
         if(O != AC_WRAP &&
            ((!S && S2) || (I - S < I2 - S2 + (QUAN_INC || (S2 && O == AC_SAT_SYM && (O2 != AC_SAT_SYM || F2 > F))))))
         { // saturation
            bool deleted_bits_zero = (!(W & 31) && S) || 0 == (Base::v[N - 1] >> (W & 31));
            bool deleted_bits_one = (!(W & 31) && S) || 0 == (~(Base::v[N - 1] >> (W & 31)));
            bool neg_src = false;
            if((F2 - F + 32 * N) < W2)
            {
               bool all_ones = ac_private::iv_equal_ones_from<F2 - F + 32 * N, N2>(op.v);
               deleted_bits_zero =
                   deleted_bits_zero && (carry ? all_ones : ac_private::iv_equal_zeros_from<F2 - F + 32 * N, N2>(op.v));
               deleted_bits_one =
                   deleted_bits_one &&
                   (carry ? ac_private::iv_equal_ones_from<1 + F2 - F + 32 * N, N2>(op.v) && !op[F2 - F + 32 * N] :
                            all_ones);
               neg_src = S2 && op.v[N2 - 1] < 0 && 0 == (carry & all_ones);
            }
            else
               neg_src = S2 && op.v[N2 - 1] < 0 && Base::v[N - 1] < 0;
            bool neg_trg = S && (bool)this->operator[](W - 1);
            bool overflow = !neg_src && (neg_trg || !deleted_bits_zero);
            bool underflow = neg_src && (!neg_trg || !deleted_bits_one);
            if(O == AC_SAT_SYM && S && S2)
               underflow |=
                   neg_src && (W > 1 ? ac_private::iv_equal_zeros_to<((W > 1) ? W - 1 : 1), N>(Base::v) : true);
            overflow_adjust(underflow, overflow);
         }
         else
            bit_adjust();
      }

      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_fixed(const ac_int<W2, S2>& op)
      {
         ac_fixed<W2, W2, S2> f_op;
         f_op.base().operator=(op);
         *this = f_op;
      }

      template <int W2>
      typename rt_priv<W2>::shiftl shiftl() const
      {
         typedef typename rt_priv<W2>::shiftl shiftl_t;
         shiftl_t r;
         Base::template const_shift_l<W2>(r);
         return r;
      }

      __FORCE_INLINE constexpr ac_fixed(bool b)
      {
         *this = (ac_int<1, false>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(char b)
      {
         *this = (ac_int<8, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(signed char b)
      {
         *this = (ac_int<8, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(unsigned char b)
      {
         *this = (ac_int<8, false>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(signed short b)
      {
         *this = (ac_int<16, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(unsigned short b)
      {
         *this = (ac_int<16, false>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(signed int b)
      {
         *this = (ac_int<32, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(unsigned int b)
      {
         *this = (ac_int<32, false>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(signed long b)
      {
         *this = (ac_int<ac_private::long_w, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(unsigned long b)
      {
         *this = (ac_int<ac_private::long_w, false>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(Slong b)
      {
         *this = (ac_int<64, true>)b;
      }
      __FORCE_INLINE constexpr ac_fixed(Ulong b)
      {
         *this = (ac_int<64, false>)b;
      }

      __FORCE_INLINE constexpr ac_fixed(double d)
      {
         // printf("%f\n",d);
         double di = ac_private::ldexpr<-(I + !S + ((32 - W - !S) & 31))>(d);
         bool o = false, qb = false, r = false;
         bool neg_src = d < 0;
         Base::conv_from_fraction(di, &qb, &r, &o);
         quantization_adjust(qb, r, neg_src);
         // a neg number may become non neg (0) after quantization
         neg_src &= o || Base::v[N - 1] < 0;

         if(O != AC_WRAP)
         { // saturation
            bool overflow = false, underflow = false;
            bool neg_trg = S && (bool)this->operator[](W - 1);
            if(o)
            {
               overflow = !neg_src;
               underflow = neg_src;
            }
            else
            {
               bool deleted_bits_zero = !(W & 31) & S || !(Base::v[N - 1] >> (W & 31));
               bool deleted_bits_one = !(W & 31) & S || !~(Base::v[N - 1] >> (W & 31));
               overflow = !neg_src && (neg_trg || !deleted_bits_zero);
               underflow = neg_src && (!neg_trg || !deleted_bits_one);
            }
            if(O == AC_SAT_SYM && S)
               underflow |=
                   neg_src && (W > 1 ? ac_private::iv_equal_zeros_to<((W > 1) ? W - 1 : 1), N>(Base::v) : true);
            overflow_adjust(underflow, overflow);
         }
         else
            bit_adjust();
      }
      __FORCE_INLINE constexpr ac_fixed(float d)
      {
         // printf("%f\n",d);
         float di = ac_private::ldexpr<-(I + !S + ((32 - W - !S) & 31))>(d);
         bool o = false, qb = false, r = false;
         bool neg_src = d < 0;
         Base::conv_from_fraction(di, &qb, &r, &o);
         quantization_adjust(qb, r, neg_src);
         // a neg number may become non neg (0) after quantization
         neg_src &= o || Base::v[N - 1] < 0;

         if(O != AC_WRAP)
         { // saturation
            bool overflow = false, underflow = false;
            bool neg_trg = S && (bool)this->operator[](W - 1);
            if(o)
            {
               overflow = !neg_src;
               underflow = neg_src;
            }
            else
            {
               bool deleted_bits_zero = !(W & 31) & S || !(Base::v[N - 1] >> (W & 31));
               bool deleted_bits_one = !(W & 31) & S || !~(Base::v[N - 1] >> (W & 31));
               overflow = !neg_src && (neg_trg || !deleted_bits_zero);
               underflow = neg_src && (!neg_trg || !deleted_bits_one);
            }
            if(O == AC_SAT_SYM && S)
               underflow |=
                   neg_src && (W > 1 ? ac_private::iv_equal_zeros_to<((W > 1) ? W - 1 : 1), N>(Base::v) : true);
            overflow_adjust(underflow, overflow);
         }
         else
            bit_adjust();
      }
      template <size_t NN>
      __FORCE_INLINE constexpr ac_fixed(const char (&str)[NN])
      {
         *this = ac_fixed((double)Base::hex2doubleConverter::get(str));
      }

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(push)
#pragma warning(disable : 4700)
#endif
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif
      template <ac_special_val V>
      __FORCE_INLINE ac_fixed& set_val()
      {
         if(V == AC_VAL_DC)
         {
            ac_fixed r;
            Base::operator=(r);
            bit_adjust();
         }
         else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
         {
            Base::operator=(0);
            if(S && V == AC_VAL_MIN)
            {
               const unsigned rem = (W - 1) & 31;
               Base::v.set(N - 1, ((unsigned)~0 << rem));
               if(O == AC_SAT_SYM)
               {
                  if(W == 1)
                     Base::v.set(0, 0);
                  else
                     Base::v.set(0, Base::v[0] | 1);
               }
            }
            else if(V == AC_VAL_QUANTUM)
               Base::v.set(0, 1);
         }
         else if(V == AC_VAL_MAX)
         {
            Base::operator=(-1);
            const unsigned int rem = (32 - W - (unsigned)!S) & 31;
            Base::v.set(N - 1, ((unsigned)(-1) >> 1) >> rem);
         }
         return *this;
      }
#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(pop)
#endif
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

      // Explicit conversion functions to ac_int that captures all integer bits
      // (bits are truncated)
      __FORCE_INLINE ac_int<AC_MAX(I, 1), S> to_ac_int() const
      {
         return ((ac_fixed<AC_MAX(I, 1), AC_MAX(I, 1), S>)*this).template slc<AC_MAX(I, 1)>(0);
      }

      // Explicit conversion functions to C built-in types -------------
      __FORCE_INLINE int to_int() const
      {
         return ((I - W) >= 32) ? 0 : (signed int)to_ac_int();
      }
      __FORCE_INLINE unsigned to_uint() const
      {
         return ((I - W) >= 32) ? 0 : (unsigned int)to_ac_int();
      }
      __FORCE_INLINE long to_long() const
      {
         return ((I - W) >= ac_private::long_w) ? 0 : (signed long)to_ac_int();
      }
      __FORCE_INLINE unsigned long to_ulong() const
      {
         return ((I - W) >= ac_private::long_w) ? 0 : (unsigned long)to_ac_int();
      }
      __FORCE_INLINE Slong to_int64() const
      {
         return ((I - W) >= 64) ? 0 : (Slong)to_ac_int();
      }
      __FORCE_INLINE Ulong to_uint64() const
      {
         return ((I - W) >= 64) ? 0 : (Ulong)to_ac_int();
      }
      __FORCE_INLINE constexpr double to_double() const
      {
         return ac_private::ldexpr<I - W>(Base::to_double());
      }
      __FORCE_INLINE constexpr float to_float() const
      {
         return ac_private::ldexpr<I - W>(Base::to_float());
      }
      __FORCE_INLINE int length() const
      {
         return W;
      }

      // Cast conversion functions to C built-in types -------------
      template <int W1, bool S1>
      __FORCE_INLINE operator ac_int<W1, S1>() const
      {
         ac_int<AC_MAX(I, 1), S> temp = to_ac_int();
         return (ac_int<W1, S1>)temp;
      }
      __FORCE_INLINE operator bool() const
      {
         return !Base::equal_zero();
      }

      __FORCE_INLINE operator char() const
      {
         return (char)to_int();
      }

      __FORCE_INLINE operator signed char() const
      {
         return (signed char)to_int();
      }

      __FORCE_INLINE operator unsigned char() const
      {
         return (unsigned char)to_uint();
      }

      __FORCE_INLINE operator short() const
      {
         return (short)to_int();
      }

      __FORCE_INLINE operator unsigned short() const
      {
         return (unsigned short)to_uint();
      }
      __FORCE_INLINE operator int() const
      {
         return to_int();
      }
      __FORCE_INLINE operator unsigned() const
      {
         return to_uint();
      }
      __FORCE_INLINE operator long() const
      {
         return to_long();
      }
      __FORCE_INLINE operator unsigned long() const
      {
         return to_ulong();
      }
      __FORCE_INLINE operator Slong() const
      {
         return to_int64();
      }
      __FORCE_INLINE operator Ulong() const
      {
         return to_uint64();
      }
      __FORCE_INLINE constexpr explicit operator double() const
      {
         return to_double();
      }
      __FORCE_INLINE constexpr explicit operator float() const
      {
         return to_float();
      }

      __FORCE_INLINE std::string to_string(ac_base_mode base_rep, bool sign_mag = false) const
      {
         // base_rep == AC_DEC => sign_mag == don't care (always print decimal in
         // sign magnitude)
         char r[(W - AC_MIN(AC_MIN(W - I, I), 0) + 31) / 32 * 32 + 5] = {0};
         int i = 0;
         if(sign_mag)
            r[i++] = is_neg() ? '-' : '+';
         else if(base_rep == AC_DEC && is_neg())
            r[i++] = '-';
         if(base_rep != AC_DEC)
         {
            r[i++] = '0';
            r[i++] = base_rep == AC_BIN ? 'b' : (base_rep == AC_OCT ? 'o' : 'x');
         }
         ac_fixed<W + 1, I + 1, true> t;
         if((base_rep == AC_DEC || sign_mag) && is_neg())
            t = operator-();
         else
            t = *this;
         ac_fixed<AC_MAX(I + 1, 1), AC_MAX(I + 1, 1), true> i_part = t;
         ac_fixed<AC_MAX(W - I, 1), 0, false> f_part = t;
         i += ac_private::to_string(i_part.v, AC_MAX(I + 1, 1), sign_mag, base_rep, false, r + i);
         if(W - I > 0)
         {
            r[i++] = '.';
            if(!ac_private::to_string(f_part.v, W - I, false, base_rep, true, r + i))
               r[--i] = 0;
         }
         if(!i)
         {
            r[0] = '0';
            r[1] = 0;
         }
         return std::string(r);
      }
      __FORCE_INLINE static std::string type_name()
      {
         const char* tf[] = {"false", "true"};
         const char* q[] = {"AC_TRN",     "AC_RND",         "AC_TRN_ZERO", "AC_RND_ZERO",
                            "AC_RND_INF", "AC_RND_MIN_INF", "AC_RND_CONV", "AC_RND_CONV_ODD"};
         const char* o[] = {"AC_WRAP", "AC_SAT", "AC_SAT_ZERO", "AC_SAT_SYM"};
         std::string r = "ac_fixed<";
         r += ac_int<32, true>(W).to_string(AC_DEC) + ',';
         r += ac_int<32, true>(I).to_string(AC_DEC) + ',';
         r += tf[S];
         r += ',';
         r += q[Q];
         r += ',';
         r += o[O];
         r += '>';
         return r;
      }

      // Arithmetic : Binary ----------------------------------------------------
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      typename rt<W2, I2, S2>::mult operator*(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         auto op1_local = *this;
         op1_local.bit_adjust();
         auto op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, I2, S2>::mult r;
         op1_local.Base::mult(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      typename rt<W2, I2, S2>::plus operator+(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         typename rt<W2, I2, S2>::plus r;
         if(F == F2)
            Base::add(op2, r);
         else if(F > F2)
            Base::add(op2.template shiftl<F - F2>(), r);
         else
            shiftl<F2 - F>().add(op2, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      typename rt<W2, I2, S2>::minus operator-(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         typename rt<W2, I2, S2>::minus r;
         if(F == F2)
            Base::sub(op2, r);
         else if(F > F2)
            Base::sub(op2.template shiftl<F - F2>(), r);
         else
            shiftl<F2 - F>().sub(op2, r);
         r.bit_adjust();
         return r;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE typename rt<W2, I2, S2>::div operator/(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         typename rt<W2, I2, S2>::div r;
         enum
         {
            Num_w = W + AC_MAX(W2 - I2, 0),
            Num_i = I,
            Num_w_minus = Num_w + S,
            Num_i_minus = Num_i + S,
            N1 = ac_fixed<Num_w, Num_i, S>::N,
            N1minus = ac_fixed<Num_w_minus, Num_i_minus, S>::N,
            N2 = ac_fixed<W2, I2, S2>::N,
            N2minus = ac_fixed<W2 + S2, I2 + S2, S2>::N,
            num_s = S + (N1minus > N1),
            den_s = S2 + (N2minus > N2),
            Nr = rt<W2, I2, S2>::div::N
         };
         ac_fixed<Num_w, Num_i, S> t = *this;
         t.template div<num_s, den_s>(op2, r);
         r.bit_adjust();
         return r;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
      // Arithmetic assign  ------------------------------------------------------
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator*=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator*(op2);
         return *this;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator+=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator+(op2);
         return *this;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator-=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator-(op2);
         return *this;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator/=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator/(op2);
         return *this;
      }
      // increment/decrement by quantum (smallest difference that can be
      // represented) Arithmetic prefix increment, decrement
      // ---------------------------------
      __FORCE_INLINE ac_fixed& operator++()
      {
         ac_fixed<1, I - W + 1, false> q;
         q.template set_val<AC_VAL_QUANTUM>();
         operator+=(q);
         return *this;
      }
      __FORCE_INLINE ac_fixed& operator--()
      {
         ac_fixed<1, I - W + 1, false> q;
         q.template set_val<AC_VAL_QUANTUM>();
         operator-=(q);
         return *this;
      }
      // Arithmetic postfix increment, decrement ---------------------------------
      __FORCE_INLINE const ac_fixed operator++(int)
      {
         ac_fixed t = *this;
         ac_fixed<1, I - W + 1, false> q;
         q.template set_val<AC_VAL_QUANTUM>();
         operator+=(q);
         return t;
      }
      __FORCE_INLINE const ac_fixed operator--(int)
      {
         ac_fixed t = *this;
         ac_fixed<1, I - W + 1, false> q;
         q.template set_val<AC_VAL_QUANTUM>();
         operator-=(q);
         return t;
      }
      // Arithmetic Unary --------------------------------------------------------
      __FORCE_INLINE ac_fixed operator+() const
      {
         ac_fixed t = *this;
         t.bit_adjust();
         return t;
      }
      __FORCE_INLINE typename rt_unary::neg operator-() const
      {
         typename rt_unary::neg r;
         Base::neg(r);
         r.bit_adjust();
         return r;
      }
      // ! ------------------------------------------------------------------------
      __FORCE_INLINE bool operator!() const
      {
         return Base::equal_zero();
      }

      // Bitwise (arithmetic) unary: complement  -----------------------------
      __FORCE_INLINE ac_fixed<W + !S, I + !S, true> operator~() const
      {
         ac_fixed<W + !S, I + !S, true> r;
         Base::bitwise_complement(r);
         r.bit_adjust();
         return r;
      }
      // Bitwise (not arithmetic) bit complement  -----------------------------
      __FORCE_INLINE ac_fixed<W, I, false> bit_complement() const
      {
         ac_fixed<W, I, false> r;
         Base::bitwise_complement(r);
         r.bit_adjust();
         return r;
      }
      // Bitwise (not arithmetic): and, or, xor ----------------------------------
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE typename rt<W2, I2, S2>::logic operator&(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         typename rt<W2, I2, S2>::logic r;
         if(F == F2)
            Base::bitwise_and(op2, r);
         else if(F > F2)
            Base::bitwise_and(op2.template shiftl<F - F2>(), r);
         else
            shiftl<F2 - F>().bitwise_and(op2, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE typename rt<W2, I2, S2>::logic operator|(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         typename rt<W2, I2, S2>::logic r;
         if(F == F2)
            Base::bitwise_or(op2, r);
         else if(F > F2)
            Base::bitwise_or(op2.template shiftl<F - F2>(), r);
         else
            shiftl<F2 - F>().bitwise_or(op2, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE typename rt<W2, I2, S2>::logic operator^(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         typename rt<W2, I2, S2>::logic r;
         if(F == F2)
            Base::bitwise_xor(op2, r);
         else if(F > F2)
            Base::bitwise_xor(op2.template shiftl<F - F2>(), r);
         else
            shiftl<F2 - F>().bitwise_xor(op2, r);
         r.bit_adjust();
         return r;
      }
      // Bitwise assign (not arithmetic): and, or, xor ----------------------------
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator&=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator&(op2);
         return *this;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator|=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator|(op2);
         return *this;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE ac_fixed& operator^=(const ac_fixed<W2, I2, S2, Q2, O2>& op2)
      {
         *this = this->operator^(op2);
         return *this;
      }
      // Shift (result constrained by left operand) -------------------------------
      template <int W2>
      __FORCE_INLINE ac_fixed operator<<(const ac_int<W2, true>& op2) const
      {
         // currently not written to overflow or quantize (neg shift)
         ac_fixed r;
         Base::shift_l2(op2.to_int(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator<<(const ac_int<W2, false>& op2) const
      {
         // currently not written to overflow
         ac_fixed r;
         Base::shift_l(op2.to_uint(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator>>(const ac_int<W2, true>& op2) const
      {
         // currently not written to quantize or overflow (neg shift)
         ac_fixed r;
         Base::shift_r2(op2.to_int(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator>>(const ac_int<W2, false>& op2) const
      {
         // currently not written to quantize
         ac_fixed r;
         Base::shift_r(op2.to_uint(), r);
         r.bit_adjust();
         return r;
      }
      // Shift assign ------------------------------------------------------------
      template <int W2>
      __FORCE_INLINE ac_fixed operator<<=(const ac_int<W2, true>& op2)
      {
         // currently not written to overflow or quantize (neg shift)
         Base r;
         Base::shift_l2(op2.to_int(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator<<=(const ac_int<W2, false>& op2)
      {
         // currently not written to overflow
         Base r;
         Base::shift_l(op2.to_uint(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator>>=(const ac_int<W2, true>& op2)
      {
         // currently not written to quantize or overflow (neg shift)
         Base r;
         Base::shift_r2(op2.to_int(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_fixed operator>>=(const ac_int<W2, false>& op2)
      {
         // currently not written to quantize
         Base r;
         Base::shift_r(op2.to_uint(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      // Relational ---------------------------------------------------------------
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator==(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return Base::equal(op2);
         else if(F > F2)
            return Base::equal(op2.template shiftl<F - F2>());
         else
            return shiftl<F2 - F>().equal(op2);
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator!=(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return !Base::equal(op2);
         else if(F > F2)
            return !Base::equal(op2.template shiftl<F - F2>());
         else
            return !shiftl<F2 - F>().equal(op2);
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator<(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return Base::less_than(op2);
         else if(F > F2)
            return Base::less_than(op2.template shiftl<F - F2>());
         else
            return shiftl<F2 - F>().less_than(op2);
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator>=(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return !Base::less_than(op2);
         else if(F > F2)
            return !Base::less_than(op2.template shiftl<F - F2>());
         else
            return !shiftl<F2 - F>().less_than(op2);
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator>(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return Base::greater_than(op2);
         else if(F > F2)
            return Base::greater_than(op2.template shiftl<F - F2>());
         else
            return shiftl<F2 - F>().greater_than(op2);
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE bool operator<=(const ac_fixed<W2, I2, S2, Q2, O2>& op2) const
      {
         enum
         {
            F = W - I,
            F2 = W2 - I2
         };
         if(F == F2)
            return !Base::greater_than(op2);
         else if(F > F2)
            return !Base::greater_than(op2.template shiftl<F - F2>());
         else
            return !shiftl<F2 - F>().greater_than(op2);
      }
      __FORCE_INLINE bool operator==(double d) const
      {
         if(is_neg() != (d < 0.0))
            return false;
         double di = ac_private::ldexpr<-(I + !S + ((32 - W - !S) & 31))>(d);
         bool overflow, qb, r;
         ac_fixed<W, I, S> t;
         t.conv_from_fraction(di, &qb, &r, &overflow);
         if(qb || r || overflow)
            return false;
         return operator==(t);
      }
      __FORCE_INLINE bool operator!=(double d) const
      {
         return !operator==(d);
      }
      __FORCE_INLINE bool operator<(double d) const
      {
         if(is_neg() != (d < 0.0))
            return is_neg();
         double di = ac_private::ldexpr<-(I + !S + ((32 - W - !S) & 31))>(d);
         bool overflow, qb, r;
         ac_fixed<W, I, S> t;
         t.conv_from_fraction(di, &qb, &r, &overflow);
         if(is_neg() && overflow)
            return false;
         return (!is_neg() && overflow) || ((qb || r) && operator<=(t)) || operator<(t);
      }
      __FORCE_INLINE bool operator>=(double d) const
      {
         return !operator<(d);
      }
      __FORCE_INLINE bool operator>(double d) const
      {
         if(is_neg() != (d < 0.0))
            return !is_neg();
         double di = ac_private::ldexpr<-(I + !S + ((32 - W - !S) & 31))>(d);
         bool overflow, qb, r;
         ac_fixed<W, I, S> t;
         t.conv_from_fraction(di, &qb, &r, &overflow);
         if(!is_neg() && overflow)
            return false;
         return (is_neg() && overflow) || operator>(t);
      }
      __FORCE_INLINE bool operator<=(double d) const
      {
         return !operator>(d);
      }

      struct range_ref_fixed
      {
         ac_fixed& ref;
         int low;
         int high;
         constexpr range_ref_fixed(ac_fixed& _ref, int _high, int _low) : ref(_ref), low(_low), high(_high)
         {
         }

         template <int W2, bool S2>
         __FORCE_INLINE constexpr const range_ref_fixed& operator=(const ac_int<W2, S2>& op) const
         {
            ref.set_slc(high, low, op);
            return *this;
         }

         template <int W2, bool S2>
         __FORCE_INLINE operator const ac_int<W2, S2>() const
         {
            const ac_int<W, S> r = ref.slc(high, low);
            return r;
         }
         template <class RRF>
         __FORCE_INLINE const range_ref_fixed& operator=(const RRF& b) const
         {
            return operator=(b.operator const ac_int<W, S>());
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const int& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE constexpr const range_ref_fixed& operator=(const unsigned& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const long& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const unsigned long& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const Slong& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const Ulong& b) const
         {
            return operator=(ac_int<W, S>(b));
         }
         __FORCE_INLINE const range_ref_fixed& operator=(const range_ref_fixed& b) const
         {
            return operator=(b.operator const ac_int<W, S>());
         }
      };

      // Bit and Slice Select -----------------------------------------------------
      template <int WS, int WX, bool SX>
      __FORCE_INLINE ac_int<WS, S> slc(const ac_int<WX, SX>& index) const
      {
         ac_int<WS, S> r;
         AC_ASSERT(index >= 0, "Attempting to read slc with negative indices");
         ac_int<WX - SX, false> uindex = index;
         Base::shift_r(uindex.to_uint(), r);
         r.bit_adjust();
         return r;
      }
      __FORCE_INLINE ac_int<W, S> operator()(int Hi, int Lo) const
      {
         return slc(Hi, Lo);
      }
      __FORCE_INLINE constexpr const range_ref_fixed operator()(int Hi, int Lo)
      {
         return range_ref_fixed(*this, Hi, Lo);
      }

      template <int W1, int W2, bool S1, bool S2>
      __FORCE_INLINE ac_int<W, S> operator()(const ac_int<W1, S1>& _Hi, const ac_int<W1, S2>& _Lo) const
      {
         int Hi = _Hi.to_int();
         int Lo = _Lo.to_int();
         AC_ASSERT(Lo >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= 0, "Attempting to read slc with negative indices");
         return operator()(Hi, Lo);
      }
      template <int W1, int W2, bool S1, bool S2>
      __FORCE_INLINE range_ref_fixed operator()(const ac_int<W1, S1>& _Hi, const ac_int<W1, S2>& _Lo)
      {
         int Hi = _Hi.to_int();
         int Lo = _Lo.to_int();
         AC_ASSERT(Lo >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= 0, "Attempting to read slc with negative indices");
         return range_ref_fixed(*this, Hi, Lo);
      }

      __FORCE_INLINE ac_int<W, S> range(int Hi, int Lo) const
      {
         return operator()(Hi, Lo);
      }
      template <int W1, int W2, bool S1, bool S2>
      __FORCE_INLINE ac_int<W, S> range(const ac_int<W1, S1>& _Hi, const ac_int<W1, S2>& _Lo) const
      {
         return operator()(_Hi, _Lo);
      }
      __FORCE_INLINE range_ref_fixed range(int Hi, int Lo)
      {
         return range_ref_fixed(*this, Hi, Lo);
      }

      template <int WS>
      __FORCE_INLINE ac_int<WS, S> slc(signed index) const
      {
         ac_int<WS, S> r;
         AC_ASSERT(index >= 0, "Attempting to read slc with negative indices");
         unsigned uindex = index & ((unsigned)~0 >> 1);
         Base::shift_r(uindex, r);
         r.bit_adjust();
         return r;
      }
      template <int WS>
      __FORCE_INLINE ac_int<WS, S> slc(unsigned uindex) const
      {
         ac_int<WS, S> r;
         Base::shift_r(uindex, r);
         r.bit_adjust();
         return r;
      }
      __FORCE_INLINE ac_int<W, S> slc(int Hi, int Lo) const
      {
         AC_ASSERT(Lo >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= Lo, "Most significant bit greater than the least significant bit");
         AC_ASSERT(W > Hi, "Out of range selection");
         ac_int<W, S> r;
         Base::shift_l(W - 1 - Hi, r);
         r.bit_adjust();
         return r >> (Lo + W - 1 - Hi);
      }

      template <int W2, bool S2, int WX, bool SX>
      __FORCE_INLINE ac_fixed& set_slc(const ac_int<WX, SX> lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb.to_int() + W2 <= W && lsb.to_int() >= 0, "Out of bounds set_slc");
         ac_int<WX - SX, false> ulsb = lsb;
         Base::set_slc(ulsb.to_uint(), W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_fixed& set_slc(signed lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb + W2 <= W && lsb >= 0, "Out of bounds set_slc");
         unsigned ulsb = lsb & ((unsigned)~0 >> 1);
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_fixed& set_slc(unsigned ulsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(ulsb + W2 <= W, "Out of bounds set_slc");
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_fixed& set_slc(int umsb, int ulsb, const ac_int<W2, S2>& slc)
      {
         // AC_ASSERT((ulsb + umsb + 1) <= W, std::string(std::string("Out of bounds set_slc; umsb: ") +
         // std::to_string(umsb) + std::string(" , ulsb: ") + std::to_string(ulsb) + std::string(" , W: ") +
         // std::to_string(W)).c_str());
         Base::set_slc(ulsb, umsb + 1 - ulsb, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }

      class ac_bitref
      {
         ac_fixed& d_bv;
         unsigned d_index;

       public:
         ac_bitref(ac_fixed* bv, unsigned index = 0) : d_bv(*bv), d_index(index)
         {
         }
         __FORCE_INLINE operator bool() const
         {
            return (d_index < W) ? (d_bv.v[d_index >> 5] >> (d_index & 31) & 1) : 0;
         }

         __FORCE_INLINE ac_bitref operator=(int val)
         {
            // lsb of int (val&1) is written to bit
            if(d_index < W)
            {
               d_bv.v.set(d_index >> 5, d_bv.v[d_index >> 5] ^
                                            ((d_bv.v[d_index >> 5] ^ (val << (d_index & 31))) & 1 << (d_index & 31)));
               d_bv.bit_adjust(); // in case sign bit was assigned
            }
            return *this;
         }
         template <int W2, bool S2>
         __FORCE_INLINE ac_bitref operator=(const ac_int<W2, S2>& val)
         {
            return operator=(val.to_int());
         }
         __FORCE_INLINE ac_bitref operator=(const ac_bitref& val)
         {
            return operator=((int)(bool)val);
         }
      };

      __FORCE_INLINE ac_bitref operator[](unsigned int uindex)
      {
         AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
         ac_bitref bvh(this, uindex);
         return bvh;
      }
      __FORCE_INLINE ac_bitref operator[](int index)
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         unsigned uindex = index & ((unsigned)~0 >> 1);
         ac_bitref bvh(this, uindex);
         return bvh;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_bitref operator[](const ac_int<W2, S2>& index)
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         ac_int<W2 - S2, false> uindex = index;
         ac_bitref bvh(this, uindex.to_uint());
         return bvh;
      }

      __FORCE_INLINE bool operator[](unsigned int uindex) const
      {
         AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex & 31) & 1) : 0;
      }
      __FORCE_INLINE bool operator[](int index) const
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         unsigned uindex = index & ((unsigned)~0 >> 1);
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex & 31) & 1) : 0;
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator[](const ac_int<W2, S2>& index) const
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         ac_int<W2 - S2, false> uindex = index;
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex.to_uint() & 31) & 1) : 0;
      }
      __FORCE_INLINE typename rt_unary::leading_sign leading_sign() const
      {
         unsigned ls = Base::leading_bits(S & (Base::v[N - 1] < 0)) - (32 * N - W) - S;
         return ls;
      }
      __FORCE_INLINE typename rt_unary::leading_sign leading_sign(bool& all_sign) const
      {
         unsigned ls = Base::leading_bits(S & (Base::v[N - 1] < 0)) - (32 * N - W) - S;
         all_sign = (ls == W - S);
         return ls;
      }
      // returns false if number is denormal
      template <int WE, bool SE>
      __FORCE_INLINE bool normalize(ac_int<WE, SE>& exp)
      {
         ac_int<W, S> m = this->template slc<W>(0);
         bool r = m.normalize(exp);
         this->set_slc(0, m);
         return r;
      }
      // returns false if number is denormal, minimum exponent is reserved (usually
      // for encoding special values/errors)
      template <int WE, bool SE>
      __FORCE_INLINE bool normalize_RME(ac_int<WE, SE>& exp)
      {
         ac_int<W, S> m = this->template slc<W>(0);
         bool r = m.normalize_RME(exp);
         this->set_slc(0, m);
         return r;
      }
      __FORCE_INLINE void bit_fill_hex(const char* str)
      {
         // Zero Pads if str is too short, throws ms bits away if str is too long
         // Asserts if anything other than 0-9a-fA-F is encountered
         ac_int<W, S> x;
         x.bit_fill_hex(str);
         set_slc(0, x);
      }
      template <int N>
      __FORCE_INLINE void bit_fill(const int (&ivec)[N], bool bigendian = true)
      {
         // bit_fill from integer vector
         //   if W > N*32, missing most significant bits are zeroed
         //   if W < N*32, additional bits in ivec are ignored (no overflow checking)
         //
         // Example:
         //   ac_fixed<80,40,false> x;    int vec[] = { 0xffffa987, 0x6543210f,
         //   0xedcba987 }; x.bit_fill(vec);   // vec[0] fill bits 79-64
         ac_int<W, S> x;
         x.bit_fill(ivec, bigendian);
         set_slc(0, x);
      }
   };

   namespace ac
   {
      template <typename T>
      struct ac_fixed_represent
      {
         enum
         {
            t_w = ac_private::c_type_params<T>::W,
            t_i = t_w,
            t_s = ac_private::c_type_params<T>::S
         };
         typedef ac_fixed<t_w, t_i, t_s> type;
      };
      template <>
      struct ac_fixed_represent<float>
      {
      };
      template <>
      struct ac_fixed_represent<double>
      {
      };
      template <int W, bool S>
      struct ac_fixed_represent<ac_int<W, S>>
      {
         typedef ac_fixed<W, W, S> type;
      };
      template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
      struct ac_fixed_represent<ac_fixed<W, I, S, Q, O>>
      {
         typedef ac_fixed<W, I, S, Q, O> type;
      };
   } // namespace ac

   namespace ac_private
   {
      // with T == ac_fixed
      template <int W2, int I2, bool S2>
      struct rt_ac_fixed_T<ac_fixed<W2, I2, S2>>
      {
         typedef ac_fixed<W2, I2, S2> fx2_t;
         template <int W, int I, bool S>
         struct op1
         {
            typedef ac_fixed<W, I, S> fx_t;
            typedef typename fx_t::template rt<W2, I2, S2>::mult mult;
            typedef typename fx_t::template rt<W2, I2, S2>::plus plus;
            typedef typename fx_t::template rt<W2, I2, S2>::minus minus;
            typedef typename fx2_t::template rt<W, I, S>::minus minus2;
            typedef typename fx_t::template rt<W2, I2, S2>::logic logic;
            typedef typename fx_t::template rt<W2, I2, S2>::div div;
            typedef typename fx2_t::template rt<W, I, S>::div div2;
         };
      };
      // with T == ac_int
      template <int W2, bool S2>
      struct rt_ac_fixed_T<ac_int<W2, S2>>
      {
         typedef ac_fixed<W2, W2, S2> fx2_t;
         template <int W, int I, bool S>
         struct op1
         {
            typedef ac_fixed<W, I, S> fx_t;
            typedef typename fx_t::template rt<W2, W2, S2>::mult mult;
            typedef typename fx_t::template rt<W2, W2, S2>::plus plus;
            typedef typename fx_t::template rt<W2, W2, S2>::minus minus;
            typedef typename fx2_t::template rt<W, I, S>::minus minus2;
            typedef typename fx_t::template rt<W2, W2, S2>::logic logic;
            typedef typename fx_t::template rt<W2, W2, S2>::div div;
            typedef typename fx2_t::template rt<W, I, S>::div div2;
         };
      };

      template <typename T>
      struct rt_ac_fixed_T<c_type<T>>
      {
         typedef typename ac::ac_fixed_represent<T>::type fx2_t;
         enum
         {
            W2 = fx2_t::width,
            I2 = W2,
            S2 = fx2_t::sign
         };
         template <int W, int I, bool S>
         struct op1
         {
            typedef ac_fixed<W, I, S> fx_t;
            typedef typename fx_t::template rt<W2, W2, S2>::mult mult;
            typedef typename fx_t::template rt<W2, W2, S2>::plus plus;
            typedef typename fx_t::template rt<W2, W2, S2>::minus minus;
            typedef typename fx2_t::template rt<W, I, S>::minus minus2;
            typedef typename fx_t::template rt<W2, W2, S2>::logic logic;
            typedef typename fx_t::template rt<W2, W2, S2>::div div;
            typedef typename fx2_t::template rt<W, I, S>::div div2;
         };
      };
   } // namespace ac_private

   // Specializations for constructors on integers that bypass bit adjusting
   //  and are therefore more efficient
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, true, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b ? -1 : 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(signed char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(signed short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(signed int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(signed long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(Ulong b)
   {
      v.set(0, (int)b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<1, 1, false, AC_TRN, AC_WRAP>::ac_fixed(Slong b)
   {
      v.set(0, (int)b & 1);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, true, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, false, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, true, AC_TRN, AC_WRAP>::ac_fixed(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, true, AC_TRN, AC_WRAP>::ac_fixed(unsigned char b)
   {
      v.set(0, (signed char)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<8, 8, false, AC_TRN, AC_WRAP>::ac_fixed(signed char b)
   {
      v.set(0, (unsigned char)b);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, true, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, false, AC_TRN, AC_WRAP>::ac_fixed(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, true, AC_TRN, AC_WRAP>::ac_fixed(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, true, AC_TRN, AC_WRAP>::ac_fixed(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, false, AC_TRN, AC_WRAP>::ac_fixed(signed char b)
   {
      v.set(0, (unsigned short)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, true, AC_TRN, AC_WRAP>::ac_fixed(signed short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, true, AC_TRN, AC_WRAP>::ac_fixed(unsigned short b)
   {
      v.set(0, (signed short)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<16, 16, false, AC_TRN, AC_WRAP>::ac_fixed(signed short b)
   {
      v.set(0, (unsigned short)b);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, true, AC_TRN, AC_WRAP>::ac_fixed(signed int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, true, AC_TRN, AC_WRAP>::ac_fixed(unsigned int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, false, AC_TRN, AC_WRAP>::ac_fixed(signed int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, false, AC_TRN, AC_WRAP>::ac_fixed(unsigned int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, true, AC_TRN, AC_WRAP>::ac_fixed(Slong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, true, AC_TRN, AC_WRAP>::ac_fixed(Ulong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, false, AC_TRN, AC_WRAP>::ac_fixed(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<32, 32, false, AC_TRN, AC_WRAP>::ac_fixed(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_fixed<64, 64, true, AC_TRN, AC_WRAP>::ac_fixed(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<64, 64, true, AC_TRN, AC_WRAP>::ac_fixed(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<64, 64, false, AC_TRN, AC_WRAP>::ac_fixed(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)((Ulong)b >> 32));
      v.set(2, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_fixed<64, 64, false, AC_TRN, AC_WRAP>::ac_fixed(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
      v.set(2, 0);
   }

   // Stream --------------------------------------------------------------------

   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   __FORCE_INLINE std::ostream& operator<<(std::ostream& os, const ac_fixed<W, I, S, Q, O>& x)
   {
#ifndef __BAMBU__
      os << x.to_string(AC_DEC);
#endif
      return os;
   }

   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   __FORCE_INLINE std::istream& operator>>(std::istream& in, ac_fixed<W, I, S, Q, O>& x)
   {
#ifndef __BAMBU__
      double d;
      in >> d;
      x = ac_fixed<W, I, S, Q, O>(d);
#endif
      return in;
   }

   // Macros for Binary Operators with C Integers
   // --------------------------------------------

#define FX_BIN_OP_WITH_INT_2I(BIN_OP, C_TYPE, WI, SI, RTYPE)                                  \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                                  \
   __FORCE_INLINE typename ac_fixed<W, I, S>::template rt<WI, WI, SI>::RTYPE operator BIN_OP( \
       const ac_fixed<W, I, S, Q, O>& op, C_TYPE i_op)                                        \
   {                                                                                          \
      return op.operator BIN_OP(ac_int<WI, SI>(i_op));                                        \
   }

#define FX_BIN_OP_WITH_INT(BIN_OP, C_TYPE, WI, SI, RTYPE)                                     \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                                  \
   __FORCE_INLINE typename ac_fixed<WI, WI, SI>::template rt<W, I, S>::RTYPE operator BIN_OP( \
       C_TYPE i_op, const ac_fixed<W, I, S, Q, O>& op)                                        \
   {                                                                                          \
      return ac_fixed<WI, WI, SI>(i_op).operator BIN_OP(op);                                  \
   }                                                                                          \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                                  \
   __FORCE_INLINE typename ac_fixed<W, I, S>::template rt<WI, WI, SI>::RTYPE operator BIN_OP( \
       const ac_fixed<W, I, S, Q, O>& op, C_TYPE i_op)                                        \
   {                                                                                          \
      return op.operator BIN_OP(ac_fixed<WI, WI, SI>(i_op));                                  \
   }

#define FX_REL_OP_WITH_INT(REL_OP, C_TYPE, W2, S2)                                    \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                          \
   __FORCE_INLINE bool operator REL_OP(const ac_fixed<W, I, S, Q, O>& op, C_TYPE op2) \
   {                                                                                  \
      return op.operator REL_OP(ac_fixed<W2, W2, S2>(op2));                           \
   }                                                                                  \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                          \
   __FORCE_INLINE bool operator REL_OP(C_TYPE op2, const ac_fixed<W, I, S, Q, O>& op) \
   {                                                                                  \
      return ac_fixed<W2, W2, S2>(op2).operator REL_OP(op);                           \
   }

#define FX_ASSIGN_OP_WITH_INT_2(ASSIGN_OP, C_TYPE, W2, S2)                                             \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                                           \
   __FORCE_INLINE ac_fixed<W, I, S, Q, O>& operator ASSIGN_OP(ac_fixed<W, I, S, Q, O>& op, C_TYPE op2) \
   {                                                                                                   \
      return op.operator ASSIGN_OP(ac_fixed<W2, W2, S2>(op2));                                         \
   }

#define FX_ASSIGN_OP_WITH_INT_2I(ASSIGN_OP, C_TYPE, W2, S2)                                     \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>                                    \
   __FORCE_INLINE ac_fixed<W, I, S> operator ASSIGN_OP(ac_fixed<W, I, S, Q, O>& op, C_TYPE op2) \
   {                                                                                            \
      return op.operator ASSIGN_OP(ac_int<W2, S2>(op2));                                        \
   }

#define FX_OPS_WITH_INT(C_TYPE, WI, SI)            \
   FX_BIN_OP_WITH_INT(*, C_TYPE, WI, SI, mult)     \
   FX_BIN_OP_WITH_INT(+, C_TYPE, WI, SI, plus)     \
   FX_BIN_OP_WITH_INT(-, C_TYPE, WI, SI, minus)    \
   FX_BIN_OP_WITH_INT(/, C_TYPE, WI, SI, div)      \
   FX_BIN_OP_WITH_INT_2I(>>, C_TYPE, WI, SI, arg1) \
   FX_BIN_OP_WITH_INT_2I(<<, C_TYPE, WI, SI, arg1) \
   FX_BIN_OP_WITH_INT(&, C_TYPE, WI, SI, logic)    \
   FX_BIN_OP_WITH_INT(|, C_TYPE, WI, SI, logic)    \
   FX_BIN_OP_WITH_INT(^, C_TYPE, WI, SI, logic)    \
                                                   \
   FX_REL_OP_WITH_INT(==, C_TYPE, WI, SI)          \
   FX_REL_OP_WITH_INT(!=, C_TYPE, WI, SI)          \
   FX_REL_OP_WITH_INT(>, C_TYPE, WI, SI)           \
   FX_REL_OP_WITH_INT(>=, C_TYPE, WI, SI)          \
   FX_REL_OP_WITH_INT(<, C_TYPE, WI, SI)           \
   FX_REL_OP_WITH_INT(<=, C_TYPE, WI, SI)          \
                                                   \
   FX_ASSIGN_OP_WITH_INT_2(+=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2(-=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2(*=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2(/=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2I(>>=, C_TYPE, WI, SI)   \
   FX_ASSIGN_OP_WITH_INT_2I(<<=, C_TYPE, WI, SI)   \
   FX_ASSIGN_OP_WITH_INT_2(&=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2(|=, C_TYPE, WI, SI)     \
   FX_ASSIGN_OP_WITH_INT_2(^=, C_TYPE, WI, SI)

   // --------------------------------------- End of Macros for Binary Operators
   // with C Integers

   namespace ac
   {
      namespace ops_with_other_types
      {
         // Binary Operators with C Integers --------------------------------------------
         FX_OPS_WITH_INT(bool, 1, false)
         FX_OPS_WITH_INT(char, 8, true)
         FX_OPS_WITH_INT(signed char, 8, true)
         FX_OPS_WITH_INT(unsigned char, 8, false)
         FX_OPS_WITH_INT(short, 16, true)
         FX_OPS_WITH_INT(unsigned short, 16, false)
         FX_OPS_WITH_INT(int, 32, true)
         FX_OPS_WITH_INT(unsigned int, 32, false)
         FX_OPS_WITH_INT(long, ac_private::long_w, true)
         FX_OPS_WITH_INT(unsigned long, ac_private::long_w, false)
         FX_OPS_WITH_INT(Slong, 64, true)
         FX_OPS_WITH_INT(Ulong, 64, false)
         // -------------------------------------- End of Binary Operators with Integers
      } // namespace ops_with_other_types

   } // namespace ac

   // Macros for Binary Operators with ac_int
   // --------------------------------------------

#define FX_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE)                                                \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                 \
   __FORCE_INLINE typename ac_fixed<WI, WI, SI>::template rt<W, I, S>::RTYPE operator BIN_OP( \
       const ac_int<WI, SI>& i_op, const ac_fixed<W, I, S, Q, O>& op)                         \
   {                                                                                          \
      return ac_fixed<WI, WI, SI>(i_op).operator BIN_OP(op);                                  \
   }

#define FX_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)                                                \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                 \
   __FORCE_INLINE typename ac_fixed<W, I, S>::template rt<WI, WI, SI>::RTYPE operator BIN_OP( \
       const ac_fixed<W, I, S, Q, O>& op, const ac_int<WI, SI>& i_op)                         \
   {                                                                                          \
      return op.operator BIN_OP(ac_fixed<WI, WI, SI>(i_op));                                  \
   }

#define FX_BIN_OP_WITH_AC_INT(BIN_OP, RTYPE) \
   FX_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE)    \
   FX_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)

#define FX_REL_OP_WITH_AC_INT(REL_OP)                                                                \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                        \
   __FORCE_INLINE bool operator REL_OP(const ac_fixed<W, I, S, Q, O>& op, const ac_int<WI, SI>& op2) \
   {                                                                                                 \
      return op.operator REL_OP(ac_fixed<WI, WI, SI>(op2));                                          \
   }                                                                                                 \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                        \
   __FORCE_INLINE bool operator REL_OP(ac_int<WI, SI>& op2, const ac_fixed<W, I, S, Q, O>& op)       \
   {                                                                                                 \
      return ac_fixed<WI, WI, SI>(op2).operator REL_OP(op);                                          \
   }

#define FX_ASSIGN_OP_WITH_AC_INT(ASSIGN_OP)                                                                           \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                                         \
   __FORCE_INLINE ac_fixed<W, I, S, Q, O>& operator ASSIGN_OP(ac_fixed<W, I, S, Q, O>& op, const ac_int<WI, SI>& op2) \
   {                                                                                                                  \
      return op.operator ASSIGN_OP(ac_fixed<WI, WI, SI>(op2));                                                        \
   }                                                                                                                  \
   template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI>                                         \
   __FORCE_INLINE ac_int<WI, SI>& operator ASSIGN_OP(ac_int<WI, SI>& op, const ac_fixed<W, I, S, Q, O>& op2)          \
   {                                                                                                                  \
      return op.operator ASSIGN_OP(op2.to_ac_int());                                                                  \
   }

   // -------------------------------------------- End of Macros for Binary
   // Operators with ac_int

   namespace ac
   {
      namespace ops_with_other_types
      {
         // Binary Operators with ac_int --------------------------------------------
         FX_BIN_OP_WITH_AC_INT(*, mult)
         FX_BIN_OP_WITH_AC_INT(+, plus)
         FX_BIN_OP_WITH_AC_INT(-, minus)
         FX_BIN_OP_WITH_AC_INT(/, div)
         FX_BIN_OP_WITH_AC_INT(&, logic)
         FX_BIN_OP_WITH_AC_INT(|, logic)
         FX_BIN_OP_WITH_AC_INT(^, logic)

         FX_REL_OP_WITH_AC_INT(==)
         FX_REL_OP_WITH_AC_INT(!=)
         FX_REL_OP_WITH_AC_INT(>)
         FX_REL_OP_WITH_AC_INT(>=)
         FX_REL_OP_WITH_AC_INT(<)
         FX_REL_OP_WITH_AC_INT(<=)

         FX_ASSIGN_OP_WITH_AC_INT(+=)
         FX_ASSIGN_OP_WITH_AC_INT(-=)
         FX_ASSIGN_OP_WITH_AC_INT(*=)
         FX_ASSIGN_OP_WITH_AC_INT(/=)
         FX_ASSIGN_OP_WITH_AC_INT(%=)
         FX_ASSIGN_OP_WITH_AC_INT(&=)
         FX_ASSIGN_OP_WITH_AC_INT(|=)
         FX_ASSIGN_OP_WITH_AC_INT(^=)
         // -------------------------------------- End of Binary Operators with ac_int

         // Relational Operators with double --------------------------------------
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator==(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator==(op);
         }
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator!=(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator!=(op);
         }
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator>(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator<(op);
         }
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator<(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator>(op);
         }
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator<=(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator>=(op);
         }
         template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
         __FORCE_INLINE bool operator>=(double op, const ac_fixed<W, I, S, Q, O>& op2)
         {
            return op2.operator<=(op);
         }
         // -------------------------------------- End of Relational Operators with
         // double

      } // namespace ops_with_other_types
   }    // namespace ac

   using namespace ac::ops_with_other_types;

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(disable : 4700)
#endif
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif

   // Global templatized functions for easy initialization to special values
   template <ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   __FORCE_INLINE ac_fixed<W, I, S, Q, O> value(ac_fixed<W, I, S, Q, O>)
   {
      ac_fixed<W, I, S> r;
      return r.template set_val<V>();
   }

   namespace ac
   {
      // PUBLIC FUNCTIONS
      // function to initialize (or uninitialize) arrays
      template <ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
      __FORCE_INLINE bool init_array(ac_fixed<W, I, S, Q, O>* a, int n)
      {
         ac_fixed<W, I, S> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }

      __FORCE_INLINE ac_fixed<54, 2, true> frexp_d(double d, ac_int<11, true>& exp)
      {
         enum
         {
            Min_Exp = -1022,
            Max_Exp = 1023,
            Mant_W = 52,
            Denorm_Min_Exp = Min_Exp - Mant_W
         };
         if(!d)
         {
            exp = 0;
            return 0;
         }
         int exp_i;
         double f0 = frexp(d, &exp_i);
         AC_ASSERT(exp_i <= Max_Exp + 1, "Exponent greater than standard double-precision float exponent "
                                         "max (+1024). It is probably an extended double");
         AC_ASSERT(exp_i >= Denorm_Min_Exp + 1, "Exponent less than standard double-precision float exponent min "
                                                "(-1021). It is probably an extended double");
         exp_i--;
         int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : (exp_i > Min_Exp && f0 < 0 && f0 >= -0.5) ? -1 : 0;
         exp = exp_i + rshift;
         ac_int<Mant_W + 2, true> f_i = f0 * ((Ulong)1 << (Mant_W + 1 - rshift));
         ac_fixed<Mant_W + 2, 2, true> r;
         r.set_slc(0, f_i);
         return r;
      }
      __FORCE_INLINE ac_fixed<25, 2, true> frexp_f(float f, ac_int<8, true>& exp)
      {
         enum
         {
            Min_Exp = -126,
            Max_Exp = 127,
            Mant_W = 23,
            Denorm_Min_Exp = Min_Exp - Mant_W
         };
         if(!f)
         {
            exp = 0;
            return 0;
         }
         int exp_i;
         float f0 = frexpf(f, &exp_i);
         AC_ASSERT(exp_i <= Max_Exp + 1, "Exponent greater than standard single-precision float exponent "
                                         "max (+128). It is probably an extended float");
         AC_ASSERT(exp_i >= Denorm_Min_Exp + 1, "Exponent less than standard single-precision float exponent min "
                                                "(-125). It is probably an extended float");
         exp_i--;
         int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : (exp_i >= Min_Exp && f0 < 0 && f0 >= -0.5) ? -1 : 0;
         exp = exp_i + rshift;
         ac_int<Mant_W + 2, true> f_i = f0 * (1 << (Mant_W + 1 - rshift));
         ac_fixed<Mant_W + 2, 2, true> r;
         r.set_slc(0, f_i);
         return r;
      }

      __FORCE_INLINE ac_fixed<53, 1, false> frexp_sm_d(double d, ac_int<11, true>& exp, bool& sign)
      {
         enum
         {
            Min_Exp = -1022,
            Max_Exp = 1023,
            Mant_W = 52,
            Denorm_Min_Exp = Min_Exp - Mant_W
         };
         if(!d)
         {
            exp = 0;
            sign = false;
            return 0;
         }
         int exp_i;
         bool s = d < 0;
         double f0 = frexp(s ? -d : d, &exp_i);
         AC_ASSERT(exp_i <= Max_Exp + 1, "Exponent greater than standard double-precision float exponent "
                                         "max (+1024). It is probably an extended double");
         AC_ASSERT(exp_i >= Denorm_Min_Exp + 1, "Exponent less than standard double-precision float exponent min "
                                                "(-1021). It is probably an extended double");
         exp_i--;
         int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : 0;
         exp = exp_i + rshift;
         ac_int<Mant_W + 1, false> f_i = f0 * ((Ulong)1 << (Mant_W + 1 - rshift));
         ac_fixed<Mant_W + 1, 1, false> r;
         r.set_slc(0, f_i);
         sign = s;
         return r;
      }
      __FORCE_INLINE ac_fixed<24, 1, false> frexp_sm_f(float f, ac_int<8, true>& exp, bool& sign)
      {
         enum
         {
            Min_Exp = -126,
            Max_Exp = 127,
            Mant_W = 23,
            Denorm_Min_Exp = Min_Exp - Mant_W
         };
         if(!f)
         {
            exp = 0;
            sign = false;
            return 0;
         }
         int exp_i;
         bool s = f < 0;
         float f0 = frexp(s ? -f : f, &exp_i);
         AC_ASSERT(exp_i <= Max_Exp + 1, "Exponent greater than standard single-precision float exponent "
                                         "max (+128). It is probably an extended float");
         AC_ASSERT(exp_i >= Denorm_Min_Exp + 1, "Exponent less than standard single-precision float exponent min "
                                                "(-125). It is probably an extended float");
         exp_i--;
         int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : 0;
         exp = exp_i + rshift;
         ac_int<24, false> f_i = f0 * (1 << (Mant_W + 1 - rshift));
         ac_fixed<24, 1, false> r;
         r.set_slc(0, f_i);
         sign = s;
         return r;
      }
   } // namespace ac

   ///////////////////////////////////////////////////////////////////////////////

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(pop)
#endif
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef __AC_NAMESPACE
}
#endif

#endif // __AC_FIXED_H
