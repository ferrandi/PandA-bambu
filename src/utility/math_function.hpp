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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

/// Utility include
#include "augmented_vector.hpp"
#include <boost/version.hpp>
#if BOOST_VERSION >= 105800
#include <boost/integer/common_factor_rt.hpp>
#else
#include <boost/math/common_factor_rt.hpp>
#endif
/**
 * Return the distance between a point and a line (represented as a couple of points) in a n-dimensional space
 */
long double get_point_line_distance(const AugmentedVector<long double>& point, AugmentedVector<long double>& line_point1, AugmentedVector<long double>& line_point2);

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

inline unsigned int resize_to_1_8_16_32_64_128_256_512(unsigned int value)
{
   if(value == 1)
      return 1;
   else if(value <= 8)
      return 8;
   else if(value <= 16)
      return 16;
   else if(value <= 32)
      return 32;
   else if(value <= 64)
      return 64;
   else if(value <= 128)
      return 128;
   else if(value <= 256)
      return 256;
   else if(value <= 512)
      return 512;
   else
      THROW_ERROR("not expected size " + boost::lexical_cast<std::string>(value));
   return 0;
}

inline unsigned int compute_n_bytes(unsigned bitsize)
{
   return bitsize / 8 + ((bitsize % 8) ? 1 : 0);
}

/// Test whether a value is zero of a power of two.
#define EXACT_POWER_OF_2_OR_ZERO_P(x) (((x) & ((x)-1)) == 0)

/// Given X, an unsigned number, return the largest int Y such that 2**Y <= X. If X is 0, return -1.
inline int floor_log2(unsigned long long int x)
{
   int t = 0;

   if(x == 0)
      return -1;

   if(x >= (static_cast<unsigned long long int>(1)) << (t + 32))
      t += 32;
   if(x >= (static_cast<unsigned long long int>(1)) << (t + 16))
      t += 16;
   if(x >= (static_cast<unsigned long long int>(1)) << (t + 8))
      t += 8;
   if(x >= (static_cast<unsigned long long int>(1)) << (t + 4))
      t += 4;
   if(x >= (static_cast<unsigned long long int>(1)) << (t + 2))
      t += 2;
   if(x >= (static_cast<unsigned long long int>(1)) << (t + 1))
      t += 1;

   return t;
}

/** Return the logarithm of X, base 2, considering X unsigned,
   if X is a power of 2.  Otherwise, returns -1.  */
inline int exact_log2(unsigned long long int x)
{
   if(x != (x & -x))
      return -1;
   return floor_log2(x);
}

/// Return the smallest n such that 2**n >= X.
inline int ceil_log2(unsigned long long int x)
{
   return floor_log2(x - 1) + 1;
}

#endif
