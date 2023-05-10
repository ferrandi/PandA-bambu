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
 *  Copyright 2004-2017, Mentor Graphics Corporation,                     *
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
//  Source:          ac_int.h
//  Description:     fast arbitrary-length bit-accurate integer types:
//                     - unsigned integer of length W:  ac_int<W,false>
//                     - signed integer of length W:  ac_int<W,true>
//  Original Author: Andres Takach, Ph.D.
//  Modified by:     Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//                   Michele Fiorito <michele.fiorito@polimi.it>
//  Notes:
//   - C++ Runtime: PandA/bambu requires optimization enabled (-O2 at least).
//
//   - Compiler support: This version of the library support both Clang and
//     GCC.
//     Clang/llvm is able to lower ac_types to builtin data types when
//     optimizations are enable.
//     GCC is working well but will not lower the ac_types to builtin when they
//     are used as top level function parameter data types.
//     To improve the efficiency of hardware generated, some optimizations have
//     been done:
//     - bit_adjust has been removed. Bitwidth restriction is now performed by
//       set function.
//     - the class iv use specialized classes to store the array of integers. In
//       particular, for ac_types requiring less than 32bits a single integer
//       object is used. This makes the SROA compiler step more effective.
//       Similar optimizations have been done even for larger data types.
//     - Inlining is forced almost everywhere. This implies many simplifications
//       and an effective hardware generation process.
//     - constexpr and some features from c++14 have been used to propagate
//       constants at compile time.
//   - Library extension: the library has been extended to make it more
//     compatible with the ap_types library. In particular, it has been added:
//     - the range operator over a slice of bits
//     - a constructor from a constant string able to convert such string
//       in a double.
//     - conversion operators from string to ac_int
//
//   - Most frequent migration issues:
//      - need to cast to common type when using question mark operator:
//          (a < 0) ? -a : a;  // a is ac_int<W,true>
//        change to:
//          (a < 0) ? -a : (ac_int<W+1,true>) a;
//        or
//          (a < 0) ? (ac_int<W+1,false>) -a : (ac_int<W+1,false>) a;
//
//      - left shift is not arithmetic ("a<<n" has same bitwidth as "a")
//          ac_int<W+1,false> b = a << 1;  // a is ac_int<W,false>
//        is not equivalent to b=2*a. In order to get 2*a behavior change to:
//          ac_int<W+1,false> b = (ac_int<W+1,false>)a << 1;
//
//      - only static length read/write slices are supported:
//         - read:  x.slc<4>(k) => returns ac_int for 4-bit slice x(4+k-1 DOWNTO k)
//         - write: x.set_slc(k,y) = writes bits of y to x starting at index k
*/

#ifndef __AC_INT_H
#define __AC_INT_H

#define AC_VERSION 3
#define AC_VERSION_MINOR 7

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

#if(defined(__GNUC__) && __GNUC__ < 3 && !defined(__EDG__))
#error GCC version 3 or greater is required to include this header file
#endif

#if(defined(_MSC_VER) && _MSC_VER < 1400 && !defined(__EDG__))
#error Microsoft Visual Studio 8 or newer is required to include this header file
#endif

#if(defined(_MSC_VER) && !defined(__EDG__))
#pragma warning(push)
#pragma warning(disable : 4127 4100 4244 4307 4310 4365 4514 4554 4706 4800)
#endif

// for safety
#if(defined(N) || defined(N2))
#error One or more of the following is defined: N, N2. Definition conflicts with their usage as template parameters.
#error DO NOT use defines before including third party header files.
#endif

// for safety
#if(defined(W) || defined(I) || defined(S) || defined(W2) || defined(I2) || defined(S2))
#error One or more of the following is defined: W, I, S, W2, I2, S2. Definition conflicts with their usage as template parameters.
#error DO NOT use defines before including third party header files.
#endif

#ifndef __FORCE_INLINE
#define __FORCE_INLINE __attribute__((always_inline)) inline
#endif

#define __INIT_VALUE = {0}
#define __INIT_VALUE_LL = {0}

#ifndef __ASSERT_H__
#define __ASSERT_H__
#include <cassert>
#endif
#include <limits>
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
#ifndef AC_USER_DEFINED_ASSERT
#include <iostream>
#else
#include <ostream>
#endif
#endif
#include <cmath>
#include <string>

#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
#ifndef __AC_INT_UTILITY_BASE
#define __AC_INT_UTILITY_BASE
#endif

#endif

#include <type_traits>
enum loop_limit
{
   exclude,
   include
};
#if __cplusplus >= 201703L
#include <utility>
namespace detail
{
   template <bool ascDesc, class T, T base, T... inds, class F, std::enable_if_t<ascDesc == false, bool> = true>
   __FORCE_INLINE constexpr void loop(std::integer_sequence<T, inds...>, F&& f)
   {
      (f(std::integral_constant<T, inds>{} + base), ...);
   }

   template <bool ascDesc, class T, T base, T... inds, class F, std::enable_if_t<ascDesc == true, bool> = true>
   __FORCE_INLINE constexpr void loop(std::integer_sequence<T, inds...>, F&& f)
   {
      (f(base - std::integral_constant<T, inds>{}), ...);
   }

} // namespace detail

template <class T, T start, loop_limit limit, T end, class F>
__FORCE_INLINE constexpr void loop(F&& f)
{
   constexpr T range = (start < end) ? end - start : start - end;
   detail::loop<(start > end), T, start>(std::make_integer_sequence<T, range + (limit == loop_limit::include)>{},
                                         std::forward<F>(f));
}

#define LOOP(ctype, cname, start, limit, end, body) loop<ctype, start, limit, end>([&](ctype cname) { body });
#else
#if defined(__VIVADO__)
#define LOOP(ctype, cname, start, limit, end, body)                                    \
   if(start <= end)                                                                    \
   {                                                                                   \
      for(ctype cname = start; cname < end + (limit == loop_limit::include); ++cname)  \
      {                                                                                \
         _Pragma("HLS UNROLL") body                                                    \
      }                                                                                \
   }                                                                                   \
   else                                                                                \
   {                                                                                   \
      for(ctype cname = start; cname >= end + (limit == loop_limit::exclude); --cname) \
      {                                                                                \
         _Pragma("HLS UNROLL") body                                                    \
      }                                                                                \
   }
#elif defined(__clang__)
#define LOOP(ctype, cname, start, limit, end, body)                                                              \
   if(start <= end)                                                                                              \
   {                                                                                                             \
      _Pragma("clang loop unroll(full)") for(ctype cname = start; cname < end + (limit == loop_limit::include);  \
                                             ++cname)                                                            \
      {                                                                                                          \
         body                                                                                                    \
      }                                                                                                          \
   }                                                                                                             \
   else                                                                                                          \
   {                                                                                                             \
      _Pragma("clang loop unroll(full)") for(ctype cname = start; cname >= end + (limit == loop_limit::exclude); \
                                             --cname)                                                            \
      {                                                                                                          \
         body                                                                                                    \
      }                                                                                                          \
   }
#else
#define LOOP(ctype, cname, start, limit, end, body)                                    \
   if(start <= end)                                                                    \
   {                                                                                   \
      for(ctype cname = start; cname < end + (limit == loop_limit::include); ++cname)  \
      {                                                                                \
         body                                                                          \
      }                                                                                \
   }                                                                                   \
   else                                                                                \
   {                                                                                   \
      for(ctype cname = start; cname >= end + (limit == loop_limit::exclude); --cname) \
      {                                                                                \
         body                                                                          \
      }                                                                                \
   }
#endif
#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

#define AC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define AC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AC_ABS(a) ((a) < 0 ? (-a) : (a))

#if defined(_MSC_VER)
   typedef unsigned __int64 Ulong;
   typedef signed __int64 Slong;
#else
using Ulong = unsigned long long;
using Slong = long long;
#endif

   enum ac_base_mode
   {
      AC_BIN = 2,
      AC_OCT = 8,
      AC_DEC = 10,
      AC_HEX = 16
   };
   enum ac_special_val
   {
      AC_VAL_DC,
      AC_VAL_0,
      AC_VAL_MIN,
      AC_VAL_MAX,
      AC_VAL_QUANTUM
   };

   template <int W, bool S>
   class ac_int;

   namespace ac_private
   {
      enum
      {
         long_w = std::numeric_limits<unsigned long>::digits
      };
      const unsigned int all_ones = (unsigned)~0;

      // PRIVATE FUNCTIONS in namespace: for implementing ac_int/ac_fixed

      template <typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type* = nullptr>
      constexpr T float_floor(T v) noexcept
      {
         constexpr int max_bits = std::numeric_limits<T>::max_exponent == 128 ? 23 : 52;
         if(v != v || v >= T(1LL << max_bits) || v <= -T(1LL << max_bits))
         {
            return v;
         }
         else if(T(-1) < v && v < T(1))
         {
            return T(0);
         }
         const T rnd = T(static_cast<long long int>(v));
         auto res = rnd - T(rnd != v && v < T(0));
         return res;
      }

      __FORCE_INLINE constexpr double mgc_floor(double d)
      {
         return float_floor(d);
      }
      __FORCE_INLINE constexpr float mgc_floor(float d)
      {
         return float_floor(d);
      }

#if defined(__BAMBU__) && !defined(__BAMBU_SIM__)
#define AC_ASSERT(cond, msg)
#else
#define AC_ASSERT(cond, msg) ac_private::ac_assert(cond, __FILE__, __LINE__, msg)
   __FORCE_INLINE void ac_assert(bool condition, const char* file = nullptr, int line = 0, const char* msg = nullptr)
   {
#ifndef AC_USER_DEFINED_ASSERT
      if(!condition)
      {
         std::cerr << "Assert";
         if(file)
         {
            std::cerr << " in file " << file << ":" << line;
         }
         if(msg)
         {
            std::cerr << " " << msg;
         }
         std::cerr << std::endl;
         assert(0);
      }
#endif
   }
#endif
      // helper structs for statically computing log2 like functions (nbits,
      // log2_floor, log2_ceil)
      //   using recursive templates
      template <unsigned char N>
      struct s_N
      {
         template <unsigned X>
         struct s_X
         {
            enum
            {
               X2 = X >> N,
               N_div_2 = N >> 1,
               nbits = X ? (X2 ? N + (int)s_N<N_div_2>::template s_X<X2>::nbits :
                                 (int)s_N<N_div_2>::template s_X<X>::nbits) :
                           0
            };
         };
      };
      template <>
      struct s_N<0>
      {
         template <unsigned X>
         struct s_X
         {
            enum
            {
               nbits = !!X
            };
         };
      };

      template <int N>
      __FORCE_INLINE constexpr double ldexpr32(double d)
      {
         double d2 = d;
         if(N < 0)
         {
            LOOP(int, i, 0, exclude, -N, { d2 /= (Ulong)1 << 32; });
         }
         else
         {
            LOOP(int, i, 0, exclude, N, { d2 *= (Ulong)1 << 32; });
         }
         return d2;
      }
      template <>
      __FORCE_INLINE constexpr double ldexpr32<0>(double d)
      {
         return d;
      }
      template <>
      __FORCE_INLINE constexpr double ldexpr32<1>(double d)
      {
         return d * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr double ldexpr32<-1>(double d)
      {
         return d / ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr double ldexpr32<2>(double d)
      {
         return (d * ((Ulong)1 << 32)) * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr double ldexpr32<-2>(double d)
      {
         return (d / ((Ulong)1 << 32)) / ((Ulong)1 << 32);
      }

      template <int N>
      __FORCE_INLINE constexpr double ldexpr(double d)
      {
         return ldexpr32<N / 32>(N < 0 ? d / ((unsigned)1 << (-N & 31)) : d * ((unsigned)1 << (N & 31)));
      }

      template <int N>
      __FORCE_INLINE constexpr float ldexpr32(float d)
      {
         float d2 = d;
         if(N < 0)
         {
            LOOP(int, i, 0, exclude, -N, { d2 /= (Ulong)1 << 32; });
         }
         else
         {
            LOOP(int, i, 0, exclude, N, { d2 *= (Ulong)1 << 32; });
         }
         return d2;
      }
      template <>
      __FORCE_INLINE constexpr float ldexpr32<0>(float d)
      {
         return d;
      }
      template <>
      __FORCE_INLINE constexpr float ldexpr32<1>(float d)
      {
         return d * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr float ldexpr32<-1>(float d)
      {
         return d / ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr float ldexpr32<2>(float d)
      {
         return (d * ((Ulong)1 << 32)) * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE constexpr float ldexpr32<-2>(float d)
      {
         return (d / ((Ulong)1 << 32)) / ((Ulong)1 << 32);
      }

      template <int N>
      __FORCE_INLINE constexpr float ldexpr(float d)
      {
         return ldexpr32<N / 32>(N < 0 ? d / ((unsigned)1 << (-N & 31)) : d * ((unsigned)1 << (N & 31)));
      }

#if __clang_major__ >= 16
      template <int N, bool C, int W0, bool S0>
      class iv_base
      {
       public:
         signed _BitInt(W0) v __INIT_VALUE;

         __FORCE_INLINE void assign_int64(Slong l)
         {
            v = l;
         }

         __FORCE_INLINE constexpr void assign_uint64(Ulong l)
         {
            v = l;
         }
         __FORCE_INLINE constexpr void set(int x, int value)
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            bool updateRes = x * 32 < W0;
            if(!updateRes)
            {
               return;
            }
            auto mask1 = static_cast<unsigned _BitInt(W0)>(all_ones);
            mask1 <<= x * 32;
            unsigned uvalue = value;
            unsigned _BitInt(W0) uvalueBI = uvalue;
            uvalueBI <<= x * 32;
            v ^= (v ^ (uvalueBI)) & mask1;
         }

         __FORCE_INLINE constexpr Slong to_int64() const
         {
            if(S0)
            {
               Slong conv = v;
               return conv;
            }
            else
            {
               unsigned _BitInt(W0) uv = v;
               Slong conv = uv;
               return conv;
            }
         }

         __FORCE_INLINE constexpr int operator[](int x) const
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            bool returnRes = x * 32 < W0;
            int res = 0;
            if(S0)
            {
               res = returnRes ? v >> (32 * x) : v >> (W0 - 1);
            }
            else
            {
               unsigned _BitInt(W0) uv = v;
               unsigned resu = returnRes ? uv >> (32 * x) : 0;
               res = resu;
            }
            return res;
         }

         iv_base() = default;
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b) : v(b.v)
         {
         }
      } __attribute__((aligned(8)));

      template <int N, bool C>
      class iv_base<N, C, 1, true>
      {
         bool v __INIT_VALUE;

       public:
         __FORCE_INLINE void assign_int64(Slong l)
         {
            set(0, static_cast<int>(l));
         }

         __FORCE_INLINE constexpr void assign_uint64(Ulong l)
         {
            set(0, static_cast<int>(l));
         }
         __FORCE_INLINE constexpr void set(int x, int value)
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            v = value & 1;
         }

         __FORCE_INLINE constexpr Slong to_int64() const
         {
            return v ? -1LL : 0;
         }

         __FORCE_INLINE constexpr int operator[](int x) const
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            int res = v ? -1 : 0;
            return res;
         }

         iv_base() = default;
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b) : v(false)
         {
            set(0, b.to_int64());
         }
      } __attribute__((aligned(8)));

      template <int N, bool C>
      class iv_base<N, C, 1, false>
      {
         bool v __INIT_VALUE;

       public:
         __FORCE_INLINE void assign_int64(Slong l)
         {
            set(0, static_cast<int>(l));
         }

         __FORCE_INLINE constexpr void assign_uint64(Ulong l)
         {
            set(0, static_cast<int>(l));
         }
         __FORCE_INLINE constexpr void set(int x, int value)
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            v = value & 1;
         }

         __FORCE_INLINE constexpr Slong to_int64() const
         {
            return v ? 1 : 0;
         }

         __FORCE_INLINE constexpr int operator[](int x) const
         {
            AC_ASSERT(x >= 0 && x < N, "unexpected condition");
            int res = v ? 1 : 0;
            return res;
         }

         iv_base() = default;
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b) : v(false)
         {
            set(0, b.to_int64());
         }
      } __attribute__((aligned(8)));

