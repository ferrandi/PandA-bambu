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
//     - bit_adjust function is called on any input and output operand of basic
//       operators. This helps the (bit) value range analysis to infer the
//       minimum number of bits for operands and results.
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

#define __FORCE_INLINE __attribute__((always_inline)) inline

#ifdef AC_TYPES_INIT
#define __INIT_VALUE = {0}
#define __INIT_VALUE_LL = {0}
#else
#define __INIT_VALUE    /*= {}*/
#define __INIT_VALUE_LL /*= {}*/
#endif

#ifndef __ASSERT_H__
#define __ASSERT_H__
#include <assert.h>
#endif
#include <limits>
#ifndef __BAMBU__
#ifndef AC_USER_DEFINED_ASSERT
#include <iostream>
#else
#include <ostream>
#endif
#endif
#include <math.h>
#include <string>

#ifndef __BAMBU__
#ifndef __AC_INT_UTILITY_BASE
#define __AC_INT_UTILITY_BASE
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
typedef unsigned long long Ulong;
typedef signed long long Slong;
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

#ifndef __BAMBU__
      __FORCE_INLINE double mgc_floor(double d)
      {
         return floor(d);
      }
      __FORCE_INLINE float mgc_floor(float d)
      {
         return floorf(d);
      }
#else
   __FORCE_INLINE double mgc_floor(double d)
   {
      return 0.0;
   }
   __FORCE_INLINE float mgc_floor(float d)
   {
      return 0.0f;
   }
#endif

