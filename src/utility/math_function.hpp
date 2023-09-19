/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file math_function.hpp
 * @brief mathematical utility function not provided by standard libraries
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef MATH_FUNCTION_HPP
#define MATH_FUNCTION_HPP

#include <boost/version.hpp>
#if BOOST_VERSION >= 105800
#include <boost/integer/common_factor_rt.hpp>
#else
#include <boost/math/common_factor_rt.hpp>
#endif

#include <limits>
#include <type_traits>

/**
 * Return the greatest common divisor
 * @param first is the first operand
 * @param second is the second operand
 * @return the greatest common divisor of first and second
 */
template <typename Integer>
Integer GreatestCommonDivisor(const Integer first, const Integer second)
{
#if BOOST_VERSION >= 105800
   return boost::integer::gcd<Integer>(first, second);
#else
   return boost::math::gcd<Integer>(first, second);
#endif
}

/**
 * Return the least common multiple of two integers
 * @param first is the first operand
 * @param second is the second operand
 * @return the least common multiple of first and second
 */
template <typename Integer>
Integer LeastCommonMultiple(const Integer first, const Integer second)
{
#if BOOST_VERSION >= 105800
   return boost::integer::lcm<Integer>(first, second);
#else
   return boost::math::lcm<Integer>(first, second);
#endif
}

template <typename T>
inline T compute_n_bytes(T bitsize)
{
   return bitsize / 8 + ((bitsize % 8) != 0);
}

/// Test whether a value is zero of a power of two.
#define EXACT_POWER_OF_2_OR_ZERO_P(x) (((x) & ((x)-1)) == 0)

/// Given X, an unsigned number, return the largest int Y such that 2**Y <= X. If X is 0, return -1.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
inline T floor_log2(T x)
{
   unsigned long long t = 0;

   if(x == 0)
   {
      return static_cast<T>(-1LL);
   }

   if(x >= 1ULL << (t + 32))
   {
      t += 32;
   }
   if(x >= 1ULL << (t + 16))
   {
      t += 16;
   }
   if(x >= 1ULL << (t + 8))
   {
      t += 8;
   }
   if(x >= 1ULL << (t + 4))
   {
      t += 4;
   }
   if(x >= 1ULL << (t + 2))
   {
      t += 2;
   }
   if(x >= 1ULL << (t + 1))
   {
      t += 1;
   }

   return static_cast<T>(t);
}

/** Return the logarithm of X, base 2, considering X unsigned,
   if X is a power of 2.  Otherwise, returns -1.  */
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
inline T exact_log2(T x)
{
   if(x != (x & -x))
   {
      return static_cast<T>(-1LL);
   }
   return floor_log2(x);
}

/// Return the smallest n such that 2**n >= X.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
inline T ceil_log2(T x)
{
   if(x == 0)
   {
      return static_cast<T>(-1LL);
   }
   return static_cast<T>(floor_log2(static_cast<T>(x - 1)) + 1);
}

/// Return the smallest n such that 2^n >= _x
template <
    typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true,
    std::enable_if_t<std::numeric_limits<T>::digits <= std::numeric_limits<unsigned long long>::digits, bool> = true>
constexpr inline T ceil_pow2(T _x)
{
   unsigned long long x = _x;
   x--;
   x |= x >> 1;
   x |= x >> 2;
   x |= x >> 4;
   x |= x >> 8;
   x |= x >> 16;
   x |= x >> 32;
   x++;
   return static_cast<T>(x);
}

template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
constexpr inline T get_aligned_bitsize(T bitsize)
{
   const auto rbw = std::max(T(8), ceil_pow2(bitsize));
   if(rbw <= 128ULL)
   {
      return rbw;
   }
   return bitsize + ((32ULL - (bitsize % 32ULL)) & 31ULL);
}

template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
constexpr inline T get_aligned_bitsize(T bitsize, T align)
{
   return std::max(align, ((bitsize / align) + (bitsize % align != 0)) * align);
}

template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
constexpr inline T get_aligned_ac_bitsize(T bitsize)
{
   return bitsize + ((64ULL - (bitsize % 64ULL)) & 63ULL);
}

template <typename T, std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
inline T resize_1_8_pow2(T value)
{
   if(value == T(1))
   {
      return T(1);
   }
   return std::max(T(8), ceil_pow2(value));
}

#endif
