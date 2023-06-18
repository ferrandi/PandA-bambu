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
  \file mig_inv_propagation.hpp
  \brief MIG inverter optimization

  \author Bugra Eryilmaz
*/

#pragma once

#include "../networks/storage.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/fanout_view.hpp"

#include <iostream>
#include <optional>

namespace mockturtle
{

/*! \brief Statistics for mig_inv_propagation. */
struct mig_inv_propagation_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Increase in the node count. */
  int node_incraese{ 0 };

  /*! \brief Total gain in terms of number of inverters. */
  int total_gain{ 0 };
};

namespace detail
{

template<class Ntk>
class mig_inv_propagation_impl
{
public:
  mig_inv_propagation_impl( Ntk& ntk, mig_inv_propagation_stats& st )
      : ntk( ntk ), st( st )
  {
  }

  void run()
  {
    stopwatch t( st.time_total );

    propagate();
  }

private:
  /*! \brief implements the inverter propagation algorithm */
  void propagate()
  {
    // starting from primary outputs, propagate the inversions
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        // if it is complemented, invert the node
        auto old_node = ntk.get_node( f );
        auto new_node = invert_node( old_node );

        // replace the po with the inverted node
        ntk.replace_in_outputs( old_node, new_node );

        // check if the old node should stay alive
        if ( ntk.fanout_size( old_node ) == 0 )
        {
          ntk.take_out_node( old_node );
        }

        // propagate the inversions to the inputs
        propagate_helper( ntk.get_node( new_node ) );
      }
      else
      {
        // propagate the inversions to the inputs
        propagate_helper( ntk.get_node( f ) );
      }
    } );
  }

  void propagate_helper( node<Ntk> n )
  {
    std::vector<node<Ntk>> complement_list;
    // for each fanin, check if it is complemented
    ntk.foreach_fanin( n, [&]( auto const& f, auto idx ) {
      // skip if it is a constant, PI
      if ( ntk.is_constant( ntk.get_node( f ) ) || ntk.is_pi( ntk.get_node( f ) ) )
        return;
      // there should not be a dead child
      if ( ntk.is_dead( ntk.get_node( f ) ) )
        std::cerr << "node " << ntk.get_node( f ) << " is dead\n";

      // lazy substitute node since child order is fixed and
      // changing one child will mix the order and create a bug
      // in the foreach_fanin loop
      if ( ntk.is_complemented( f ) )
      {
        complement_list.push_back( ntk.get_node( f ) );
      }
      else
      {
        // propagate the inversions to the inputs
        propagate_helper( ntk.get_node( f ) );
      }
    } );

    // lazy invert the complemented fanins
    for ( auto const& f : complement_list )
    {
      // for each complemented fanin, invert the node
      auto new_node = invert_node( f );
      // replace the fanin with the inverted node
      if ( auto simplification = ntk.replace_in_node( n, f, new_node ) )
        ntk.substitute_node( simplification->first, simplification->second );

      // check if the old node should stay alive
      if ( ntk.fanout_size( f ) == 0 )
      {
        ntk.take_out_node( f );
      }

      // propagate the inversions to the inputs
      propagate_helper( ntk.get_node( new_node ) );
    }
  }

  /*! \brief gets maj(a,b,c) returns !maj(!a,!b,!c). */
  signal<Ntk> invert_node( node<Ntk> n )
  {
    signal<Ntk> a, b, c;
    ntk.foreach_fanin( n, [&]( auto const& f, auto idx ) {
      if ( idx == 0 )
        a = f;
      else if ( idx == 1 )
        b = f;
      else if ( idx == 2 )
        c = f;
    } );
    signal<Ntk> new_node = !create_maj_directly( !a, !b, !c );
    return new_node;
  }

  /*! \brief original create_maj function was inverting the node
             if more than 2 of the inputs were inverted which is
             not suitable for the algorithm, so I removed that part. */
  signal<Ntk> create_maj_directly( signal<Ntk> a, signal<Ntk> b, signal<Ntk> c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }
    else
    {
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }

    std::shared_ptr<storage<regular_node<3, 2, 1>>>::element_type::node_type nd;
    nd.children[0] = a;
    nd.children[1] = b;
    nd.children[2] = c;

    /* structural hashing */
    const auto it = ntk._storage->hash.find( nd );
    if ( it != ntk._storage->hash.end() )
    {
      return { it->second, 0 };
    }

    const auto index = ntk._storage->nodes.size();

    if ( index >= .9 * ntk._storage->nodes.capacity() )
    {
      ntk._storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      ntk._storage->hash.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    ntk._storage->nodes.push_back( nd );

    ntk._storage->hash[nd] = index;

    /* increase ref-count to children */
    ntk._storage->nodes[a.index].data[0].h1++;
    ntk._storage->nodes[b.index].data[0].h1++;
    ntk._storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : ntk._events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

private:
  Ntk& ntk;
  mig_inv_propagation_stats& st;
};

} // namespace detail

/*! \brief MIG inverter propagation.
 *
 * This algorithm tries to push all
 * the inverters to the inputs.
 * However, it can increase the number
 * of nodes while doing so.
 *
 * **Required network functions:**
 * get_node
 * substitute_node
 * take_out_node
 * foreach_fanin
 * replace_in_node
 * fanout_size
 * is_complemented
 * is_dead
 * is_constant
 * is_pi
 * foreach_po
 */
template<class Ntk>
void mig_inv_propagation( Ntk& ntk, mig_inv_propagation_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_take_out_node_v<Ntk>, "Ntk does not implement the take_out_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_replace_in_node_v<Ntk>, "Ntk does not implement the replace_in_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );

  mig_inv_propagation_stats st;
  detail::mig_inv_propagation_impl<Ntk> p( ntk, st );
  p.run();

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */