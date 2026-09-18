/*! \file */
/*
 * kmp_os.h -- KPTS runtime header file.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/////////////////////////////////////////////////////////////////////////////////////////////
//    bambu customized OpenMP runtime library
//
//    Author Fabrizio Ferrandi - Politecnico di Milano
//
//    November 6, 2020.
//    The original library has been taken from https://github.com/llvm/llvm-project/tree/master/openmp
//    For the additional notes check the associated README.md.
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef KMP_OS_H
#define KMP_OS_H

#include <assert.h>
#include <stddef.h>
#define KMP_ASSERT(cond) assert(cond)

typedef char kmp_int8;
typedef unsigned char kmp_uint8;
typedef short kmp_int16;
typedef unsigned short kmp_uint16;
typedef int kmp_int32;
typedef unsigned int kmp_uint32;
typedef long long kmp_int64;
typedef unsigned long long kmp_uint64;

#ifdef __cplusplus
// macros to cast out qualifiers and to re-interpret types
#define CCAST(type, var) const_cast<type>(var)
#define RCAST(type, var) reinterpret_cast<type>(var)
//-------------------------------------------------------------------------
// template for debug prints specification ( d, u, lld, llu ), and to obtain
// signed/unsigned flavors of a type
template <typename T>
struct traits_t
{
};
// int
template <>
struct traits_t<signed int>
{
   typedef signed int signed_t;
   typedef unsigned int unsigned_t;
   typedef double floating_t;
   static char const* spec;
   static const signed_t max_value = 0x7fffffff;
   static const signed_t min_value = 0x80000000;
   static const int type_size = sizeof(signed_t);
};
// unsigned int
template <>
struct traits_t<unsigned int>
{
   typedef signed int signed_t;
   typedef unsigned int unsigned_t;
   typedef double floating_t;
   static char const* spec;
   static const unsigned_t max_value = 0xffffffff;
   static const unsigned_t min_value = 0x00000000;
   static const int type_size = sizeof(unsigned_t);
};
// long
template <>
struct traits_t<signed long>
{
   typedef signed long signed_t;
   typedef unsigned long unsigned_t;
   typedef long double floating_t;
   static char const* spec;
   static const int type_size = sizeof(signed_t);
};
// long long
template <>
struct traits_t<signed long long>
{
   typedef signed long long signed_t;
   typedef unsigned long long unsigned_t;
   typedef long double floating_t;
   static char const* spec;
   static const signed_t max_value = 0x7fffffffffffffffLL;
   static const signed_t min_value = 0x8000000000000000LL;
   static const int type_size = sizeof(signed_t);
};
// unsigned long long
template <>
struct traits_t<unsigned long long>
{
   typedef signed long long signed_t;
   typedef unsigned long long unsigned_t;
   typedef long double floating_t;
   static char const* spec;
   static const unsigned_t max_value = 0xffffffffffffffffLL;
   static const unsigned_t min_value = 0x0000000000000000LL;
   static const int type_size = sizeof(unsigned_t);
};
//-------------------------------------------------------------------------
#else
#define CCAST(type, var) (type)(var)
#define RCAST(type, var) (type)(var)
#endif // __cplusplus

#define KMP_EXPORT extern /* export declaration in guide libraries */

#if __GNUC__ >= 4 && !defined(__MINGW32__)
#define __forceinline __inline
#endif

#endif /* KMP_OS_H */
