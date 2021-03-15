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
  \file window_view.hpp
  \brief Implements an isolated view on a window in a network

  \author Heinz Riener
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <set>
#include <cassert>

#include "../traits.hpp"
#include "../networks/detail/foreach.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

/*! \brief Implements an isolated view on a window in a network. */
template<typename Ntk>
class window_view : public immutable_view<Ntk>
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit window_view( Ntk const& ntk, std::vector<node> const& leaves, std::vector<node> const& pivots, bool auto_extend = true )
    : immutable_view<Ntk>( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );

    this->incr_trav_id();

    /* constants */
    add_node( this->get_node( this->get_constant( false ) ) );
    this->set_visited( this->get_node( this->get_constant( false ) ), this->trav_id() );
    if ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) )
    {
      add_node( this->get_node( this->get_constant( true ) ) );
      this->set_visited( this->get_node( this->get_constant( true ) ), this->trav_id() );
      ++_num_constants;
    }

    /* primary inputs */
    for ( auto const& leaf : leaves )
    {
      if ( this->visited( leaf ) == this->trav_id() )
        continue;
      this->set_visited( leaf, this->trav_id() );

      add_node( leaf );
      ++_num_leaves;
    }

    for ( auto const& p : pivots )
    {
      traverse( p );
    }

    if ( auto_extend )
    {
      extend( ntk );
    }

    add_roots( ntk );
  }

  inline auto size() const { return _nodes.size(); }
  inline auto num_pis() const { return _num_leaves; }
  inline auto num_pos() const { return _roots.size(); }
  inline auto num_gates() const { return _nodes.size() - _num_leaves - _num_constants; }

  inline auto node_to_index( const node& n ) const { return _node_to_index.at( n ); }
  inline auto index_to_node( uint32_t index ) const { return _nodes[index]; }

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
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _roots.begin(), _roots.end(), fn );
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

  uint32_t fanout_size( node const& n ) const
  {
    return _fanout_size.at( node_to_index( n ) );
  }

private:
  void add_node( node const& n )
  {
    _node_to_index[n] = static_cast<unsigned int>( _nodes.size() );
    _nodes.push_back( n );

    auto fanout_counter = 0;
    this->foreach_fanin( n, [&]( const auto& f ) {
        if ( std::find( _nodes.begin(), _nodes.end(), this->get_node( f ) ) != _nodes.end() )
        {
          fanout_counter++;
        }
      });
    _fanout_size.push_back( fanout_counter );
  }

  void traverse( node const& n )
  {
    if ( this->visited( n ) == this->trav_id() )
      return;
    this->set_visited( n, this->trav_id() );

    this->foreach_fanin( n, [&]( const auto& f ) {
      traverse( this->get_node( f ) );
    } );

    add_node( n );
  }

  void extend( Ntk const& ntk )
  {
    std::set<node> new_nodes;
    do
    {
      new_nodes.clear();
      for ( const auto& n : _nodes )
      {
        ntk.foreach_fanout( n, [&]( auto const& p ){
            /* skip node if it is already in _nodes */
            if ( std::find( _nodes.begin(), _nodes.end(), p ) != _nodes.end() ) return;

            auto all_children_in_nodes = true;
            ntk.foreach_fanin( p, [&]( auto const& s ){
                auto const& child = ntk.get_node( s );
                if ( std::find( _nodes.begin(), _nodes.end(), child ) == _nodes.end() )
                {
                  all_children_in_nodes = false;
                  return false;
                }
                return true;
              });

            if ( all_children_in_nodes )
            {
              assert( p != 0 );
              assert( !is_pi( p ) );
              new_nodes.insert( p );
            }
          });
      }

      for ( const auto& n : new_nodes )
      {
        add_node( n );
      }
    } while ( !new_nodes.empty() );
  }

  void add_roots( Ntk const& ntk )
  {
    /* compute po nodes */
    std::vector<node> pos;
    ntk.foreach_po( [&]( auto const& s ){
        pos.push_back( ntk.get_node( s ) );
      });

    /* compute window outputs */
    for ( const auto& n : _nodes )
    {
      // if ( ntk.is_constant( n ) || ntk.is_pi( n ) ) continue;

      if ( std::find( pos.begin(), pos.end(), n ) != pos.end() )
      {
        auto s = this->make_signal( n );
        if ( std::find( _roots.begin(), _roots.end(), s ) == _roots.end() )
        {
          _roots.push_back( s );
        }
        continue;
      }

      ntk.foreach_fanout( n, [&]( auto const& p ){
          if ( std::find( _nodes.begin(), _nodes.end(), p ) == _nodes.end() )
          {
            auto s = this->make_signal( n );
            if ( std::find( _roots.begin(), _roots.end(), s ) == _roots.end() )
            {
              _roots.push_back( s );
              return false;
            }
          }
          return true;
      });
    }
  }

public:
  unsigned _num_constants{1};
  unsigned _num_leaves{0};
  std::vector<node> _nodes;
  std::unordered_map<node, uint32_t> _node_to_index;
  std::vector<signal> _roots;
  std::vector<unsigned> _fanout_size;
};

} /* namespace mockturtle */
