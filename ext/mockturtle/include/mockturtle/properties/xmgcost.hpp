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
  \file migcost.hpp
  \brief Cost functions for xmg-based networks 

  \author Shubham Rai  
*/

#pragma once

#include <cstdint>
#include <unordered_set>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Counts number of inverters.
 *
 * This number counts all nodes that need to be inverted.  Multiple signals
 * with complements to the same node are counted once.
 *
 * \param ntk Network
 */

struct xmg_cost_params
{
  /*! \brief Total number of XOR3 . */
  uint32_t total_xor3{0};

  /*! \brief Actual number of XOR3 with three number of inputs. */
  uint32_t actual_xor3{0};

  /*! \brief Actual number of XOR2. */
  uint32_t actual_xor2{0};

  /*! \brief Total number of Majority. */
  uint32_t total_maj{0};

  /*! \brief Actual number of Majority with three inputs. */
  uint32_t actual_maj{0};

  /*! \brief Remaining number of Maj where one of the inputs is a constant. */
  uint32_t remaining_maj{0};

  void report() const
  {
    fmt::print( "#total_xor3 = {} / #total_maj = {} / #xor2 = {} / #xor3 = {} / #actual_maj = {} / #remaining_maj = {} \n", total_xor3, total_maj, actual_xor2, actual_xor3, actual_maj, remaining_maj );
  }
};

template<class Ntk>
uint32_t num_inverters( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );

  std::unordered_set<node<Ntk>> inverted_nodes;

  ntk.foreach_gate( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        inverted_nodes.insert( ntk.get_node( f ) );
      }
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_complemented( f ) )
    {
      inverted_nodes.insert( ntk.get_node( f ) );
    }
  } );

  return inverted_nodes.size();
}

/*! \brief Counts fanins which are primary inputs.
 *
 * \param ntk Network
 */

template<class Ntk>
uint32_t num_dangling_inputs( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );

  uint32_t costs{0u};

  ntk.foreach_gate( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_pi( ntk.get_node( f ) ) )
      {
        costs++;
      }
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_pi( ntk.get_node( f ) ) )
    {
      costs++;
    }
  } );

  return costs;
}

template<class Ntk>
void num_gate_profile( Ntk const& ntk, xmg_cost_params& xmg_ps )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );

  ntk.foreach_gate( [&]( auto const& node ) {
    bool has_const = false;
    ntk.foreach_fanin( node, [&]( auto const& f ) {
      // Check whether all of the fanin node are not constant
      if ( ntk.is_constant( ntk.get_node( f ) ) )
      {
        has_const = true;
        return false;
      }
      else
        return true;
    } ); // Foreach_fanin

    if ( ntk.is_maj( node ) )
    {
      if ( has_const )
        ++xmg_ps.remaining_maj;
      else
        ++xmg_ps.actual_maj;
    }
    else if ( ntk.is_xor3( node ) )
    {
      if ( has_const )
        ++xmg_ps.actual_xor2;
      else
        ++xmg_ps.actual_xor3;
    }
  } );

  xmg_ps.total_xor3 = xmg_ps.actual_xor2 + xmg_ps.actual_xor3;
  xmg_ps.total_maj = xmg_ps.remaining_maj + xmg_ps.actual_maj;
}

} // namespace mockturtle
