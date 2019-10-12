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
  \file cleanup.hpp
  \brief Cleans up networks

  \author Mathias Soeken
*/

#pragma once

#include <iostream>
#include <type_traits>
#include <vector>

#include <kitty/operations.hpp>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

template<typename NtkSource, typename NtkDest, typename LeavesIterator>
std::vector<signal<NtkDest>> cleanup_dangling( NtkSource const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end )
{
  (void)end;

  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );
  static_assert( has_foreach_po_v<NtkSource>, "NtkSource does not implement the foreach_po method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_clone_node_v<NtkDest>, "NtkDest does not implement the clone_node method" );

  node_map<signal<NtkDest>, NtkSource> old_to_new( ntk );
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );

  if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
  {
    old_to_new[ntk.get_constant( true )] = dest.get_constant( true );
  }

  /* create inputs in same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  assert( it == end );

  /* foreach node in topological order */
  topo_view topo{ntk};
  topo.foreach_node( [&]( auto node ) {
    if ( ntk.is_constant( node ) || ntk.is_pi( node ) )
      return;

    /* collect children */
    std::vector<signal<NtkDest>> children;
    ntk.foreach_fanin( node, [&]( auto child, auto ) {
      const auto f = old_to_new[child];
      if ( ntk.is_complemented( child ) )
      {
        children.push_back( dest.create_not( f ) );
      }
      else
      {
        children.push_back( f );
      }
    } );
    if constexpr ( std::is_same_v<NtkSource, NtkDest> )
    {
      old_to_new[node] = dest.clone_node( ntk, node, children );
    }
    else
    {
      do
      {
        if constexpr ( has_is_and_v<NtkSource> )
        {
          static_assert( has_create_and_v<NtkDest>, "NtkDest cannot create AND gates" );
          if ( ntk.is_and( node ) )
          {
            old_to_new[node] = dest.create_and( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_or_v<NtkSource> )
        {
          static_assert( has_create_or_v<NtkDest>, "NtkDest cannot create OR gates" );
          if ( ntk.is_or( node ) )
          {
            old_to_new[node] = dest.create_or( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_xor_v<NtkSource> )
        {
          static_assert( has_create_xor_v<NtkDest>, "NtkDest cannot create XOR gates" );
          if ( ntk.is_xor( node ) )
          {
            old_to_new[node] = dest.create_xor( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_maj_v<NtkSource> )
        {
          static_assert( has_create_maj_v<NtkDest>, "NtkDest cannot create MAJ gates" );
          if ( ntk.is_maj( node ) )
          {
            old_to_new[node] = dest.create_maj( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_ite_v<NtkSource> )
        {
          static_assert( has_create_ite_v<NtkDest>, "NtkDest cannot create ITE gates" );
          if ( ntk.is_ite( node ) )
          {
            old_to_new[node] = dest.create_ite( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_xor3_v<NtkSource> )
        {
          static_assert( has_create_xor3_v<NtkDest>, "NtkDest cannot create XOR3 gates" );
          if ( ntk.is_xor3( node ) )
          {
            old_to_new[node] = dest.create_xor3( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_function_v<NtkSource> )
        {
          static_assert( has_create_node_v<NtkDest>, "NtkDest cannot create arbitrary function gates" );
          old_to_new[node] = dest.create_node( children, ntk.node_function( node ) );
          break;
        }
        std::cerr << "[e] something went wrong, could not copy node " << ntk.node_to_index( node ) << "\n";
      } while ( false );
    }
  } );

  /* create outputs in same order */
  std::vector<signal<NtkDest>> fs;
  ntk.foreach_po( [&]( auto po ) {
    const auto f = old_to_new[po];
    if ( ntk.is_complemented( po ) )
    {
      fs.push_back( dest.create_not( f ) );
    }
    else
    {
      fs.push_back( f );
    }
  } );

  return fs;
}

/*! \brief Cleans up dangling nodes.
 *
 * This method reconstructs a network and omits all dangling nodes.  The
 * network types of the source and destination network are the same.
 *
   \verbatim embed:rst

   .. note::

      This method returns the cleaned up network as a return value.  It does
      *not* modify the input network.
   \endverbatim
 *
 * **Required network functions:**
 * - `get_node`
 * - `node_to_index`
 * - `get_constant`
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `is_complemented`
 * - `foreach_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `clone_node`
 * - `is_pi`
 * - `is_constant`
 */
template<typename Ntk>
Ntk cleanup_dangling( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_clone_node_v<Ntk>, "Ntk does not implement the clone_node method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );

  Ntk dest;
  std::vector<signal<Ntk>> pis;
  ntk.foreach_pi( [&]( auto ) {
    pis.push_back( dest.create_pi() );
  } );

  for ( auto f : cleanup_dangling( ntk, dest, pis.begin(), pis.end() ) )
  {
    dest.create_po( f );
  }

  return dest;
}

/*! \brief Cleans up LUT nodes.
 *
 * This method reconstructs a LUT network and optimizes LUTs when they do not
 * depend on all their fanin, or when some of the fanin are constant inputs.
 *
 * Constant gate inputs will be propagated.
 *
   \verbatim embed:rst

   .. note::

      This method returns the cleaned up network as a return value.  It does
      *not* modify the input network.
   \endverbatim
 *
 * **Required network functions:**
 * - `get_node`
 * - `get_constant`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `create_pi`
 * - `create_po`
 * - `create_node`
 * - `create_not`
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `node_function`
 */
template<class Ntk>
Ntk cleanup_luts( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );
  static_assert( has_create_node_v<Ntk>, "Ntk does not implement the create_node method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

  Ntk dest;
  node_map<signal<Ntk>, Ntk> old_to_new( ntk );

  // PIs and constants
  ntk.foreach_pi( [&]( auto const& n ) {
    old_to_new[n] = dest.create_pi();
  } );
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
  {
    old_to_new[ntk.get_constant( true )] = dest.get_constant( true );
  }

  // iterate through nodes
  topo_view topo{ntk};
  topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) ) return true; /* continue */

    auto func = ntk.node_function( n );

    /* constant propagation */
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      if ( dest.is_constant( old_to_new[f] ) )
      {
        if ( dest.constant_value( old_to_new[f] ) != ntk.is_complemented( f ) )
        {
          kitty::cofactor1_inplace( func, i );
        }
        else
        {
          kitty::cofactor0_inplace( func, i );
        }
      }
    } );


    const auto support = kitty::min_base_inplace( func );
    auto new_func = kitty::shrink_to( func, support.size() );

    std::vector<signal<Ntk>> children;
    if ( auto var = support.begin(); var != support.end() )
    {
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
        if ( *var == i )
        {
          auto const& new_f = old_to_new[f];
          children.push_back( ntk.is_complemented( f ) ? dest.create_not( new_f ) : new_f );
          if ( ++var == support.end() )
          {
            return false;
          }
        }
        return true;
      } );
    }

    if ( new_func.num_vars() == 0u )
    {
      old_to_new[n] = dest.get_constant( !kitty::is_const0( new_func ) );
    }
    else if ( new_func.num_vars() == 1u )
    {
      old_to_new[n] = *( new_func.begin() ) == 0b10 ? children.front() : dest.create_not( children.front() );
    }
    else
    {
      old_to_new[n] = dest.create_node( children, new_func );
    }

    return true;
  } );

  // POs
  ntk.foreach_po( [&]( auto const& f ) {
    auto const& new_f = old_to_new[f];
    dest.create_po( ntk.is_complemented( f ) ? dest.create_not( new_f ) : new_f );
  });

  return dest;
}

} // namespace mockturtle
