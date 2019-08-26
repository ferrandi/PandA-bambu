/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL
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
  \file control.hpp
  \brief Generate control logic networks

  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Creates a word from a constant
 *
 * Creates a vector of `bitwidth` constants that represent the positive number
 * `value`.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> constant_word( Ntk& ntk, uint64_t value, uint32_t bitwidth )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  std::vector<signal<Ntk>> word( bitwidth );
  for ( auto i = 0u; i < bitwidth; ++i )
  {
    word[i] = ntk.get_constant( static_cast<bool>( ( value >> i ) & 1 ) );
  }
  return word;
}

/*! \brief Extends a word by leading zeros
 *
 * Adds leading zeros as most-significant bits to word `a`.  The size of `a`
 * must be smaller or equal to `bitwidth`, which is the width of the resulting
 * word.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> zero_extend( Ntk& ntk, std::vector<signal<Ntk>> const& a, uint32_t bitwidth )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  assert( bitwidth >= a.size() );

  auto ret{a};
  for ( auto i = a.size(); i < bitwidth; ++i )
  {
    ret.emplace_back( ntk.get_constant( false ) );
  }
  return ret;
}

/*! \brief Creates a 2k-k MUX (array of k 2-1 MUXes).
 *
 * This creates *k* MUXes using `cond` as condition signal and `t` for the then
 * signals and `e` for the else signals.  The method works in-place and writes
 * the outputs of the networ into `t`.
 */
template<class Ntk>
inline void mux_inplace( Ntk& ntk, signal<Ntk> const& cond, std::vector<signal<Ntk>>& t, std::vector<signal<Ntk>> const& e )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );

  std::transform( t.begin(), t.end(), e.begin(), t.begin(), [&]( auto const& a, auto const& b ) { return ntk.create_ite( cond, a, b ); } );
}

} // namespace mockturtle
