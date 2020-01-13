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
 * @file math_function.cpp
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

#include <cmath>
#include <cstddef>

/// Utility include
#include "augmented_vector.hpp"
#include "exceptions.hpp"
/// Header include
#include "math_function.hpp"

long double get_point_line_distance(const AugmentedVector<long double>& point, AugmentedVector<long double>& line_point1, AugmentedVector<long double>& line_point2)
{
   long double t;
   long double dq = 0;
   THROW_ASSERT(point.size() == line_point1.size() and line_point1.size() == line_point2.size(), "Different dimension of space");

   t = -1 * ((line_point1 - point) * (line_point2 - line_point1)) / powl((line_point2 - line_point1).Norm2(), 2);

   for(size_t i = 0; i < point.size(); i++)
   {
      dq += powl((line_point1[i] - point[i]) + (line_point2[i] - line_point1[i]) * t, 2);
   }

   return sqrtl(dq);
}
#endif