#ifdef __BAMBU__
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
            std::cerr << " in file " << file << ":" << line;
         if(msg)
            std::cerr << " " << msg;
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
               nbits = X ? (X2 ? N + (int)s_N<N_div_2>::template s_X<X2>::nbits : (int)s_N<N_div_2>::template s_X<X>::nbits) : 0
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
      __FORCE_INLINE double ldexpr32(double d)
      {
         double d2 = d;
         if(N < 0)
            for(int i = 0; i < -N; i++)
               d2 /= (Ulong)1 << 32;
         else
            for(int i = 0; i < N; i++)
               d2 *= (Ulong)1 << 32;
         return d2;
      }
      template <>
      __FORCE_INLINE double ldexpr32<0>(double d)
      {
         return d;
      }
      template <>
      __FORCE_INLINE double ldexpr32<1>(double d)
      {
         return d * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE double ldexpr32<-1>(double d)
      {
         return d / ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE double ldexpr32<2>(double d)
      {
         return (d * ((Ulong)1 << 32)) * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE double ldexpr32<-2>(double d)
      {
         return (d / ((Ulong)1 << 32)) / ((Ulong)1 << 32);
      }

      template <int N>
      __FORCE_INLINE double ldexpr(double d)
      {
         return ldexpr32<N / 32>(N < 0 ? d / ((unsigned)1 << (-N & 31)) : d * ((unsigned)1 << (N & 31)));
      }

      template <int N>
      __FORCE_INLINE float ldexpr32(float d)
      {
         float d2 = d;
         if(N < 0)
            for(int i = 0; i < -N; i++)
               d2 /= (Ulong)1 << 32;
         else
            for(int i = 0; i < N; i++)
               d2 *= (Ulong)1 << 32;
         return d2;
      }
      template <>
      __FORCE_INLINE float ldexpr32<0>(float d)
      {
         return d;
      }
      template <>
      __FORCE_INLINE float ldexpr32<1>(float d)
      {
         return d * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE float ldexpr32<-1>(float d)
      {
         return d / ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE float ldexpr32<2>(float d)
      {
         return (d * ((Ulong)1 << 32)) * ((Ulong)1 << 32);
      }
      template <>
      __FORCE_INLINE float ldexpr32<-2>(float d)
      {
         return (d / ((Ulong)1 << 32)) / ((Ulong)1 << 32);
      }

      template <int N>
      __FORCE_INLINE float ldexpr(float d)
      {
         return ldexpr32<N / 32>(N < 0 ? d / ((unsigned)1 << (-N & 31)) : d * ((unsigned)1 << (N & 31)));
      }

      template <int N, bool C>
      class iv_base
      {
         //#if defined(__clang__)
         //  typedef int type __attribute__((ext_vector_type(N)));
         //#else
         //  typedef int type __attribute__((vector_size(sizeof(int)*N)));
         //#endif
         //      type v = {};
         int v[N] __INIT_VALUE;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
            constexpr const unsigned rem = (32 - W) & 31;
            set(N - 1, S ? ((v[N - 1] << rem) >> rem) : (rem ? ((unsigned)v[N - 1] << rem) >> rem : 0));
         }

         __FORCE_INLINE void assign_int64(Slong l)
         {
            set(0, static_cast<int>(l));
            if(N > 1)
            {
               set(1, static_cast<int>(l >> 32));
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 2; i < N; i++)
                  set(i, (v[1] < 0) ? ~0 : 0);
            }
         }

         __FORCE_INLINE void assign_uint64(Ulong l)
         {
            set(0, static_cast<int>(l));
            if(N > 1)
            {
               set(1, static_cast<int>(l >> 32));
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 2; i < N; i++)
                  set(i, 0);
            }
         }
         __FORCE_INLINE void set(int x, int value)
         {
            v[x] = value;
         }

         __FORCE_INLINE Slong to_int64() const
         {
            return N == 1 ? v[0] : ((Ulong)v[1] << 32) | (Ulong)(unsigned)v[0];
         }

         __FORCE_INLINE constexpr int operator[](int x) const
         {
            return v[x];
         }

         __FORCE_INLINE constexpr iv_base()
         {
         }

         template <int N2, bool C2>
         __FORCE_INLINE iv_base(const iv_base<N2, C2>& b)
         {
            const int M = AC_MIN(N, N2);
            for(auto idx = 0; idx < M; ++idx)
               set(idx, b[idx]);
            auto last = v[M - 1] < 0 ? ~0 : 0;
            for(auto idx = M; idx < N; ++idx)
               set(idx, last);
         }
      };

      template <int N>
      class iv_base<N, true>
      {
         //#if defined(__clang__)
         //  typedef int type __attribute__((ext_vector_type(N)));
         //#else
         //  typedef int type __attribute__((vector_size(sizeof(int)*N)));
         //#endif
         //      type v = {};
         int v[N - 1] __INIT_VALUE;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
         }

         __FORCE_INLINE void assign_int64(Slong l)
         {
            set(0, static_cast<int>(l));
            if(N > 2)
            {
               set(1, static_cast<int>(l >> 32));
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 2; i < N - 1; i++)
                  set(i, (v[1] < 0) ? ~0 : 0);
            }
         }

         __FORCE_INLINE void assign_uint64(Ulong l)
         {
            set(0, static_cast<int>(l));
            if(N > 2)
            {
               set(1, static_cast<int>(l >> 32));
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 2; i < N - 1; i++)
                  set(i, 0);
            }
         }

         __FORCE_INLINE void set(int x, int value)
         {
            if(x != N - 1)
               v[x] = value;
            //            else
            //               assert(value==0);
         }

         __FORCE_INLINE Slong to_int64() const
         {
            return N <= 2 ? v[0] : ((Ulong)v[1] << 32) | (Ulong)(unsigned)v[0];
         }

         __FORCE_INLINE constexpr int operator[](int x) const
         {
            if(x != N - 1)
               return v[x];
            else
               return 0;
         }

         __FORCE_INLINE constexpr iv_base()
         {
         }

         template <int N2, bool C2>
         __FORCE_INLINE iv_base(const iv_base<N2, C2>& b)
         {
            const int M = AC_MIN(N - 1, N2);
            for(auto idx = 0; idx < M; ++idx)
               set(idx, b[idx]);
            auto last = v[M - 1] < 0 ? ~0 : 0;
            for(auto idx = M; idx < N - 1; ++idx)
               set(idx, last);
         }
      };

      template <>
      class iv_base<1, false>
      {
         int v __INIT_VALUE;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
            constexpr const unsigned rem = (32 - W) & 31;
            v = S ? ((v << rem) >> rem) : (rem ? ((unsigned)v << rem) >> rem : 0);
         }
         void assign_int64(Slong l)
         {
            v = static_cast<int>(l);
         }
         void assign_uint64(Ulong l)
         {
            v = static_cast<int>(l);
         }
         void set(int, int value)
         {
            v = value;
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return v;
         }
         /*constexpr*/ int operator[](int) const
         {
            return v;
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b) : v(b[0])
         {
         }
      };

      template <>
      class iv_base<1, true>
      {
         /// not possible to instantiate this class specialization
      };

      template <>
      class iv_base<2, false>
      {
         long long int v __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         void bit_adjust()
         {
            constexpr const unsigned rem = (64 - W) & 63;
            v = S ? ((v << rem) >> rem) : (rem ? ((unsigned long long)v << rem) >> rem : 0);
         }
         void assign_int64(Slong l)
         {
            v = l;
         }
         void assign_uint64(Ulong l)
         {
            v = static_cast<Slong>(l);
         }
         void set(int x, int value)
         {
            if(x)
               v = (all_ones & v) | (((Slong)value) << 32);
            else
               v = ((((Ulong)all_ones) << 32) & v) | ((Ulong)((unsigned)value));
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return v;
         }
         /*constexpr*/ int operator[](int x) const
         {
            return x ? (int)(v >> 32) : (int)v;
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
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
      };

      template <>
      class iv_base<2, true>
      {
         int v __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         void bit_adjust()
         {
         }
         void assign_int64(Slong l)
         {
            v = l;
         }
         void assign_uint64(Ulong l)
         {
            v = static_cast<Slong>(l);
         }
         void set(int x, int value)
         {
            if(!x)
               v = value;
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return v;
         }
         /*constexpr*/ int operator[](int x) const
         {
            return x ? 0 : v;
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
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
      };

      template <>
      class iv_base<3, false>
      {
         long long int va __INIT_VALUE_LL;
         long long int v2 __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
            constexpr const unsigned rem = (64 - W) & 63;
            v2 = S ? ((v2 << rem) >> rem) : (rem ? ((unsigned long long)v2 << rem) >> rem : 0);
         }
         void assign_int64(Slong l)
         {
            va = l;
            v2 = va < 0 ? ~0LL : 0;
         }
         void assign_uint64(Ulong l)
         {
            va = static_cast<Slong>(l);
            v2 = 0;
         }
         void set(int x, int value)
         {
            x = x & 3;
            if(x == 0)
               va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
            else if(x == 1)
               va = (all_ones & va) | (((Slong)value) << 32);
            else
               v2 = value;
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return va;
         }
         /*constexpr*/ int operator[](int x) const
         {
            x = x & 3;
            return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : v2);
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
         {
            const int M = AC_MIN(2, N2);
            if(M == 3)
            {
               va = b.to_int64();
               v2 = b[2];
            }
            else if(M == 2)
            {
               va = b.to_int64();
               v2 = va < 0 ? ~0 : 0;
            }
            else
            {
               AC_ASSERT(M == 1, "unexpected condition");
               va = b.to_int64();
               v2 = va < 0 ? ~0LL : 0;
            }
         }
      };

      template <>
      class iv_base<3, true>
      {
         long long int va __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
         }
         void assign_int64(Slong l)
         {
            va = l;
         }
         void assign_uint64(Ulong l)
         {
            va = static_cast<Slong>(l);
         }
         void set(int x, int value)
         {
            x = x & 3;
            if(x == 0)
               va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
            else if(x == 1)
               va = (all_ones & va) | (((Slong)value) << 32);
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return va;
         }
         /*constexpr*/ int operator[](int x) const
         {
            x = x & 3;
            return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : 0);
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
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
      };

      template <>
      class iv_base<4, false>
      {
         long long int va __INIT_VALUE_LL;
         long long int vb __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
            constexpr const unsigned rem = (64 - W) & 63;
            vb = S ? ((vb << rem) >> rem) : (rem ? ((unsigned long long)vb << rem) >> rem : 0);
         }
         void assign_int64(Slong l)
         {
            va = l;
            vb = va < 0 ? ~0LL : 0;
         }
         void assign_uint64(Ulong l)
         {
            va = static_cast<Slong>(l);
            vb = 0;
         }
         void set(int x, int value)
         {
            x = x & 3;
            if(x == 0)
               va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
            else if(x == 1)
               va = (all_ones & va) | (((Slong)value) << 32);
            else if(x == 2)
               vb = ((((Ulong)all_ones) << 32) & vb) | ((Ulong)((unsigned)value));
            else
               vb = (all_ones & vb) | (((Slong)value) << 32);
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return va;
         }
         /*constexpr*/ int operator[](int x) const
         {
            x = x & 3;
            return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : (x == 2 ? (int)vb : (int)(vb >> 32)));
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
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
         }
      };

      template <>
      class iv_base<4, true> : public iv_base<4, false>
      {
      };

      template <>
      class iv_base<5, true>
      {
         long long int va __INIT_VALUE_LL;
         long long int vb __INIT_VALUE_LL;

       public:
         template <int W, bool S>
         __FORCE_INLINE void bit_adjust()
         {
         }
         void assign_int64(Slong l)
         {
            va = l;
            vb = va < 0 ? ~0LL : 0;
         }
         void assign_uint64(Ulong l)
         {
            va = static_cast<Slong>(l);
            vb = 0;
         }
         void set(int x, int value)
         {
            x = x & 7;
            if(x == 0)
               va = ((((Ulong)all_ones) << 32) & va) | ((Ulong)((unsigned)value));
            else if(x == 1)
               va = (all_ones & va) | (((Slong)value) << 32);
            else if(x == 2)
               vb = ((((Ulong)all_ones) << 32) & vb) | ((Ulong)((unsigned)value));
            else if(x == 3)
               vb = (all_ones & vb) | (((Slong)value) << 32);
         }
         __FORCE_INLINE Slong to_int64() const
         {
            return va;
         }
         /*constexpr*/ int operator[](int x) const
         {
            x = x & 3;
            return x == 0 ? (int)va : (x == 1 ? (int)(va >> 32) : (x == 2 ? (int)vb : (x == 2 ? (int)(vb >> 32) : 0)));
         }
         /*constexpr*/ iv_base()
         {
         }
         template <int N2, bool C2>
         iv_base(const iv_base<N2, C2>& b)
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
      };

      template <int N, int START, int N1, bool C1, int Nr, bool Cr>
      __FORCE_INLINE void iv_copy(const iv_base<N1, C1>& op, iv_base<Nr, Cr>& r)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START; i < N; i++)
            r.set(i, op[i]);
      }

      template <int START, int N, int N1, bool C1>
      __FORCE_INLINE bool iv_equal_zero(const iv_base<N1, C1>& op)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START; i < N; i++)
            if(op[i])
               return false;
         return true;
      }

      template <int START, int N, int N1, bool C1>
      __FORCE_INLINE bool iv_equal_ones(const iv_base<N1, C1>& op)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START; i < N; i++)
            if(~op[i])
               return false;
         return true;
      }

      template <int N1, bool C1, int N2, bool C2>
      __FORCE_INLINE bool iv_equal(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2)
      {
         const int M1 = AC_MAX(N1, N2);
         const int M2 = AC_MIN(N1, N2);
         const bool M1C1 = N1 >= N2 ? C1 : C2;
         const iv_base<M1, M1C1>& OP1 = N1 >= N2 ? static_cast<iv_base<M1, M1C1>>(op1) : static_cast<iv_base<M1, M1C1>>(op2);
         const bool M2C1 = N1 >= N2 ? C2 : C1;
         const iv_base<M2, M2C1>& OP2 = N1 >= N2 ? static_cast<iv_base<M2, M2C1>>(op2) : static_cast<iv_base<M2, M2C1>>(op1);
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < M2; i++)
            if(OP1[i] != OP2[i])
               return false;
         int ext = OP2[M2 - 1] < 0 ? ~0 : 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = M2; i < M1; i++)
            if(OP1[i] != ext)
               return false;
         return true;
      }

      template <int B, int N, bool C>
      __FORCE_INLINE bool iv_equal_ones_from(const iv_base<N, C>& op)
      {
         if((B >= 32 * N && op[N - 1] >= 0) || (B & 31 && ~(op[B / 32] >> (B & 31))))
            return false;
         return iv_equal_ones<(B + 31) / 32, N>(op);
      }

      template <int B, int N, bool C>
      __FORCE_INLINE bool iv_equal_zeros_from(const iv_base<N, C>& op)
      {
         if((B >= 32 * N && op[N - 1] < 0) || (B & 31 && (op[B / 32] >> (B & 31))))
            return false;
         return iv_equal_zero<(B + 31) / 32, N>(op);
      }

      template <int B, int N, bool C>
      __FORCE_INLINE bool iv_equal_ones_to(const iv_base<N, C>& op)
      {
         if((B >= 32 * N && op[N - 1] >= 0) || (B & 31 && ~(op[B / 32] | (all_ones << (B & 31)))))
            return false;
         return iv_equal_ones<0, B / 32>(op);
      }

      template <int B, int N, bool C>
      __FORCE_INLINE bool iv_equal_zeros_to(const iv_base<N, C>& op)
      {
         if((B >= 32 * N && op[N - 1] < 0) || (B & 31 && (op[B / 32] & ~(all_ones << (B & 31)))))
            return false;
         if(B < 32)
            return true;
         return iv_equal_zero<0, (B / 32)>(op);
      }

      template <bool greater, int N1, bool C1, int N2, bool C2>
      __FORCE_INLINE bool iv_compare(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2)
      {
         const int M1 = AC_MAX(N1, N2);
         const int M2 = AC_MIN(N1, N2);
         const bool M1C1 = N1 >= N2 ? C1 : C2;
         const iv_base<M1, M1C1>& OP1 = N1 >= N2 ? static_cast<iv_base<M1, M1C1>>(op1) : static_cast<iv_base<M1, M1C1>>(op2);
         const bool M2C1 = N1 >= N2 ? C2 : C1;
         const iv_base<M2, M2C1>& OP2 = N1 >= N2 ? static_cast<iv_base<M2, M2C1>>(op2) : static_cast<iv_base<M2, M2C1>>(op1);
         const bool b = (N1 >= N2) == greater;
         int ext = OP2[M2 - 1] < 0 ? ~0 : 0;
         int i2 = M1 > M2 ? ext : OP2[M1 - 1];
         if(OP1[M1 - 1] != i2)
            return b ^ (OP1[M1 - 1] < i2);
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = M1 - 2; i >= M2; i--)
         {
            if((unsigned)OP1[i] != (unsigned)ext)
               return b ^ ((unsigned)OP1[i] < (unsigned)ext);
         }
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = M2 - 1; i >= 0; i--)
         {
            if((unsigned)OP1[i] != (unsigned)OP2[i])
               return b ^ ((unsigned)OP1[i] < (unsigned)OP2[i]);
         }
         return false;
      }

      template <int START, int N, bool C>
      __FORCE_INLINE void iv_extend(iv_base<N, C>& r, int ext)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START; i < N; i++)
            r.set(i, ext);
      }

      template <int Nr, bool Cr>
      __FORCE_INLINE void iv_assign_int64(iv_base<Nr, Cr>& r, Slong l)
      {
         r.assign_int64(l);
      }

      template <int Nr, bool Cr>
      __FORCE_INLINE void iv_assign_uint64(iv_base<Nr, Cr>& r, Ulong l)
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

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_mult(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         if(Nr == 1)
            r.set(0, op1[0] * op2[0]);
         else if(Nr == 2)
            iv_assign_int64(r, (op1.to_int64() * op2.to_int64()));
         else if(N1 == 1 && N2 == 1)
            iv_assign_int64(r, (op1.to_int64() * op2.to_int64()));
         else
         {
            const int M1 = AC_MAX(N1, N2);
            const int M2 = AC_MIN(N1, N2);
            const bool M1C1 = N1 >= N2 ? C1 : C2;
            const iv_base<M1, M1C1> OP1 = N1 >= N2 ? static_cast<iv_base<M1, M1C1>>(op1) : static_cast<iv_base<M1, M1C1>>(op2);
            const bool M2C1 = N1 >= N2 ? C2 : C1;
            const iv_base<M2, M2C1> OP2 = N1 >= N2 ? static_cast<iv_base<M2, M2C1>>(op2) : static_cast<iv_base<M2, M2C1>>(op1);
            const int T1 = AC_MIN(M2 - 1, Nr);
            const int T2 = AC_MIN(M1 - 1, Nr);
            const int T3 = AC_MIN(M1 + M2 - 2, Nr);

            Ulong l1 = 0;
            Slong l2 = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int k = 0; k < T1; k++)
            {
               for(int i = 0; i < k + 1; i++)
                  accumulate(mult_u_u(OP1[k - i], OP2[i]), l1, l2);
               l2 += (Ulong)(unsigned)(l1 >> 32);
               r.set(k, (int)l1);
               l1 = (unsigned)l2;
               l2 >>= 32;
            }
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int k = T1; k < T2; k++)
            {
               accumulate(mult_u_s(OP1[k - M2 + 1], OP2[M2 - 1]), l1, l2);
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 0; i < M2 - 1; i++)
                  accumulate(mult_u_u(OP1[k - i], OP2[i]), l1, l2);
               l2 += (Ulong)(unsigned)(l1 >> 32);
               r.set(k, (int)l1);
               l1 = (unsigned)l2;
               l2 >>= 32;
            }
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int k = T2; k < T3; k++)
            {
               accumulate(mult_u_s(OP1[k - M2 + 1], OP2[M2 - 1]), l1, l2);
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = k - T2 + 1; i < M2 - 1; i++)
                  accumulate(mult_u_u(OP1[k - i], OP2[i]), l1, l2);
               accumulate(mult_s_u(OP1[M1 - 1], OP2[k - M1 + 1]), l1, l2);
               l2 += (Ulong)(unsigned)(l1 >> 32);
               r.set(k, (int)l1);
               l1 = (unsigned)l2;
               l2 >>= 32;
            }
            if(Nr >= M1 + M2 - 1)
            {
               accumulate(mult_s_s(OP1[M1 - 1], OP2[M2 - 1]), l1, l2);
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

      template <int N, bool C>
      __FORCE_INLINE bool iv_uadd_carry(const iv_base<N, C>& op1, bool carry, iv_base<N, C>& r)
      {
         Slong l = carry;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
         {
            l += (Ulong)(unsigned)op1[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         return l != 0;
      }

      template <int START, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE bool iv_add_int_carry(const iv_base<N, C>& op1, int op2, bool carry, iv_base<Nr, Cr>& r)
      {
         if(N == START)
            return carry;
         if(N == START + 1)
         {
            Ulong l = carry + (Slong)op1[START] + (Slong)op2;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = carry + (Ulong)(unsigned)op1[START] + (Slong)op2;
         r.set(START, (int)l);
         l >>= 32;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START + 1; i < N - 1; i++)
         {
            l += (Ulong)(unsigned)op1[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         l += (Slong)op1[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE bool iv_uadd_n(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         AC_ASSERT(AC_MIN(N, AC_MIN(N1, AC_MIN(N2, Nr))) == N, "unexpected condition");
         Ulong l = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
         {
            l += (Ulong)(unsigned)op1[i] + (Ulong)(unsigned)op2[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         return l & 1;
      }

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_add(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         if(Nr == 1)
            r.set(0, op1[0] + op2[0]);
         else
         {
            const int M1 = AC_MAX(N1, N2);
            const int M2 = AC_MIN(N1, N2);
            const bool M1C1 = N1 >= N2 ? C1 : C2;
            const iv_base<M1, M1C1> OP1 = N1 >= N2 ? static_cast<iv_base<M1, M1C1>>(op1) : static_cast<iv_base<M1, M1C1>>(op2);
            const bool M2C1 = N1 >= N2 ? C2 : C1;
            const iv_base<M2, M2C1> OP2 = N1 >= N2 ? static_cast<iv_base<M2, M2C1>>(op2) : static_cast<iv_base<M2, M2C1>>(op1);
            const int T1 = AC_MIN(M2 - 1, Nr);
            const int T2 = AC_MIN(M1, Nr);

            bool carry = iv_uadd_n<T1>(OP1, OP2, r);
            carry = iv_add_int_carry<T1>(OP1, OP2[T1], carry, r);
            iv_extend<T2>(r, carry ? ~0 : 0);
         }
      }

      template <int N, int START, int N1, bool C1, int Nr, bool Cr>
      __FORCE_INLINE bool iv_sub_int_borrow(const iv_base<N1, C1>& op1, int op2, bool borrow, iv_base<Nr, Cr>& r)
      {
         if(START == N)
            return borrow;
         if(N == (START + 1))
         {
            Ulong l = (Slong)op1[START] - (Slong)op2 - borrow;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = (Ulong)(unsigned)op1[START] - (Slong)op2 - borrow;
         r.set(START, (int)l);
         l >>= 32;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START + 1; i < N - 1; i++)
         {
            l += (Ulong)(unsigned)op1[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         l += (Slong)op1[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int START, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE bool iv_sub_int_borrow(int op1, const iv_base<N2, C2>& op2, bool borrow, iv_base<Nr, Cr>& r)
      {
         if(START == N)
            return borrow;
         if(N == START + 1)
         {
            Ulong l = (Slong)op1 - (Slong)op2[START] - borrow;
            r.set(START, (int)l);
            return (l >> 32) & 1;
         }
         Slong l = (Slong)op1 - (Ulong)(unsigned)op2[START] - borrow;
         r.set(START, (int)l);
         l >>= 32;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START + 1; i < N - 1; i++)
         {
            l -= (Ulong)(unsigned)op2[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         l -= (Slong)op2[N - 1];
         r.set(N - 1, (int)l);
         return (l >> 32) & 1;
      }

      template <int N, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE bool iv_usub_n(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         AC_ASSERT(AC_MIN(N, AC_MIN(N1, AC_MIN(N2, Nr))) == N, "unexpected condition");
         Slong l = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
         {
            l += (Ulong)(unsigned)op1[i] - (Ulong)(unsigned)op2[i];
            r.set(i, (int)l);
            l >>= 32;
         }
         return l & 1;
      }

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_sub(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         if(Nr == 1)
            r.set(0, op1[0] - op2[0]);
         else
         {
            const int M1 = AC_MAX(N1, N2);
            const int M2 = AC_MIN(N1, N2);
            const int T1 = AC_MIN(M2 - 1, Nr);
            const int T2 = AC_MIN(M1, Nr);

            bool borrow = iv_usub_n<T1>(op1, op2, r);
            if(N1 > N2)
               borrow = iv_sub_int_borrow<T2, T1>(op1, op2[T1], borrow, r);
            else
               borrow = iv_sub_int_borrow<T2, T1>(op1[T1], op2, borrow, r);
            iv_extend<T2>(r, borrow ? ~0 : 0);
         }
      }

      template <int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_neg(const iv_base<N, C>& op1, iv_base<Nr, Cr>& r)
      {
         Slong l = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int k = 0; k < AC_MIN(N, Nr); k++)
         {
            l -= (Ulong)(unsigned)op1[k];
            r.set(k, (unsigned)l);
            l >>= 32;
         }
         if(Nr > N)
         {
            r.set(N, (unsigned)(l - (op1[N - 1] < 0 ? ~0 : 0)));
            iv_extend<N + 1>(r, r[N] < 0 ? ~0 : 0);
         }
      }

      template <bool S, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_abs(const iv_base<N, C>& op1, iv_base<Nr, Cr>& r)
      {
         if(S && op1[N - 1] < 0)
         {
            iv_neg(op1, r);
         }
         else
         {
            iv_copy<AC_MIN(N, Nr), 0>(op1, r);
            iv_extend<N>(r, 0);
         }
      }

      template <int N, int D, int Q, int R, typename uw2, typename sw4, typename uw4, int w1_length, int Nn, bool Cn, int Nd, bool Cd, int Nq, bool Cq, int Nr, bool Cr>
      __FORCE_INLINE void iv_udiv(const iv_base<Nn, Cn>& n, const iv_base<Nd, Cd>& d, iv_base<Nq, Cq>& q, iv_base<Nr, Cr>& r)
      {
         const int w2_length = 2 * w1_length;
         int d_msi; // most significant int for d
         for(d_msi = D - 1; d_msi > 0 && !d[d_msi]; d_msi--)
         {
         }
         uw4 d1 = 0;
         if(!d_msi && !d[0])
         {
            d1 = n[0] / d[0]; // d is zero => divide by zero
            return;
         }
         int n_msi; // most significant int for n
         for(n_msi = N - 1; n_msi > 0 && !n[n_msi]; n_msi--)
         {
         }
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < Q; i++)
            q.set(i, 0);
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < R; i++)
            r.set(i, n[i]);
         // write most significant "words" into d1
         bool d_mss_odd = (bool)(d[d_msi] >> w1_length);
         int d_mss = 2 * d_msi + d_mss_odd; // index to most significant short (16-bit)
         d1 = (uw4)(uw2)d[d_msi] << (w1_length << (int)!d_mss_odd);
         if(d_msi)
            d1 |= (uw2)d[d_msi - 1] >> (d_mss_odd ? w1_length : 0);
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
            for(int k = n_msi; k >= 0; k--)
               r1[k] = n[k];
            for(int k = n_mss; k >= d_mss; k--)
            {
               //#if defined(__clang__)
               //#pragma clang loop unroll(full)
               //#endif
               //    for (int k = N; k >= 0; k--)
               //      if(k<=n_msi) r1[k] = n[k];
               //#if defined(__clang__)
               //#pragma clang loop unroll(full)
               //#endif
               //    for (int k = 2*N-1; k >= 0; k--)
               //    if(k<=n_mss&&k>=d_mss){
               int k_msi = k >> 1;
               bool odd = k & 1;
               uw2 r1m1 = k_msi > 0 ? r1[k_msi - 1] : (uw2)0;
               uw4 n1 = odd ? (uw4)((r1[k_msi + 1] << w1_length) | (r1[k_msi] >> w1_length)) << w2_length | ((r1[k_msi] << w1_length) | (r1m1 >> w1_length)) : (uw4)r1[k_msi] << w2_length | r1m1;
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
            if(R)
            {
               int r_msi = AC_MIN(R - 1, n_msi);
               for(int j = 0; j <= r_msi; j++)
                  r.set(j, r1[j]);
               for(int j = r_msi + 1; j < R; j++)
                  r.set(j, 0);
            }
         }
      }

      template <int Num_s, int Den_s, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_div(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
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
            iv_assign_int64(r, op1.to_int64() / op2.to_int64());
         else if(!Num_s && !Den_s)
         {
            iv_base<1, false> dummy;
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
            iv_base<N1_neg, false> numerator;
            iv_base<N2_neg, false> denominator;
            iv_base<N1_neg, false> quotient;
            iv_abs<(bool)Num_s>(op1, numerator);
            iv_abs<(bool)Den_s>(op2, denominator);
            iv_base<1, false> dummy;
            iv_udiv<N1_neg, N2_neg, N1_neg, 0, unsigned, Slong, Ulong, 16>(numerator, denominator, quotient, dummy);
            if((Num_s && op1[N1 - 1] < 0) ^ (Den_s && op2[N2 - 1] < 0))
               iv_neg(quotient, r);
            else
            {
               iv_copy<AC_MIN(N1_neg, Nr), 0>(quotient, r);
               iv_extend<N1_neg>(r, (Num_s || Den_s) && r[N1_neg - 1] < 0 ? ~0 : 0);
            }
         }
      }

      template <int Num_s, int Den_s, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_rem(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
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
            iv_assign_int64(r, op1.to_int64() % op2.to_int64());
         else if(!Num_s && !Den_s)
         {
            iv_base<1, false> dummy;
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
            iv_base<N1_neg, false> numerator;
            iv_base<N2_neg, false> denominator;
            iv_base<N2, false> remainder;
            iv_abs<(bool)Num_s>(op1, numerator);
            iv_abs<(bool)Den_s>(op2, denominator);
            iv_base<1, false> dummy;
            iv_udiv<N1_neg, N2_neg, 0, N2, unsigned, Slong, Ulong, 16>(numerator, denominator, dummy, remainder);
            if((Num_s && op1[N1 - 1] < 0))
               iv_neg(remainder, r);
            else
            {
               iv_copy<AC_MIN(N2, Nr), 0>(remainder, r);
               iv_extend<N2>(r, Num_s && r[N2 - 1] < 0 ? ~0 : 0);
            }
         }
      }

      template <int N, int START, int N1, bool C1, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_complement_n(const iv_base<N1, C1>& op, iv_base<Nr, Cr>& r)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = START; i < N; i++)
            r.set(i, ~op[i]);
      }

      template <int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_complement(const iv_base<N, C>& op, iv_base<Nr, Cr>& r)
      {
         const int M = AC_MIN(N, Nr);
         iv_bitwise_complement_n<M, 0>(op, r);
         iv_extend<M>(r, (r[M - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_and_n(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
            r.set(i, op1[i] & op2[i]);
      }

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_and(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         const int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         const int M2 = AC_MIN(AC_MIN(N1, N2), Nr);
         const bool M1C1 = N1 > N2 ? C1 : C2;
         const iv_base<AC_MAX(N1, N2), M1C1>& OP1 = N1 > N2 ? static_cast<iv_base<AC_MAX(N1, N2), M1C1>>(op1) : static_cast<iv_base<AC_MAX(N1, N2), M1C1>>(op2);
         const bool M2C1 = N1 > N2 ? C2 : C1;
         const iv_base<AC_MIN(N1, N2), M2C1>& OP2 = N1 > N2 ? static_cast<iv_base<AC_MIN(N1, N2), M2C1>>(op2) : static_cast<iv_base<AC_MIN(N1, N2), M2C1>>(op1);

         iv_bitwise_and_n<M2>(op1, op2, r);
         if(OP2[M2 - 1] < 0)
            iv_copy<M1, M2>(OP1, r);
         else
            iv_extend<M2>(r, 0);
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_or_n(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
            r.set(i, op1[i] | op2[i]);
      }

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_or(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         const int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         const int M2 = AC_MIN(AC_MIN(N1, N2), Nr);
         const bool M1C1 = N1 >= N2 ? C1 : C2;
         const iv_base<M1, M1C1>& OP1 = N1 >= N2 ? static_cast<iv_base<M1, M1C1>>(op1) : static_cast<iv_base<M1, M1C1>>(op2);
         const bool M2C1 = N1 >= N2 ? C2 : C1;
         const iv_base<M2, M2C1>& OP2 = N1 >= N2 ? static_cast<iv_base<M2, M2C1>>(op2) : static_cast<iv_base<M2, M2C1>>(op1);

         iv_bitwise_or_n<M2>(op1, op2, r);
         if(OP2[M2 - 1] < 0)
            iv_extend<M2>(r, ~0);
         else
            iv_copy<M1, M2>(OP1, r);
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_xor_n(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < N; i++)
            r.set(i, op1[i] ^ op2[i]);
      }

      template <int N1, bool C1, int N2, bool C2, int Nr, bool Cr>
      __FORCE_INLINE void iv_bitwise_xor(const iv_base<N1, C1>& op1, const iv_base<N2, C2>& op2, iv_base<Nr, Cr>& r)
      {
         const int M1 = AC_MIN(AC_MAX(N1, N2), Nr);
         const int M2 = AC_MIN(AC_MIN(N1, N2), Nr);
         const bool M1C1 = N1 >= N2 ? C1 : C2;
         const iv_base<AC_MAX(N1, N2), M1C1>& OP1 = N1 >= N2 ? static_cast<iv_base<AC_MAX(N1, N2), M1C1>>(op1) : static_cast<iv_base<AC_MAX(N1, N2), M1C1>>(op2);
         const bool M2C1 = N1 >= N2 ? C2 : C1;
         const iv_base<AC_MIN(N1, N2), M2C1>& OP2 = N1 >= N2 ? static_cast<iv_base<AC_MIN(N1, N2), M2C1>>(op2) : static_cast<iv_base<AC_MIN(N1, N2), M2C1>>(op1);

         iv_bitwise_xor_n<M2>(op1, op2, r);
         if(OP2[M2 - 1] < 0)
            iv_bitwise_complement_n<M1, M2>(OP1, r);
         else
            iv_copy<M1, M2>(OP1, r);
         iv_extend<M1>(r, (r[M1 - 1] < 0) ? ~0 : 0);
      }

      template <int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_shift_l(const iv_base<N, C>& op1, unsigned op2, iv_base<Nr, Cr>& r)
      {
         AC_ASSERT(Nr <= N, "iv_shift_l, incorrect usage Nr > N");
         unsigned s31 = op2 & 31;
         unsigned ishift = (op2 >> 5) > Nr ? Nr : (op2 >> 5);
         if(s31 && ishift != Nr)
         {
            unsigned lw = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(unsigned i = 0; i < Nr; i++)
            {
               unsigned hw = (i >= ishift) ? op1[i - ishift] : 0;
               r.set(i, (hw << s31) | (lw >> (32 - s31)));
               lw = hw;
            }
         }
         else
         {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(unsigned i = 0; i < Nr; i++)
               r.set(i, (i >= ishift) ? op1[i - ishift] : 0);
         }
      }

      template <int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_shift_r(const iv_base<N, C>& op1, unsigned op2, iv_base<Nr, Cr>& r)
      {
         unsigned s31 = op2 & 31;
         unsigned ishift = (op2 >> 5) > N ? N : (op2 >> 5);
         int ext = op1[N - 1] < 0 ? ~0 : 0;
         if(s31 && ishift != N)
         {
            unsigned lw = (ishift < N) ? op1[ishift] : ext;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(unsigned i = 0; i < Nr; i++)
            {
               unsigned hw = (i + ishift + 1 < N) ? op1[i + ishift + 1] : ext;
               r.set(i, (lw >> s31) | (hw << (32 - s31)));
               lw = hw;
            }
         }
         else
         {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(unsigned i = 0; i < Nr; i++)
               r.set(i, (i + ishift < N) ? op1[i + ishift] : ext);
         }
      }

      template <bool S, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_shift_l2(const iv_base<N, C>& op1, signed op2, iv_base<Nr, Cr>& r)
      {
         if(S && op2 < 0)
            iv_shift_r(op1, -op2, r);
         else
            iv_shift_l(op1, op2, r);
      }

      template <bool S, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_shift_r2(const iv_base<N, C>& op1, signed op2, iv_base<Nr, Cr>& r)
      {
         if(S && op2 < 0)
            iv_shift_l(op1, -op2, r);
         else
            iv_shift_r(op1, op2, r);
      }

      template <int B, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_const_shift_l(const iv_base<N, C>& op1, iv_base<Nr, Cr>& r)
      {
         // B >= 0
         if(!B)
         {
            const int M1 = AC_MIN(N, Nr);
            iv_copy<M1, 0>(op1, r);
            iv_extend<M1>(r, r[M1 - 1] < 0 ? -1 : 0);
         }
         else
         {
            const unsigned s31 = B & 31;
            const int ishift = (((B >> 5) > Nr) ? Nr : (B >> 5));
            for(auto idx = 0; idx < ishift; ++idx)
               r.set(idx, 0);
            const int M1 = AC_MIN(N + ishift, Nr);
            if(s31)
            {
               unsigned lw = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = ishift; i < M1; i++)
               {
                  unsigned hw = op1[i - ishift];
                  r.set(i, (hw << s31) | (lw >> ((32 - s31) & 31))); // &31 is to quiet compilers
                  lw = hw;
               }
               if(Nr > M1)
               {
                  r.set(M1, (signed)lw >> ((32 - s31) & 31)); // &31 is to quiet compilers
                  iv_extend<M1 + 1>(r, r[M1] < 0 ? ~0 : 0);
               }
            }
            else
            {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = ishift; i < M1; i++)
                  r.set(i, op1[i - ishift]);
               iv_extend<M1>(r, r[M1 - 1] < 0 ? -1 : 0);
            }
         }
      }

      template <int B, int N, bool C, int Nr, bool Cr>
      __FORCE_INLINE void iv_const_shift_r(const iv_base<N, C>& op1, iv_base<Nr, Cr>& r)
      {
         if(!B)
         {
            const int M1 = AC_MIN(N, Nr);
            iv_copy<M1, 0>(op1, r);
            iv_extend<M1>(r, r[M1 - 1] < 0 ? ~0 : 0);
         }
         else
         {
            const unsigned s31 = B & 31;
            const int ishift = (((B >> 5) > N) ? N : (B >> 5));
            int ext = op1[N - 1] < 0 ? ~0 : 0;
            if(s31 && ishift != N)
            {
               unsigned lw = (ishift < N) ? op1[ishift] : ext;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 0; i < Nr; i++)
               {
                  unsigned hw = (i + ishift + 1 < N) ? op1[i + ishift + 1] : ext;
                  r.set(i, (lw >> s31) | (hw << ((32 - s31) & 31))); // &31 is to quiet compilers
                  lw = hw;
               }
            }
            else
            {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 0; i < Nr; i++)
                  r.set(i, (i + ishift < N) ? op1[i + ishift] : ext);
            }
         }
      }

      template <int N, bool C>
      __FORCE_INLINE void iv_conv_from_fraction(const double d, iv_base<N, C>& r, bool* qb, bool* rbits, bool* o)
      {
         bool b = d < 0;
         double d2 = b ? -d : d;
         double dfloor = mgc_floor(d2);
         *o = dfloor != 0.0;
         d2 = d2 - dfloor;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = N - 1; i >= 0; i--)
         {
            d2 *= (Ulong)1 << 32;
            unsigned k = (unsigned int)d2;
            r.set(i, b ? ~k : k);
            d2 -= k;
         }
         d2 *= 2;
         bool k = ((int)d2) != 0; // is 0 or 1
         d2 -= k;
         *rbits = d2 != 0.0;
         *qb = (b && *rbits) ^ k;
         if(b && !*rbits && !*qb)
            iv_uadd_carry(r, true, r);
         *o |= b ^ (r[N - 1] < 0);
      }

      template <int N, bool C>
      __FORCE_INLINE void iv_conv_from_fraction(const float d, iv_base<N, C>& r, bool* qb, bool* rbits, bool* o)
      {
         bool b = d < 0;
         float d2 = b ? -d : d;
         float dfloor = mgc_floor(d2);
         *o = dfloor != 0.0;
         d2 = d2 - dfloor;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = N - 1; i >= 0; i--)
         {
            d2 *= (Ulong)1 << 32;
            unsigned k = (unsigned int)d2;
            r.set(i, b ? ~k : k);
            d2 -= k;
         }
         d2 *= 2;
         bool k = ((int)d2) != 0; // is 0 or 1
         d2 -= k;
         *rbits = d2 != 0.0;
         *qb = (b && *rbits) ^ k;
         if(b && !*rbits && !*qb)
            iv_uadd_carry(r, true, r);
         *o |= b ^ (r[N - 1] < 0);
      }

      template <ac_base_mode b, int N, bool C>
      struct to_strImpl
      {
         static __FORCE_INLINE int to_str(iv_base<N, C>& v, int w, bool left_just, char* r)
         {
            const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            const unsigned char B = b == AC_BIN ? 1 : (b == AC_OCT ? 3 : (b == AC_HEX ? 4 : 0));
            int k = (w + B - 1) / B;
            int n = (w + 31) >> 5;
            int bits = 0;
            if(b != AC_BIN && left_just)
            {
               if((bits = -(w % B)))
                  r[--k] = 0;
            }
            for(int i = 0; i < n; i++)
            {
               if(b != AC_BIN && bits < 0)
                  r[k] += (unsigned char)((v[i] << (B + bits)) & (b - 1));
               unsigned int m = (unsigned)v[i] >> -bits;
               for(bits += 32; bits > 0 && k; bits -= B)
               {
                  r[--k] = (char)(m & (b - 1));
                  m >>= B;
               }
            }
            for(int i = 0; i < (w + B - 1) / B; i++)
               r[i] = digits[(int)r[i]];
            return (w + B - 1) / B;
         }
      };
      template <int N, bool C>
      struct to_strImpl<AC_DEC, N, C>
      {
         static __FORCE_INLINE int to_str(iv_base<N, C>& v, int w, bool left_just, char* r)
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
                     v.set(i, v[i] << left_shift | (unsigned)v[i - 1] >> bits_msw);
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
                        lsw++;
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
                     msw--;
                  bool last = msw == -1;
                  unsigned rem = (unsigned)nl;
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
      template <int N, bool C>
      __FORCE_INLINE int to_string(iv_base<N, C>& v, int w, bool sign_mag, ac_base_mode base, bool left_just, char* r)
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
                     w2 -= i;
                  else
                     m >>= i;
               }
            }
            if(w2 < w)
               w = w2;
            w += !sign_mag;
         }
         if(base == AC_DEC)
            return to_strImpl<AC_DEC, N, C>::to_str(v, w, left_just, r);
         else if(base == AC_HEX)
            return to_strImpl<AC_HEX, N, C>::to_str(v, w, left_just, r);
         else if(base == AC_OCT)
            return to_strImpl<AC_OCT, N, C>::to_str(v, w, left_just, r);
         else if(base == AC_BIN)
            return to_strImpl<AC_BIN, N, C>::to_str(v, w, left_just, r);
         return 0;
      }

      template <int N, bool C>
      __FORCE_INLINE unsigned iv_leading_bits_base(const iv_base<N, C>& op, bool bit, int POS)
      {
         const unsigned char tab[] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
         unsigned t = bit ? ~op[POS] : op[POS];
         unsigned cnt = 0;
         if(t >> 16)
            t >>= 16;
         else
            cnt += 16;
         if(t >> 8)
            t >>= 8;
         else
            cnt += 8;
         if(t >> 4)
            t >>= 4;
         else
            cnt += 4;
         cnt += tab[t];
         return cnt;
      }

      template <int N, bool C>
      __FORCE_INLINE unsigned iv_leading_bits(const iv_base<N, C>& op, bool bit)
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
      template <int N, bool C>
      class iv
      {
       protected:
         class hex2doubleConverter
         {
            using size_t = decltype(sizeof(0));                     // avoid including extra headers
            static constexpr const long double _0x1p256 = 0x1p256L; // 2^256
            typedef union {
               double dvalue;
               unsigned long long int ull_value;
            } ieee_double_shape_type;

            template <size_t begin, char charToFind, size_t NN>
            struct HfindCharImpl
            {
               static constexpr size_t HfindChar(const char (&str)[NN])
               {
                  return (str[begin] == charToFind) * (begin + 1) + HfindCharImpl<begin - 1, charToFind, NN>::HfindChar(str);
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
               return '0' <= c && c <= '9' ? c - '0' : 'a' <= c && c <= 'f' ? c - 'a' + 0xa : 'A' <= c && c <= 'F' ? c - 'A' + 0xA : 0;
            }

            static constexpr double Hscalbn(const double value, const int exponent)
            {
               ieee_double_shape_type in;
               in.dvalue = value;
               if(in.ull_value << 1 == 0)
                  return value;
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
               return mantissa_end == NN ? 0 : (str[mantissa_end + 1] == '-' ? -1 : 1) * getNumberImpl<NN - 1, 10, int, NN>::getNumber(str, mantissa_end + 1 + HisSign(mantissa_end + 1), NN - 1);
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
            __FORCE_INLINE static constexpr unsigned long long HbeforePoint(const char (&str)[NN], const size_t point_pos)
            {
               return getNumberImpl<NN - 1, 16, unsigned long long, NN>::getNumber(str, HmantissaBegin(str), point_pos);
            }

            template <size_t NN>
            __FORCE_INLINE static constexpr unsigned long long Hfraction(const char (&str)[NN], const size_t point_pos, const size_t mantissa_end)
            {
               return getNumberImpl<NN - 1, 16, unsigned long long, NN>::getNumber(str, point_pos + 1, mantissa_end);
            }

            template <size_t NN>
            __FORCE_INLINE static constexpr double get0(const char (&str)[NN], const size_t mantissa_end, const size_t point_pos, const size_t exp)
            {
               //            printf("%d\n", mantissa_end);
               //            printf("%d\n", point_pos);
               //            printf("%d\n", exp);
               return (str[0] == '-' ? -1 : 1) * (Hscalbn(HbeforePoint(str, point_pos), exp) + Hscalbn(Hfraction(str, point_pos, mantissa_end), exp - 4 * (mantissa_end - point_pos - 1)));
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
         iv_base<N, C> v;

       public:
         template <int N2, bool C2>
         friend class iv;
         __FORCE_INLINE
         constexpr iv()
         {
         }

         template <int N2, bool C2>
         __FORCE_INLINE constexpr iv(const iv<N2, C2>& b)
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
               iv_assign_int64(v, t);
         }

         __FORCE_INLINE constexpr iv(unsigned long t)
         {
            if(long_w == 32)
            {
               v.set(0, t);
               iv_extend<1>(v, 0);
            }
            else
               iv_assign_uint64(v, t);
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
         __FORCE_INLINE Slong to_int64() const
         {
            return v.to_int64();
         }
         __FORCE_INLINE Ulong to_uint64() const
         {
            return (Ulong)v.to_int64();
         }
         __FORCE_INLINE double to_double() const
         {
            double a = v[N - 1];
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int i = N - 2; i >= 0; i--)
            {
               a *= (Ulong)1 << 32;
               a += (unsigned)v[i];
            }
            return a;
         }
         __FORCE_INLINE float to_float() const
         {
            float a = v[N - 1];
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int i = N - 2; i >= 0; i--)
            {
               a *= (Ulong)1 << 32;
               a += (unsigned)v[i];
            }
            return a;
         }
         __FORCE_INLINE void conv_from_fraction(double d, bool* qb, bool* rbits, bool* o)
         {
            iv_conv_from_fraction(d, v, qb, rbits, o);
         }
         __FORCE_INLINE void conv_from_fraction(float d, bool* qb, bool* rbits, bool* o)
         {
            iv_conv_from_fraction(d, v, qb, rbits, o);
         }

         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void mult(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_mult(v, op2.v, r.v);
         }
         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void add(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_add(v, op2.v, r.v);
         }
         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void sub(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_sub(v, op2.v, r.v);
         }
         template <int Num_s, int Den_s, int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void div(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_div<Num_s, Den_s>(v, op2.v, r.v);
         }
         template <int Num_s, int Den_s, int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void rem(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
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
         template <int Nr, bool Cr>
         __FORCE_INLINE void neg(iv<Nr, Cr>& r) const
         {
            iv_neg(v, r.v);
         }
         template <int Nr, bool Cr>
         __FORCE_INLINE void shift_l(unsigned op2, iv<Nr, Cr>& r) const
         {
            iv_shift_l(v, op2, r.v);
         }
         template <int Nr, bool Cr>
         __FORCE_INLINE void shift_l2(signed op2, iv<Nr, Cr>& r) const
         {
            iv_shift_l2<true>(v, op2, r.v);
         }
         template <int Nr, bool Cr>
         __FORCE_INLINE void shift_r(unsigned op2, iv<Nr, Cr>& r) const
         {
            iv_shift_r(v, op2, r.v);
         }
         template <int Nr, bool Cr>
         __FORCE_INLINE void shift_r2(signed op2, iv<Nr, Cr>& r) const
         {
            iv_shift_r2<true>(v, op2, r.v);
         }
         template <int B, int Nr, bool Cr>
         __FORCE_INLINE void const_shift_l(iv<Nr, Cr>& r) const
         {
            iv_const_shift_l<B>(v, r.v);
         }
         template <int B, int Nr, bool Cr>
         __FORCE_INLINE void const_shift_r(iv<Nr, Cr>& r) const
         {
            iv_const_shift_r<B>(v, r.v);
         }
         template <int Nr, bool Cr>
         __FORCE_INLINE void bitwise_complement(iv<Nr, Cr>& r) const
         {
            iv_bitwise_complement(v, r.v);
         }
         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void bitwise_and(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_bitwise_and(v, op2.v, r.v);
         }
         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void bitwise_or(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_bitwise_or(v, op2.v, r.v);
         }
         template <int N2, bool C2, int Nr, bool Cr>
         __FORCE_INLINE void bitwise_xor(const iv<N2, C2>& op2, iv<Nr, Cr>& r) const
         {
            iv_bitwise_xor(v, op2.v, r.v);
         }
         template <int N2, bool C2>
         __FORCE_INLINE bool equal(const iv<N2, C2>& op2) const
         {
            return iv_equal(v, op2.v);
         }
         template <int N2, bool C2>
         __FORCE_INLINE bool greater_than(const iv<N2, C2>& op2) const
         {
            return iv_compare<true>(v, op2.v);
         }
         template <int N2, bool C2>
         __FORCE_INLINE bool less_than(const iv<N2, C2>& op2) const
         {
            return iv_compare<false>(v, op2.v);
         }
         __FORCE_INLINE bool equal_zero() const
         {
            return iv_equal_zero<0, N>(v);
         }
         template <int N2, bool C2>
         __FORCE_INLINE void set_slc(unsigned lsb, int WS, const iv<N2, C2>& op2)
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
                  v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & ((WS == 32 ? ~0 : ((1u << WS) - 1)) << lsb_b)));
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
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 1; i < N2 - 1; i++)
                  v.set(lsb_v + i, (op2.v[i] << lsb_b) | (((unsigned)op2.v[i - 1] >> 1) >> (31 - lsb_b)));
               unsigned t = (op2.v[N2 - 1] << lsb_b) | (((unsigned)op2.v[N2 - 2] >> 1) >> (31 - lsb_b));
               unsigned m;
               if(msb_v - lsb_v == N2)
               {
                  v.set(msb_v - 1, t);
                  m = (((unsigned)op2.v[N2 - 1] >> 1) >> (31 - lsb_b));
               }
               else
                  m = t;
               v.set(msb_v, v[msb_v] ^ ((v[msb_v] ^ m) & ~((all_ones << 1) << msb_b)));
            }
         }

         template <int N_2, bool C_2>
         __FORCE_INLINE void set_slc2(unsigned lsb, int WS, const iv<N_2, C_2>& op2)
         {
            AC_ASSERT((31 + WS) / 32 <= N_2, "Bad usage: WS greater than length of slice");
            auto N2 = (31 + WS) / 32;
            unsigned msb = lsb + WS - 1;
            unsigned lsb_v = lsb >> 5;
            unsigned lsb_b = lsb & 31;
            unsigned msb_v = msb >> 5;
            unsigned msb_b = msb & 31;
            if(N2 == 1)
            {
               if(msb_v == lsb_v)
                  v.set(lsb_v, v[lsb_v] ^ ((v[lsb_v] ^ (op2.v[0] << lsb_b)) & ((WS == 32 ? ~0 : ((1u << WS) - 1)) << lsb_b)));
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
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
               for(int i = 1; i < N2 - 1; i++)
                  v.set(lsb_v + i, (op2.v[i] << lsb_b) | (((unsigned)op2.v[i - 1] >> 1) >> (31 - lsb_b)));
               unsigned t = (op2.v[N2 - 1] << lsb_b) | (((unsigned)op2.v[N2 - 2] >> 1) >> (31 - lsb_b));
               unsigned m;
               if(static_cast<int>(msb_v - lsb_v) == N2)
               {
                  v.set(msb_v - 1, t);
                  m = (((unsigned)op2.v[N2 - 1] >> 1) >> (31 - lsb_b));
               }
               else
                  m = t;
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
      __FORCE_INLINE void iv<1, false>::set_slc(unsigned lsb, int WS, const iv<1, false>& op2)
      {
         v.set(0, v[0] ^ ((v[0] ^ (op2.v[0] << lsb)) & ((WS == 32 ? ~0u : ((1u << WS) - 1)) << lsb)));
      }
      template <>
      template <>
      __FORCE_INLINE void iv<2, false>::set_slc(unsigned lsb, int WS, const iv<1, false>& op2)
      {
         Ulong l = to_uint64();
         Ulong l2 = op2.to_uint64();
         l ^= (l ^ (l2 << lsb)) & (((1ULL << WS) - 1) << lsb); // WS <= 32
         *this = iv(l);
      }
      template <>
      template <>
      __FORCE_INLINE void iv<2, false>::set_slc(unsigned lsb, int WS, const iv<2, false>& op2)
      {
         Ulong l = to_uint64();
         Ulong l2 = op2.to_uint64();
         l ^= (l ^ (l2 << lsb)) & ((WS == 64 ? ~0ULL : ((1ULL << WS) - 1)) << lsb);
         *this = iv(l);
      }

      // add automatic conversion to Slong/Ulong depending on S and LTE64
      template <int N, bool S, bool LTE64, bool C, int W>
      class iv_conv : public iv<N, C>
      {
       protected:
         __FORCE_INLINE
         iv_conv()
         {
         }
         template <class T>
         __FORCE_INLINE iv_conv(const T& t) : iv<N, C>(t)
         {
         }
      };

      template <int N, bool C, int W>
      class iv_conv<N, false, true, C, W> : public iv<N, C>
      {
       public:
         __FORCE_INLINE
         operator Ulong() const
         {
            auto res = iv<N, C>::to_uint64();
            if(W != 64)
               res = (res << (64 - W)) >> (64 - W);
            return res;
         }

       protected:
         __FORCE_INLINE
         iv_conv()
         {
         }
         template <class T>
         __FORCE_INLINE iv_conv(const T& t) : iv<N, C>(t)
         {
         }
      };

      template <int N, bool C, int W>
      class iv_conv<N, true, true, C, W> : public iv<N, C>
      {
       public:
         __FORCE_INLINE
         operator Slong() const
         {
            auto res = iv<N, C>::to_int64();
            if(W != 64)
               res = (res << (64 - W)) >> (64 - W);
            return res;
         }

       protected:
         __FORCE_INLINE
         iv_conv()
         {
         }
         template <class T>
         __FORCE_INLINE iv_conv(const T& t) : iv<N, C>(t)
         {
         }
      };

      // Set default to promote to int as this is the case for almost all types
      //  create exceptions using specializations
      template <typename T>
      struct c_prom
      {
         typedef int promoted_type;
      };
      template <>
      struct c_prom<unsigned>
      {
         typedef unsigned promoted_type;
      };
      template <>
      struct c_prom<long>
      {
         typedef long promoted_type;
      };
      template <>
      struct c_prom<unsigned long>
      {
         typedef unsigned long promoted_type;
      };
      template <>
      struct c_prom<Slong>
      {
         typedef Slong promoted_type;
      };
      template <>
      struct c_prom<Ulong>
      {
         typedef Ulong promoted_type;
      };
      template <>
      struct c_prom<float>
      {
         typedef float promoted_type;
      };
      template <>
      struct c_prom<double>
      {
         typedef double promoted_type;
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
         typedef T arith_conv;
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
         typedef T t;
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
            typedef typename T::template rt_T<c_type<T2>>::mult mult;
            typedef typename T::template rt_T<c_type<T2>>::plus plus;
            typedef typename T::template rt_T<c_type<T2>>::minus2 minus;
            typedef typename T::template rt_T<c_type<T2>>::minus minus2;
            typedef typename T::template rt_T<c_type<T2>>::logic logic;
            typedef typename T::template rt_T<c_type<T2>>::div2 div;
            typedef typename T::template rt_T<c_type<T2>>::div div2;
         };
      };
      template <typename T>
      struct c_type
      {
         typedef typename c_prom<T>::promoted_type c_prom_T;
         struct rt_unary
         {
            typedef c_prom_T neg;
            typedef c_prom_T mag_sqr;
            typedef c_prom_T mag;
            template <unsigned N>
            struct set
            {
               typedef c_prom_T sum;
            };
         };
         template <typename T2>
         struct rt_T
         {
            typedef typename rt_c_type_T<T2>::template op1<T>::mult mult;
            typedef typename rt_c_type_T<T2>::template op1<T>::plus plus;
            typedef typename rt_c_type_T<T2>::template op1<T>::minus minus;
            typedef typename rt_c_type_T<T2>::template op1<T>::minus2 minus2;
            typedef typename rt_c_type_T<T2>::template op1<T>::logic logic;
            typedef typename rt_c_type_T<T2>::template op1<T>::div div;
            typedef typename rt_c_type_T<T2>::template op1<T>::div2 div2;
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
         typedef typename c_prom<T>::promoted_type c_prom_T;
         template <typename T2>
         struct op1
         {
            typedef typename c_prom<T2>::promoted_type c_prom_T2;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv mult;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv plus;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv minus;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv minus2;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv logic;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv div;
            typedef typename c_arith<c_prom_T, c_prom_T2>::arith_conv div2;
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
            typedef typename T::template rt_T<ac_int<W, S>>::mult mult;
            typedef typename T::template rt_T<ac_int<W, S>>::plus plus;
            typedef typename T::template rt_T<ac_int<W, S>>::minus2 minus;
            typedef typename T::template rt_T<ac_int<W, S>>::minus minus2;
            typedef typename T::template rt_T<ac_int<W, S>>::logic logic;
            typedef typename T::template rt_T<ac_int<W, S>>::div2 div;
            typedef typename T::template rt_T<ac_int<W, S>>::div div2;
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
         typedef ac_int<nbits, signedness> type;
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
#ifndef __BAMBU__
                      __AC_INT_UTILITY_BASE
#endif
   {
      enum
      {
         N = (W + 31 + !S) / 32
      };
      typedef ac_private::iv_conv<N, S, W <= 64, !S && ((W % 32) == 0), W> ConvBase;
      typedef ac_private::iv<N, !S && ((W % 32) == 0)> Base;

      __FORCE_INLINE void bit_adjust()
      {
         Base::v.template bit_adjust<W, S>();
      }
      template <size_t N>
      __FORCE_INLINE void bit_fill(const char (&str)[N])
      {
         if(str[0] == '0' && str[1] == 'x')
            bit_fill_hex(str, 2);
         if(str[0] == '0' && str[1] == 'o')
            bit_fill_oct(str, 2);
         else if(str[0] == '0' && str[1] == 'b')
            bit_fill_bin(str, 2);
         else
            AC_ASSERT(false, "unexpected string format");
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
         bit_adjust();
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
         typedef ac_int<mult_w, mult_s> mult;
         typedef ac_int<plus_w, plus_s> plus;
         typedef ac_int<minus_w, minus_s> minus;
         typedef ac_int<logic_w, logic_s> logic;
         typedef ac_int<div_w, div_s> div;
         typedef ac_int<mod_w, mod_s> mod;
         typedef ac_int<W, S> arg1;
      };

      template <typename T>
      struct rt_T
      {
         typedef typename ac_private::map<T>::t map_T;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::mult mult;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::plus plus;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::minus minus;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::minus2 minus2;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::logic logic;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::div div;
         typedef typename ac_private::rt_ac_int_T<map_T>::template op1<W, S>::div2 div2;
         typedef ac_int<W, S> arg1;
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
         typedef ac_int<neg_w, neg_s> neg;
         typedef ac_int<mag_sqr_w, mag_sqr_s> mag_sqr;
         typedef ac_int<mag_w, mag_s> mag;
         typedef ac_int<leading_sign_w, leading_sign_s> leading_sign;
         template <unsigned N>
         struct set
         {
            enum
            {
               sum_w = W + ac::log2_ceil<N>::val,
               sum_s = S
            };
            typedef ac_int<sum_w, sum_s> sum;
         };
      };

      template <int W2, bool S2>
      friend class ac_int;
      template <int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
      friend class ac_fixed;

      __FORCE_INLINE
      ac_int()
      {
#if !defined(__BAMBU__) && defined(AC_DEFAULT_IN_RANGE)
         bit_adjust();
#endif
      }

      template <int W2, bool S2>
      __FORCE_INLINE ac_int(const ac_int<W2, S2>& op)
      {
         Base::operator=(op);
         bit_adjust();
      }

      __FORCE_INLINE ac_int(bool b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(char b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(signed char b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(unsigned char b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(signed short b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(unsigned short b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(signed int b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(unsigned int b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(signed long b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(unsigned long b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(Slong b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(Ulong b) : ConvBase(b)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(float d) : ConvBase(d)
      {
         bit_adjust();
      }
      __FORCE_INLINE ac_int(double d) : ConvBase(d)
      {
         bit_adjust();
      }

      template <size_t N>
      __FORCE_INLINE ac_int(const char (&str)[N])
      {
         bit_fill(str);
         bit_adjust();
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
      __FORCE_INLINE ac_int& set_val()
      {
         if(V == AC_VAL_DC)
         {
            ac_int r;
            Base::operator=(r);
            bit_adjust();
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

      // Explicit conversion functions to C built-in types -------------
      __FORCE_INLINE int to_int() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         return op1_local.v[0];
      }
      __FORCE_INLINE unsigned to_uint() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         return op1_local.v[0];
      }
      __FORCE_INLINE long to_long() const
      {
         return ac_private::long_w == 32 ? (long)Base::v[0] : (long)Base::to_int64();
      }
      __FORCE_INLINE unsigned long to_ulong() const
      {
         return ac_private::long_w == 32 ? (unsigned long)Base::v[0] : (unsigned long)Base::to_uint64();
      }
      __FORCE_INLINE Slong to_int64() const
      {
         return Base::to_int64();
      }
      __FORCE_INLINE Ulong to_uint64() const
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

      __FORCE_INLINE explicit operator bool() const { return !Base::equal_zero(); }

      __FORCE_INLINE explicit operator char() const { return (char)to_int(); }

      __FORCE_INLINE explicit operator signed char() const { return (signed char)to_int(); }

      __FORCE_INLINE explicit operator unsigned char() const { return (unsigned char)to_uint(); }

      __FORCE_INLINE explicit operator short() const { return (short)to_int(); }

      __FORCE_INLINE explicit operator unsigned short() const { return (unsigned short)to_uint(); }
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
            r[i++] = is_neg() ? '-' : '+';
         else if(base_rep == AC_DEC && is_neg())
            r[i++] = '-';
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
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::mult r;
         op1_local.Base::mult(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::plus operator+(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::plus r;
         op1_local.Base::add(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::minus operator-(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::minus r;
         op1_local.Base::sub(op2_local, r);
         r.bit_adjust();
         return r;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif

      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::div operator/(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
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
         op1_local.Base::template div<num_s, den_s>(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::mod operator%(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
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
         op1_local.Base::template rem<num_s, den_s>(op2_local, r);
         r.bit_adjust();
         return r;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
      // Arithmetic assign  ------------------------------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator*=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::mult(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator+=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::add(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator-=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::sub(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator/=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
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
         Base::template div<num_s, den_s>(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator%=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
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
         Base::template rem<num_s, den_s>(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
#if(defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
      // Arithmetic prefix increment, decrement ----------------------------------
      __FORCE_INLINE ac_int& operator++()
      {
         bit_adjust();
         Base::increment();
         bit_adjust();
         return *this;
      }
      __FORCE_INLINE ac_int& operator--()
      {
         bit_adjust();
         Base::decrement();
         bit_adjust();
         return *this;
      }
      // Arithmetic postfix increment, decrement ---------------------------------
      __FORCE_INLINE const ac_int operator++(int)
      {
         bit_adjust();
         ac_int t = *this;
         Base::increment();
         t.bit_adjust();
         return t;
      }
      __FORCE_INLINE const ac_int operator--(int)
      {
         bit_adjust();
         ac_int t = *this;
         Base::decrement();
         t.bit_adjust();
         return t;
      }
      // Arithmetic Unary --------------------------------------------------------
      __FORCE_INLINE const ac_int operator+() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         return op1_local;
      }
      __FORCE_INLINE const typename rt_unary::neg operator-() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         typename rt_unary::neg r;
         op1_local.Base::neg(r);
         r.bit_adjust();
         return r;
      }
      // ! ------------------------------------------------------------------------
      __FORCE_INLINE bool operator!() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         return op1_local.Base::equal_zero();
      }
      __FORCE_INLINE const ac_int<W + !S, true> operator~() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W + !S, true> r;
         op1_local.Base::bitwise_complement(r);
         r.bit_adjust();
         return r;
      }
      // Bitwise (non-arithmetic) bit_complement  -----------------------------
      __FORCE_INLINE const ac_int<W, false> bit_complement() const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W, false> r;
         op1_local.Base::bitwise_complement(r);
         r.bit_adjust();
         return r;
      }
      // Bitwise (arithmetic): and, or, xor ----------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator&(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::logic r;
         op1_local.Base::bitwise_and(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator|(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::logic r;
         op1_local.Base::bitwise_or(op2_local, r);
         r.bit_adjust();
         return r;
      }
      template <int W2, bool S2>
      __FORCE_INLINE const typename rt<W2, S2>::logic operator^(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         typename rt<W2, S2>::logic r;
         op1_local.Base::bitwise_xor(op2_local, r);
         r.bit_adjust();
         return r;
      }
      // Bitwise assign (not arithmetic): and, or, xor ----------------------------
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator&=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::bitwise_and(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator|=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::bitwise_or(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& operator^=(const ac_int<W2, S2>& op2)
      {
         bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::bitwise_xor(op2_local, r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      // Shift (result constrained by left operand) -------------------------------
      template <int W2>
      __FORCE_INLINE const ac_int operator<<(const ac_int<W2, true>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, true> op2_local = op2;
         op2_local.bit_adjust();
         ac_int r;
         op1_local.Base::shift_l2(op2_local.to_int(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator<<(const ac_int<W2, false>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, false> op2_local = op2;
         op2_local.bit_adjust();
         ac_int r;
         op1_local.Base::shift_l(op2_local.to_uint(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator>>(const ac_int<W2, true>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, true> op2_local = op2;
         op2_local.bit_adjust();
         ac_int r;
         op1_local.Base::shift_r2(op2_local.to_int(), r);
         r.bit_adjust();
         return r;
      }
      template <int W2>
      __FORCE_INLINE const ac_int operator>>(const ac_int<W2, false>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, false> op2_local = op2;
         op2_local.bit_adjust();
         ac_int r;
         op1_local.Base::shift_r(op2_local.to_uint(), r);
         r.bit_adjust();
         return r;
      }

      // Shift assign ------------------------------------------------------------
      template <int W2>
      __FORCE_INLINE ac_int& operator<<=(const ac_int<W2, true>& op2)
      {
         bit_adjust();
         ac_int<W2, true> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::shift_l2(op2_local.to_int(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator<<=(const ac_int<W2, false>& op2)
      {
         bit_adjust();
         ac_int<W2, false> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::shift_l(op2_local.to_uint(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator>>=(const ac_int<W2, true>& op2)
      {
         bit_adjust();
         ac_int<W2, true> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::shift_r2(op2_local.to_int(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      template <int W2>
      __FORCE_INLINE ac_int& operator>>=(const ac_int<W2, false>& op2)
      {
         bit_adjust();
         ac_int<W2, false> op2_local = op2;
         op2_local.bit_adjust();
         Base r;
         Base::shift_r(op2_local.to_uint(), r);
         Base::operator=(r);
         bit_adjust();
         return *this;
      }
      // Relational ---------------------------------------------------------------
      template <int W2, bool S2>
      __FORCE_INLINE bool operator==(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return op1_local.Base::equal(op2_local);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator!=(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return !op1_local.Base::equal(op2_local);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator<(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return op1_local.Base::less_than(op2_local);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator>=(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return !op1_local.Base::less_than(op2_local);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator>(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return op1_local.Base::greater_than(op2);
      }
      template <int W2, bool S2>
      __FORCE_INLINE bool operator<=(const ac_int<W2, S2>& op2) const
      {
         ac_int<W, S> op1_local = *this;
         op1_local.bit_adjust();
         ac_int<W2, S2> op2_local = op2;
         op2_local.bit_adjust();
         return !op1_local.Base::greater_than(op2_local);
      }

      // Bit and Slice Select -----------------------------------------------------
      template <int WS, int WX, bool SX>
      __FORCE_INLINE ac_int<WS, S> slc(const ac_int<WX, SX>& index) const
      {
         using ac_intX = ac_int<WX, SX>;
         ac_int<WS, S> r;
         AC_ASSERT(index >= ac_intX(0), "Attempting to read slc with negative indices");
         ac_int<WX - SX, false> uindex = index;
         Base::shift_r(uindex.to_uint(), r);
         r.bit_adjust();
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
         ac_int<W, false> r(*this);
         r = r << ac_int<W, false>(W - 1 - Hi);
         r.bit_adjust();
         return r >> ac_int<W, false>(Lo + W - 1 - Hi);
      }

      template <int W2, bool S2, int WX, bool SX>
      __FORCE_INLINE ac_int& set_slc(const ac_int<WX, SX> lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb.to_int() + W2 <= W && lsb.to_int() >= 0, "Out of bounds set_slc");
         ac_int<WX - SX, false> ulsb = lsb;
         Base::set_slc(ulsb.to_uint(), W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& set_slc(signed lsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(lsb + W2 <= W && lsb >= 0, "Out of bounds set_slc");
         unsigned ulsb = lsb & ((unsigned)~0 >> 1);
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& set_slc(unsigned ulsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT(ulsb + W2 <= W, "Out of bounds set_slc");
         Base::set_slc(ulsb, W2, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
         return *this;
      }
      template <int W2, bool S2>
      __FORCE_INLINE ac_int& set_slc(int umsb, int ulsb, const ac_int<W2, S2>& slc)
      {
         AC_ASSERT((umsb + 1) <= W, "Out of bounds set_slc");
         Base::set_slc2(ulsb, umsb + 1 - ulsb, (ac_int<W2, true>)slc);
         bit_adjust(); // in case sign bit was assigned
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
               d_bv.v.set(d_index >> 5, d_bv.v[d_index >> 5] ^ ((d_bv.v[d_index >> 5] ^ (val << (d_index & 31))) & 1 << (d_index & 31)));
               d_bv.bit_adjust(); // in case sign bit was assigned
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
            r ^= Base::v[N - 2];
         if(N > 2)
         {
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(int i = 0; i < N - 2; i++)
               r ^= Base::v[i];
         }
         if(W > 16)
            r ^= r >> 16;
         if(W > 8)
            r ^= r >> 8;
         if(W > 4)
            r ^= r >> 4;
         if(W > 2)
            r ^= r >> 2;
         if(W > 1)
            r ^= r >> 1;
         return r & 1;
      }

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_bin(const char (&str)[NN], unsigned start = 0)
      {
         ac_int<W, S> res = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(auto i = start; i < NN; ++i)
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
               break;
            }
            res <<= ac_int<1, false>(1);
            res |= ac_int<1, false>(h);
         }
         *this = res;
      }

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_oct(const char (&str)[NN], unsigned start = 0)
      {
         // Zero Pads if str is too short, throws ms bits away if str is too long
         // Asserts if anything other than 0-9a-fA-F is encountered
         ac_int<W, S> res = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(auto i = start; i < NN; ++i)
         {
            char c = str[i];
            int h = 0;
            if(c >= '0' && c <= '8')
               h = c - '0';
            else
            {
               AC_ASSERT(!c, "Invalid hex digit");
               break;
            }
            res <<= ac_int<W, false>(3);
            res |= ac_int<4, false>(h);
         }
         *this = res;
      }

      template <size_t NN>
      __FORCE_INLINE constexpr void bit_fill_hex(const char (&str)[NN], unsigned start = 0)
      {
         // Zero Pads if str is too short, throws ms bits away if str is too long
         // Asserts if anything other than 0-9a-fA-F is encountered
         ac_int<W, S> res = 0;
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(auto i = start; i < NN; ++i)
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
               break;
            }
            res <<= ac_int<W, false>(4);
            res |= ac_int<4, false>(h);
         }
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
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(int i = 0; i < M; i++)
            res.set_slc(i * 32, ac_int<32>(ivec[bigendian ? M - 1 - i : i]));
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
      __FORCE_INLINE Slong to_int64() const
      {
         const ac_int<W1, false> r = ref.slc(high, low);
         return r.to_int64();
      }
      __FORCE_INLINE Ulong to_uint64() const
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
         typedef typename ac_private::map<T>::t map_T;
         typedef typename ac_private::map<T2>::t map_T2;
         typedef typename map_T::template rt_T<map_T2>::mult mult;
         typedef typename map_T::template rt_T<map_T2>::plus plus;
         typedef typename map_T::template rt_T<map_T2>::minus minus;
         typedef typename map_T::template rt_T<map_T2>::minus2 minus2;
         typedef typename map_T::template rt_T<map_T2>::logic logic;
         typedef typename map_T::template rt_T<map_T2>::div div;
         typedef typename map_T::template rt_T<map_T2>::div2 div2;
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
         typedef ac_int<t_w, t_s> type;
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
         typedef ac_int<W, S> type;
      };
   } // namespace ac

   namespace ac_private
   {
      template <int W2, bool S2>
      struct rt_ac_int_T<ac_int<W2, S2>>
      {
         typedef ac_int<W2, S2> i2_t;
         template <int W, bool S>
         struct op1
         {
            typedef ac_int<W, S> i_t;
            typedef typename i_t::template rt<W2, S2>::mult mult;
            typedef typename i_t::template rt<W2, S2>::plus plus;
            typedef typename i_t::template rt<W2, S2>::minus minus;
            typedef typename i2_t::template rt<W, S>::minus minus2;
            typedef typename i_t::template rt<W2, S2>::logic logic;
            typedef typename i_t::template rt<W2, S2>::div div;
            typedef typename i2_t::template rt<W, S>::div div2;
            typedef typename i_t::template rt<W2, S2>::mod mod;
            typedef typename i2_t::template rt<W, S>::mod mod2;
         };
      };

      template <typename T>
      struct rt_ac_int_T<c_type<T>>
      {
         typedef typename ac::ac_int_represent<T>::type i2_t;
         enum
         {
            W2 = i2_t::width,
            S2 = i2_t::sign
         };
         template <int W, bool S>
         struct op1
         {
            typedef ac_int<W, S> i_t;
            typedef typename i_t::template rt<W2, S2>::mult mult;
            typedef typename i_t::template rt<W2, S2>::plus plus;
            typedef typename i_t::template rt<W2, S2>::minus minus;
            typedef typename i2_t::template rt<W, S>::minus minus2;
            typedef typename i_t::template rt<W2, S2>::logic logic;
            typedef typename i_t::template rt<W2, S2>::div div;
            typedef typename i2_t::template rt<W, S>::div div2;
            typedef typename i_t::template rt<W2, S2>::mod mod;
            typedef typename i2_t::template rt<W, S>::mod mod2;
         };
      };
   } // namespace ac_private

   // Specializations for constructors on integers that bypass bit adjusting
   //  and are therefore more efficient
   template <>
   __FORCE_INLINE ac_int<1, true>::ac_int(bool b)
   {
      v.set(0, b ? -1 : 0);
   }

   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(signed char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(unsigned char b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(signed short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(unsigned short b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(signed int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(unsigned int b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(signed long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(unsigned long b)
   {
      v.set(0, b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b & 1);
   }
   template <>
   __FORCE_INLINE ac_int<1, false>::ac_int(Slong b)
   {
      v.set(0, (int)b & 1);
   }

   template <>
   __FORCE_INLINE ac_int<8, true>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<8, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<8, true>::ac_int(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<8, false>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<8, true>::ac_int(unsigned char b)
   {
      v.set(0, (signed char)b);
   }
   template <>
   __FORCE_INLINE ac_int<8, false>::ac_int(signed char b)
   {
      v.set(0, (unsigned char)b);
   }

   template <>
   __FORCE_INLINE ac_int<16, true>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, false>::ac_int(bool b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, true>::ac_int(signed char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, false>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, true>::ac_int(unsigned char b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, false>::ac_int(signed char b)
   {
      v.set(0, (unsigned short)b);
   }
   template <>
   __FORCE_INLINE ac_int<16, true>::ac_int(signed short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, false>::ac_int(unsigned short b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<16, true>::ac_int(unsigned short b)
   {
      v.set(0, (signed short)b);
   }
   template <>
   __FORCE_INLINE ac_int<16, false>::ac_int(signed short b)
   {
      v.set(0, (unsigned short)b);
   }

   template <>
   __FORCE_INLINE ac_int<32, true>::ac_int(signed int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<32, true>::ac_int(unsigned int b)
   {
      v.set(0, b);
   }
   template <>
   __FORCE_INLINE ac_int<32, false>::ac_int(signed int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE ac_int<32, false>::ac_int(unsigned int b)
   {
      v.set(0, b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE ac_int<32, true>::ac_int(Slong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE ac_int<32, true>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
   }
   template <>
   __FORCE_INLINE ac_int<32, false>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }
   template <>
   __FORCE_INLINE ac_int<32, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, 0);
   }

   template <>
   __FORCE_INLINE ac_int<64, true>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE ac_int<64, true>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
   }
   template <>
   __FORCE_INLINE ac_int<64, false>::ac_int(Slong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)((Ulong)b >> 32));
      v.set(2, 0);
   }
   template <>
   __FORCE_INLINE ac_int<64, false>::ac_int(Ulong b)
   {
      v.set(0, (int)b);
      v.set(1, (int)(b >> 32));
      v.set(2, 0);
   }

   // Stream --------------------------------------------------------------------

   template <int W, bool S>
   __FORCE_INLINE std::ostream& operator<<(std::ostream& os, const ac_int<W, S>& x)
   {
#ifndef __BAMBU__
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
#ifndef __BAMBU__

      std::string str;
      in >> str;
      const std::ios_base::fmtflags basefield = in.flags() & std::ios_base::basefield;
      unsigned radix = (basefield == std::ios_base::dec) ? 0 : (
                                                                   (basefield == std::ios_base::oct) ? 8 : (
                                                                                                               (basefield == std::ios_base::hex) ? 16 : 0));
      //x = convert a char * str.c_str() with specified radix into ac_int; //TODO
#endif
      return in;

   }

   // Macros for Binary Operators with Integers
   // --------------------------------------------

#define BIN_OP_WITH_INT(BIN_OP, C_TYPE, WI, SI, RTYPE)                                                                   \
   template <int W, bool S>                                                                                              \
   __FORCE_INLINE typename ac_int<WI, SI>::template rt<W, S>::RTYPE operator BIN_OP(C_TYPE i_op, const ac_int<W, S>& op) \
   {                                                                                                                     \
      return ac_int<WI, SI>(i_op).operator BIN_OP(op);                                                                   \
   }                                                                                                                     \
   template <int W, bool S>                                                                                              \
   __FORCE_INLINE typename ac_int<W, S>::template rt<WI, SI>::RTYPE operator BIN_OP(const ac_int<W, S>& op, C_TYPE i_op) \
   {                                                                                                                     \
      return op.operator BIN_OP(ac_int<WI, SI>(i_op));                                                                   \
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

#define OPS_WITH_INT(C_TYPE, WI, SI)          \
   BIN_OP_WITH_INT(*, C_TYPE, WI, SI, mult)   \
   BIN_OP_WITH_INT(+, C_TYPE, WI, SI, plus)   \
   BIN_OP_WITH_INT(-, C_TYPE, WI, SI, minus)  \
   BIN_OP_WITH_INT(/, C_TYPE, WI, SI, div)    \
   BIN_OP_WITH_INT(%, C_TYPE, WI, SI, mod)    \
   BIN_OP_WITH_INT(>>, C_TYPE, WI, SI, arg1)  \
   BIN_OP_WITH_INT(<<, C_TYPE, WI, SI, arg1)  \
   BIN_OP_WITH_INT(&, C_TYPE, WI, SI, logic)  \
   BIN_OP_WITH_INT(|, C_TYPE, WI, SI, logic)  \
   BIN_OP_WITH_INT (^, C_TYPE, WI, SI, logic) \
                                              \
   REL_OP_WITH_INT(==, C_TYPE, WI, SI)        \
   REL_OP_WITH_INT(!=, C_TYPE, WI, SI)        \
   REL_OP_WITH_INT(>, C_TYPE, WI, SI)         \
   REL_OP_WITH_INT(>=, C_TYPE, WI, SI)        \
   REL_OP_WITH_INT(<, C_TYPE, WI, SI)         \
   REL_OP_WITH_INT(<=, C_TYPE, WI, SI)        \
                                              \
   ASSIGN_OP_WITH_INT(+=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(-=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(*=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(/=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(%=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(>>=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(<<=, C_TYPE, WI, SI)    \
   ASSIGN_OP_WITH_INT(&=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(|=, C_TYPE, WI, SI)     \
   ASSIGN_OP_WITH_INT(^=, C_TYPE, WI, SI)

// ------------------------------------- End of Macros for Binary Operators with
// Integers

// --- Macro for range_ref
#define OP_BIN_RANGE(BIN_OP, RTYPE)                                                                                                               \
   template <int W1, bool S1, int W2, bool S2>                                                                                                    \
   __FORCE_INLINE typename ac_int<W1, S1>::template rt<W2, S2>::RTYPE operator BIN_OP(const range_ref<W1, S1>& op1, const range_ref<W2, S2>& op2) \
   {                                                                                                                                              \
      return ac_int<W1, false>(op1) BIN_OP(ac_int<W2, false>(op2));                                                                               \
   }                                                                                                                                              \
   template <int W1, bool S1, int W2, bool S2>                                                                                                    \
   __FORCE_INLINE typename ac_int<W1, S1>::template rt<W2, S2>::RTYPE operator BIN_OP(const range_ref<W1, S1>& op1, const ac_int<W2, S2>& op2)    \
   {                                                                                                                                              \
      return ac_int<W1, false>(op1) BIN_OP(op2);                                                                                                  \
   }                                                                                                                                              \
   template <int W1, bool S1, int W2, bool S2>                                                                                                    \
   __FORCE_INLINE typename ac_int<W1, S1>::template RType<W2, S2>::RTYPE operator BIN_OP(const ac_int<W1, S1>& op1, const range_ref<W2, S2>& op2) \
   {                                                                                                                                              \
      return op1 BIN_OP(ac_int<W2, false>(op2));                                                                                                  \
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
         OP_BIN_RANGE (^, logic)
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
      typedef ac_int<1, true> int1;
      typedef ac_int<1, false> uint1;
      typedef ac_int<2, true> int2;
      typedef ac_int<2, false> uint2;
      typedef ac_int<3, true> int3;
      typedef ac_int<3, false> uint3;
      typedef ac_int<4, true> int4;
      typedef ac_int<4, false> uint4;
      typedef ac_int<5, true> int5;
      typedef ac_int<5, false> uint5;
      typedef ac_int<6, true> int6;
      typedef ac_int<6, false> uint6;
      typedef ac_int<7, true> int7;
      typedef ac_int<7, false> uint7;
      typedef ac_int<8, true> int8;
      typedef ac_int<8, false> uint8;
      typedef ac_int<9, true> int9;
      typedef ac_int<9, false> uint9;
      typedef ac_int<10, true> int10;
      typedef ac_int<10, false> uint10;
      typedef ac_int<11, true> int11;
      typedef ac_int<11, false> uint11;
      typedef ac_int<12, true> int12;
      typedef ac_int<12, false> uint12;
      typedef ac_int<13, true> int13;
      typedef ac_int<13, false> uint13;
      typedef ac_int<14, true> int14;
      typedef ac_int<14, false> uint14;
      typedef ac_int<15, true> int15;
      typedef ac_int<15, false> uint15;
      typedef ac_int<16, true> int16;
      typedef ac_int<16, false> uint16;
      typedef ac_int<17, true> int17;
      typedef ac_int<17, false> uint17;
      typedef ac_int<18, true> int18;
      typedef ac_int<18, false> uint18;
      typedef ac_int<19, true> int19;
      typedef ac_int<19, false> uint19;
      typedef ac_int<20, true> int20;
      typedef ac_int<20, false> uint20;
      typedef ac_int<21, true> int21;
      typedef ac_int<21, false> uint21;
      typedef ac_int<22, true> int22;
      typedef ac_int<22, false> uint22;
      typedef ac_int<23, true> int23;
      typedef ac_int<23, false> uint23;
      typedef ac_int<24, true> int24;
      typedef ac_int<24, false> uint24;
      typedef ac_int<25, true> int25;
      typedef ac_int<25, false> uint25;
      typedef ac_int<26, true> int26;
      typedef ac_int<26, false> uint26;
      typedef ac_int<27, true> int27;
      typedef ac_int<27, false> uint27;
      typedef ac_int<28, true> int28;
      typedef ac_int<28, false> uint28;
      typedef ac_int<29, true> int29;
      typedef ac_int<29, false> uint29;
      typedef ac_int<30, true> int30;
      typedef ac_int<30, false> uint30;
      typedef ac_int<31, true> int31;
      typedef ac_int<31, false> uint31;
      typedef ac_int<32, true> int32;
      typedef ac_int<32, false> uint32;
      typedef ac_int<33, true> int33;
      typedef ac_int<33, false> uint33;
      typedef ac_int<34, true> int34;
      typedef ac_int<34, false> uint34;
      typedef ac_int<35, true> int35;
      typedef ac_int<35, false> uint35;
      typedef ac_int<36, true> int36;
      typedef ac_int<36, false> uint36;
      typedef ac_int<37, true> int37;
      typedef ac_int<37, false> uint37;
      typedef ac_int<38, true> int38;
      typedef ac_int<38, false> uint38;
      typedef ac_int<39, true> int39;
      typedef ac_int<39, false> uint39;
      typedef ac_int<40, true> int40;
      typedef ac_int<40, false> uint40;
      typedef ac_int<41, true> int41;
      typedef ac_int<41, false> uint41;
      typedef ac_int<42, true> int42;
      typedef ac_int<42, false> uint42;
      typedef ac_int<43, true> int43;
      typedef ac_int<43, false> uint43;
      typedef ac_int<44, true> int44;
      typedef ac_int<44, false> uint44;
      typedef ac_int<45, true> int45;
      typedef ac_int<45, false> uint45;
      typedef ac_int<46, true> int46;
      typedef ac_int<46, false> uint46;
      typedef ac_int<47, true> int47;
      typedef ac_int<47, false> uint47;
      typedef ac_int<48, true> int48;
      typedef ac_int<48, false> uint48;
      typedef ac_int<49, true> int49;
      typedef ac_int<49, false> uint49;
      typedef ac_int<50, true> int50;
      typedef ac_int<50, false> uint50;
      typedef ac_int<51, true> int51;
      typedef ac_int<51, false> uint51;
      typedef ac_int<52, true> int52;
      typedef ac_int<52, false> uint52;
      typedef ac_int<53, true> int53;
      typedef ac_int<53, false> uint53;
      typedef ac_int<54, true> int54;
      typedef ac_int<54, false> uint54;
      typedef ac_int<55, true> int55;
      typedef ac_int<55, false> uint55;
      typedef ac_int<56, true> int56;
      typedef ac_int<56, false> uint56;
      typedef ac_int<57, true> int57;
      typedef ac_int<57, false> uint57;
      typedef ac_int<58, true> int58;
      typedef ac_int<58, false> uint58;
      typedef ac_int<59, true> int59;
      typedef ac_int<59, false> uint59;
      typedef ac_int<60, true> int60;
      typedef ac_int<60, false> uint60;
      typedef ac_int<61, true> int61;
      typedef ac_int<61, false> uint61;
      typedef ac_int<62, true> int62;
      typedef ac_int<62, false> uint62;
      typedef ac_int<63, true> int63;
      typedef ac_int<63, false> uint63;
   } // namespace ac_intN

#ifndef AC_NOT_USING_INTN
   using namespace ac_intN;
#endif

   ///////////////////////////////////////////////////////////////////////////////

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
   template <ac_special_val V, int W, bool S>
   __FORCE_INLINE ac_int<W, S> value(ac_int<W, S>)
   {
      ac_int<W, S> r;
      return r.template set_val<V>();
   }
   // forward declaration, otherwise GCC errors when calling init_array
   template <ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
   __FORCE_INLINE ac_fixed<W, I, S, Q, O> value(ac_fixed<W, I, S, Q, O>);

#define SPECIAL_VAL_FOR_INTS_DC(C_TYPE, WI, SI)   \
   template <>                                    \
   __FORCE_INLINE C_TYPE value<AC_VAL_DC>(C_TYPE) \
   {                                              \
      C_TYPE x;                                   \
      return x;                                   \
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
            a[i] = t;
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

#endif // __AC_INT_H