#else

   template <int N, bool C, int W0, bool S0>
   class iv_base
   {
      int v[(W0 + 31) / 32] __INIT_VALUE;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
         if(N > 1)
         {
            set(1, static_cast<int>(l >> 32));
            auto val = (operator[](1) < 0) ? ~0 : 0;
            LOOP(int, i, 2, exclude, N, { set(i, val); });
         }
      }

      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
         if(N > 1)
         {
            set(1, static_cast<int>(l >> 32));
            LOOP(int, i, 2, exclude, N, { set(i, 0); });
         }
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < N, "unexpected condition");
         bool updateRes = x * 32 < W0;
         if(!updateRes)
         {
            return;
         }
         if(x == N - 1)
         {
            constexpr const unsigned rem = (32 - W0) & 31;
            if(rem)
            {
               v[x] = S0 ? (((int)(((unsigned)value) << rem)) >> rem) : ((unsigned)value << rem) >> rem;
            }
            else
            {
               v[x] = value;
            }
         }
         else
         {
            v[x] = value;
         }
      }

      __FORCE_INLINE constexpr Slong to_int64() const
      {
         if(S0)
         {
            Slong conv = ((W0 + 31) / 32) == 1 ? v[0] : (((Ulong)v[1] << 32) | (Ulong)(unsigned)v[0]);
            return conv;
         }
         else
         {
            Ulong uv = ((W0 + 31) / 32) == 1 ? v[0] : ((Ulong)v[1] << 32) | (Ulong)(unsigned)v[0];
            Slong conv = uv;
            return conv;
         }
      }

      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < N, "unexpected condition");
         bool returnRes = x * 32 < W0;
         int res = 0;
         if(S0)
         {
            res = returnRes ? v[x] : v[x - 1] >> 31;
         }
         else
         {
            unsigned resu = returnRes ? v[x] : 0;
            res = resu;
         }
         return res;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         constexpr int M = AC_MIN(N, N2);
         LOOP(int, idx, 0, exclude, M, { set(idx, b[idx]); });
         auto last = v[M - 1] < 0 ? ~0 : 0;
         LOOP(int, idx, M, exclude, N, { set(idx, last); });
      }
   } __attribute__((aligned(8)));

   template <bool C, int W0, bool S0>
   class iv_base<1, C, W0, S0>
   {
      int v __INIT_VALUE;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
      }
      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < 1, "unexpected condition");
         constexpr const unsigned rem = (32 - W0) & 31;
         if(rem)
         {
            v = S0 ? (((int)(((unsigned)value) << rem)) >> rem) : (((unsigned)value << rem) >> rem);
         }
         else
         {
            v = value;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         if(S0)
         {
            Slong conv = (int)v;
            return conv;
         }
         else
         {
            Ulong uv = (unsigned)v;
            Slong conv = uv;
            return conv;
         }
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < 1, "unexpected condition");
         return v;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         set(0, b[0]);
      }
   } __attribute__((aligned(8)));

   template <bool C>
   struct select_type_iv_base2
   {
   };
   template <>
   struct select_type_iv_base2<true>
   {
      using type = int;
   };
   template <>
   struct select_type_iv_base2<false>
   {
      using type = long long;
   };

   template <bool C, int W0, bool S0>
   class iv_base<2, C, W0, S0>
   {
      typename select_type_iv_base2<C>::type v __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
      }
      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < 2, "unexpected condition");
         if(x == 1 && !C)
         {
            constexpr const unsigned rem = (32 - W0) & 31;
            if(rem)
            {
               v = ((unsigned)v) |
                   (((long long int)(S0 ? (((int)(((unsigned)value) << rem)) >> rem) : ((unsigned)value << rem) >> rem))
                    << 32);
            }
            else
            {
               v = ((unsigned)v) | (((long long int)value) << 32);
            }
         }
         else if(x == 1 && C)
         {
            return;
         }
         else if(C)
         {
            v = value;
         }
         else
         {
            v = ((v >> 32) << 32) | (unsigned)value;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         if(S0)
         {
            Slong conv = ((W0 + 31) / 32) == 1 ? operator[](0) : v;
            return conv;
         }
         else
         {
            Ulong uv = ((W0 + 31) / 32) == 1 ? (unsigned)operator[](0) : v;
            Slong conv = uv;
            return conv;
         }
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < 2, "unexpected condition");
         bool returnRes = x * 32 < W0;
         int res = 0;
         if(S0)
         {
            res = returnRes ? v >> (x * 32) : ((int)v) >> 31;
         }
         else
         {
            unsigned resu = returnRes ? ((unsigned long long)v) >> (x * 32) : 0;
            res = resu;
         }
         return res;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         constexpr int M = AC_MIN(2, N2);
         LOOP(int, idx, 0, exclude, M, { set(idx, b[idx]); });
         auto last = operator[](M - 1) < 0 ? ~0 : 0;
         LOOP(int, idx, M, exclude, 2, { set(idx, last); });
      }
   } __attribute__((aligned(8)));

   template <bool C, int W0, bool S0>
   class iv_base<3, C, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      int v2 __INIT_VALUE;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         auto val = (operator[](1) < 0) ? ~0 : 0;
         set(2, val);
      }
      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         set(2, 0);
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < 3, "unexpected condition");
         if(x == 0)
         {
            va = ((va >> 32) << 32) | (unsigned)value;
         }
         else if(x == 1)
         {
            va = ((unsigned)va) | ((long long int)value) << 32;
         }
         else if(x == 2 && !C)
         {
            constexpr const unsigned rem = (32 - W0) & 31;
            if(rem)
            {
               v2 = S0 ? (((int)(((unsigned)value) << rem)) >> rem) : ((unsigned)value << rem) >> rem;
            }
            else
            {
               v2 = value;
            }
         }
         else
         {
            return;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < 3, "unexpected condition");
         int res = 0;
         if(x == 0)
         {
            res = (int)va;
         }
         else if(x == 1)
         {
            res = va >> 32;
         }
         else if(x == 2 && C)
         {
            res = S0 ? va >> 63 : 0;
         }
         else
         {
            res = v2;
         }
         return res;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         constexpr int M = AC_MIN(3, N2);
         LOOP(int, idx, 0, exclude, M, { set(idx, b[idx]); });
         auto last = operator[](M - 1) < 0 ? ~0 : 0;
         LOOP(int, idx, M, exclude, 3, { set(idx, last); });
      }
   } __attribute__((aligned(8)));

   template <bool C, int W0, bool S0>
   class iv_base<4, C, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      typename select_type_iv_base2<C>::type v2 __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         auto val = (operator[](1) < 0) ? ~0 : 0;
         set(2, val);
         set(3, val);
      }
      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         set(2, 0);
         set(3, 0);
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < 4, "unexpected condition");
         if(x == 0)
         {
            va = ((va >> 32) << 32) | (unsigned)value;
         }
         else if(x == 1)
         {
            va = ((unsigned)va) | ((long long int)value) << 32;
         }
         else if(x == 2)
         {
            if(C)
            {
               v2 = value;
            }
            else
            {
               v2 = ((v2 >> 32) << 32) | (unsigned)value;
            }
         }
         else if(x == 3 && !C)
         {
            constexpr const unsigned rem = (32 - W0) & 31;
            if(rem)
            {
               v2 =
                   ((unsigned)v2) |
                   (((long long int)(S0 ? (((int)(((unsigned)value) << rem)) >> rem) : ((unsigned)value << rem) >> rem))
                    << 32);
            }
            else
            {
               v2 = ((unsigned)v2) | (((long long int)value) << 32);
            }
         }
         else
         {
            return;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < 4, "unexpected condition");
         int res = 0;
         if(x == 0)
         {
            res = (int)va;
         }
         else if(x == 1)
         {
            res = va >> 32;
         }
         else if(x == 2)
         {
            res = v2;
         }
         else if(x == 3 && C)
         {
            res = S0 ? (((int)v2) >> 31) : 0;
         }
         else
         {
            res = v2 >> 32;
         }
         return res;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         constexpr int M = AC_MIN(4, N2);
         LOOP(int, idx, 0, exclude, M, { set(idx, b[idx]); });
         auto last = operator[](M - 1) < 0 ? ~0 : 0;
         LOOP(int, idx, M, exclude, 4, { set(idx, last); });
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<5, true, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      long long int v2 __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         auto val = (operator[](1) < 0) ? ~0 : 0;
         set(2, val);
         set(3, val);
      }
      __FORCE_INLINE constexpr void assign_uint64(Ulong l)
      {
         set(0, static_cast<int>(l));
         set(1, static_cast<int>(l >> 32));
         set(2, 0);
         set(3, 0);
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         AC_ASSERT(x >= 0 && x < 5, "unexpected condition");
         if(x == 0)
         {
            va = ((va >> 32) << 32) | (unsigned)value;
         }
         else if(x == 1)
         {
            va = ((unsigned)va) | ((long long int)value) << 32;
         }
         else if(x == 2)
         {
            v2 = value;
         }
         else if(x == 3)
         {
            v2 = ((unsigned)v2) | (((long long int)value) << 32);
         }
         else
         {
            return;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         AC_ASSERT(x >= 0 && x < 5, "unexpected condition");
         int res = 0;
         if(x == 0)
         {
            res = (int)va;
         }
         else if(x == 1)
         {
            res = va >> 32;
         }
         else if(x == 2)
         {
            res = v2;
         }
         else if(x == 3)
         {
            res = v2 >> 32;
         }
         else
         {
            res = S0 ? (v2 >> 63) : 0;
         }
         return res;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         constexpr int M = AC_MIN(5, N2);
         LOOP(int, idx, 0, exclude, M, { set(idx, b[idx]); });
         auto last = operator[](M - 1) < 0 ? ~0 : 0;
         LOOP(int, idx, M, exclude, 5, { set(idx, last); });
      }
   } __attribute__((aligned(8)));

#if 0
   template <int W0, bool S0>
   class iv_base<1, false, W0, S0>
   {
      int v __INIT_VALUE;

      __FORCE_INLINE constexpr void bit_adjust()
      {
         constexpr const unsigned rem = (32 - W0) & 31;
         v = S0 ? (((int)(((unsigned)v) << rem)) >> rem) : (rem ? ((unsigned)v << rem) >> rem : 0);
      }

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         v = static_cast<int>(l);
         bit_adjust();
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         v = static_cast<int>(l);
         bit_adjust();
      }
      __FORCE_INLINE constexpr void set(int, int value)
      {
         v = value;
         bit_adjust();
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return v;
      }
      __FORCE_INLINE constexpr int operator[](int) const
      {
         return v;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b) : v(b[0])
      {
         bit_adjust();
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<1, true, W0, S0>
   {
      /// not possible to instantiate this class specialization
   };
#endif
#if 0

   template <int W0, bool S0>
   class iv_base<2, false, W0, S0>
   {
      long long int v __INIT_VALUE_LL;

      __FORCE_INLINE constexpr void bit_adjust()
      {
         constexpr const unsigned rem = (64 - W0) & 63;
         v = S0 ? (((long long int)(((unsigned long long)v) << rem)) >> rem) :
                  (rem ? ((unsigned long long)v << rem) >> rem : 0);
      }

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         v = l;
         bit_adjust();
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         v = static_cast<Slong>(l);
         bit_adjust();
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         if(x)
         {
            v = (all_ones & v) | (((Slong)value) << 32);
            bit_adjust();
         }
         else
         {
            v = ((((Ulong)all_ones) << 32) & v) | ((Ulong)((unsigned)value));
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return v;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         return x ? (int)(v >> 32) : (int)v;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 2)
         {
            v = b.to_int64();
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            v = b[0];
         }
         bit_adjust();
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<2, true, W0, S0>
   {
      int v __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         v = l;
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         v = static_cast<Slong>(l);
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         if(!x)
         {
            v = value;
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return v;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         return x ? 0 : v;
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 2)
         {
            v = b.to_int64();
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            v = b[0];
         }
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<3, false, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      long long int v2 __INIT_VALUE_LL;

      __FORCE_INLINE constexpr void bit_adjust()
      {
         constexpr const unsigned rem = (64 - W0) & 63;
         v2 = S0 ? (((long long int)(((unsigned long long)v2) << rem)) >> rem) :
                   (rem ? ((unsigned long long)v2 << rem) >> rem : 0);
      }

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         va = l;
         v2 = va < 0 ? ~0LL : 0;
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         va = static_cast<Slong>(l);
         v2 = 0;
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         x = x & 3;
         if(x == 0)
         {
            va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
         }
         else if(x == 1)
         {
            va = (all_ones & va) | (((Slong)value) << 32);
         }
         else
         {
            v2 = value;
            bit_adjust();
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         x = x & 3;
         return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : v2);
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 3)
         {
            va = b.to_int64();
            v2 = b[2];
            bit_adjust();
         }
         else if(M == 2)
         {
            va = b.to_int64();
            v2 = va < 0 ? ~0 : 0;
            bit_adjust();
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            va = b.to_int64();
            v2 = va < 0 ? ~0LL : 0;
            bit_adjust();
         }
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<3, true, W0, S0>
   {
      long long int va __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         va = l;
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         va = static_cast<Slong>(l);
      }
      __FORCE_INLINE constexpr void set(int x, int value)
      {
         x = x & 3;
         if(x == 0)
         {
            va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
         }
         else if(x == 1)
         {
            va = (all_ones & va) | (((Slong)value) << 32);
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         x = x & 3;
         return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : 0);
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 3)
         {
            va = b.to_int64();
         }
         else if(M == 2)
         {
            va = b.to_int64();
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            va = b.to_int64();
         }
      }
   } __attribute__((aligned(8)));

 template <int W0, bool S0>
   class iv_base<4, false, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      long long int vb __INIT_VALUE_LL;

      __FORCE_INLINE constexpr void bit_adjust()
      {
         constexpr const unsigned rem = (64 - W0) & 63;
         vb = S0 ? (((long long int)(((unsigned long long)vb) << rem)) >> rem) :
                   (rem ? ((unsigned long long)vb << rem) >> rem : 0);
      }

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         va = l;
         vb = va < 0 ? ~0LL : 0;
         bit_adjust();
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         va = static_cast<Slong>(l);
         vb = 0;
      }
      __FORCE_INLINE void set(int x, int value)
      {
         x = x & 3;
         if(x == 0)
         {
            va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
         }
         else if(x == 1)
         {
            va = (all_ones & va) | (((Slong)value) << 32);
         }
         else
         {
            if(x == 2)
            {
               vb = ((((Ulong)all_ones) << 32) & vb) | ((Ulong)((unsigned)value));
            }
            else
            {
               vb = (all_ones & vb) | (((Slong)value) << 32);
            }
            bit_adjust();
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         x = x & 3;
         return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : (x == 2 ? (int)vb : (int)(vb >> 32)));
      }

      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE constexpr iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 4)
         {
            va = b.to_int64();
            vb = (((Slong)b[3]) << 32) | ((Ulong)((unsigned)b[2]));
         }
         else if(M == 3)
         {
            va = b.to_int64();
            vb = b[2];
         }
         else if(M == 2)
         {
            va = b.to_int64();
            vb = va < 0 ? ~0 : 0;
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            va = b.to_int64();
            vb = va < 0 ? ~0LL : 0;
         }
         bit_adjust();
      }
   } __attribute__((aligned(8)));

   template <int W0, bool S0>
   class iv_base<4, true, W0, S0> : public iv_base<4, false, W0, S0>
   {
   };

   template <int W0, bool S0>
   class iv_base<5, true, W0, S0>
   {
      long long int va __INIT_VALUE_LL;
      long long int vb __INIT_VALUE_LL;

    public:
      __FORCE_INLINE void assign_int64(Slong l)
      {
         va = l;
         vb = va < 0 ? ~0LL : 0;
      }
      __FORCE_INLINE void constexpr assign_uint64(Ulong l)
      {
         va = static_cast<Slong>(l);
         vb = 0;
      }
      __FORCE_INLINE void set(int x, int value)
      {
         x = x & 7;
         if(x == 0)
         {
            va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
         }
         else if(x == 1)
         {
            va = (all_ones & va) | (((Slong)value) << 32);
         }
         else if(x == 2)
         {
            vb = ((((Ulong)all_ones) << 32) & vb) | ((Ulong)((unsigned)value));
         }
         else if(x == 3)
         {
            vb = (all_ones & vb) | (((Slong)value) << 32);
         }
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return va;
      }
      __FORCE_INLINE constexpr int operator[](int x) const
      {
         x = x & 3;
         return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : (x == 2 ? (int)vb : (x == 2 ? (int)(vb >> 32) : 0)));
      }
      iv_base() = default;
      template <int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE iv_base(const iv_base<N2, C2, W2, S2>& b)
      {
         const int M = AC_MIN(2, N2);
         if(M == 4 || M == 5)
         {
            va = b.to_int64();
            vb = (((Slong)b[3]) << 32) | ((Ulong)((unsigned)b[2]));
         }
         else if(M == 3)
         {
            va = b.to_int64();
            vb = b[2];
         }
         else if(M == 2)
         {
            va = b.to_int64();
            vb = va < 0 ? ~0 : 0;
         }
         else
         {
            AC_ASSERT(M == 1, "unexpected condition");
            va = b.to_int64();
            vb = va < 0 ? ~0LL : 0;
         }
      }
   } __attribute__((aligned(8)));
