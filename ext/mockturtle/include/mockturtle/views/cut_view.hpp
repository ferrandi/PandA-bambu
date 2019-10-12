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
  \file cut_view.hpp
  \brief Implements an isolated view on a single cut in a network

  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "immutable_view.hpp"

#include <sparsepp/spp.h>

namespace mockturtle
{

/*! \brief Implements an isolated view on a single cut in a network.
 *
 * This view can create a network from a single cut in a larget network.  This
 * cut has a single output `root` and set of `leaves`.  The view reimplements
 * the methods `size`, `num_pis`, `num_pos`, `foreach_pi`, `foreach_po`,
 * `foreach_node`, `foreach_gate`, `is_pi`, `node_to_index`, and
 * `index_to_node`.
 *
 * This view assumes that all nodes' visited flags are set 0 before creating
 * the view.  The view guarantees that all the nodes in the view will have a 0
 * visited flag after the construction.
 *
 * **Required network functions:**
 * - `set_visited`
 * - `visited`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `make_signal`
 */
template<typename Ntk>
class cut_view : public immutable_view<Ntk>
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr bool is_topologically_sorted = true;

public:
  explicit cut_view( Ntk const& ntk, std::vector<node> const& leaves, node const& root )
      : immutable_view<Ntk>( ntk ), _root( root )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

    /* constants */
    add_constants();

    /* primary inputs */
    for ( auto const& leaf : leaves )
    {
      add_leaf( leaf );
    }

    traverse( root );

    /* restore visited */
    for ( auto const& n : _nodes )
    {
      this->set_visited( n, 0 );
    }
  }

  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  explicit cut_view( Ntk const& ntk, std::vector<signal> const& leaves, node const& root )
      : immutable_view<Ntk>( ntk ), _root( root )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

    /* constants */
    add_constants();

    /* primary inputs */
    for ( auto const& f : leaves )
    {
      const auto leaf = this->get_node( f );
      add_leaf( leaf );
    }

    traverse( root );

    /* restore visited */
    for ( auto const& n : _nodes )
    {
      this->set_visited( n, 0 );
    }
  }

  inline auto size() const { return _nodes.size(); }
  inline auto num_pis() const { return _num_leaves; }
  inline auto num_pos() const { return 1; }
  inline auto num_gates() const { return _nodes.size() - _num_leaves - _num_constants; }

  inline auto node_to_index( const node& n ) const { return _node_to_index.at( n ); }
  inline auto index_to_node( uint32_t index ) const { return _nodes[index]; }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    std::vector<signal> signals( 1, this->make_signal( _root ) );
    detail::foreach_element( signals.begin(), signals.end(), fn );
  }

  inline bool is_pi( node const& pi ) const
  {
    const auto beg = _nodes.begin() + _num_constants;
    return std::find( beg, beg + _num_leaves, pi ) != beg + _num_leaves;
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin() + _num_constants, _nodes.begin() + _num_constants + _num_leaves, fn );
  }

  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin(), _nodes.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin() + _num_constants + _num_leaves, _nodes.end(), fn );
  }

private:
  inline void add_constants()
  {
    add_node( this->get_node( this->get_constant( false ) ) );
    this->set_visited( this->get_node( this->get_constant( false ) ), 1 );
    if ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) )
    {
      add_node( this->get_node( this->get_constant( true ) ) );
      this->set_visited( this->get_node( this->get_constant( true ) ), 1 );
      ++_num_constants;
    }
  }

  inline void add_leaf( node const& leaf )
  {
    if ( this->visited( leaf ) == 1 )
      return;

    add_node( leaf );
    this->set_visited( leaf, 1 );
    ++_num_leaves;
  }

  inline void add_node( node const& n )
  {
    _node_to_index[n] = static_cast<uint32_t>( _nodes.size() );
    _nodes.push_back( n );
  }

  void traverse( node const& n )
  {
    if ( this->visited( n ) == 1 )
      return;

    this->foreach_fanin( n, [&]( const auto& f ) {
      traverse( this->get_node( f ) );
    } );

    add_node( n );
    this->set_visited( n, 1 );
  }

public:
  unsigned _num_constants{1};
  unsigned _num_leaves{0};
  std::vector<node> _nodes;
  spp::sparse_hash_map<node, uint32_t> _node_to_index;
  node _root;
};

template<class T>
cut_view(T const&, std::vector<node<T>> const&, node<T> const&) -> cut_view<T>;

template<class T, typename = std::enable_if_t<!std::is_same_v<typename T::signal, typename T::node>>>
cut_view(T const&, std::vector<signal<T>> const&, node<T> const&) -> cut_view<T>;

} /* namespace mockturtle */
