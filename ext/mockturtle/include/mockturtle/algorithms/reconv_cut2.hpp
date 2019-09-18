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
  \file reconv_cut.hpp
  \brief Reconvergence-driven cut

  Based on `abcReconv.c`

  \author Heinz Riener
*/

#pragma once

#include "../traits.hpp"

#include <optional>
#include <cassert>

namespace mockturtle
{

template<typename Ntk>
struct cut_manager
{
  explicit cut_manager( int node_size_max, int node_fan_stop = 100000 )
    : node_size_max( node_size_max )
    , node_fan_stop( node_fan_stop )
  {
  }

  /* \brief limit on the size of the supernode */
  int node_size_max;

  /* \brief limit on the size of the supernode */
  int node_fan_stop;

  /* \brief fanins of the collapsed node (the cut) */
  std::vector<node<Ntk>> node_leaves;

  /* \brief visited nodes */
  std::vector<node<Ntk>> visited;
};

namespace detail
{

template<typename Ntk>
int node_get_leaf_cost_one( Ntk const& ntk, typename Ntk::node const &node, int fanin_limit )
{
  /* make sure the node is in the construction zone */
  assert( ntk.visited( node ) == ntk.trav_id() );

  /* cannot expand over the PI node */
  if ( ntk.is_constant( node ) || ntk.is_pi( node ) )
    return 999;

  /* get the cost of the cone */
  uint32_t cost = 0;
  ntk.foreach_fanin( node, [&]( const auto& f ){
      cost += ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() ) ? 0 : 1;
    } );

  /* always accept if the number of leaves does not increase */
  if ( cost < ntk.fanin_size( node ) )
    return cost;

  /* skip nodes with many fanouts */
  if ( int( ntk.fanout_size( node ) ) > fanin_limit )
    return 999;

  /* return the number of nodes that will be on the leaves if this node is removed */
  return cost;
}

template<typename Ntk>
bool node_build_cut_level_one_int( Ntk const& ntk, std::vector<typename Ntk::node>& visited, std::vector<typename Ntk::node>& leaves, uint64_t size_limit, int fanin_limit )
{
  uint32_t best_cost = 100;

  std::optional<typename Ntk::node> best_fanin;
  int best_pos;

  /* evaluate fanins of the cut */
  auto pos = 0;
  for ( const auto& l : leaves )
  {
    uint32_t cost_curr = node_get_leaf_cost_one( ntk, l, fanin_limit );
    if ( best_cost > cost_curr ||
         ( best_cost == cost_curr && best_fanin && ntk.level( l ) > ntk.level( *best_fanin ) ) )
    {
      best_cost = cost_curr;
      best_fanin = std::make_optional( l );
      best_pos = pos;
    }

    if ( best_cost == 0 )
      break;

    ++pos;
  }

  if ( !best_fanin )
    return false;

  // assert( best_cost < max_fanin_of_graph_structure );
  if ( leaves.size() - 1 + best_cost > size_limit )
      return false;

  /* remove the best node from the array */
  leaves.erase( leaves.begin() + best_pos );

  /* add the fanins of best to leaves and visited */
  ntk.foreach_fanin( *best_fanin, [&]( const auto& f ){
      auto const& n = ntk.get_node( f );
      if ( n != 0 && ( ntk.visited( n ) != ntk.trav_id() ) )
      {
        ntk.set_visited( n, ntk.trav_id() );
        visited.push_back( n );
        leaves.push_back( n );
      }
    });

  assert( leaves.size() <= size_limit );

  return true;
}

template<class Ntk>
std::vector<typename Ntk::node> node_find_cut( cut_manager<Ntk>& mgr, Ntk const& ntk, typename Ntk::node const& root )
{
  ntk.incr_trav_id();

  /* start the visited nodes and mark them */
  mgr.visited.clear();
  mgr.visited.push_back( root );
  ntk.set_visited( root, 1 );
  ntk.foreach_fanin( root, [&]( const auto& f ){
      auto const& n = ntk.get_node( f );
      if ( n == 0 ) return true;
      mgr.visited.push_back( n );
      ntk.set_visited( n, ntk.trav_id() );
      return true;
    } );

  /* start the cut */
  mgr.node_leaves.clear();
  ntk.foreach_fanin( root, [&]( const auto& f ){
      auto const& n = ntk.get_node( f );
      if ( n == 0 ) return true;
      mgr.node_leaves.push_back( n );
      return true;
    } );

  if ( mgr.node_leaves.size() > uint32_t( mgr.node_size_max ) )
  {
    /* special case: cut already overflows at the current node
       bc. the cut size limit is very low */
    mgr.node_leaves.clear();
    return {};
  }

  /* compute the cut */
  while ( node_build_cut_level_one_int( ntk, mgr.visited, mgr.node_leaves, mgr.node_size_max, mgr.node_fan_stop ) );
  assert( int( mgr.node_leaves.size() ) <= mgr.node_size_max );

  return mgr.node_leaves;
}

} /* namespace detail */

template<typename Ntk>
std::vector<node<Ntk>> reconv_driven_cut( cut_manager<Ntk>& mgr, Ntk const& ntk, node<Ntk> const& pivot )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the has_visited method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );

  return detail::node_find_cut( mgr, ntk, pivot );
}

} /* namespace mockturtle */