#endif
#endif

      template <int N, int START, int N1, bool C1, int W1, bool S1, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_copy(const iv_base<N1, C1, W1, S1>& op, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(START < N)
         {
            LOOP(int, i, START, exclude, N, { r.set(i, op[i]); });
         }
      }

      template <int START, int N, int N1, bool C1, int W1, bool S1>
      __FORCE_INLINE bool iv_equal_zero(const iv_base<N1, C1, W1, S1>& op)
      {
         bool retval = true;
         if(START < N)
         {
            LOOP(int, i, START, exclude, N, { retval &= !op[i]; });
         }
         return retval;
      }

      template <int START, int N, int N1, bool C1, int W1, bool S1>
      __FORCE_INLINE bool iv_equal_ones(const iv_base<N1, C1, W1, S1>& op)
      {
         bool retval = true;
         if(START < N)
         {
            LOOP(int, i, START, exclude, N, { retval &= !(~op[i]); });
         }
         return retval;
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE bool iv_equal(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2)
      {
         constexpr int M1 = AC_MAX(N1, N2);
         constexpr int M2 = AC_MIN(N1, N2);
         bool retval = true;
         LOOP(int, i, 0, exclude, M2, { retval &= op1[i] == op2[i]; });
         if(!retval)
         {
            return retval;
         }
         if(N1 >= N2)
         {
            int ext = op2[M2 - 1] < 0 ? ~0 : 0;
            retval = true;
            LOOP(int, i, M2, exclude, M1, { retval &= op1[i] == ext; });
         }
         else
         {
            int ext = op1[M2 - 1] < 0 ? ~0 : 0;
            retval = true;
            LOOP(int, i, M2, exclude, M1, { retval &= op2[i] == ext; });
         }
         return retval;
      }

      template <int B, int N, bool C, int W, bool S>
      __FORCE_INLINE bool iv_equal_ones_from(const iv_base<N, C, W, S>& op)
      {
         if((B >= 32 * N && op[N - 1] >= 0) || (B & 31 && ~(op[B / 32] >> (B & 31))))
         {
            return false;
         }
         return iv_equal_ones<(B + 31) / 32, N>(op);
      }

      template <int B, int N, bool C, int W, bool S>
      __FORCE_INLINE bool iv_equal_zeros_from(const iv_base<N, C, W, S>& op)
      {
         if((B >= 32 * N && op[N - 1] < 0) || (B & 31 && (op[B / 32] >> (B & 31))))
         {
            return false;
         }
         return iv_equal_zero<(B + 31) / 32, N>(op);
      }

      template <int B, int N, bool C, int W, bool S>
      __FORCE_INLINE bool iv_equal_ones_to(const iv_base<N, C, W, S>& op)
      {
         if((B >= 32 * N && op[N - 1] >= 0) || (B & 31 && ~(op[B / 32] | (all_ones << (B & 31)))))
         {
            return false;
         }
         return iv_equal_ones<0, B / 32>(op);
      }

      template <int B, int N, bool C, int W, bool S>
      __FORCE_INLINE bool iv_equal_zeros_to(const iv_base<N, C, W, S>& op)
      {
         if((B >= 32 * N && op[N - 1] < 0) || (B & 31 && (op[B / 32] & ~(all_ones << (B & 31)))))
         {
            return false;
         }
         if(B < 32)
         {
            return true;
         }
         return iv_equal_zero<0, (B / 32)>(op);
      }

      template <bool greater, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2>
      __FORCE_INLINE bool iv_compare(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2)
      {
         constexpr int M1 = AC_MAX(N1, N2);
         constexpr int M2 = AC_MIN(N1, N2);

         const bool b = (N1 >= N2) == greater;
         bool retval = false;
         bool invalidate = false;
         if(N1 >= N2)
         {
            int ext = op2[M2 - 1] < 0 ? ~0 : 0;
            int i2 = M1 > M2 ? ext : op2[M1 - 1];
            if(op1[M1 - 1] != i2)
            {
               return b ^ (op1[M1 - 1] < i2);
            }
            if((M1 - 2) >= M2)
            {
               LOOP(int, i, M1 - 2, include, M2, {
                  if(!invalidate)
                  {
                     if((unsigned)op1[i] != (unsigned)ext)
                     {
                        retval = b ^ ((unsigned)op1[i] < (unsigned)ext);
                        invalidate = true;
                     }
                  }
               });
            }
            if(invalidate)
            {
               return retval;
            }
            if((M2 - 1) >= 0)
            {
               LOOP(int, i, M2 - 1, include, 0, {
                  if(!invalidate)
                  {
                     if((unsigned)op1[i] != (unsigned)op2[i])
                     {
                        retval = b ^ ((unsigned)op1[i] < (unsigned)op2[i]);
                        invalidate = true;
                     }
                  }
               });
            }
         }
         else
         {
            int ext = op1[M2 - 1] < 0 ? ~0 : 0;
            int i2 = M1 > M2 ? ext : op1[M1 - 1];
            if(op2[M1 - 1] != i2)
            {
               return b ^ (op2[M1 - 1] < i2);
            }
            if((M1 - 2) >= M2)
            {
               LOOP(int, i, M1 - 2, include, M2, {
                  if(!invalidate)
                  {
                     if((unsigned)op2[i] != (unsigned)ext)
                     {
                        retval = b ^ ((unsigned)op2[i] < (unsigned)ext);
                        invalidate = true;
                     }
                  }
               });
            }
            if(invalidate)
            {
               return retval;
            }
            if((M2 - 1) >= 0)
            {
               LOOP(int, i, M2 - 1, include, 0, {
                  if(!invalidate)
                  {
                     if((unsigned)op2[i] != (unsigned)op1[i])
                     {
                        retval = b ^ ((unsigned)op2[i] < (unsigned)op1[i]);
                        invalidate = true;
                     }
                  }
               });
            }
         }
         if(invalidate)
         {
            return retval;
         }
         return false;
      }

      template <int START, int N, bool C, int W, bool S>
      __FORCE_INLINE constexpr void iv_extend(iv_base<N, C, W, S>& r, int ext)
      {
         if(START < N)
         {
            LOOP(int, i, START, exclude, N, { r.set(i, ext); });
         }
      }

      template <int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_assign_int64(iv_base<Nr, Cr, Wr, Sr>& r, Slong l)
      {
         r.assign_int64(l);
      }

      template <int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE constexpr void iv_assign_uint64(iv_base<Nr, Cr, Wr, Sr>& r, Ulong l)
      {
         r.assign_uint64(l);
      }

      __FORCE_INLINE Ulong mult_u_u(int a, int b)
      {
         return (Ulong)(unsigned)a * (Ulong)(unsigned)b;
      }
      __FORCE_INLINE Slong mult_u_s(int a, int b)
      {
         return (Ulong)(unsigned)a * (Slong)(signed)b;
      }
      __FORCE_INLINE Slong mult_s_u(int a, int b)
      {
         return (Slong)(signed)a * (Ulong)(unsigned)b;
      }
      __FORCE_INLINE Slong mult_s_s(int a, int b)
      {
         return (Slong)(signed)a * (Slong)(signed)b;
      }
      __FORCE_INLINE void accumulate(Ulong a, Ulong& l1, Slong& l2)
      {
         l1 += (Ulong)(unsigned)a;
         l2 += a >> 32;
      }
      __FORCE_INLINE void accumulate(Slong a, Ulong& l1, Slong& l2)
      {
         l1 += (Ulong)(unsigned)a;
         l2 += a >> 32;
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_mult(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                  iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(Nr == 1)
         {
            r.set(0, op1[0] * op2[0]);
         }
         else if(Nr == 2)
         {
            iv_assign_int64(r, (op1.to_int64() * op2.to_int64()));
         }
         else if(N1 == 1 && N2 == 1)
         {
            iv_assign_int64(r, (op1.to_int64() * op2.to_int64()));
         }
         else
         {
            constexpr int M1 = AC_MAX(N1, N2);
            constexpr int M2 = AC_MIN(N1, N2);
            constexpr int T1 = AC_MIN(M2 - 1, Nr);
            constexpr int T2 = AC_MIN(M1 - 1, Nr);
            constexpr int T3 = AC_MIN(M1 + M2 - 2, Nr);

            Ulong l1 = 0;
            Slong l2 = 0;
            if(N1 >= N2)
            {
               LOOP(int, k, 0, exclude, T1, {
                  LOOP(int, i, 0, include, T1, {
                     if(i < k + 1)
                     {
                        accumulate(mult_u_u(op1[k - i], op2[i]), l1, l2);
                     }
                  });
                  l2 += (Ulong)(unsigned)(l1 >> 32);
                  r.set(k, (int)l1);
                  l1 = (unsigned)l2;
                  l2 >>= 32;
               });
               if(T1 < T2)
               {
                  LOOP(int, k, T1, exclude, T2, {
                     accumulate(mult_u_s(op1[k - M2 + 1], op2[M2 - 1]), l1, l2);
                     LOOP(int, i, 0, exclude, M2 - 1, { accumulate(mult_u_u(op1[k - i], op2[i]), l1, l2); });
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(k, (int)l1);
                     l1 = (unsigned)l2;
                     l2 >>= 32;
                  });
               }
               if(T2 < T3)
               {
                  LOOP(int, k, T2, exclude, T3, {
                     accumulate(mult_u_s(op1[k - M2 + 1], op2[M2 - 1]), l1, l2);
                     LOOP(int, i, 0, exclude, M2 - 1, {
                        if(i >= (k - T2 + 1))
                        {
                           accumulate(mult_u_u(op1[k - i], op2[i]), l1, l2);
                        }
                     });
                     accumulate(mult_s_u(op1[M1 - 1], op2[k - M1 + 1]), l1, l2);
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(k, (int)l1);
                     l1 = (unsigned)l2;
                     l2 >>= 32;
                  });
               }
               if(Nr >= M1 + M2 - 1)
               {
                  accumulate(mult_s_s(op1[M1 - 1], op2[M2 - 1]), l1, l2);
                  r.set(M1 + M2 - 2, (int)l1);
                  if(Nr >= M1 + M2)
                  {
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(M1 + M2 - 1, (int)l2);
                     iv_extend<M1 + M2>(r, (r[M1 + M2 - 1] < 0) ? ~0 : 0);
                  }
               }
            }
            else
            {
               LOOP(int, k, 0, exclude, T1, {
                  LOOP(int, i, 0, include, T1, {
                     if(i < k + 1)
                     {
                        accumulate(mult_u_u(op2[k - i], op1[i]), l1, l2);
                     }
                  });
                  l2 += (Ulong)(unsigned)(l1 >> 32);
                  r.set(k, (int)l1);
                  l1 = (unsigned)l2;
                  l2 >>= 32;
               });
               if(T1 < T2)
               {
                  LOOP(int, k, T1, exclude, T2, {
                     accumulate(mult_u_s(op2[k - M2 + 1], op1[M2 - 1]), l1, l2);
                     LOOP(int, i, 0, exclude, M2 - 1, { accumulate(mult_u_u(op2[k - i], op1[i]), l1, l2); });
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(k, (int)l1);
                     l1 = (unsigned)l2;
                     l2 >>= 32;
                  });
               }
               if(T2 < T3)
               {
                  LOOP(int, k, T2, exclude, T3, {
                     accumulate(mult_u_s(op2[k - M2 + 1], op1[M2 - 1]), l1, l2);
                     LOOP(int, i, 0, exclude, M2 - 1, {
                        if(i >= (k - T2 + 1))
                        {
                           accumulate(mult_u_u(op2[k - i], op1[i]), l1, l2);
                        }
                     });
                     accumulate(mult_s_u(op2[M1 - 1], op1[k - M1 + 1]), l1, l2);
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(k, (int)l1);
                     l1 = (unsigned)l2;
                     l2 >>= 32;
                  });
               }
               if(Nr >= M1 + M2 - 1)
               {
                  accumulate(mult_s_s(op2[M1 - 1], op1[M2 - 1]), l1, l2);
                  r.set(M1 + M2 - 2, (int)l1);
                  if(Nr >= M1 + M2)
                  {
                     l2 += (Ulong)(unsigned)(l1 >> 32);
                     r.set(M1 + M2 - 1, (int)l2);
                     iv_extend<M1 + M2>(r, (r[M1 + M2 - 1] < 0) ? ~0 : 0);
                  }
               }
            }
         }
      }

      template <int N, bool C, int W, bool S>
      __FORCE_INLINE constexpr bool iv_uadd_carry(const iv_base<N, C, W, S>& op1, bool carry, iv_base<N, C, W, S>& r)
      {
         Slong l = carry;
         LOOP(int, i, 0, exclude, N, {
            l += (Ulong)(unsigned)op1[i];
            r.set(i, (int)l);
            l >>= 32;
         });
         return l != 0;
      }

      template <int START, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE bool iv_add_int_carry(const iv_base<N, C, W, S>& op1, int op2, bool carry,
                                           iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(N == START)
         {
            return carry;
         }
         if(N == START + 1)
         {
            Ulong l = carry + (Slong)op1[START] + (Slong)op2;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = carry + (Ulong)(unsigned)op1[START] + (Slong)op2;
         r.set(START, (int)l);
         l >>= 32;
         if((START + 1) < (N - 1))
         {
            LOOP(int, i, START + 1, exclude, N - 1, {
               l += (Ulong)(unsigned)op1[i];
               r.set(i, (int)l);
               l >>= 32;
            });
         }
         l += (Slong)op1[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr,
                bool Sr>
      __FORCE_INLINE bool iv_uadd_n(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                    iv_base<Nr, Cr, Wr, Sr>& r)
      {
         AC_ASSERT(AC_MIN(N, AC_MIN(N1, AC_MIN(N2, Nr))) == N, "unexpected condition");
         Ulong l = 0;
         LOOP(int, i, 0, exclude, N, {
            l += (Ulong)(unsigned)op1[i] + (Ulong)(unsigned)op2[i];
            r.set(i, (int)l);
            l >>= 32;
         });
         return l & 1;
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_add(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                 iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(Nr == 1)
         {
            r.set(0, op1[0] + op2[0]);
         }
         else
         {
            constexpr int M1 = AC_MAX(N1, N2);
            constexpr int M2 = AC_MIN(N1, N2);
            constexpr int T1 = AC_MIN(M2 - 1, Nr);
            constexpr int T2 = AC_MIN(M1, Nr);
            if(N1 >= N2)
            {
               bool carry = iv_uadd_n<T1>(op1, op2, r);
               carry = iv_add_int_carry<T1>(op1, op2[T1], carry, r);
               iv_extend<T2>(r, carry ? ~0 : 0);
            }
            else
            {
               bool carry = iv_uadd_n<T1>(op2, op1, r);
               carry = iv_add_int_carry<T1>(op2, op1[T1], carry, r);
               iv_extend<T2>(r, carry ? ~0 : 0);
            }
         }
      }

      template <int N, int START, int N1, bool C1, int W1, bool S1, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE bool iv_sub_int_borrow(const iv_base<N1, C1, W1, S1>& op1, int op2, bool borrow,
                                            iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(START == N)
         {
            return borrow;
         }
         if(N == (START + 1))
         {
            Ulong l = (Slong)op1[START] - (Slong)op2 - borrow;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = (Ulong)(unsigned)op1[START] - (Slong)op2 - borrow;
         r.set(START, (int)l);
         l >>= 32;
         if((START + 1) < (N - 1))
         {
            LOOP(int, i, START + 1, exclude, N - 1, {
               l += (Ulong)(unsigned)op1[i];
               r.set(i, (int)l);
               l >>= 32;
            });
         }
         l += (Slong)op1[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int START, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE bool iv_sub_int_borrow(int op1, const iv_base<N2, C2, W2, S2>& op2, bool borrow,
                                            iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(START == N)
         {
            return borrow;
         }
         if(N == START + 1)
         {
            Ulong l = (Slong)op1 - (Slong)op2[START] - borrow;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = (Slong)op1 - (Ulong)(unsigned)op2[START] - borrow;
         r.set(START, (int)l);
         l >>= 32;
         if((START + 1) < (N - 1))
         {
            LOOP(int, i, START + 1, exclude, N - 1, {
               l -= (Ulong)(unsigned)op2[i];
               r.set(i, (int)l);
               l >>= 32;
            });
         }
         l -= (Slong)op2[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr,
                bool Sr>
      __FORCE_INLINE bool iv_usub_n(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                    iv_base<Nr, Cr, Wr, Sr>& r)
      {
         AC_ASSERT(AC_MIN(N, AC_MIN(N1, AC_MIN(N2, Nr))) == N, "unexpected condition");
         Slong l = 0;
         LOOP(int, i, 0, exclude, N, {
            l += (Ulong)(unsigned)op1[i] - (Ulong)(unsigned)op2[i];
            r.set(i, (int)l);
            l >>= 32;
         });
         return l & 1;
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_sub(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                 iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(Nr == 1)
         {
            r.set(0, op1[0] - op2[0]);
         }
         else
         {
            constexpr int M1 = AC_MAX(N1, N2);
            constexpr int M2 = AC_MIN(N1, N2);
            constexpr int T1 = AC_MIN(M2 - 1, Nr);
            constexpr int T2 = AC_MIN(M1, Nr);

            bool borrow = iv_usub_n<T1>(op1, op2, r);
            if(N1 > N2)
            {
               borrow = iv_sub_int_borrow<T2, T1>(op1, op2[T1], borrow, r);
            }
            else
            {
               borrow = iv_sub_int_borrow<T2, T1>(op1[T1], op2, borrow, r);
            }
            iv_extend<T2>(r, borrow ? ~0 : 0);
         }
      }

      template <int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      void iv_neg(const iv_base<N, C, W, S>& op1, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         Slong l = 0;
         LOOP(int, k, 0, exclude, AC_MIN(N, Nr), {
            l -= (Ulong)(unsigned)op1[k];
            r.set(k, (unsigned)l);
            l >>= 32;
         });
         if(Nr > N)
         {
            r.set(N, (unsigned)(l - (op1[N - 1] < 0 ? ~0 : 0)));
            iv_extend<N + 1>(r, r[N] < 0 ? ~0 : 0);
         }
      }

      template <bool S0, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_abs(const iv_base<N, C, W, S>& op1, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(S0 && op1[N - 1] < 0)
         {
            iv_neg(op1, r);
         }
         else
         {
            iv_copy<AC_MIN(N, Nr), 0>(op1, r);
            iv_extend<N>(r, 0);
         }
      }

      template <int N, int D, int Q, int R, typename uw2, typename sw4, typename uw4, int w1_length, int Nn, bool Cn,
                int Wn, bool Sn, int Nd, bool Cd, int Wd, bool Sd, int Nq, bool Cq, int Wq, bool Sq, int Nr, bool Cr,
                int Wr, bool Sr>
      __FORCE_INLINE void iv_udiv(const iv_base<Nn, Cn, Wn, Sn>& n, const iv_base<Nd, Cd, Wd, Sd>& d,
                                  iv_base<Nq, Cq, Wq, Sq>& q, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         constexpr int w2_length = 2 * w1_length;
         int d_msi = D - 1; // most significant int for d
         bool loop_finished = false;
         LOOP(int, index, D - 1, exclude, 0, {
            if(!loop_finished && d_msi > 0 && !d[d_msi])
               d_msi--;
            else
               loop_finished = true;
         });
         uw4 d1 = 0;
         if(!d_msi && !d[0])
         {
            d1 = n[0] / d[0]; // d is zero => divide by zero
            return;
         }
         int n_msi = N - 1; // most significant int for n
         loop_finished = false;
         LOOP(int, index, N - 1, exclude, 0, {
            if(!loop_finished && n_msi > 0 && !n[n_msi])
               n_msi--;
            else
               loop_finished = true;
         });
         LOOP(int, i, 0, exclude, Q, { q.set(i, 0); });
         LOOP(int, i, 0, exclude, R, { r.set(i, n[i]); });
         // write most significant "words" into d1
         bool d_mss_odd = (bool)(d[d_msi] >> w1_length);
         int d_mss = 2 * d_msi + d_mss_odd; // index to most significant short (16-bit)
         d1 = (uw4)(uw2)d[d_msi] << (w1_length << (int)!d_mss_odd);
         if(d_msi)
         {
            d1 |= (uw2)d[d_msi - 1] >> (d_mss_odd ? w1_length : 0);
         }
         bool n_mss_odd = (bool)(n[n_msi] >> w1_length);
         int n_mss = 2 * n_msi + n_mss_odd;
         if(n_mss < d_mss)
         {
            // q already initialized to 0
            // r already initialized to n
         }
         else
         {
            uw2 r1[N + 1];
            r1[n_msi + 1] = 0;
            LOOP(int, index, 0, include, N, {
               if(index <= n_msi)
                  r1[index] = n[index];
            });
            LOOP(int, k, 2 * N - 1, include, 0, {
               if(k <= n_mss && k >= d_mss)
               {
                  int k_msi = k >> 1;
                  bool odd = k & 1;
                  uw2 r1m1 = k_msi > 0 ? r1[k_msi - 1] : (uw2)0;
                  uw4 n1 = odd ? (uw4)((r1[k_msi + 1] << w1_length) | (r1[k_msi] >> w1_length)) << w2_length |
                                     ((r1[k_msi] << w1_length) | (r1m1 >> w1_length)) :
                                 (uw4)r1[k_msi] << w2_length | r1m1;
                  uw2 q1 = n1 / d1;
                  if(q1 >> w1_length)
                     q1--;
                  AC_ASSERT(!(q1 >> w1_length), "Problem detected in long division algorithm, Please report");
                  unsigned k2 = k - d_mss;
                  unsigned k2_i = k2 >> 1;
                  bool odd_2 = k2 & 1;
                  uw2 q2 = q1 << (odd_2 ? w1_length : 0);
                  sw4 l = 0;
                  for(int j = 0; j <= d_msi; j++)
                  {
                     l += r1[k2_i + j];
                     bool l_sign = l < 0;
                     sw4 prod = (uw4)(uw2)d[j] * (uw4)q2;
                     l -= prod;
                     bool ov1 = (l >= 0) & ((prod < 0) | l_sign);
                     bool ov2 = (l < 0) & (prod < 0) & l_sign;
                     r1[k2_i + j] = (uw2)l;
                     l >>= w2_length;
                     if(ov1)
                        l |= ((uw4)-1 << w2_length);
                     if(ov2)
                        l ^= ((sw4)1 << w2_length);
                  }
                  if(odd_2 | d_mss_odd)
                  {
                     l += r1[k2_i + d_msi + 1];
                     r1[k2_i + d_msi + 1] = (uw2)l;
                  }
                  if(l < 0)
                  {
                     l = 0;
                     for(int j = 0; j <= d_msi; j++)
                     {
                        l += (sw4)(uw2)d[j] << (odd_2 ? w1_length : 0);
                        l += r1[k2_i + j];
                        r1[k2_i + j] = (uw2)l;
                        l >>= w2_length;
                     }
                     if(odd_2 | d_mss_odd)
                        r1[k2_i + d_msi + 1] += (uw2)l;
                     q1--;
                  }
                  if(Q && k2_i < Q)
                  {
                     if(odd_2)
                        q.set(k2_i, q1 << w1_length);
                     else
                        q.set(k2_i, q[k2_i] | q1);
                  }
               }
            });
            if(R)
            {
               int r_msi = AC_MIN(R - 1, n_msi);
               for(int j = 0; j <= r_msi; j++)
               {
                  r.set(j, r1[j]);
               }
               for(int j = r_msi + 1; j < R; j++)
               {
                  r.set(j, 0);
               }
            }
         }
      }

      template <int Num_s, int Den_s, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr,
                bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_div(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                 iv_base<Nr, Cr, Wr, Sr>& r)
      {
         enum
         {
            N1_over = N1 + (Den_s && (Num_s == 2))
         };
         if(N1_over == 1 && N2 == 1)
         {
            r.set(0, op1[0] / op2[0]);
            iv_extend<1>(r, ((Num_s || Den_s) && (r[0] < 0)) ? ~0 : 0);
         }
         else if(N1_over <= 2 && N2 <= 2)
         {
            iv_assign_int64(r, op1.to_int64() / op2.to_int64());
         }
         else if(!Num_s && !Den_s)
         {
            iv_base<1, false, 32, false> dummy{};
            iv_udiv<N1, N2, N1, 0, unsigned, Slong, Ulong, 16>(op1, op2, r, dummy);
            iv_extend<N1>(r, 0);
         }
         else
         {
            enum
            {
               N1_neg = N1 + (Num_s == 2),
               N2_neg = N2 + (Den_s == 2)
            };
            iv_base<N1_neg, false, 32 * N1_neg, false> numerator;
            iv_base<N2_neg, false, 32 * N2_neg, false> denominator;
            iv_base<N1_neg, false, 32 * N1_neg, false> quotient;
            iv_abs<(bool)Num_s>(op1, numerator);
            iv_abs<(bool)Den_s>(op2, denominator);
            iv_base<1, false, 32, false> dummy{};
            iv_udiv<N1_neg, N2_neg, N1_neg, 0, unsigned, Slong, Ulong, 16>(numerator, denominator, quotient, dummy);
            if((Num_s && op1[N1 - 1] < 0) ^ (Den_s && op2[N2 - 1] < 0))
            {
               iv_neg(quotient, r);
            }
            else
            {
               iv_copy<AC_MIN(N1_neg, Nr), 0>(quotient, r);
               iv_extend<N1_neg>(r, (Num_s || Den_s) && r[N1_neg - 1] < 0 ? ~0 : 0);
            }
         }
      }

      template <int Num_s, int Den_s, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr,
                bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_rem(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                 iv_base<Nr, Cr, Wr, Sr>& r)
      {
         enum
         {
            N1_over = N1 + (Den_s && (Num_s == 2))
         }; // N1_over corresponds to the division
         if(N1_over == 1 && N2 == 1)
         {
            r.set(0, op1[0] % op2[0]);
            iv_extend<1>(r, Num_s && r[0] < 0 ? ~0 : 0);
         }
         else if(N1_over <= 2 && N2 <= 2)
         {
            iv_assign_int64(r, op1.to_int64() % op2.to_int64());
         }
         else if(!Num_s && !Den_s)
         {
            iv_base<1, false, 32, false> dummy{};
            iv_udiv<N1, N2, 0, N2, unsigned, Slong, Ulong, 16>(op1, op2, dummy, r);
            iv_extend<N2>(r, 0);
         }
         else
         {
            enum
            {
               N1_neg = N1 + (Num_s == 2),
               N2_neg = N2 + (Den_s == 2)
            };
            iv_base<N1_neg, false, 32 * N1_neg, false> numerator;
            iv_base<N2_neg, false, 32 * N2_neg, false> denominator;
            iv_base<N2, false, 32 * N2, false> remainder;
            iv_abs<(bool)Num_s>(op1, numerator);
            iv_abs<(bool)Den_s>(op2, denominator);
            iv_base<1, false, 32, false> dummy{};
            iv_udiv<N1_neg, N2_neg, 0, N2, unsigned, Slong, Ulong, 16>(numerator, denominator, dummy, remainder);
            if((Num_s && op1[N1 - 1] < 0))
            {
               iv_neg(remainder, r);
            }
            else
            {
               iv_copy<AC_MIN(N2, Nr), 0>(remainder, r);
               iv_extend<N2>(r, Num_s && r[N2 - 1] < 0 ? ~0 : 0);
            }
         }
      }

      template <int N, int START, int N1, bool C1, int W1, bool S1, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_bitwise_complement_n(const iv_base<N1, C1, W1, S1>& op, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(START < N)
         {
            LOOP(int, i, START, exclude, N, { r.set(i, ~op[i]); });
         }
      }

      template <int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_bitwise_complement(const iv_base<N, C, W, S>& op, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         const int M = AC_MIN(N, Nr);
         iv_bitwise_complement_n<M, 0>(op, r);
         iv_extend<M>(r, (r[M - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr,
                bool Sr>
      __FORCE_INLINE void iv_bitwise_and_n(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                           iv_base<Nr, Cr, Wr, Sr>& r)
      {
         LOOP(int, i, 0, exclude, N, { r.set(i, op1[i] & op2[i]); });
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_bitwise_and(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                         iv_base<Nr, Cr, Wr, Sr>& r)
      {
         constexpr int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         constexpr int M2 = AC_MIN(AC_MIN(N1, N2), Nr);

         iv_bitwise_and_n<M2>(op1, op2, r);
         if(N1 >= N2)
         {
            if(op2[M2 - 1] < 0)
            {
               iv_copy<M1, M2>(op1, r);
            }
            else
            {
               iv_extend<M2>(r, 0);
            }
         }
         else
         {
            if(op1[M2 - 1] < 0)
            {
               iv_copy<M1, M2>(op2, r);
            }
            else
            {
               iv_extend<M2>(r, 0);
            }
         }
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr,
                bool Sr>
      __FORCE_INLINE void iv_bitwise_or_n(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                          iv_base<Nr, Cr, Wr, Sr>& r)
      {
         LOOP(int, i, 0, exclude, N, { r.set(i, op1[i] | op2[i]); });
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_bitwise_or(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                        iv_base<Nr, Cr, Wr, Sr>& r)
      {
         constexpr int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         constexpr int M2 = AC_MIN(AC_MIN(N1, N2), Nr);

         iv_bitwise_or_n<M2>(op1, op2, r);
         if(N1 >= N2)
         {
            if(op2[M2 - 1] < 0)
            {
               iv_extend<M2>(r, ~0);
            }
            else
            {
               iv_copy<M1, M2>(op1, r);
            }
         }
         else
         {
            if(op1[M2 - 1] < 0)
            {
               iv_extend<M2>(r, ~0);
            }
            else
            {
               iv_copy<M1, M2>(op2, r);
            }
         }
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr,
                bool Sr>
      __FORCE_INLINE void iv_bitwise_xor_n(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                           iv_base<Nr, Cr, Wr, Sr>& r)
      {
         LOOP(int, i, 0, exclude, N, { r.set(i, op1[i] ^ op2[i]); });
      }

      template <int N1, bool C1, int W1, bool S1, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_bitwise_xor(const iv_base<N1, C1, W1, S1>& op1, const iv_base<N2, C2, W2, S2>& op2,
                                         iv_base<Nr, Cr, Wr, Sr>& r)
      {
         constexpr int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         constexpr int M2 = AC_MIN(AC_MIN(N1, N2), Nr);

         iv_bitwise_xor_n<M2>(op1, op2, r);
         if(N1 >= N2)
         {
            if(op2[M2 - 1] < 0)
            {
               iv_bitwise_complement_n<M1, M2>(op1, r);
            }
            else
            {
               iv_copy<M1, M2>(op1, r);
            }
         }
         else
         {
            if(op1[M2 - 1] < 0)
            {
               iv_bitwise_complement_n<M1, M2>(op2, r);
            }
            else
            {
               iv_copy<M1, M2>(op2, r);
            }
         }
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_shift_l(const iv_base<N, C, W, S>& op1, unsigned op2, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         AC_ASSERT(Nr <= N, "iv_shift_l, incorrect usage Nr > N");
         unsigned s31 = op2 & 31;
         unsigned ishift = (op2 >> 5) > Nr ? Nr : (op2 >> 5);
         if(s31 && ishift != Nr)
         {
            unsigned lw = 0;
            LOOP(unsigned, i, 0, exclude, Nr, {
               unsigned hw = (i >= ishift) ? op1[i - ishift] : 0;
               r.set(i, (hw << s31) | (lw >> (32 - s31)));
               lw = hw;
            });
         }
         else
         {
            LOOP(unsigned, i, 0, exclude, Nr, { r.set(i, (i >= ishift) ? op1[i - ishift] : 0); });
         }
      }

      template <int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_shift_r(const iv_base<N, C, W, S>& op1, unsigned op2, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         unsigned s31 = op2 & 31;
         unsigned ishift = (op2 >> 5) > N ? N : (op2 >> 5);
         int ext = op1[N - 1] < 0 ? ~0 : 0;
         if(s31 && ishift != N)
         {
            unsigned lw = (ishift < N) ? op1[ishift] : ext;
            LOOP(unsigned, i, 0, exclude, Nr, {
               unsigned hw = (i + ishift + 1 < N) ? op1[i + ishift + 1] : ext;
               r.set(i, (lw >> s31) | (hw << (32 - s31)));
               lw = hw;
            });
         }
         else
         {
            LOOP(unsigned, i, 0, exclude, Nr, { r.set(i, (i + ishift < N) ? op1[i + ishift] : ext); });
         }
      }

      template <bool S0, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_shift_l2(const iv_base<N, C, W, S>& op1, signed op2, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(S0 && op2 < 0)
         {
            iv_shift_r(op1, -op2, r);
         }
         else
         {
            iv_shift_l(op1, op2, r);
         }
      }

      template <bool S0, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_shift_r2(const iv_base<N, C, W, S>& op1, signed op2, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(S0 && op2 < 0)
         {
            iv_shift_l(op1, -op2, r);
         }
         else
         {
            iv_shift_r(op1, op2, r);
         }
      }

      template <int B, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE constexpr void iv_const_shift_l(const iv_base<N, C, W, S>& op1, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         // B >= 0
         if(!B)
         {
            constexpr int M1 = AC_MIN(N, Nr);
            iv_copy<M1, 0>(op1, r);
            iv_extend<M1>(r, r[M1 - 1] < 0 ? -1 : 0);
         }
         else
         {
            constexpr unsigned s31 = B & 31;
            constexpr int ishift = (((B >> 5) > Nr) ? Nr : (B >> 5));
            constexpr int M1 = AC_MIN(N + ishift, Nr);
            LOOP(int, idx, 0, exclude, ishift, { r.set(idx, 0); });
            if(s31)
            {
               unsigned lw = 0;
               if(ishift < M1)
               {
                  LOOP(int, i, ishift, exclude, M1, {
                     unsigned hw = op1[i - ishift];
                     r.set(i, (hw << s31) | (lw >> ((32 - s31) & 31))); // &31 is to quiet compilers
                     lw = hw;
                  });
               }
               if(Nr > M1)
               {
                  r.set(M1, (signed)lw >> ((32 - s31) & 31)); // &31 is to quiet compilers
                  iv_extend<M1 + 1>(r, r[M1] < 0 ? ~0 : 0);
               }
            }
            else
            {
               if(ishift < M1)
               {
                  LOOP(int, i, ishift, exclude, M1, { r.set(i, op1[i - ishift]); });
               }
               iv_extend<M1>(r, r[M1 - 1] < 0 ? -1 : 0);
            }
         }
      }

      template <int B, int N, bool C, int W, bool S, int Nr, bool Cr, int Wr, bool Sr>
      __FORCE_INLINE void iv_const_shift_r(const iv_base<N, C, W, S>& op1, iv_base<Nr, Cr, Wr, Sr>& r)
      {
         if(!B)
         {
            constexpr int M1 = AC_MIN(N, Nr);
            iv_copy<M1, 0>(op1, r);
            iv_extend<M1>(r, r[M1 - 1] < 0 ? ~0 : 0);
         }
         else
         {
            constexpr unsigned s31 = B & 31;
            constexpr int ishift = (((B >> 5) > N) ? N : (B >> 5));
            int ext = op1[N - 1] < 0 ? ~0 : 0;
            if(s31 && ishift != N)
            {
               unsigned lw = (ishift < N) ? op1[ishift] : ext;
               LOOP(int, i, 0, exclude, Nr, {
                  unsigned hw = (i + ishift + 1 < N) ? op1[i + ishift + 1] : ext;
                  r.set(i, (lw >> s31) | (hw << ((32 - s31) & 31))); // &31 is to quiet compilers
                  lw = hw;
               });
            }
            else
            {
               LOOP(int, i, 0, exclude, Nr, { r.set(i, (i + ishift < N) ? op1[i + ishift] : ext); });
            }
         }
      }

      template <int N, bool C, int W, bool S>
      __FORCE_INLINE constexpr void iv_conv_from_fraction(const double d, iv_base<N, C, W, S>& r, bool* qb, bool* rbits,
                                                          bool* o)
      {
         bool b = d < 0;
         double d2 = b ? -d : d;
         double dfloor = mgc_floor(d2);
         *o = dfloor != 0.0;
         d2 = d2 - dfloor;
         LOOP(int, i, N - 1, include, 0, {
            d2 *= 1ULL << 32;
            auto k = (unsigned int)d2;
            r.set(i, b ? ~k : k);
            constexpr const unsigned rem = W & 31;
            auto kr = rem ? k >> rem : k;
            *o |= (0 != kr) && rem != 0 && (i == (N - 1));
            d2 -= k;
         });
         d2 *= 2;
         bool k = ((int)d2) != 0; // is 0 or 1
         d2 -= k;
         *rbits = d2 != 0.0;
         *qb = (b && *rbits) ^ k;
         if(b && !*rbits && !*qb)
         {
            iv_uadd_carry(r, true, r);
         }
         *o |= b ^ (r[N - 1] < 0);
      }

      template <int N, bool C, int W, bool S>
      __FORCE_INLINE constexpr void iv_conv_from_fraction(const float d, iv_base<N, C, W, S>& r, bool* qb, bool* rbits,
                                                          bool* o)
      {
         bool b = d < 0;
         float d2 = b ? -d : d;
         float dfloor = mgc_floor(d2);
         *o = dfloor != 0.0;
         d2 = d2 - dfloor;
         LOOP(int, i, N - 1, include, 0, {
            d2 *= 1ULL << 32;
            auto k = static_cast<unsigned int>(d2);
            r.set(i, b ? ~k : k);
            constexpr const unsigned rem = W & 31;
            auto kr = rem ? k >> rem : k;
            *o |= (0 != kr) && rem != 0 && (i == (N - 1));
            d2 -= k;
         });
         d2 *= 2;
         bool k = ((int)d2) != 0; // is 0 or 1
         d2 -= k;
         *rbits = d2 != 0.0;
         *qb = (b && *rbits) ^ k;
         if(b && !*rbits && !*qb)
         {
            iv_uadd_carry(r, true, r);
         }
         *o |= b ^ (r[N - 1] < 0);
      }

      template <ac_base_mode b, int N, bool C, int W, bool S>
      struct to_strImpl
      {
         static __FORCE_INLINE int to_str(iv_base<N, C, W, S>& v, int w, bool left_just, char* r)
         {
            const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            const unsigned char B = b == AC_BIN ? 1 : (b == AC_OCT ? 3 : (b == AC_HEX ? 4 : 0));
            int k = (w + B - 1) / B;
            int n = (w + 31) >> 5;
            int bits = 0;
            if(b != AC_BIN && left_just)
            {
               if((bits = -(w % B)))
               {
                  r[--k] = 0;
               }
            }
            for(int i = 0; i < n; i++)
            {
               if(b != AC_BIN && bits < 0)
               {
                  r[k] += (unsigned char)((v[i] << (B + bits)) & (b - 1));
               }
               unsigned int m = (unsigned)v[i] >> -bits;
               for(bits += 32; bits > 0 && k; bits -= B)
               {
                  r[--k] = (char)(m & (b - 1));
                  m >>= B;
               }
            }
            for(int i = 0; i < (w + B - 1) / B; i++)
            {
               r[i] = digits[(int)r[i]];
            }
            return (w + B - 1) / B;
         }
      };
      template <int N, bool C, int W, bool S>
      struct to_strImpl<AC_DEC, N, C, W, S>
      {
         static __FORCE_INLINE int to_str(iv_base<N, C, W, S>& v, int w, bool left_just, char* r)
         {
            int k = 0;
            int msw = (w - 1) >> 5;
            if(left_just)
            {
               unsigned bits_msw = w & 31;
               if(bits_msw)
               {
                  unsigned left_shift = 32 - bits_msw;
                  for(int i = msw; i > 0; i--)
                  {
                     v.set(i, v[i] << left_shift | (unsigned)v[i - 1] >> bits_msw);
                  }
                  v.set(0, v[0] << left_shift);
               }
               int lsw = 0;
               while(lsw < msw || v[msw])
               {
                  Ulong l = 0;
                  for(int i = lsw; i <= msw; i++)
                  {
                     l += (Ulong)(unsigned)v[i] * 10;
                     v.set(i, l);
                     l >>= 32;
                     if(i == lsw && !v[i])
                     {
                        lsw++;
                     }
                  }
                  r[k++] = (char)('0' + (int)l);
               }
            }
            else
            {
               const unsigned d = 1000000000; // 10E9
               for(; msw > 0 && !v[msw]; msw--)
               {
               }
               while(msw >= 0)
               {
                  Ulong nl = 0;
                  for(int i = msw; i >= 0; i--)
                  {
                     nl <<= 32;
                     nl |= (unsigned)v[i];
                     unsigned q = nl / d;
                     nl -= (Ulong)q * d;
                     v.set(i, q);
                  }
                  if(!v[msw])
                  {
                     msw--;
                  }
                  bool last = msw == -1;
                  auto rem = (unsigned)nl;
                  for(int i = 0; (i < 9 && !last) || rem; i++)
                  {
                     r[k++] = (char)('0' + (int)(rem % 10));
                     rem /= 10;
                  }
               }
               for(int i = 0; i < k / 2; i++)
               {
                  char c = r[i];
                  r[i] = r[k - 1 - i];
                  r[k - 1 - i] = c;
               }
            }
            r[k] = 0;
            return k;
         }
      };
      template <int N, bool C, int W, bool S>
      __FORCE_INLINE int to_string(iv_base<N, C, W, S>& v, int w, bool sign_mag, ac_base_mode base, bool left_just,
                                   char* r)
      {
         int n = (w + 31) >> 5;
         bool neg = !sign_mag && v[n - 1] < 0;
         if(!left_just)
         {
            while(n-- && v[n] == (neg ? ~0 : 0))
            {
            }
            int w2 = 32 * (n + 1);
            if(w2)
            {
               int m = v[n];
               for(int i = 16; i > 0; i >>= 1)
               {
                  if((m >> i) == (neg ? ~0 : 0))
                  {
                     w2 -= i;
                  }
                  else
                  {
                     m >>= i;
                  }
               }
            }
            if(w2 < w)
            {
               w = w2;
            }
            w += !sign_mag;
         }
         if(base == AC_DEC)
         {
            return to_strImpl<AC_DEC, N, C, W, S>::to_str(v, w, left_just, r);
         }
         else if(base == AC_HEX)
         {
            return to_strImpl<AC_HEX, N, C, W, S>::to_str(v, w, left_just, r);
         }
         else if(base == AC_OCT)
         {
            return to_strImpl<AC_OCT, N, C, W, S>::to_str(v, w, left_just, r);
         }
         else if(base == AC_BIN)
         {
            return to_strImpl<AC_BIN, N, C, W, S>::to_str(v, w, left_just, r);
         }
         return 0;
      }

      template <int N, bool C, int W, bool S>
      __FORCE_INLINE unsigned iv_leading_bits_base(const iv_base<N, C, W, S>& op, bool bit, int POS)
      {
         const unsigned char tab[] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
         unsigned t = bit ? ~op[POS] : op[POS];
         unsigned cnt = 0;
         if(t >> 16)
         {
            t >>= 16;
         }
         else
         {
            cnt += 16;
         }
         if(t >> 8)
         {
            t >>= 8;
         }
         else
         {
            cnt += 8;
         }
         if(t >> 4)
         {
            t >>= 4;
         }
         else
         {
            cnt += 4;
         }
         cnt += tab[t];
         return cnt;
      }

      template <int N, bool C, int W, bool S>
      __FORCE_INLINE unsigned iv_leading_bits(const iv_base<N, C, W, S>& op, bool bit)
      {
         int ext_sign = bit ? -1 : 0;
         int k;
         for(k = N - 1; k >= 0 && op[k] == ext_sign; k--)
         {
         }
         return 32 * (N - 1 - k) + (k < 0 ? 0 : iv_leading_bits_base(op, bit, k));
      }

      //////////////////////////////////////////////////////////////////////////////
      //  Integer Vector class: iv
      //////////////////////////////////////////////////////////////////////////////
      template <int N, bool C, int W, bool S>
      class iv
      {
       protected:
         class hex2doubleConverter
         {
            using size_t = decltype(sizeof(0));                     // avoid including extra headers
            static constexpr const long double _0x1p256 = 0x1p256L; // 2^256
            using ieee_double_shape_type = union
            {
               double dvalue;
               unsigned long long int ull_value;
            };

            template <size_t begin, char charToFind, size_t NN>
            struct HfindCharImpl
            {
               static constexpr size_t HfindChar(const char (&str)[NN])
               {
                  return (str[begin] == charToFind) * (begin + 1) +
                         HfindCharImpl<begin - 1, charToFind, NN>::HfindChar(str);
               }
            };
            template <char charToFind, size_t NN>
            struct HfindCharImpl<0, charToFind, NN>
            {
               static constexpr size_t HfindChar(const char (&str)[NN])
               {
                  return (str[0] == charToFind);
               }
            };
            template <size_t Index, int base, typename Int, size_t NN>
            struct getNumberImpl
            {
               __FORCE_INLINE static constexpr Int getNumber(const char (&str)[NN], size_t begin, size_t end)
               {
                  Int increment = (Index < begin || Index >= end) ? 0 : HhexDigit(str[Index]);
                  Int multiplier = (Index < begin || Index >= end) ? 1 : base;
                  return increment + multiplier * getNumberImpl<Index - 1, base, Int, NN>::getNumber(str, begin, end);
               }
            };
            template <int base, typename Int, size_t NN>
            struct getNumberImpl<0, base, Int, NN>
            {
               __FORCE_INLINE static constexpr Int getNumber(const char (&str)[NN], size_t begin, size_t end)
               {
                  Int increment = (0 < begin || 0 >= end) ? 0 : HhexDigit(str[0]);
                  return increment;
               }
            };

            // Unportable, but will work for ANSI charset
            static constexpr int HhexDigit(char c)
            {
               return '0' <= c && c <= '9' ? c - '0' :
                      'a' <= c && c <= 'f' ? c - 'a' + 0xa :
                      'A' <= c && c <= 'F' ? c - 'A' + 0xA :
                                             0;
            }

            static constexpr double Hscalbn(const double value, const int exponent)
            {
               ieee_double_shape_type in;
               in.dvalue = value;
               if(in.ull_value << 1 == 0)
               {
                  return value;
               }
               unsigned long long cur_exp = (((int)((in.ull_value >> 52) & 0x7ff)) + exponent);
               in.ull_value = (in.ull_value & (~(0x7ffULL << 52))) | (cur_exp << 52);
               return in.dvalue;
            }

            template <size_t NN>
            static constexpr size_t HmantissaEnd0(const char (&str)[NN])
            {
               return HfindCharImpl<NN - 1, 'p', NN>::HfindChar(str);
            }
            template <size_t NN>
            static constexpr size_t HmantissaEnd1(const size_t mantissa_end)
            {
               return mantissa_end == 0 ? NN : mantissa_end - 1;
            }

            template <size_t NN>
            static constexpr size_t HmantissaEnd(const char (&str)[NN])
            {
               return HmantissaEnd1<NN>(HmantissaEnd0(str));
            }

            template <size_t NN>
            static constexpr size_t HpointPos0(const char (&str)[NN])
            {
               return HfindCharImpl<NN - 1, '.', NN>::HfindChar(str);
            }
            template <size_t NN>
            static constexpr size_t HpointPos1(const size_t mantissa_end, const size_t pp0)
            {
               return pp0 == 0 ? mantissa_end : pp0 - 1;
            }
            template <size_t NN>
            static constexpr size_t HpointPos(const char (&str)[NN], const size_t mantissa_end)
            {
               return HpointPos1<NN>(mantissa_end, HpointPos0(str));
            }

            template <size_t NN>
            __FORCE_INLINE static constexpr int Hexponent(const char (&str)[NN], const size_t mantissa_end)
            {
               return mantissa_end == NN ? 0 :
                                           (str[mantissa_end + 1] == '-' ? -1 : 1) *
                                               getNumberImpl<NN - 1, 10, int, NN>::getNumber(
                                                   str, mantissa_end + 1 + HisSign(mantissa_end + 1), NN - 1);
            }

            static constexpr bool HisSign(char ch)
            {
               return ch == '+' || ch == '-';
            }
            template <size_t NN>
            static constexpr size_t HmantissaBegin(const char (&str)[NN])
            {
               return HisSign(str[0]) + 2 * (str[HisSign(str[0])] == '0' && str[HisSign(str[0]) + 1] == 'x');
            }
            template <size_t NN>
            __FORCE_INLINE static constexpr unsigned long long HbeforePoint(const char (&str)[NN],
                                                                            const size_t point_pos)
            {
               return getNumberImpl<NN - 1, 16, unsigned long long, NN>::getNumber(str, HmantissaBegin(str), point_pos);
            }

            template <size_t NN>
            __FORCE_INLINE static constexpr unsigned long long Hfraction(const char (&str)[NN], const size_t point_pos,
                                                                         const size_t mantissa_end)
            {
               return getNumberImpl<NN - 1, 16, unsigned long long, NN>::getNumber(str, point_pos + 1, mantissa_end);
            }

            template <size_t NN>
            __FORCE_INLINE static constexpr double get0(const char (&str)[NN], const size_t mantissa_end,
                                                        const size_t point_pos, const size_t exp)
            {
               //            printf("%d\n", mantissa_end);
               //            printf("%d\n", point_pos);
               //            printf("%d\n", exp);
               return (str[0] == '-' ? -1 : 1) *
                      (Hscalbn(HbeforePoint(str, point_pos), exp) +
                       Hscalbn(Hfraction(str, point_pos, mantissa_end), exp - 4 * (mantissa_end - point_pos - 1)));
            }
            template <size_t NN>
            __FORCE_INLINE static constexpr double get1(const char (&str)[NN], const size_t mantissa_end)
            {
               return get0(str, mantissa_end, HpointPos(str, mantissa_end), Hexponent(str, mantissa_end));
            }

          public:
            template <size_t NN>
            __FORCE_INLINE static constexpr double get(const char (&str)[NN])
            {
               return get1(str, HmantissaEnd(str));
            }
         };
         iv_base<N, C, W, S> v;

       public:
         template <int N2, bool C2, int W2, bool S2>
         friend class iv;
         __FORCE_INLINE
         constexpr iv() = default;

         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE constexpr iv(const iv<N2, C2, W2, S2>& b)
         {
            const int M = AC_MIN(N, N2);
            iv_copy<M, 0>(b.v, v);
            iv_extend<M>(v, (v[M - 1] < 0) ? ~0 : 0);
         }

         __FORCE_INLINE constexpr iv(Slong t)
         {
            iv_assign_int64(v, t);
         }

         __FORCE_INLINE constexpr iv(Ulong t)
         {
            iv_assign_uint64(v, t);
         }

         __FORCE_INLINE constexpr iv(int t)
         {
            v.set(0, t);
            iv_extend<1>(v, (t < 0) ? ~0 : 0);
         }

         __FORCE_INLINE constexpr iv(unsigned int t)
         {
            v.set(0, t);
            iv_extend<1>(v, 0);
         }

         __FORCE_INLINE constexpr iv(long t)
         {
            if(long_w == 32)
            {
               v.set(0, t);
               iv_extend<1>(v, (t < 0) ? ~0 : 0);
            }
            else
            {
               iv_assign_int64(v, t);
            }
         }

         __FORCE_INLINE constexpr iv(unsigned long t)
         {
            if(long_w == 32)
            {
               v.set(0, t);
               iv_extend<1>(v, 0);
            }
            else
            {
               iv_assign_uint64(v, t);
            }
         }

         __FORCE_INLINE constexpr iv(double d)
         {
            double d2 = ldexpr32<-N>(d);
            bool qb = false, rbits = false, o = false;
            iv_conv_from_fraction(d2, v, &qb, &rbits, &o);
         }

         __FORCE_INLINE constexpr iv(float d)
         {
            float d2 = ldexpr32<-N>(d);
            bool qb = false, rbits = false, o = false;
            iv_conv_from_fraction(d2, v, &qb, &rbits, &o);
         }

         template <size_t NN>
         __FORCE_INLINE constexpr iv(const char (&str)[NN])
         {
            *this = iv(hex2doubleConverter::get(str));
         }

         // Explicit conversion functions to C built-in types -------------
         __FORCE_INLINE constexpr Slong to_int64() const
         {
            return v.to_int64();
         }
         __FORCE_INLINE constexpr Ulong to_uint64() const
         {
            return (Ulong)v.to_int64();
         }
         __FORCE_INLINE double to_double() const
         {
            double a = v[N - 1];
            if((N - 2) >= 0)
            {
               LOOP(int, i, N - 2, include, 0, {
                  a *= (Ulong)1 << 32;
                  a += (unsigned)v[i];
               });
            }
            return a;
         }
         __FORCE_INLINE constexpr float to_float() const
         {
            float a = v[N - 1];
            if((N - 2) >= 0)
            {
               LOOP(int, i, N - 2, include, 0, {
                  a *= (Ulong)1 << 32;
                  a += (unsigned)v[i];
               });
            }
            return a;
         }
         __FORCE_INLINE constexpr void conv_from_fraction(double d, bool* qb, bool* rbits, bool* o)
         {
            iv_conv_from_fraction(d, v, qb, rbits, o);
         }
         __FORCE_INLINE constexpr void conv_from_fraction(float d, bool* qb, bool* rbits, bool* o)
         {
            iv_conv_from_fraction(d, v, qb, rbits, o);
         }

         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void mult(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_mult(v, op2.v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void add(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_add(v, op2.v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void sub(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_sub(v, op2.v, r.v);
         }
         template <int Num_s, int Den_s, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void div(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_div<Num_s, Den_s>(v, op2.v, r.v);
         }
         template <int Num_s, int Den_s, int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void rem(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_rem<Num_s, Den_s>(v, op2.v, r.v);
         }
         __FORCE_INLINE void increment()
         {
            iv_uadd_carry(v, true, v);
         }
         __FORCE_INLINE void decrement()
         {
            iv_sub_int_borrow<N>(v, 0, true, v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         void neg(iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_neg(v, r.v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void shift_l(unsigned op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_shift_l(v, op2, r.v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void shift_l2(signed op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_shift_l2<true>(v, op2, r.v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void shift_r(unsigned op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_shift_r(v, op2, r.v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void shift_r2(signed op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_shift_r2<true>(v, op2, r.v);
         }
         template <int B, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE constexpr void const_shift_l(iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_const_shift_l<B>(v, r.v);
         }
         template <int B, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void const_shift_r(iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_const_shift_r<B>(v, r.v);
         }
         template <int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void bitwise_complement(iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_bitwise_complement(v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void bitwise_and(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_bitwise_and(v, op2.v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void bitwise_or(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_bitwise_or(v, op2.v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2, int Nr, bool Cr, int Wr, bool Sr>
         __FORCE_INLINE void bitwise_xor(const iv<N2, C2, W2, S2>& op2, iv<Nr, Cr, Wr, Sr>& r) const
         {
            iv_bitwise_xor(v, op2.v, r.v);
         }
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE bool equal(const iv<N2, C2, W2, S2>& op2) const
         {
            return iv_equal(v, op2.v);
         }
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE bool greater_than(const iv<N2, C2, W2, S2>& op2) const
         {
            return iv_compare<true>(v, op2.v);
         }
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE bool less_than(const iv<N2, C2, W2, S2>& op2) const
         {
            auto res = iv_compare<false>(v, op2.v);
            return res;
         }
         __FORCE_INLINE bool equal_zero() const
         {
            return iv_equal_zero<0, N>(v);
         }
         template <int N2, bool C2, int W2, bool S2>
         __FORCE_INLINE constexpr void set_slc(unsigned lsb, int WS, const iv<N2, C2, W2, S2>& op2)
         {
            AC_ASSERT((31 + WS) / 32 == N2, "Bad usage: WS greater than length of slice");
            unsigned msb = lsb + WS - 1;
            unsigned lsb_v = lsb >> 5;
            unsigned lsb_b = lsb & 31;
            unsigned msb_v = msb >> 5;
            unsigned msb_b = msb & 31;
            if(N2 == 1)
            {
               if(msb_v == lsb_v)
               {
                  v.set(lsb_v,
                        v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & ((WS == 32 ? ~0 : ((1u << WS) - 1)) << lsb_b)));
               }
               else
               {
                  v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & (all_ones << lsb_b)));
                  unsigned m = (((unsigned)op2.v[0] >> 1) >> (31 - lsb_b));
                  v.set(msb_v, v[msb_v] ^ ((v[msb_v] ^ m) & ~((all_ones << 1) << msb_b)));
               }
            }
            else
            {
               v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & (all_ones << lsb_b)));
               LOOP(int, i, 1, exclude, N2 - 1,
                    { v.set(lsb_v + i, (op2.v[i] << lsb_b) | (((unsigned)op2.v[i - 1] >> 1) >> (31 - lsb_b))); });
               unsigned t = (op2.v[N2 - 1] << lsb_b) | (((unsigned)op2.v[N2 - 2] >> 1) >> (31 - lsb_b));
               unsigned m = t;
               if(msb_v - lsb_v == N2)
               {
                  v.set(msb_v - 1, t);
                  m = (((unsigned)op2.v[N2 - 1] >> 1) >> (31 - lsb_b));
               }
               else
               {
                  m = t;
               }
               v.set(msb_v, v[msb_v] ^ ((v[msb_v] ^ m) & ~((all_ones << 1) << msb_b)));
            }
         }

         template <int N_2, bool C_2, int W2, bool S2>
         __FORCE_INLINE constexpr void set_slc2(unsigned lsb, int WS, const iv<N_2, C_2, W2, S2>& op2)
         {
            AC_ASSERT((31 + WS) / 32 <= N_2, "Bad usage: WS greater than length of slice");
            const int N2 = (31 + WS) / 32;
            unsigned msb = lsb + WS - 1;
            unsigned lsb_v = lsb >> 5;
            unsigned lsb_b = lsb & 31;
            unsigned msb_v = msb >> 5;
            unsigned msb_b = msb & 31;
            if(N2 == 1)
            {
               if(msb_v == lsb_v)
               {
                  v.set(lsb_v,
                        v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & ((WS == 32 ? ~0 : ((1u << WS) - 1)) << lsb_b)));
               }
               else
               {
                  v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & (all_ones << lsb_b)));
                  unsigned m = (((unsigned)op2.v[0] >> 1) >> (31 - lsb_b));
                  v.set(msb_v, v[msb_v] ^ ((v[msb_v] ^ m) & ~((all_ones << 1) << msb_b)));
               }
            }
            else
            {
               v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & (all_ones << lsb_b)));
               LOOP(int, i, 1, exclude, N2 - 1,
                    { v.set(lsb_v + i, (op2.v[i] << lsb_b) | (((unsigned)op2.v[i - 1] >> 1) >> (31 - lsb_b))); });
               unsigned t = (op2.v[N2 - 1] << lsb_b) | (((unsigned)op2.v[N2 - 2] >> 1) >> (31 - lsb_b));
               unsigned m = 0;
               if(static_cast<int>(msb_v - lsb_v) == N2)
               {
                  v.set(msb_v - 1, t);
                  m = (((unsigned)op2.v[N2 - 1] >> 1) >> (31 - lsb_b));
               }
               else
               {
                  m = t;
               }
               v.set(msb_v, v[msb_v] ^ ((v[msb_v] ^ m) & ~((all_ones << 1) << msb_b)));
            }
         }
         unsigned leading_bits(bool bit) const
         {
            return iv_leading_bits(v, bit);
         }
      };

      template <>
      template <>
      __FORCE_INLINE constexpr void iv<1, false, 32, false>::set_slc(unsigned lsb, int WS,
                                                                     const iv<1, false, 32, false>& op2)
      {
         v.set(0, v[0] ^ ((v[0] ^ ((unsigned)op2.v[0] << lsb)) & ((WS == 32 ? ~0u : ((1u << WS) - 1)) << lsb)));
      }
      template <>
      template <>
      __FORCE_INLINE constexpr void iv<2, false, 64, false>::set_slc(unsigned lsb, int WS,
                                                                     const iv<1, false, 32, false>& op2)
      {
         Ulong l = to_uint64();
         Ulong l2 = op2.to_uint64();
         l ^= (l ^ (l2 << lsb)) & (((1ULL << WS) - 1) << lsb); // WS <= 32
         *this = iv(l);
      }
      template <>
      template <>
      __FORCE_INLINE constexpr void iv<2, false, 64, false>::set_slc(unsigned lsb, int WS,
                                                                     const iv<2, false, 64, false>& op2)
      {
         Ulong l = to_uint64();
         Ulong l2 = op2.to_uint64();
         l ^= (l ^ (l2 << lsb)) & ((WS == 64 ? ~0ULL : ((1ULL << WS) - 1)) << lsb);
         *this = iv(l);
      }

      // add automatic conversion to Slong/Ulong depending on S and LTE64
      template <int N, bool S, bool LTE64, bool C, int W>
      class iv_conv : public iv<N, C, W, S>
      {
       protected:
         __FORCE_INLINE
         constexpr iv_conv() = default;
         template <class T>
         __FORCE_INLINE constexpr iv_conv(const T& t) : iv<N, C, W, S>(t)
         {
         }
      };

      template <int N, bool C, int W>
      class iv_conv<N, false, true, C, W> : public iv<N, C, W, false>
      {
       public:
         __FORCE_INLINE
         operator Ulong() const
         {
            auto res = iv<N, C, W, false>::to_uint64();
            if(W != 64)
            {
               res = (res << (64 - W)) >> (64 - W);
            }
            return res;
         }

       protected:
         __FORCE_INLINE
         constexpr iv_conv() = default;
         template <class T>
         __FORCE_INLINE constexpr iv_conv(const T& t) : iv<N, C, W, false>(t)
         {
         }
      };

      template <int N, bool C, int W>
      class iv_conv<N, true, true, C, W> : public iv<N, C, W, true>
      {
       public:
         __FORCE_INLINE
         operator Slong() const
         {
            auto res = iv<N, C, W, true>::to_int64();
            if(W != 64)
            {
               res = (res << (64 - W)) >> (64 - W);
            }
            return res;
         }

       protected:
         __FORCE_INLINE
         constexpr iv_conv() = default;
         template <class T>
         __FORCE_INLINE constexpr iv_conv(const T& t) : iv<N, C, W, true>(t)
         {
         }
      };

      // Set default to promote to int as this is the case for almost all types
      //  create exceptions using specializations
      template <typename T>
      struct c_prom
      {
         using promoted_type = int;
      };
      template <>
      struct c_prom<unsigned>
      {
         using promoted_type = unsigned int;
      };
      template <>
      struct c_prom<long>
      {
         using promoted_type = long;
      };
      template <>
      struct c_prom<unsigned long>
      {
         using promoted_type = unsigned long;
      };
      template <>
      struct c_prom<Slong>
      {
         using promoted_type = Slong;
      };
      template <>
      struct c_prom<Ulong>
      {
         using promoted_type = Ulong;
      };
      template <>
      struct c_prom<float>
      {
         using promoted_type = float;
      };
      template <>
      struct c_prom<double>
      {
         using promoted_type = double;
      };

      template <typename T, typename T2>
      struct c_arith
      {
         // will error out for pairs of T and T2 that are not defined through
         // specialization
      };
      template <typename T>
      struct c_arith<T, T>
      {
         using arith_conv = T;
      };

#define C_ARITH(C_TYPE1, C_TYPE2)   \
   template <>                      \
   struct c_arith<C_TYPE1, C_TYPE2> \
   {                                \
      typedef C_TYPE1 arith_conv;   \
   };                               \
   template <>                      \
   struct c_arith<C_TYPE2, C_TYPE1> \
   {                                \
      typedef C_TYPE1 arith_conv;   \
   };

      C_ARITH(double, float)
      C_ARITH(double, int)
      C_ARITH(double, unsigned)
      C_ARITH(double, long)
      C_ARITH(double, unsigned long)
      C_ARITH(double, Slong)
      C_ARITH(double, Ulong)
      C_ARITH(float, int)
      C_ARITH(float, unsigned)
      C_ARITH(float, long)
      C_ARITH(float, unsigned long)
      C_ARITH(float, Slong)
      C_ARITH(float, Ulong)

      C_ARITH(Slong, int)
      C_ARITH(Slong, unsigned)
      C_ARITH(Ulong, int)
      C_ARITH(Ulong, unsigned)

      template <typename T>
      struct map
      {
         using t = T;
      };
      template <typename T>
      struct c_type_params
      {
         // will error out for T for which this template struct is not specialized
      };

      template <typename T>
      __FORCE_INLINE const char* c_type_name()
      {
         return "unknown";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<bool>()
      {
         return "bool";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<char>()
      {
         return "char";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<signed char>()
      {
         return "signed char";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<unsigned char>()
      {
         return "unsigned char";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<signed short>()
      {
         return "signed short";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<unsigned short>()
      {
         return "unsigned short";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<int>()
      {
         return "int";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<unsigned>()
      {
         return "unsigned";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<signed long>()
      {
         return "signed long";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<unsigned long>()
      {
         return "unsigned long";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<signed long long>()
      {
         return "signed long long";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<unsigned long long>()
      {
         return "unsigned long long";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<float>()
      {
         return "float";
      }
      template <>
      __FORCE_INLINE const char* c_type_name<double>()
      {
         return "double";
      }

      template <typename T>
      struct c_type;

      template <typename T>
      struct rt_c_type_T
      {
         template <typename T2>
         struct op1
         {
            using mult = typename T::template rt_T<c_type<T2>>::mult;
            using plus = typename T::template rt_T<c_type<T2>>::plus;
            using minus = typename T::template rt_T<c_type<T2>>::minus2;
            using minus2 = typename T::template rt_T<c_type<T2>>::minus;
            using logic = typename T::template rt_T<c_type<T2>>::logic;
            using div = typename T::template rt_T<c_type<T2>>::div2;
            using div2 = typename T::template rt_T<c_type<T2>>::div;
         };
      };
      template <typename T>
      struct c_type
      {
         using c_prom_T = typename c_prom<T>::promoted_type;
         struct rt_unary
         {
            using neg = c_prom_T;
            using mag_sqr = c_prom_T;
            using mag = c_prom_T;
            template <unsigned N>
            struct set
            {
               using sum = c_prom_T;
            };
         };
         template <typename T2>
         struct rt_T
         {
            using mult = typename rt_c_type_T<T2>::template op1<T>::mult;
            using plus = typename rt_c_type_T<T2>::template op1<T>::plus;
            using minus = typename rt_c_type_T<T2>::template op1<T>::minus;
            using minus2 = typename rt_c_type_T<T2>::template op1<T>::minus2;
            using logic = typename rt_c_type_T<T2>::template op1<T>::logic;
            using div = typename rt_c_type_T<T2>::template op1<T>::div;
            using div2 = typename rt_c_type_T<T2>::template op1<T>::div2;
         };
         __FORCE_INLINE static std::string type_name()
         {
            std::string r = c_type_name<T>();
            return r;
         }
      };
      // with T == c_type
      template <typename T>
      struct rt_c_type_T<c_type<T>>
      {
         using c_prom_T = typename c_prom<T>::promoted_type;
         template <typename T2>
         struct op1
         {
            using c_prom_T2 = typename c_prom<T2>::promoted_type;
            using mult = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using plus = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using minus = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using minus2 = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using logic = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using div = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
            using div2 = typename c_arith<c_prom_T, c_prom_T2>::arith_conv;
         };
      };

#define C_TYPE_MAP(C_TYPE)      \
   template <>                  \
   struct map<C_TYPE>           \
   {                            \
      typedef c_type<C_TYPE> t; \
   };

#define C_TYPE_PARAMS(C_TYPE, WI, SI) \
   template <>                        \
   struct c_type_params<C_TYPE>       \
   {                                  \
      enum                            \
      {                               \
         W = WI,                      \
         I = WI,                      \
         E = 0,                       \
         S = SI,                      \
         floating_point = 0           \
      };                              \
   };

#define C_TYPE_MAP_INT(C_TYPE, WI, SI) \
   C_TYPE_MAP(C_TYPE)                  \
   C_TYPE_PARAMS(C_TYPE, WI, SI)

#define C_TYPE_MAP_FLOAT(C_TYPE, FP, WFP, IFP, EFP) \
   C_TYPE_MAP(C_TYPE)                               \
   template <>                                      \
   struct c_type_params<C_TYPE>                     \
   {                                                \
      enum                                          \
      {                                             \
         W = WFP,                                   \
         I = IFP,                                   \
         E = EFP,                                   \
         S = true,                                  \
         floating_point = FP                        \
      };                                            \
   };

      C_TYPE_MAP_INT(bool, 1, false)
      C_TYPE_MAP_INT(char, 8, true)
      C_TYPE_MAP_INT(signed char, 8, true)
      C_TYPE_MAP_INT(unsigned char, 8, false)
      C_TYPE_MAP_INT(signed short, 16, true)
      C_TYPE_MAP_INT(unsigned short, 16, false)
      C_TYPE_MAP_INT(signed int, 32, true)
      C_TYPE_MAP_INT(unsigned int, 32, false)
      C_TYPE_MAP_INT(signed long, ac_private::long_w, true)
      C_TYPE_MAP_INT(unsigned long, ac_private::long_w, false)
      C_TYPE_MAP_INT(signed long long, 64, true)
      C_TYPE_MAP_INT(unsigned long long, 64, false)
      C_TYPE_MAP_FLOAT(float, 1, 25, 1, 8)
      C_TYPE_MAP_FLOAT(double, 2, 54, 1, 11)

#undef C_TYPE_INT
#undef C_TYPE_PARAMS
#undef C_TYPE_FLOAT
#undef C_TYPE_MAP

      // specializations for following struct declared/defined after definition of
      // ac_int
      template <typename T>
      struct rt_ac_int_T
      {
         template <int W, bool S>
         struct op1
         {
            using mult = typename T::template rt_T<ac_int<W, S>>::mult;
            using plus = typename T::template rt_T<ac_int<W, S>>::plus;
            using minus = typename T::template rt_T<ac_int<W, S>>::minus2;
            using minus2 = typename T::template rt_T<ac_int<W, S>>::minus;
            using logic = typename T::template rt_T<ac_int<W, S>>::logic;
            using div = typename T::template rt_T<ac_int<W, S>>::div2;
            using div2 = typename T::template rt_T<ac_int<W, S>>::div;
         };
      };
   } // namespace ac_private

   namespace ac
   {
      // compiler time constant for log2 like functions
      template <unsigned X>
      struct nbits
      {
         enum
         {
            val = ac_private::s_N<16>::s_X<X>::nbits
         };
      };

      template <unsigned X>
      struct log2_floor
      {
         enum
         {
            val = nbits<X>::val - 1
         };
      };

      // log2 of 0 is not defined: generate compiler error
      template <>
      struct log2_floor<0>
      {
      };

      template <unsigned X>
      struct log2_ceil
      {
         enum
         {
            lf = log2_floor<X>::val,
            val = (X == (1 << lf) ? lf : lf + 1)
         };
      };

      // log2 of 0 is not defined: generate compiler error
      template <>
      struct log2_ceil<0>
      {
      };

      template <int LowerBound, int UpperBound>
      struct int_range
      {
         enum
         {
            l_s = (LowerBound < 0),
            u_s = (UpperBound < 0),
            signedness = l_s || u_s,
            l_nbits = nbits<AC_ABS(LowerBound + l_s) + l_s>::val,
            u_nbits = nbits<AC_ABS(UpperBound + u_s) + u_s>::val,
            nbits = AC_MAX(l_nbits, u_nbits + (!u_s && signedness))
         };
         using type = ac_int<nbits, signedness>;
      };
   } // namespace ac

   enum ac_q_mode
   {
      AC_TRN,
      AC_RND,
      AC_TRN_ZERO,
      AC_RND_ZERO,
      AC_RND_INF,
      AC_RND_MIN_INF,
      AC_RND_CONV,
      AC_RND_CONV_ODD
   };
   enum ac_o_mode
   {
      AC_WRAP,
      AC_SAT,
      AC_SAT_ZERO,
      AC_SAT_SYM
   };
   template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
   class ac_fixed;
   template <int W1, bool S1>
   struct range_ref;

   //////////////////////////////////////////////////////////////////////////////
   //  Arbitrary-Length Integer: ac_int
   //////////////////////////////////////////////////////////////////////////////

   template <int W, bool S = true>
   class ac_int : public ac_private::iv_conv<(W + 31 + !S) / 32, S, W <= 64, !S && ((W % 32) == 0), W>
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
                      __AC_INT_UTILITY_BASE
#endif
   {
      enum
      {
         N = (W + 31 + !S) / 32
      };
      using ConvBase = ac_private::iv_conv<N, S, W <= 64, !S && ((W % 32) == 0), W>;
      using Base = ac_private::iv<N, !S && ((W % 32) == 0), W, S>;

      template <size_t N>
      __FORCE_INLINE void bit_fill(const char (&str)[N])
      {
         if(str[0] == '0' && str[1] == 'x')
         {
            bit_fill_hex(str, 2);
         }
         if(str[0] == '0' && str[1] == 'o')
         {
            bit_fill_oct(str, 2);
         }
         else if(str[0] == '0' && str[1] == 'b')
         {
            bit_fill_bin(str, 2);
         }
         else
         {
            AC_ASSERT(false, "unexpected string format");
         }
      }
      __FORCE_INLINE bool is_neg() const
      {
         return S && Base::v[N - 1] < 0;
      }

      // returns false if number is denormal
      template <int WE, bool SE>
      __FORCE_INLINE bool normalize_private(ac_int<WE, SE>& exp, bool reserved_min_exp = false)
      {
         int expt = exp;
         int lshift = leading_sign();
         bool fully_normalized = true;
         ac_int<WE, SE> min_exp;
         min_exp.template set_val<AC_VAL_MIN>();
         int max_shift = exp - min_exp - reserved_min_exp;
         if(lshift > max_shift)
         {
            lshift = ac_int<WE, false>(max_shift);
            expt = min_exp + reserved_min_exp;
            fully_normalized = false;
         }
         else
         {
            expt -= lshift;
         }
         if(Base::equal_zero())
         {
            expt = 0;
            fully_normalized = true;
         }
         exp = expt;
         Base r;
         Base::shift_l(lshift, r);
         Base::operator=(r);
         return fully_normalized;
      }

    public:
      static const int width = W;
      static const int i_width = W;
      static const bool sign = S;
      static const ac_q_mode q_mode = AC_TRN;
      static const ac_o_mode o_mode = AC_WRAP;
      static const int e_width = 0;

      template <int W2, bool S2>
      struct rt
      {
         enum
         {
            mult_w = W + W2,
            mult_s = S || S2,
            plus_w = AC_MAX(W + (S2 && !S), W2 + (S && !S2)) + 1,
            plus_s = S || S2,
            minus_w = AC_MAX(W + (S2 && !S), W2 + (S && !S2)) + 1,
            minus_s = true,
            div_w = W + S2,
            div_s = S || S2,
            mod_w = AC_MIN(W, W2 + (!S2 && S)),
            mod_s = S,
            logic_w = AC_MAX(W + (S2 && !S), W2 + (S && !S2)),
            logic_s = S || S2
         };
         using mult = ac_int<mult_w, mult_s>;
         using plus = ac_int<plus_w, plus_s>;
         using minus = ac_int<minus_w, minus_s>;
         using logic = ac_int<logic_w, logic_s>;
         using div = ac_int<div_w, div_s>;
         using mod = ac_int<mod_w, mod_s>;
         using arg1 = ac_int<W, S>;
      };

      template <typename T>
      struct rt_T
      {
         using map_T = typename ac_private::map<T>::t;
         using mult = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::mult;
         using plus = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::plus;
         using minus = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::minus;
         using minus2 = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::minus2;
         using logic = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::logic;
         using div = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::div;
         using div2 = typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::div2;
         using arg1 = ac_int<W, S>;
      };

      struct rt_unary
      {
         enum
         {
            neg_w = W + 1,
            neg_s = true,
            mag_sqr_w = 2 * W - S,
            mag_sqr_s = false,
            mag_w = W + S,
            mag_s = false,
            leading_sign_w = ac::log2_ceil<W + !S>::val,
            leading_sign_s = false
         };
         using neg = ac_int<neg_w, neg_s>;
         using mag_sqr = ac_int<mag_sqr_w, mag_sqr_s>;
         using mag = ac_int<mag_w, mag_s>;
         using leading_sign = ac_int<leading_sign_w, leading_sign_s>;
         template <unsigned N>
         struct set
         {
            enum
            {
               sum_w = W + ac::log2_ceil<N>::val,
               sum_s = S
            };
            using sum = ac_int<sum_w, sum_s>;
         };
      };

      template <int W2, bool S2>
      friend class ac_int;
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      friend class ac_fixed;

      __FORCE_INLINE
      constexpr ac_int() = default;

      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_int(const ac_int<W2, S2>& op)
      {
         Base::operator=(op);
      }

      __FORCE_INLINE constexpr ac_int(bool b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(char b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(signed char b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(unsigned char b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(signed short b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(unsigned short b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(signed int b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(unsigned int b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(signed long b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(unsigned long b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(Slong b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(Ulong b) : ConvBase(b)
      {
      }
      __FORCE_INLINE constexpr ac_int(float d) : ConvBase(d)
      {
      }
      __FORCE_INLINE constexpr ac_int(double d) : ConvBase(d)
      {
      }

      template <size_t N>
      __FORCE_INLINE constexpr ac_int(const char (&str)[N])
      {
         bit_fill(str);
      }

      template <ac_special_val V>
      __FORCE_INLINE ac_int& set_val()
      {
         if(V == AC_VAL_DC)
         {
            ac_int r = 0;
            Base::operator=(r);
            Base::v.set(0, 0);
         }
         else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
         {
            Base::operator=(0);
            if(S && V == AC_VAL_MIN)
            {
               const unsigned int rem = (W - 1) & 31;
               Base::v.set(N - 1, (~0u << rem));
            }
            else if(V == AC_VAL_QUANTUM)
            {
               Base::v.set(0, 1);
            }
         }
         else if(V == AC_VAL_MAX)
         {
            Base::operator=(-1);
            const unsigned int rem = (32 - W - (unsigned)!S) & 31;
            Base::v.set(N - 1, ((unsigned)(-1) >> 1) >> rem);
         }
         return *this;
      }

      // Explicit conversion functions to C built-in types -------------
      __FORCE_INLINE int to_int() const
      {
         return this->v[0];
      }
      __FORCE_INLINE unsigned to_uint() const
      {
         return this->v[0];
      }
      __FORCE_INLINE long to_long() const
      {
         return ac_private::long_w == 32 ? (long)Base::v[0] : (long)Base::to_int64();
      }
      __FORCE_INLINE unsigned long to_ulong() const
      {
         return ac_private::long_w == 32 ? (unsigned long)Base::v[0] : (unsigned long)Base::to_uint64();
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         return Base::to_int64();
      }
      __FORCE_INLINE constexpr Ulong to_uint64() const
      {
         return Base::to_uint64();
      }
      __FORCE_INLINE double to_double() const
      {
         return Base::to_double();
      }
      __FORCE_INLINE float to_float() const
      {
         return Base::to_float();
      }
      __FORCE_INLINE int length() const
      {
         return W;
      }

      __FORCE_INLINE explicit operator bool() const
      {
         return !Base::equal_zero();
      }

      __FORCE_INLINE explicit operator char() const
      {
         return (char)to_int();
      }

      __FORCE_INLINE explicit operator signed char() const
      {
         return (signed char)to_int();
      }

      __FORCE_INLINE explicit operator unsigned char() const
      {
         return (unsigned char)to_uint();
      }

      __FORCE_INLINE explicit operator short() const
      {
         return (short)to_int();
      }

      __FORCE_INLINE explicit operator unsigned short() const
      {
         return (unsigned short)to_uint();
      }
      __FORCE_INLINE explicit operator int() const
      {
         return to_int();
      }
      __FORCE_INLINE explicit operator unsigned() const
      {
         return to_uint();
      }
      __FORCE_INLINE explicit operator long() const
      {
         return to_long();
      }
      __FORCE_INLINE explicit operator unsigned long() const
      {
         return to_ulong();
      }
      __FORCE_INLINE explicit operator double() const
      {
         return to_double();
      }
      __FORCE_INLINE explicit operator float() const
      {
         return to_float();
      }

      __FORCE_INLINE std::string to_string(ac_base_mode base_rep, bool sign_mag = false) const
      {
         // base_rep == AC_DEC => sign_mag == don't care (always print decimal in
         // sign magnitude)
         char r[N * 32 + 4] = {0};
         int i = 0;
         if(sign_mag)
         {
            r[i++] = is_neg() ? '-' : '+';
         }
         else if(base_rep == AC_DEC && is_neg())
         {
            r[i++] = '-';
         }
         if(base_rep != AC_DEC)
         {
            r[i++] = '0';
            r[i++] = base_rep == AC_BIN ? 'b' : (base_rep == AC_OCT ? 'o' : 'x');
         }
         int str_w;
         if((base_rep == AC_DEC || sign_mag) && is_neg())
         {
            ac_int<W, false> mag = operator-();
            str_w = ac_private::to_string(mag.v, W + 1, sign_mag, base_rep, false, r + i);
         }
         else
         {
            ac_int<W, S> tmp = *this;
            str_w = ac_private::to_string(tmp.v, W + !S, sign_mag, base_rep, false, r + i);
         }
         if(!str_w)
         {
            r[i] = '0';
            r[i + 1] = 0;
         }
         return std::string(r);
      }
      __FORCE_INLINE
      static std::string type_name()
      {
         const char* tf[] = {",false>", ",true>"};
         std::string r = "ac_int<";
         r += ac_int<32, true>(W).to_string(AC_DEC);
         r += tf[S];
         return r;
      }

      // Arithmetic : Binary ----------------------------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::mult operator*(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::mult r;
         this->Base::mult(op2, r);
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::plus operator+(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::plus r;
         this->Base::add(op2, r);
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::minus operator-(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::minus r;
         this->Base::sub(op2, r);
         return r;
      }

      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::div operator/(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::div r;
         enum
         {
            Nminus = ac_int<W + S, S>::N,
            N2 = ac_int<W2, S2>::N,
            N2minus = ac_int<W2 + S2, S2>::N,
            num_s = S + (Nminus > N),
            den_s = S2 + (N2minus > N2),
            Nr = rt<W2, S2>::div::N
         };
         this->Base::template div<num_s, den_s>(op2, r);
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::mod operator%(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::mod r;
         enum
         {
            Nminus = ac_int<W + S, S>::N,
            N2 = ac_int<W2, S2>::N,
            N2minus = ac_int<W2 + S2, S2>::N,
            num_s = S + (Nminus > N),
            den_s = S2 + (N2minus > N2),
            Nr = rt<W2, S2>::mod::N
         };
         this->Base::template rem<num_s, den_s>(op2, r);
         return r;
      }
      // Arithmetic assign  ------------------------------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator*=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::mult(op2, r);
         Base::operator=(r);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator+=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::add(op2, r);
         Base::operator=(r);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator-=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::sub(op2, r);
         Base::operator=(r);
         return *this;
      }

      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator/=(const ac_int<W2, S2>& op2)
      {
         enum
         {
            Nminus = ac_int<W + S, S>::N,
            N2 = ac_int<W2, S2>::N,
            N2minus = ac_int<W2 + S2, S2>::N,
            num_s = S + (Nminus > N),
            den_s = S2 + (N2minus > N2),
            Nr = N
         };
         Base r;
         Base::template div<num_s, den_s>(op2, r);
         Base::operator=(r);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator%=(const ac_int<W2, S2>& op2)
      {
         enum
         {
            Nminus = ac_int<W + S, S>::N,
            N2 = ac_int<W2, S2>::N,
            N2minus = ac_int<W2 + S2, S2>::N,
            num_s = S + (Nminus > N),
            den_s = S2 + (N2minus > N2),
            Nr = N
         };
         Base r;
         Base::template rem<num_s, den_s>(op2, r);
         Base::operator=(r);
         return *this;
      }

      // Arithmetic prefix increment, decrement ----------------------------------
      __FORCE_INLINE ac_int& operator++()
      {
         Base::increment();
         return *this;
      }
      __FORCE_INLINE ac_int& operator--()
      {
         Base::decrement();
         return *this;
      }
      // Arithmetic postfix increment, decrement ---------------------------------
      __FORCE_INLINE const ac_int operator++(int)
      {
         ac_int t = *this;
         Base::increment();
         return t;
      }
      __FORCE_INLINE const ac_int operator--(int)
      {
         ac_int t = *this;
         Base::decrement();
         return t;
      }
      // Arithmetic Unary --------------------------------------------------------
      __FORCE_INLINE const ac_int operator+() const
      {
         return *this;
      }
      __FORCE_INLINE const typename rt_unary::neg operator-() const
      {
         typename rt_unary::neg r;
         this->Base::neg(r);
         return r;
      }
      // ! ------------------------------------------------------------------------
      __FORCE_INLINE bool operator!() const
      {
         return this->Base::equal_zero();
      }
      __FORCE_INLINE const ac_int<W + !S, true> operator~() const
      {
         ac_int<W + !S, true> r;
         this->Base::bitwise_complement(r);
         return r;
      }
      // Bitwise (non-arithmetic) bit_complement  -----------------------------
      __FORCE_INLINE const ac_int<W, false> bit_complement() const
      {
         ac_int<W, false> r;
         this->Base::bitwise_complement(r);
         return r;
      }
      // Bitwise (arithmetic): and, or, xor ----------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator&(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::logic r;
         this->Base::bitwise_and(op2, r);
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator|(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::logic r;
         this->Base::bitwise_or(op2, r);
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator^(const ac_int<W2, S2>& op2) const
      {
         typename rt<W2, S2>::logic r;
         this->Base::bitwise_xor(op2, r);
         return r;
      }
      // Bitwise assign (not arithmetic): and, or, xor ----------------------------
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator&=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::bitwise_and(op2, r);
         Base::operator=(r);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator|=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::bitwise_or(op2, r);
         Base::operator=(r);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator^=(const ac_int<W2, S2>& op2)
      {
         Base r;
         Base::bitwise_xor(op2, r);
         Base::operator=(r);
         return *this;
      }
      // Shift (result constrained by left operand) -------------------------------
      template <int W2>
      __FORCE_INLINE const ac_int operator<<(const ac_int<W2, true>& op2) const
      {
         ac_int r;
         this->Base::shift_l2(op2.to_int(), r);
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator<<(const ac_int<W2, false>& op2) const
      {
         ac_int r;
         this->Base::shift_l(op2.to_uint(), r);
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator>>(const ac_int<W2, true>& op2) const
      {
         ac_int r;
         this->Base::shift_r2(op2.to_int(), r);
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator>>(const ac_int<W2, false>& op2) const
      {
         ac_int r;
         this->Base::shift_r(op2.to_uint(), r);
         return r;
      }

      // Shift assign ------------------------------------------------------------
      template <int W2>
      __FORCE_INLINE ac_int& operator<<=(const ac_int<W2, true>& op2)
      {
         Base r;
         Base::shift_l2(op2.to_int(), r);
         Base::operator=(r);
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator<<=(const ac_int<W2, false>& op2)
      {
         Base r;
         Base::shift_l(op2.to_uint(), r);
         Base::operator=(r);
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator>>=(const ac_int<W2, true>& op2)
      {
         Base r;
         Base::shift_r2(op2.to_int(), r);
         Base::operator=(r);
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator>>=(const ac_int<W2, false>& op2)
      {
         Base r;
         Base::shift_r(op2.to_uint(), r);
         Base::operator=(r);
         return *this;
      }
      // Relational ---------------------------------------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE bool operator==(const ac_int<W2, S2>& op2) const
      {
         return this->Base::equal(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator!=(const ac_int<W2, S2>& op2) const
      {
         return !this->Base::equal(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator<(const ac_int<W2, S2>& op2) const
      {
         return this->Base::less_than(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator>=(const ac_int<W2, S2>& op2) const
      {
         return !this->Base::less_than(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator>(const ac_int<W2, S2>& op2) const
      {
         return this->Base::greater_than(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator<=(const ac_int<W2, S2>& op2) const
      {
         return !this->Base::greater_than(op2);
      }

      // Bit and Slice Select -----------------------------------------------------
      template <int WS, int WX, bool SX>
      __FORCE_INLINE ac_int<WS, S> slc(const ac_int<WX, SX>& index) const
      {
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
         using ac_intX = ac_int<WX, SX>;
#endif
         ac_int<WS, S> r;
         AC_ASSERT(index >= ac_intX(0), "Attempting to read slc with negative indices");
         ac_int<WX - SX, false> uindex = index;
         Base::shift_r(uindex.to_uint(), r);
         return r;
      }
      __FORCE_INLINE
      ac_int<W, S> operator()(int Hi, int Lo) const
      {
         return slc(Hi, Lo);
      }

      __FORCE_INLINE
      range_ref<W, S> operator()(int Hi, int Lo)
      {
         return range_ref<W, S>(*this, Hi, Lo);
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
      __FORCE_INLINE range_ref<W, S> operator()(const ac_int<W1, S1>& _Hi, const ac_int<W1, S2>& _Lo)
      {
         int Hi = _Hi.to_int();
         int Lo = _Lo.to_int();
         AC_ASSERT(Lo >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= 0, "Attempting to read slc with negative indices");
         return range_ref<W, S>(*this, Hi, Lo);
      }

      __FORCE_INLINE
      ac_int<W, S> range(int Hi, int Lo) const
      {
         return operator()(Hi, Lo);
      }
      template <int W1, int W2, bool S1, bool S2>
      __FORCE_INLINE ac_int<W, S> range(const ac_int<W1, S1>& _Hi, const ac_int<W1, S2>& _Lo) const
      {
         return operator()(_Hi, _Lo);
      }
      __FORCE_INLINE
      range_ref<W, S> range(int Hi, int Lo)
      {
         return range_ref<W, S>(*this, Hi, Lo);
      }

      template <int WS>
      __FORCE_INLINE ac_int<WS, S> slc(signed index) const
      {
         ac_int<WS, S> r;
         AC_ASSERT(index >= 0, "Attempting to read slc with negative indices");
         unsigned uindex = index & ((unsigned)~0 >> 1);
         Base::shift_r(uindex, r);
         return r;
      }
      template <int WS>
      __FORCE_INLINE ac_int<WS, S> slc(unsigned uindex) const
      {
         ac_int<WS, S> r;
         Base::shift_r(uindex, r);
         return r;
      }
      __FORCE_INLINE ac_int<W, S> slc(int Hi, int Lo) const
      {
         AC_ASSERT(Lo >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= 0, "Attempting to read slc with negative indices");
         AC_ASSERT(Hi >= Lo, "Most significant bit greater than the least significant bit");
         AC_ASSERT(W > Hi, "Out of range selection");
         ac_int<W, false> r(*this);
         r = r << ac_int<W, false>(W - 1 - Hi);
         return r >> ac_int<W, false>(Lo + W - 1 - Hi);
      }

      template <int W2, bool S2, int WX, bool SX>
      __FORCE_INLINE constexpr ac_int& set_slc(const ac_int<WX, SX> lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb.to_int() + W2 <= W && lsb.to_int() >= 0, "Out of bounds set_slc");
         ac_int<WX - SX, false> ulsb = lsb;
         Base::set_slc(ulsb.to_uint(), W2, (ac_int<W2, true>)slc);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_int& set_slc(signed lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb + W2 <= W && lsb >= 0, "Out of bounds set_slc");
         unsigned ulsb = lsb & ((unsigned)~0 >> 1);
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_int& set_slc(unsigned ulsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(ulsb + W2 <= W, "Out of bounds set_slc");
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE constexpr ac_int& set_slc(int umsb, int ulsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT((umsb + 1) <= W, "Out of bounds set_slc");
         Base::set_slc2(ulsb, umsb + 1 - ulsb, (ac_int<W2, true>)slc);
         return *this;
      }

      class ac_bitref
      {
         ac_int& d_bv;
         unsigned d_index;

       public:
         __FORCE_INLINE ac_bitref(ac_int* bv, unsigned index = 0) : d_bv(*bv), d_index(index)
         {
         }
         constexpr ac_bitref(const ac_bitref& rhs) = default;
         __FORCE_INLINE
         operator bool() const
         {
            return (d_index < W) ? (d_bv.v[d_index >> 5] >> (d_index & 31) & 1) : 0;
         }

         template <int W2, bool S2>
         __FORCE_INLINE operator ac_int<W2, S2>() const
         {
            return operator bool();
         }

         __FORCE_INLINE
         ac_bitref operator=(int val)
         {
            // lsb of int (val&1) is written to bit
            if(d_index < W)
            {
               // it works even in case value is undefined
               unsigned pos = d_index >> 5;
               auto value = static_cast<unsigned>(d_bv.v[pos]);
               unsigned d_index_masked = d_index & 31;
               unsigned bool_val = val & 1;
               unsigned mask_0 = 1U << d_index_masked;
               unsigned mask_0_neg = ~mask_0;
               value &= mask_0_neg;
               value |= bool_val << d_index_masked;
               d_bv.v.set(d_index >> 5, static_cast<int>(value));
            }
            return *this;
         }
         template <int W2, bool S2>
         __FORCE_INLINE ac_bitref operator=(const ac_int<W2, S2>& val)
         {
            return operator=(val.to_int());
         }
         __FORCE_INLINE
         ac_bitref operator=(const ac_bitref& val)
         {
            return operator=((int)(bool)val);
         }
      };

      __FORCE_INLINE
      ac_bitref operator[](unsigned int uindex)
      {
         AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
         ac_bitref bvh(this, uindex);
         return bvh;
      }
      __FORCE_INLINE
      ac_bitref operator[](int index)
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
      __FORCE_INLINE
      constexpr bool operator[](unsigned int uindex) const
      {
         AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex & 31) & 1) : 0;
      }
      __FORCE_INLINE
      constexpr bool operator[](int index) const
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         unsigned uindex = index & ((unsigned)~0 >> 1);
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex & 31) & 1) : 0;
      }
      template <int W2, bool S2>
      __FORCE_INLINE constexpr bool operator[](const ac_int<W2, S2>& index) const
      {
         AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
         AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
         ac_int<W2 - S2, false> uindex = index;
         return (uindex < W) ? (Base::v[uindex >> 5] >> (uindex.to_uint() & 31) & 1) : 0;
      }
#if 0
    unsigned int leading_bits(bool bit) const {
        return Base::leading_bits(bit) - (32*N - W);
    }
#endif
      __FORCE_INLINE
      typename rt_unary::leading_sign leading_sign() const
      {
         unsigned ls = Base::leading_bits(S & (Base::v[N - 1] < 0)) - (32 * N - W) - S;
         return ls;
      }
      __FORCE_INLINE
      typename rt_unary::leading_sign leading_sign(bool& all_sign) const
      {
         unsigned ls = Base::leading_bits(S & (Base::v[N - 1] < 0)) - (32 * N - W) - S;
         all_sign = (ls == W - S);
         return ls;
      }
      // returns false if number is denormal
      template <int WE, bool SE>
      __FORCE_INLINE bool normalize(ac_int<WE, SE>& exp)
      {
         return normalize_private(exp);
      }
      // returns false if number is denormal, minimum exponent is reserved (usually
      // for encoding special values/errors)
      template <int WE, bool SE>
      __FORCE_INLINE bool normalize_RME(ac_int<WE, SE>& exp)
      {
         return normalize_private(exp, true);
      }
      __FORCE_INLINE
      bool and_reduce() const
      {
         return ac_private::iv_equal_ones_to<W, N>(Base::v);
      }
      __FORCE_INLINE
      bool or_reduce() const
      {
         return !Base::equal_zero();
      }
      __FORCE_INLINE
      bool xor_reduce() const
      {
         unsigned r = Base::v[N - 1];
         if(S)
         {
            const unsigned rem = (32 - W) & 31;
            r = (r << rem) >> rem;
         }
         if(N > 1)
         {
            r ^= Base::v[N - 2];
         }
         if(N > 2)
         {
            LOOP(int, i, 0, exclude, N - 2, { r ^= Base::v[i]; });
         }
         if(W > 16)
         {
            r ^= r >> 16;
         }
         if(W > 8)
         {
            r ^= r >> 8;
         }
         if(W > 4)
         {
            r ^= r >> 4;
         }
         if(W > 2)
         {
            r ^= r >> 2;
         }
         if(W > 1)
         {
            r ^= r >> 1;
         }
         return r & 1;
      }

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_bin(const char (&str)[NN], int start = 0);

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_oct(const char (&str)[NN], int start = 0)
      {
         // Zero Pads if str is too short, throws ms bits away if str is too long
         // Asserts if anything other than 0-9a-fA-F is encountered
         ac_int<W, S> res = 0;
         bool loop_exit = false;
#if __cplusplus >= 201703L
         LOOP(int, i, 0, exclude, NN, {
            if(!loop_exit && i >= start)
            {
               char c = str[i];
               int h = 0;
               if(c >= '0' && c <= '8')
                  h = c - '0';
               else
               {
                  AC_ASSERT(!c, "Invalid hex digit");
                  loop_exit = true;
                  return;
               }
               res <<= (ac_int<W, false>(3));
               res |= (ac_int<4, false>(h));
            }
         });
#else
      LOOP(int, i, 0, exclude, NN, {
         if(!loop_exit && i >= start)
         {
            char c = str[i];
            int h = 0;
            if(c >= '0' && c <= '8')
               h = c - '0';
            else
            {
               AC_ASSERT(!c, "Invalid hex digit");
               loop_exit = true;
               continue;
            }
            res <<= (ac_int<W, false>(3));
            res |= (ac_int<4, false>(h));
         }
      });
#endif
         *this = res;
      }

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_hex(const char (&str)[NN], int start = 0)
      {
         // Zero Pads if str is too short, throws ms bits away if str is too long
         // Asserts if anything other than 0-9a-fA-F is encountered
         ac_int<W, S> res = 0;
         bool loop_exit = false;
#if __cplusplus >= 201703L
         LOOP(int, i, 0, exclude, NN, {
            if(!loop_exit && i >= start)
            {
               char c = str[i];
               int h = 0;
               if(c >= '0' && c <= '9')
                  h = c - '0';
               else if(c >= 'A' && c <= 'F')
                  h = c - 'A' + 10;
               else if(c >= 'a' && c <= 'f')
                  h = c - 'a' + 10;
               else
               {
                  AC_ASSERT(!c, "Invalid hex digit");
                  loop_exit = true;
                  return;
               }
               res <<= (ac_int<W, false>(4));
               res |= (ac_int<4, false>(h));
            }
         });
#else
      LOOP(int, i, 0, exclude, NN, {
         if(!loop_exit && i >= start)
         {
            char c = str[i];
            int h = 0;
            if(c >= '0' && c <= '9')
               h = c - '0';
            else if(c >= 'A' && c <= 'F')
               h = c - 'A' + 10;
            else if(c >= 'a' && c <= 'f')
               h = c - 'a' + 10;
            else
            {
               AC_ASSERT(!c, "Invalid hex digit");
               loop_exit = true;
               continue;
            }
            res <<= (ac_int<W, false>(4));
            res |= (ac_int<4, false>(h));
         }
      });
#endif
         *this = res;
      }

      template <int Na>
      __FORCE_INLINE void bit_fill(const int (&ivec)[Na], bool bigendian = true)
      {
         // bit_fill from integer vector
         //   if W > N*32, missing most significant bits are zeroed
         //   if W < N*32, additional bits in ivec are ignored (no overflow checking)
         // Example:
         //   ac_int<80,false> x;    int vec[] = { 0xffffa987, 0x6543210f, 0xedcba987
         //   }; x.bit_fill(vec);   // vec[0] fill bits 79-64
         enum
         {
            N0 = (W + 31) / 32,
            M = AC_MIN(N0, Na)
         };
         ac_int<M * 32, false> res = 0;
         LOOP(int, i, 0, exclude, M, { res.set_slc(i * 32, ac_int<32>(ivec[bigendian ? M - 1 - i : i])); });
         *this = res;
      }
   };

   template <int W1, bool S1>
   struct range_ref
   {
      ac_int<W1, S1>& ref;
      int low;
      int high;
      __FORCE_INLINE range_ref(ac_int<W1, S1>& _ref, int _high, int _low) : ref(_ref), low(_low), high(_high)
      {
         AC_ASSERT(_high < W1, "Out of bounds range_ref");
         AC_ASSERT(_low < W1, "Out of bounds range_ref");
         AC_ASSERT(_low <= _high, "low and high inverted in range_ref");
      }

      template <int W2, bool S2>
      __FORCE_INLINE const range_ref& operator=(const ac_int<W2, S2>& op) const
      {
         ref.set_slc(high, low, op);
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE operator ac_int<W2, S2>() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r;
      }
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      __FORCE_INLINE const range_ref& operator=(const ac_fixed<W2, I2, S2, Q2, O2>& b) const
      {
         return operator=(b.to_ac_int());
      }
      template <int W2, bool S2>
      __FORCE_INLINE const range_ref& operator=(const range_ref<W2, S2>& b) const
      {
         const ac_int<W1, false> r = b.ref.slc(b.high, b.low);
         return operator=(r);
      }
      __FORCE_INLINE int to_int() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_int();
      }
      __FORCE_INLINE unsigned to_uint() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_uint();
      }
      __FORCE_INLINE long to_long() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_long();
      }
      __FORCE_INLINE unsigned long to_ulong() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_ulong();
      }
      __FORCE_INLINE constexpr Slong to_int64() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_int64();
      }
      __FORCE_INLINE constexpr Ulong to_uint64() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_uint64();
      }
      __FORCE_INLINE double to_double() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_double();
      }

      __FORCE_INLINE int length() const
      {
         return W1;
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
      __FORCE_INLINE operator double() const
      {
         return to_double();
      }
   };

   namespace ac
   {
      template <typename T, typename T2>
      struct rt_2T
      {
         using map_T = typename ac_private::map<T>::t;
         using map_T2 = typename ac_private::map<T2>::t;
         using mult = typename map_T::template rt_T<map_T2>::mult;
         using plus = typename map_T::template rt_T<map_T2>::plus;
         using minus = typename map_T::template rt_T<map_T2>::minus;
         using minus2 = typename map_T::template rt_T<map_T2>::minus2;
         using logic = typename map_T::template rt_T<map_T2>::logic;
         using div = typename map_T::template rt_T<map_T2>::div;
         using div2 = typename map_T::template rt_T<map_T2>::div2;
      };
   } // namespace ac

   namespace ac
   {
      template <typename T>
      struct ac_int_represent
      {
         enum
         {
            t_w = ac_private::c_type_params<T>::W,
            t_s = ac_private::c_type_params<T>::S
         };
         using type = ac_int<t_w, t_s>;
      };
      template <>
      struct ac_int_represent<float>
      {
      };
      template <>
      struct ac_int_represent<double>
      {
      };
      template <int W, bool S>
      struct ac_int_represent<ac_int<W, S>>
      {
         using type = ac_int<W, S>;
      };
   } // namespace ac

   namespace ac_private
   {
      template <int W2, bool S2>
      struct rt_ac_int_T<ac_int<W2, S2>>
      {
         using i2_t = ac_int<W2, S2>;
         template <int W, bool S>
         struct op1
         {
            using i_t = ac_int<W, S>;
            using mult = typename i_t::template rt<W2, S2>::mult;
            using plus = typename i_t::template rt<W2, S2>::plus;
            using minus = typename i_t::template rt<W2, S2>::minus;
            using minus2 = typename i2_t::template rt<W, S>::minus;
            using logic = typename i_t::template rt<W2, S2>::logic;
            using div = typename i_t::template rt<W2, S2>::div;
            using div2 = typename i2_t::template rt<W, S>::div;
            using mod = typename i_t::template rt<W2, S2>::mod;
            using mod2 = typename i2_t::template rt<W, S>::mod;
         };
      };

      template <typename T>
      struct rt_ac_int_T<c_type<T>>
      {
         using i2_t = typename ac::ac_int_represent<T>::type;
         enum
         {
            W2 = i2_t::width,
            S2 = i2_t::sign
         };
         template <int W, bool S>
         struct op1
         {
            using i_t = ac_int<W, S>;
            using mult = typename i_t::template rt<W2, S2>::mult;
            using plus = typename i_t::template rt<W2, S2>::plus;
            using minus = typename i_t::template rt<W2, S2>::minus;
            using minus2 = typename i2_t::template rt<W, S>::minus;
            using logic = typename i_t::template rt<W2, S2>::logic;
            using div = typename i_t::template rt<W2, S2>::div;
            using div2 = typename i2_t::template rt<W, S>::div;
            using mod = typename i_t::template rt<W2, S2>::mod;
            using mod2 = typename i2_t::template rt<W, S>::mod;
         };
      };
   } // namespace ac_private

   // Specializations for constructors on integers that bypass bit adjusting
   //  and are therefore more efficient
   template <>
   __FORCE_INLINE constexpr ac_int<1, true>::ac_int(bool b)
   {
      v.set(0, b ? -1 : 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(signed char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(unsigned char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(signed short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(unsigned short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(signed int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(unsigned int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(signed long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(unsigned long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b & 1);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<1, false>::ac_int(Slong b)
   {
      v.set(0, (int)b & 1);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<8, true>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<8, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<8, true>::ac_int(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<8, false>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<8, true>::ac_int(unsigned char b)
   {
      v.set(0, (signed char)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<8, false>::ac_int(signed char b)
   {
      v.set(0, (unsigned char)b);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<16, true>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, true>::ac_int(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, false>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, true>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, false>::ac_int(signed char b)
   {
      v.set(0, (unsigned short)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, true>::ac_int(signed short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, false>::ac_int(unsigned short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, true>::ac_int(unsigned short b)
   {
      v.set(0, (signed short)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<16, false>::ac_int(signed short b)
   {
      v.set(0, (unsigned short)b);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<32, true>::ac_int(signed int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, true>::ac_int(unsigned int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, false>::ac_int(signed int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, false>::ac_int(unsigned int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<32, true>::ac_int(Slong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, true>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, false>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<32, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE constexpr ac_int<64, true>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE constexpr ac_int<64, true>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE constexpr ac_int<64, false>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)((Ulong)b >> 32));
      v.set(2, 0);
   }
   template <>
   __FORCE_INLINE constexpr ac_int<64, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
      v.set(2, 0);
   }

   template <int W, bool S>
   template <size_t NN>
   __FORCE_INLINE constexpr void ac_int<W, S>::bit_fill_bin(const char (&str)[NN], int start)
   {
      ac_int<W, S> res = 0;
      bool loop_exit = false;
#if __cplusplus >= 201703L
      LOOP(int, i, 0, exclude, NN, {
         if(!loop_exit && i >= start)
         {
            char c = str[i];
            int h = 0;
            if(c == '0')
               h = 0;
            else if(c == '1')
               h = 1;
            else
            {
               AC_ASSERT(!c, "Invalid hex digit");
               loop_exit = true;
               return;
            }
            res <<= (ac_int<1, false>(1));
            res |= (ac_int<1, false>(h));
         }
      });
#else
   LOOP(int, i, 0, exclude, NN, {
      if(!loop_exit && i >= start)
      {
         char c = str[i];
         int h = 0;
         if(c == '0')
            h = 0;
         else if(c == '1')
            h = 1;
         else
         {
            AC_ASSERT(!c, "Invalid hex digit");
            loop_exit = true;
            continue;
         }
         res <<= (ac_int<1, false>(1));
         res |= (ac_int<1, false>(h));
      }
   });
#endif
      *this = res;
   }
   // Stream --------------------------------------------------------------------

   template <int W, bool S>
   __FORCE_INLINE std::ostream& operator<<(std::ostream& os, const ac_int<W, S>& x)
   {
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
      if((os.flags() & std::ios::hex) != 0)
      {
         os << x.to_string(AC_HEX);
      }
      else if((os.flags() & std::ios::oct) != 0)
      {
         os << x.to_string(AC_OCT);
      }
      else
      {
         os << x.to_string(AC_DEC);
      }
#endif
      return os;
   }

   template <int W, bool S>
   __FORCE_INLINE std::istream& operator>>(std::istream& in, ac_int<W, S>& x)
   {
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)

      std::string str;
      in >> str;
      const std::ios_base::fmtflags basefield = in.flags() & std::ios_base::basefield;
      unsigned radix = (basefield == std::ios_base::dec) ?
                           0 :
                           ((basefield == std::ios_base::oct) ? 8 : ((basefield == std::ios_base::hex) ? 16 : 0));
      // x = convert a char * str.c_str() with specified radix into ac_int; //TODO
#endif
      return in;
   }

   // Macros for Binary Operators with Integers
   // --------------------------------------------

#define BIN_OP_WITH_INT(BIN_OP, C_TYPE, WI, SI, RTYPE)                                                      \
   template <int W, bool S>                                                                                 \
   __FORCE_INLINE typename ac_int<WI, SI>::template rt<W, S>::RTYPE operator BIN_OP(C_TYPE i_op,            \
                                                                                    const ac_int<W, S>& op) \
   {                                                                                                        \
      return ac_int<WI, SI>(i_op).operator BIN_OP(op);                                                      \
   }                                                                                                        \
   template <int W, bool S>                                                                                 \
   __FORCE_INLINE typename ac_int<W, S>::template rt<WI, SI>::RTYPE operator BIN_OP(const ac_int<W, S>& op, \
                                                                                    C_TYPE i_op)            \
   {                                                                                                        \
      return op.operator BIN_OP(ac_int<WI, SI>(i_op));                                                      \
   }

#define REL_OP_WITH_INT(REL_OP, C_TYPE, W2, S2)                            \
   template <int W, bool S>                                                \
   __FORCE_INLINE bool operator REL_OP(const ac_int<W, S>& op, C_TYPE op2) \
   {                                                                       \
      return op.operator REL_OP(ac_int<W2, S2>(op2));                      \
   }                                                                       \
   template <int W, bool S>                                                \
   __FORCE_INLINE bool operator REL_OP(C_TYPE op2, const ac_int<W, S>& op) \
   {                                                                       \
      return ac_int<W2, S2>(op2).operator REL_OP(op);                      \
   }

#define ASSIGN_OP_WITH_INT(ASSIGN_OP, C_TYPE, W2, S2)                            \
   template <int W, bool S>                                                      \
   __FORCE_INLINE ac_int<W, S>& operator ASSIGN_OP(ac_int<W, S>& op, C_TYPE op2) \
   {                                                                             \
      return op.operator ASSIGN_OP(ac_int<W2, S2>(op2));                         \
   }

#define OPS_WITH_INT(C_TYPE, WI, SI)         \
   BIN_OP_WITH_INT(*, C_TYPE, WI, SI, mult)  \
   BIN_OP_WITH_INT(+, C_TYPE, WI, SI, plus)  \
   BIN_OP_WITH_INT(-, C_TYPE, WI, SI, minus) \
   BIN_OP_WITH_INT(/, C_TYPE, WI, SI, div)   \
   BIN_OP_WITH_INT(%, C_TYPE, WI, SI, mod)   \
   BIN_OP_WITH_INT(>>, C_TYPE, WI, SI, arg1) \
   BIN_OP_WITH_INT(<<, C_TYPE, WI, SI, arg1) \
   BIN_OP_WITH_INT(&, C_TYPE, WI, SI, logic) \
   BIN_OP_WITH_INT(|, C_TYPE, WI, SI, logic) \
   BIN_OP_WITH_INT(^, C_TYPE, WI, SI, logic) \
                                             \
   REL_OP_WITH_INT(==, C_TYPE, WI, SI)       \
   REL_OP_WITH_INT(!=, C_TYPE, WI, SI)       \
   REL_OP_WITH_INT(>, C_TYPE, WI, SI)        \
   REL_OP_WITH_INT(>=, C_TYPE, WI, SI)       \
   REL_OP_WITH_INT(<, C_TYPE, WI, SI)        \
   REL_OP_WITH_INT(<=, C_TYPE, WI, SI)       \
                                             \
   ASSIGN_OP_WITH_INT(+=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(-=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(*=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(/=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(%=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(>>=, C_TYPE, WI, SI)   \
   ASSIGN_OP_WITH_INT(<<=, C_TYPE, WI, SI)   \
   ASSIGN_OP_WITH_INT(&=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(|=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(^=, C_TYPE, WI, SI)

// ------------------------------------- End of Macros for Binary Operators with
// Integers

// --- Macro for range_ref
#define OP_BIN_RANGE(BIN_OP, RTYPE)                                                                                    \
   template <int W1, bool S1, int W2, bool S2>                                                                         \
   __FORCE_INLINE typename ac_int<W1, S1>::template rt<W2, S2>::RTYPE operator BIN_OP(const range_ref<W1, S1>& op1,    \
                                                                                      const range_ref<W2, S2>& op2)    \
   {                                                                                                                   \
      return ac_int<W1, false>(op1) BIN_OP(ac_int<W2, false>(op2));                                                    \
   }                                                                                                                   \
   template <int W1, bool S1, int W2, bool S2>                                                                         \
   __FORCE_INLINE typename ac_int<W1, S1>::template rt<W2, S2>::RTYPE operator BIN_OP(const range_ref<W1, S1>& op1,    \
                                                                                      const ac_int<W2, S2>& op2)       \
   {                                                                                                                   \
      return ac_int<W1, false>(op1) BIN_OP(op2);                                                                       \
   }                                                                                                                   \
   template <int W1, bool S1, int W2, bool S2>                                                                         \
   __FORCE_INLINE typename ac_int<W1, S1>::template RType<W2, S2>::RTYPE operator BIN_OP(const ac_int<W1, S1>& op1,    \
                                                                                         const range_ref<W2, S2>& op2) \
   {                                                                                                                   \
      return op1 BIN_OP(ac_int<W2, false>(op2));                                                                       \
   }

#define OP_REL_RANGE(REL_OP)                                                                       \
   template <int W1, bool S1, int W2, bool S2>                                                     \
   __FORCE_INLINE bool operator REL_OP(const range_ref<W1, S1>& op1, const range_ref<W2, S2>& op2) \
   {                                                                                               \
      return ac_int<W1, false>(op1).operator REL_OP(op2.operator ac_int<W2, false>());             \
   }                                                                                               \
   template <int W1, bool S1, int W2, bool S2>                                                     \
   __FORCE_INLINE bool operator REL_OP(const range_ref<W1, S1>& op1, const ac_int<W2, S2>& op2)    \
   {                                                                                               \
      return ac_int<W1, false>(op1).operator REL_OP(op2);                                          \
   }                                                                                               \
   template <int W1, bool S1, int W2, bool S2>                                                     \
   __FORCE_INLINE bool operator REL_OP(const ac_int<W1, S1>& op1, const range_ref<W2, S2>& op2)    \
   {                                                                                               \
      return op1.operator REL_OP(op2.operator ac_int<W2, false>());                                \
   }

#define OP_ASSIGN_RANGE(ASSIGN_OP)                                                                      \
   template <int W1, bool S1, int W2, bool S2>                                                          \
   __FORCE_INLINE ac_int<W1, S1>& operator ASSIGN_OP(ac_int<W1, S1>& op1, range_ref<W2, S2>& op2)       \
   {                                                                                                    \
      return op1.operator ASSIGN_OP(ac_int<W2, false>(op2));                                            \
   }                                                                                                    \
   template <int W1, bool S1, int W2, bool S2>                                                          \
   __FORCE_INLINE range_ref<W1, S1>& operator ASSIGN_OP(range_ref<W1, S1>& op1, ac_int<W2, S2>& op2)    \
   {                                                                                                    \
      ac_int<W1, false> tmp(op1);                                                                       \
      tmp.operator ASSIGN_OP(op2);                                                                      \
      op1 = tmp;                                                                                        \
      return op1;                                                                                       \
   }                                                                                                    \
   template <int W1, bool S1, int W2, bool S2>                                                          \
   __FORCE_INLINE range_ref<W1, S1>& operator ASSIGN_OP(range_ref<W1, S1>& op1, range_ref<W2, S2>& op2) \
   {                                                                                                    \
      ac_int<W1, false> tmp(op1);                                                                       \
      tmp.operator ASSIGN_OP(ac_int<W2, false>(op2));                                                   \
      op1 = tmp;                                                                                        \
      return op1;                                                                                       \
   }

   namespace ac
   {
      namespace ops_with_other_types
      {
         //  Mixed Operators with Integers
         //  -----------------------------------------------
         OPS_WITH_INT(bool, 1, false)
         OPS_WITH_INT(char, 8, true)
         OPS_WITH_INT(signed char, 8, true)
         OPS_WITH_INT(unsigned char, 8, false)
         OPS_WITH_INT(short, 16, true)
         OPS_WITH_INT(unsigned short, 16, false)
         OPS_WITH_INT(int, 32, true)
         OPS_WITH_INT(unsigned int, 32, false)
         OPS_WITH_INT(long, ac_private::long_w, true)
         OPS_WITH_INT(unsigned long, ac_private::long_w, false)
         OPS_WITH_INT(Slong, 64, true)
         OPS_WITH_INT(Ulong, 64, false)
         // -----------------------------------------  End of Mixed Operators with
         // Integers
      } // namespace ops_with_other_types

      namespace ops_with_range_types
      {
         OP_ASSIGN_RANGE(+=)
         OP_ASSIGN_RANGE(-=)
         OP_ASSIGN_RANGE(*=)
         OP_ASSIGN_RANGE(/=)
         OP_ASSIGN_RANGE(%=)
         OP_ASSIGN_RANGE(>>=)
         OP_ASSIGN_RANGE(<<=)
         OP_ASSIGN_RANGE(&=)
         OP_ASSIGN_RANGE(|=)
         OP_ASSIGN_RANGE(^=)

         OP_REL_RANGE(==)
         OP_REL_RANGE(!=)
         OP_REL_RANGE(>)
         OP_REL_RANGE(>=)
         OP_REL_RANGE(<)
         OP_REL_RANGE(<=)

         OP_BIN_RANGE(+, plus)
         OP_BIN_RANGE(-, minus)
         OP_BIN_RANGE(*, mult)
         OP_BIN_RANGE(/, div)
         OP_BIN_RANGE(%, mod)
         OP_BIN_RANGE(>>, arg1)
         OP_BIN_RANGE(<<, arg1)
         OP_BIN_RANGE(&, logic)
         OP_BIN_RANGE(|, logic)
         OP_BIN_RANGE(^, logic)
      } // namespace ops_with_range_types

      // Functions to fill bits

      template <typename T>
      __FORCE_INLINE T bit_fill_hex(const char* str)
      {
         T res;
         res.bit_fill_hex(str);
         return res;
      }

      // returns bit_fill for type
      //   example:
      //   ac_int<80,false> x = ac::bit_fill< ac_int<80,false> > ((int [3])
      //   {0xffffa987, 0x6543210f, 0xedcba987 });
      template <typename T, int N>
      __FORCE_INLINE T bit_fill(const int (&ivec)[N], bool bigendian = true)
      {
         T res;
         res.bit_fill(ivec, bigendian);
         return res;
      }

   } // namespace ac

   //  Mixed Operators with Pointers
   //  -----------------------------------------------

   // Addition of ac_int and  pointer
   template <typename T, int W, bool S>
   __FORCE_INLINE T* operator+(T* ptr, const ac_int<W, S>& op2)
   {
      return ptr + op2.to_int64();
   }
   template <typename T, int W, bool S>
   __FORCE_INLINE T* operator+(const ac_int<W, S>& op2, T* ptr)
   {
      return ptr + op2.to_int64();
   }
   // Subtraction of ac_int from pointer
   template <typename T, int W, bool S>
   __FORCE_INLINE T* operator-(T* ptr, const ac_int<W, S>& op2)
   {
      return ptr - op2.to_int64();
   }
   // -----------------------------------------  End of Mixed Operators with
   // Pointers

   using namespace ac::ops_with_other_types;
   using namespace ac::ops_with_range_types;

   namespace ac_intN
   {
      ///////////////////////////////////////////////////////////////////////////////
      //  Predefined for ease of use
      ///////////////////////////////////////////////////////////////////////////////
      using int1 = ac_int<1, true>;
      using uint1 = ac_int<1, false>;
      using int2 = ac_int<2, true>;
      using uint2 = ac_int<2, false>;
      using int3 = ac_int<3, true>;
      using uint3 = ac_int<3, false>;
      using int4 = ac_int<4, true>;
      using uint4 = ac_int<4, false>;
      using int5 = ac_int<5, true>;
      using uint5 = ac_int<5, false>;
      using int6 = ac_int<6, true>;
      using uint6 = ac_int<6, false>;
      using int7 = ac_int<7, true>;
      using uint7 = ac_int<7, false>;
      using int8 = ac_int<8, true>;
      using uint8 = ac_int<8, false>;
      using int9 = ac_int<9, true>;
      using uint9 = ac_int<9, false>;
      using int10 = ac_int<10, true>;
      using uint10 = ac_int<10, false>;
      using int11 = ac_int<11, true>;
      using uint11 = ac_int<11, false>;
      using int12 = ac_int<12, true>;
      using uint12 = ac_int<12, false>;
      using int13 = ac_int<13, true>;
      using uint13 = ac_int<13, false>;
      using int14 = ac_int<14, true>;
      using uint14 = ac_int<14, false>;
      using int15 = ac_int<15, true>;
      using uint15 = ac_int<15, false>;
      using int16 = ac_int<16, true>;
      using uint16 = ac_int<16, false>;
      using int17 = ac_int<17, true>;
      using uint17 = ac_int<17, false>;
      using int18 = ac_int<18, true>;
      using uint18 = ac_int<18, false>;
      using int19 = ac_int<19, true>;
      using uint19 = ac_int<19, false>;
      using int20 = ac_int<20, true>;
      using uint20 = ac_int<20, false>;
      using int21 = ac_int<21, true>;
      using uint21 = ac_int<21, false>;
      using int22 = ac_int<22, true>;
      using uint22 = ac_int<22, false>;
      using int23 = ac_int<23, true>;
      using uint23 = ac_int<23, false>;
      using int24 = ac_int<24, true>;
      using uint24 = ac_int<24, false>;
      using int25 = ac_int<25, true>;
      using uint25 = ac_int<25, false>;
      using int26 = ac_int<26, true>;
      using uint26 = ac_int<26, false>;
      using int27 = ac_int<27, true>;
      using uint27 = ac_int<27, false>;
      using int28 = ac_int<28, true>;
      using uint28 = ac_int<28, false>;
      using int29 = ac_int<29, true>;
      using uint29 = ac_int<29, false>;
      using int30 = ac_int<30, true>;
      using uint30 = ac_int<30, false>;
      using int31 = ac_int<31, true>;
      using uint31 = ac_int<31, false>;
      using int32 = ac_int<32, true>;
      using uint32 = ac_int<32, false>;
      using int33 = ac_int<33, true>;
      using uint33 = ac_int<33, false>;
      using int34 = ac_int<34, true>;
      using uint34 = ac_int<34, false>;
      using int35 = ac_int<35, true>;
      using uint35 = ac_int<35, false>;
      using int36 = ac_int<36, true>;
      using uint36 = ac_int<36, false>;
      using int37 = ac_int<37, true>;
      using uint37 = ac_int<37, false>;
      using int38 = ac_int<38, true>;
      using uint38 = ac_int<38, false>;
      using int39 = ac_int<39, true>;
      using uint39 = ac_int<39, false>;
      using int40 = ac_int<40, true>;
      using uint40 = ac_int<40, false>;
      using int41 = ac_int<41, true>;
      using uint41 = ac_int<41, false>;
      using int42 = ac_int<42, true>;
      using uint42 = ac_int<42, false>;
      using int43 = ac_int<43, true>;
      using uint43 = ac_int<43, false>;
      using int44 = ac_int<44, true>;
      using uint44 = ac_int<44, false>;
      using int45 = ac_int<45, true>;
      using uint45 = ac_int<45, false>;
      using int46 = ac_int<46, true>;
      using uint46 = ac_int<46, false>;
      using int47 = ac_int<47, true>;
      using uint47 = ac_int<47, false>;
      using int48 = ac_int<48, true>;
      using uint48 = ac_int<48, false>;
      using int49 = ac_int<49, true>;
      using uint49 = ac_int<49, false>;
      using int50 = ac_int<50, true>;
      using uint50 = ac_int<50, false>;
      using int51 = ac_int<51, true>;
      using uint51 = ac_int<51, false>;
      using int52 = ac_int<52, true>;
      using uint52 = ac_int<52, false>;
      using int53 = ac_int<53, true>;
      using uint53 = ac_int<53, false>;
      using int54 = ac_int<54, true>;
      using uint54 = ac_int<54, false>;
      using int55 = ac_int<55, true>;
      using uint55 = ac_int<55, false>;
      using int56 = ac_int<56, true>;
      using uint56 = ac_int<56, false>;
      using int57 = ac_int<57, true>;
      using uint57 = ac_int<57, false>;
      using int58 = ac_int<58, true>;
      using uint58 = ac_int<58, false>;
      using int59 = ac_int<59, true>;
      using uint59 = ac_int<59, false>;
      using int60 = ac_int<60, true>;
      using uint60 = ac_int<60, false>;
      using int61 = ac_int<61, true>;
      using uint61 = ac_int<61, false>;
      using int62 = ac_int<62, true>;
      using uint62 = ac_int<62, false>;
      using int63 = ac_int<63, true>;
      using uint63 = ac_int<63, false>;
   } // namespace ac_intN

#ifndef AC_NOT_USING_INTN
   using namespace ac_intN;
#endif

   ///////////////////////////////////////////////////////////////////////////////

   // Global templatized functions for easy initialization to special values
   template <ac_special_val V, int W, bool S>
   __FORCE_INLINE ac_int<W, S> value(ac_int<W, S> v)
   {
      ac_int<W, S> r;
      return r.template set_val<V>();
   }
   // forward declaration, otherwise GCC errors when calling init_array
   template <ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   __FORCE_INLINE ac_fixed<W, I, S, Q, O> value(ac_fixed<W, I, S, Q, O>);

#define SPECIAL_VAL_FOR_INTS_DC(C_TYPE, WI, SI)     \
   template <>                                      \
   __FORCE_INLINE C_TYPE value<AC_VAL_DC>(C_TYPE b) \
   {                                                \
      C_TYPE x = b;                                 \
      return x;                                     \
   }

// -- C int types
// -----------------------------------------------------------------
#define SPECIAL_VAL_FOR_INTS(C_TYPE, WI, SI)                         \
   template <ac_special_val val>                                     \
   __FORCE_INLINE C_TYPE value(C_TYPE);                              \
   template <>                                                       \
   __FORCE_INLINE C_TYPE value<AC_VAL_0>(C_TYPE)                     \
   {                                                                 \
      return (C_TYPE)0;                                              \
   }                                                                 \
   SPECIAL_VAL_FOR_INTS_DC(C_TYPE, WI, SI)                           \
   template <>                                                       \
   __FORCE_INLINE C_TYPE value<AC_VAL_QUANTUM>(C_TYPE)               \
   {                                                                 \
      return (C_TYPE)1;                                              \
   }                                                                 \
   template <>                                                       \
   __FORCE_INLINE C_TYPE value<AC_VAL_MAX>(C_TYPE)                   \
   {                                                                 \
      return (C_TYPE)(SI ? ~(((C_TYPE)1) << (WI - 1)) : (C_TYPE)-1); \
   }                                                                 \
   template <>                                                       \
   __FORCE_INLINE C_TYPE value<AC_VAL_MIN>(C_TYPE)                   \
   {                                                                 \
      return (C_TYPE)(SI ? ((C_TYPE)1) << (WI - 1) : (C_TYPE)0);     \
   }

   SPECIAL_VAL_FOR_INTS(bool, 1, false)
   SPECIAL_VAL_FOR_INTS(char, 8, true)
   SPECIAL_VAL_FOR_INTS(signed char, 8, true)
   SPECIAL_VAL_FOR_INTS(unsigned char, 8, false)
   SPECIAL_VAL_FOR_INTS(short, 16, true)
   SPECIAL_VAL_FOR_INTS(unsigned short, 16, false)
   SPECIAL_VAL_FOR_INTS(int, 32, true)
   SPECIAL_VAL_FOR_INTS(unsigned int, 32, false)
   SPECIAL_VAL_FOR_INTS(long, ac_private::long_w, true)
   SPECIAL_VAL_FOR_INTS(unsigned long, ac_private::long_w, false)
   SPECIAL_VAL_FOR_INTS(Slong, 64, true)
   SPECIAL_VAL_FOR_INTS(Ulong, 64, false)

#define INIT_ARRAY_SPECIAL_VAL_FOR_INTS(C_TYPE)     \
   template <ac_special_val V>                      \
   __FORCE_INLINE bool init_array(C_TYPE* a, int n) \
   {                                                \
      C_TYPE t = value<V>(*a);                      \
      for(int i = 0; i < n; i++)                    \
         a[i] = t;                                  \
      return true;                                  \
   }

   namespace ac
   {
      // PUBLIC FUNCTIONS
      // function to initialize (or uninitialize) arrays
      template <ac_special_val V, int W, bool S>
      __FORCE_INLINE bool init_array(ac_int<W, S>* a, int n)
      {
         ac_int<W, S> t = value<V>(*a);
         for(int i = 0; i < n; i++)
         {
            a[i] = t;
         }
         return true;
      }

      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(bool)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(char)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(signed char)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(unsigned char)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(signed short)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(unsigned short)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(signed int)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(unsigned int)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(signed long)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(unsigned long)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(signed long long)
      INIT_ARRAY_SPECIAL_VAL_FOR_INTS(unsigned long long)
   } // namespace ac

#ifdef __AC_NAMESPACE
}
#endif

#endif // __AC_INT_H
