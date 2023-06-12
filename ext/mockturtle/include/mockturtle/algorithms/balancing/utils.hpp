/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file utils.hpp
  \brief Balancing data types

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <queue>

#include "../../traits.hpp"

namespace mockturtle
{

template<class Ntk>
struct arrival_time_compare
{
  bool operator()( arrival_time_pair<Ntk> const& p1, arrival_time_pair<Ntk> const& p2 ) const
  {
    return p1.level > p2.level;
  }
};

template<class Ntk>
using arrival_time_queue = std::priority_queue<arrival_time_pair<Ntk>, std::vector<arrival_time_pair<Ntk>>, arrival_time_compare<Ntk>>;

} // namespace mockturtle