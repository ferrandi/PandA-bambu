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
  \file aqfp_assumptions.hpp
  \brief Technology assumptions for AQFP

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

namespace mockturtle
{

/*! \brief AQFP technology assumptions.
 *
 * POs count toward the fanout sizes and always have to be branched.
 * If PIs need to be balanced, then they must also need to be branched.
 */
struct aqfp_assumptions
{
  /*! \brief Whether PIs need to be branched with splitters. */
  bool branch_pis{ false };

  /*! \brief Whether PIs need to be path-balanced. */
  bool balance_pis{ false };

  /*! \brief Whether POs need to be path-balanced. */
  bool balance_pos{ true };

  /*! \brief The maximum number of fanouts each splitter (buffer) can have. */
  uint32_t splitter_capacity{ 3u };
};

} // namespace mockturtle
