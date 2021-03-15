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
  \file cost_functions.hpp
  \brief Various cost functions for (optimization) algorithms

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>

#include "../traits.hpp"

namespace mockturtle
{

template<class Ntk>
struct unit_cost
{
  uint32_t operator()( Ntk const& ntk, node<Ntk> const& node ) const
  {
    (void)ntk;
    (void)node;
    return 1u;
  }
};

template<class Ntk>
struct mc_cost
{
  uint32_t operator()( Ntk const& ntk, node<Ntk> const& node ) const
  {
    if constexpr ( has_is_xor_v<Ntk> )
    {
      if ( ntk.is_xor( node ) )
      {
        return 0u;
      }
    }

    if constexpr ( has_is_xor3_v<Ntk> )
    {
      if ( ntk.is_xor3( node ) )
      {
        return 0u;
      }
    }

    if constexpr ( has_is_nary_and_v<Ntk> )
    {
      if ( ntk.is_nary_and( node ) )
      {
        if ( ntk.fanin_size( node ) > 1u )
        {
          return ntk.fanin_size( node ) - 1u;
        }
        return 0u;
      }
    }

    if constexpr ( has_is_nary_or_v<Ntk> )
    {
      if ( ntk.is_nary_or( node ) )
      {
        if ( ntk.fanin_size( node ) > 1u )
        {
          return ntk.fanin_size( node ) - 1u;
        }
        return 0u;
      }
    }

    if constexpr ( has_is_nary_xor_v<Ntk> )
    {
      if ( ntk.is_nary_xor( node ) )
      {
        return 0u;
      }
    }

    // TODO (Does not take into account general node functions)
    return 1u;
  }
};

template<class Ntk, class NodeCostFn = unit_cost<Ntk>>
uint32_t costs( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );

  uint32_t total{0u};
  NodeCostFn cost_fn{};
  ntk.foreach_gate( [&]( auto const& n ) {
    total += cost_fn( ntk, n );
  });
  return total;
}

} /* namespace mockturtle */
