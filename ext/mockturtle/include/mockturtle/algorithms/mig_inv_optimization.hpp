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
  \file mig_inv_optimization.hpp
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

/*! \brief Statistics for mig_inv_optimization. */
struct mig_inv_optimization_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Number of one level inverted nodes. */
  int num_inverted{ 0 };

  /*! \brief Number of two level inverted nodes. */
  int num_two_level_inverted{ 0 };

  /*! \brief Total gain in terms of number of inverters. */
  int total_gain{ 0 };
};

namespace detail
{

template<class Ntk>
class mig_inv_optimization_impl
{
public:
  mig_inv_optimization_impl( Ntk& ntk, mig_inv_optimization_stats& st )
      : ntk( ntk ), st( st )
  {
  }

  void run()
  {
    stopwatch t( st.time_total );

    minimize();
  }

private:
  /*! \brief implements the inverter minimization algorithm */
  void minimize()
  {
    bool changed = true;
    while ( changed )
    {
      changed = false;
      ntk.foreach_gate( [&]( auto const& f ) {
        int _gain = gain( f );
        if ( _gain > 0 )
        {
          st.num_inverted++;
          st.total_gain += _gain;
          changed = true;
          invert_node( f );
        }
        else if ( two_level_gain( f ) > 0 )
        {
          st.num_two_level_inverted++;
          st.total_gain += _gain;
          changed = true;
          std::vector<node<Ntk>> _nodes_to_invert;
          ntk.foreach_fanout( f, [&]( auto const& parent ) {
            // convert each fanout if inverting it makes sense
            int _subgain = 0;
            _subgain += gain( parent );
            if ( is_complemented_parent( parent, f ) )
              // if the connection between f and parent is complemented, we counted the same gain twice which will not be inverted at all
              _subgain -= 2;
            else
              // if the connection between f and parent is not complemented, we counted the same negative gain twice which will not be inverted at all
              _subgain += 2;
            if ( _subgain > 0 )
            {
              st.total_gain += _subgain;
              _nodes_to_invert.push_back( parent );
            }
          } );
          invert_node( f );
          for ( auto const& n : _nodes_to_invert )
          {
            invert_node( n );
          }
        }
      } );
    }
  }

  /*! \brief calculates the decrease in the number of inverters if this node is inverted and all fanouts that is beneficial also inverted */
  int two_level_gain( node<Ntk> n )
  {
    int _gain = 0;
    _gain += gain( n );

    ntk.foreach_fanout( n, [&]( auto const& f ) {
      int _subgain = 0;
      _subgain += gain( f );
      if ( is_complemented_parent( f, n ) )
        // if the connection between f and parent is complemented, we counted the same gain twice which will not be inverted at all
        _subgain -= 2;
      else
        // if the connection between f and parent is not complemented, we counted the same negative gain twice which will not be inverted at all
        _subgain += 2;

      // convert each fanout if inverting it makes sense
      if ( _subgain > 0 )
      {
        _gain += _subgain;
      }
    } );

    return _gain;
  }

  /*! \brief calculates the decrease in the number of inverters if this node is inverted */
  int gain( node<Ntk> n )
  {
    if ( ntk.is_dead( n ) )
    {
      std::cerr << "node" << n << " is dead\n";
      return 0;
    }
    int _gain = 0;

    // count the inverted and non-inverted fanins
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_constant( ntk.get_node( f ) ) )
        return;
      update_gain_is_complemented( f, _gain );
    } );

    // count the inverted and non-inverted fanouts
    ntk.foreach_fanout( n, [&]( auto const& parent ) {
      if ( is_complemented_parent( parent, n ) )
        _gain++;
      else
        _gain--;
    } );

    // count the inverted and non-inverted POs
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.get_node( f ) == n )
        update_gain_is_complemented( f, _gain );
    } );
    return _gain;
  }

  /*! \brief increases the gain if signal f is complemented, decreases otherwise. */
  void update_gain_is_complemented( signal<Ntk> f, int& _gain )
  {
    if ( ntk.is_complemented( f ) )
      _gain++;
    else
      _gain--;
  }

  /*! \brief checks if parent is parent of child and returns if the connection is complemented. */
  bool is_complemented_parent( node<Ntk> parent, node<Ntk> child )
  {
    bool ret = false;
    bool changed = false;
    ntk.foreach_fanin( parent, [&]( auto const& f ) {
      if ( ntk.get_node( f ) == child )
      {
        changed = true;
        ret = ntk.is_complemented( f );
      }
    } );
    if ( !changed )
    {
      std::cerr << "parent " << parent << " is not parent of child " << child << "\n";
    }
    return ret;
  }

  /*! \brief inverts the inputs and changes all occurances of the node with the !inverted_node. */
  void invert_node( node<Ntk> n )
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
    ntk.substitute_node( n, new_node );
    ntk.replace_in_outputs( n, new_node );
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
  mig_inv_optimization_stats& st;
};

} // namespace detail

/*! \brief MIG inverter optimization.
 *
 * This algorithm tries to reduce the number
 * of inverters in a MIG network without
 * increasing the node number. It checks each
 * node for 1 level and 2 level optimization
 * opportunuties and inverts the node if it
 * decreases the number of inverted connections.
 * It does not count constant values as inverted
 * even if they are complemented in the graph.
 *
 * **Required network functions:**
 * 'foreach_fanin'
 * 'foreach_fanout'
 * 'substitute_node'
 * 'replace_in_outputs'
 * 'is_complemented'
 * 'get_node'
 * 'is_dead'
 * 'is_constant'
 * 'foreach_gate'
 */
template<class Ntk>
void mig_inv_optimization( Ntk& ntk, mig_inv_optimization_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_replace_in_outputs_v<Ntk>, "Ntk does not implement the replace_in_outputs method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );

  mig_inv_optimization_stats st;
  detail::mig_inv_optimization_impl<Ntk> p( ntk, st );
  p.run();

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */