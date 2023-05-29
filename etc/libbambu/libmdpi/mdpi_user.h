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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file mdpi_user.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_USER_H
#define __MDPI_USER_H

#ifndef EXTERN
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#include <stddef.h>
#include <stdint.h>

EXTERN_C void m_alloc_param(uint8_t idx, size_t size);

#ifndef __cplusplus
EXTERN_C float m_float_distancef(float, float) __attribute__((const));
EXTERN_C double m_float_distance(double, double) __attribute__((const));
EXTERN_C long double m_float_distancel(long double, long double) __attribute__((const));
#else
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

/*
 * m_float_distance was implemented based on the Boost implementation
 * whose copyright notice is reported below
 *
 *  (C) Copyright John Maddock 2008.
 *  Use, modification and distribution are subject to the
 *  Boost Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
template <class T>
T m_float_distance(const T& a, const T& b)
{
   //
   // Error handling:
   //
   if(std::isfinite(a) ^ std::isfinite(b))
      return std::numeric_limits<T>::max();

   //
   // Special cases:
   //
   if(a == b)
      return T(0);
   if(std::isinf(a) && std::isinf(b))
      return T(0);
   if(std::isnan(a) && std::isnan(b))
      return T(0);
   if(a > b)
      return m_float_distance(b, a);
   if(a == 0)
      return 1 + std::fabs(m_float_distance(static_cast<T>((b < 0) ? T(-std::numeric_limits<T>::epsilon()) :
                                                                     std::numeric_limits<T>::epsilon()),
                                            b));
   if(b == 0)
      return 1 + std::fabs(m_float_distance(static_cast<T>((a < 0) ? T(-std::numeric_limits<T>::epsilon()) :
                                                                     std::numeric_limits<T>::epsilon()),
                                            a));
   if((a > 0) != (b > 0))
      return 2 +
             std::fabs(m_float_distance(
                 static_cast<T>((b < 0) ? T(-std::numeric_limits<T>::epsilon()) : std::numeric_limits<T>::epsilon()),
                 b)) +
             std::fabs(m_float_distance(
                 static_cast<T>((a < 0) ? T(-std::numeric_limits<T>::epsilon()) : std::numeric_limits<T>::epsilon()),
                 a));
   //
   // By the time we get here, both a and b must have the same sign, we want
   // b > a and both positive for the following logic:
   //
   if(a < 0)
      return m_float_distance(static_cast<T>(-b), static_cast<T>(-a));

   assert(a >= 0);
   assert(b >= a);

   int expon;
   //
   // Note that if a is a denorm then the usual formula fails
   // because we actually have fewer than std::numeric_limits<T>::digits
   // significant bits in the representation:
   //
   (void)std::frexp(((std::fpclassify)(a) == (int)FP_SUBNORMAL) ? std::numeric_limits<T>::min() : a, &expon);
   T upper = std::ldexp(T(1), expon);
   T result = T(0);
   //
   // If b is greater than upper, then we *must* split the calculation
   // as the size of the ULP changes with each order of magnitude change:
   //
   if(b > upper)
   {
      int expon2;
      (void)std::frexp(b, &expon2);
      T upper2 = std::ldexp(T(0.5), expon2);
      result = m_float_distance(upper2, b);
      result += (expon2 - expon - 1) * std::ldexp(T(1), std::numeric_limits<T>::digits - 1);
   }
   //
   // Use compensated double-double addition to avoid rounding
   // errors in the subtraction:
   //
   expon = std::numeric_limits<T>::digits - expon;
   T mb, x, y, z;
   if(((std::fpclassify)(a) == (int)FP_SUBNORMAL) || (b - a < std::numeric_limits<T>::min()))
   {
      //
      // Special case - either one end of the range is a denormal, or else the difference is.
      // The regular code will fail if we're using the SSE2 registers on Intel and either
      // the FTZ or DAZ flags are set.
      //
      T a2 = std::ldexp(a, std::numeric_limits<T>::digits);
      T b2 = std::ldexp(b, std::numeric_limits<T>::digits);
      mb = -(std::min)(T(ldexp(upper, std::numeric_limits<T>::digits)), b2);
      x = a2 + mb;
      z = x - a2;
      y = (a2 - (x - z)) + (mb - z);

      expon -= std::numeric_limits<T>::digits;
   }
   else
   {
      mb = -(std::min)(upper, b);
      x = a + mb;
      z = x - a;
      y = (a - (x - z)) + (mb - z);
   }
   if(x < 0)
   {
      x = -x;
      y = -y;
   }
   result += ldexp(x, expon) + ldexp(y, expon);
   //
   // Result must be an integer:
   //
   assert(result == floor(result));
   return result;
}
#endif

#endif // __MDPI_ALLOC_H