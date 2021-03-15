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
  \file fanout_view.hpp
  \brief Implements fanout for a network

  \author Heinz Riener
*/

#pragma once

#include <cstdint>
#include <stack>
#include <vector>

#include "../traits.hpp"
#include "../networks/detail/foreach.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

struct fanout_view_params
{
  bool update_on_add{true};
  bool update_on_modified{true};
  bool update_on_delete{true};
};

/*! \brief Implements `foreach_fanout` methods for networks.
 *
 * This view computes the fanout of each node of the network.
 * It implements the network interface method `foreach_fanout`.  The
 * fanout are computed at construction and can be recomputed by
 * calling the `update_fanout` method.
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool has_fanout_interface = has_foreach_fanout_v<Ntk>>
class fanout_view
{
};

template<typename Ntk>
class fanout_view<Ntk, true> : public Ntk
{
public:
  fanout_view( Ntk const& ntk, fanout_view_params const& ps = {} ) : Ntk( ntk )
  {
    (void)ps;
  }
};

template<typename Ntk>
class fanout_view<Ntk, false> : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node    = typename Ntk::node;
  using signal  = typename Ntk::signal;

  explicit fanout_view( fanout_view_params const& ps = {} )
    : Ntk()
    , _fanout( *this )
    , _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_fanout();

    if ( _ps.update_on_add )
    {
      Ntk::events().on_add.push_back( [this]( auto const& n ) {
        _fanout.resize();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_modified )
    {
      Ntk::events().on_modified.push_back( [this]( auto const& n, auto const& previous ) {
        (void)previous;
        for ( auto const& f : previous ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        }
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_delete )
    {
      Ntk::events().on_delete.push_back( [this]( auto const& n ) {
        _fanout[n].clear();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        } );
      } );
    }
  }

  explicit fanout_view( Ntk const& ntk, fanout_view_params const& ps = {} )
    : Ntk( ntk )
    , _fanout( ntk )
    , _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_fanout();

    if ( _ps.update_on_add )
    {
      Ntk::events().on_add.push_back( [this]( auto const& n ) {
        _fanout.resize();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_modified )
    {
      Ntk::events().on_modified.push_back( [this]( auto const& n, auto const& previous ) {
        (void)previous;
        for ( auto const& f : previous ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        }
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_delete )
    {
      Ntk::events().on_delete.push_back( [this]( auto const& n ) {
        _fanout[n].clear();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        } );
      } );
    }
  }

  template<typename Fn>
  void foreach_fanout( node const& n, Fn&& fn ) const
  {
    assert( n < this->size() );
    detail::foreach_element( _fanout[n].begin(), _fanout[n].end(), fn );
  }

  void update_fanout()
  {
    compute_fanout();
  }

  std::vector<node> fanout( node const& n ) const /* deprecated */
  {
    return _fanout[n];
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( {old_node, new_signal} );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      const auto parents = _fanout[_old];
      for ( auto n : parents )
      {
        if ( const auto repl = Ntk::replace_in_node( n, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      Ntk::replace_in_outputs( _old, _new );

      /* reset fan-in of old node */
      Ntk::take_out_node( _old );
    }
  }

private:
  void compute_fanout()
  {
    _fanout.reset();

    this->foreach_gate( [&]( auto const& n ){
        this->foreach_fanin( n, [&]( auto const& c ){
            auto& fanout = _fanout[c];
            if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
            {
              fanout.push_back( n );
            }
          });
      });
  }

  node_map<std::vector<node>, Ntk> _fanout;
  fanout_view_params _ps;
};

template<class T>
fanout_view( T const&, fanout_view_params const& ps = {} ) -> fanout_view<T>;

} // namespace mockturtle
