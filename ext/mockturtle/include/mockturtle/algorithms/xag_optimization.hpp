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
  \file xag_constant_fanin_optimization.hpp
  \brief Finds constant transitive linear fanin to AND gates

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <string>

#include "cleanup.hpp"
#include "dont_cares.hpp"
#include "../networks/xag.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

namespace detail
{

class xag_constant_fanin_optimization_impl
{
public:
  xag_constant_fanin_optimization_impl( xag_network const& xag )
      : xag( xag ),
        old2new( xag ),
        lfi( xag )
  {
  }

  xag_network run()
  {
    xag_network dest;

    old2new[xag.get_node( xag.get_constant( false ) )] = dest.get_constant( false );
    if ( xag.get_node( xag.get_constant( true ) ) != xag.get_node( xag.get_constant( true ) ) )
    {
      old2new[xag.get_node( xag.get_constant( true ) )] = dest.get_constant( true );
    }
    xag.foreach_pi( [&]( auto const& n ) {
      old2new[n] = dest.create_pi();
    } );
    topo_view{xag}.foreach_node( [&]( auto const& n ) {
      if ( xag.is_constant( n ) || xag.is_pi( n ) )
        return;

      if ( xag.is_xor( n ) )
      {
        std::vector<xag_network::signal> children;
        xag.foreach_fanin( n, [&]( auto const& f ) {
          children.push_back( old2new[xag.get_node( f )] );
        } );
        old2new[n] = dest.create_xor( children[0], children[1] );
      }
      else /* is AND */
      {
        // 1st bool is true, if LFI is empty
        // 2nd bool is complement flag of child
        // 3rd is corresponding dest child
        std::array<std::tuple<bool, bool, xag_network::signal>, 2> lfi_info;
        xag.foreach_fanin( n, [&]( auto const& f, auto i ) {
          lfi_info[i] = {compute_lfi( xag.get_node( f ) ).empty(), xag.is_complemented( f ), old2new[xag.get_node( f )] ^ xag.is_complemented(f)};
        } );

        if ( std::get<0>( lfi_info[0] ) )
        {
          old2new[n] = std::get<1>( lfi_info[0] ) ? std::get<2>( lfi_info[1] ) : dest.get_constant( false );
        }
        else if ( std::get<0>( lfi_info[1] ) )
        {
          old2new[n] = std::get<1>( lfi_info[1] ) ? std::get<2>( lfi_info[0] ) : dest.get_constant( false );
        }
        else
        {
          old2new[n] = dest.create_and( std::get<2>( lfi_info[0] ), std::get<2>( lfi_info[1] ) );
        }
      }
    } );

    xag.foreach_po( [&]( auto const& f ) {
      dest.create_po( old2new[xag.get_node( f )] ^ xag.is_complemented( f ) );
    } );

    return cleanup_dangling( dest );
  }

private:
  std::vector<xag_network::node> const& compute_lfi( xag_network::node const& n )
  {
    if ( lfi.has( n ) )
    {
      return lfi[n];
    }

    assert( !xag.is_constant( n ) );

    if ( xag.is_pi( n ) || xag.is_and( n ) )
    {
      return lfi[n] = {n};
    }

    // TODO generalize for n-ary XOR
    assert( xag.is_xor( n ) && xag.fanin_size( n ) == 2u );
    std::array<std::vector<xag_network::node>, 2> child_lfi;
    xag.foreach_fanin( n, [&]( auto const& f, auto i ) {
      child_lfi[i] = compute_lfi( xag.get_node( f ) );
    } );

    // merge LFIs
    std::vector<xag_network::node> node_lfi;
    auto it1 = child_lfi[0].begin();
    auto it2 = child_lfi[1].begin();
    while ( it1 != child_lfi[0].end() && it2 != child_lfi[1].end() )
    {
      if ( *it1 < *it2 )
      {
        node_lfi.push_back( *it1++ );
      }
      else if ( *it2 < *it1 )
      {
        node_lfi.push_back( *it2++ );
      }
      else
      {
        ++it1;
        ++it2;
      }
    }
    std::copy( it1, child_lfi[0].end(), std::back_inserter( node_lfi ) );
    std::copy( it2, child_lfi[1].end(), std::back_inserter( node_lfi ) );

    return lfi[n] = node_lfi;
  }

private:
  xag_network const& xag;
  node_map<xag_network::signal, xag_network> old2new;
  unordered_node_map<std::vector<xag_network::node>, xag_network> lfi;
};

}

/*! \brief Optimizes some AND gates by computing transitive linear fanin
 *
 * This function reevaluates the transitive linear fanin for each AND gate.
 * This is a subnetwork composed of all immediate XOR gates in the transitive
 * fanin cone until primary inputs or AND gates are reached.  This linear
 * transitive fanin might be constant for some fanin due to the cancellation
 * property of the XOR operation.  In such cases the AND gate can be replaced
 * by a constant or a fanin.
 */
xag_network xag_constant_fanin_optimization( xag_network const& xag )
{
  return detail::xag_constant_fanin_optimization_impl( xag ).run();
}

/*! \brief Optimizes some AND gates using satisfiability don't cares
 *
 * If an AND gate is satisfiability don't care for assignment 00, it can be
 * replaced by an XNOR gate, therefore reducing the multiplicative complexity.
 */
xag_network xag_dont_cares_optimization( xag_network const& xag )
{
  node_map<xag_network::signal, xag_network> old_to_new( xag );

  xag_network dest;
  old_to_new[xag.get_constant( false )] = dest.get_constant( false );

  xag.foreach_pi( [&]( auto const& n ) {
    old_to_new[n] = dest.create_pi();
  } );

  satisfiability_dont_cares_checker<xag_network> checker( xag );

  topo_view<xag_network>{xag}.foreach_node( [&]( auto const& n ) {
    if ( xag.is_constant( n ) || xag.is_pi( n ) ) return;

    std::array<xag_network::signal, 2> fanin;
    xag.foreach_fanin( n, [&]( auto const& f, auto i ) {
      fanin[i] = old_to_new[f] ^ xag.is_complemented( f );
    } );

    if ( xag.is_and( n ) )
    {
      if ( checker.is_dont_care( n, {false, false} ) )
      {
        old_to_new[n] = dest.create_xnor( fanin[0], fanin[1] );
      }
      else
      {
        old_to_new[n] = dest.create_and( fanin[0], fanin[1] );
      }
    }
    else /* is XOR */
    {
      old_to_new[n] = dest.create_xor( fanin[0], fanin[1] );
    }
  } );

  xag.foreach_po( [&]( auto const& f ) {
    dest.create_po( old_to_new[f] ^ xag.is_complemented( f ) );
  });

  return dest;
}

} /* namespace mockturtle */
