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
#include <vector>

#include "../traits.hpp"
#include "../networks/detail/foreach.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

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
  fanout_view( Ntk const& ntk ) : Ntk( ntk )
  {
  }
};

template<typename Ntk>
class fanout_view<Ntk, false> : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node    = typename Ntk::node;
  using signal  = typename Ntk::signal;

  fanout_view( Ntk const& ntk ) : Ntk( ntk ), _fanout( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_fanout();
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

  void resize_fanout()
  {
    _fanout.resize();
  }

  std::vector<node> fanout( node const& n ) const /* deprecated */
  {
    return _fanout[ n ];
  }

  void set_fanout( node const& n, std::vector<node> const& fanout )
  {
    _fanout[ n ] = fanout;
  }

  void add_fanout( node const& n, node const& p )
  {
    _fanout[ n ].emplace_back( p );
  }

  void remove_fanout( node const& n, node const& p )
  {
    auto &f = _fanout[ n ];
    f.erase( std::remove( f.begin(), f.end(), p ), f.end() );
  }

  void substitute_node_of_parents( std::vector<node> const& parents, node const& old_node, signal const& new_signal ) /* deprecated */
  {
    Ntk::substitute_node_of_parents( parents, old_node, new_signal );

    std::vector<node> old_node_fanout = _fanout[ old_node ];
    std::sort( old_node_fanout.begin(), old_node_fanout.end() );

    std::vector<node> parents_copy( parents );
    std::sort( parents_copy.begin(), parents_copy.end() );

    _fanout[ old_node ] = {};

    std::vector<node> intersection;
    std::set_intersection( parents_copy.begin(), parents_copy.end(), old_node_fanout.begin(), old_node_fanout.end(),
                           std::back_inserter( intersection ) );

    resize_fanout();
    set_fanout( this->get_node( new_signal ), intersection );
  }

private:
  void compute_fanout()
  {
    _fanout.reset();

    this->foreach_gate( [&]( auto const& n ){
        this->foreach_fanin( n, [&]( auto const& c ){
            auto& fanout = _fanout[ c ];
            if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
            {
              fanout.push_back( n );
            }
          });
      });
  }

  node_map<std::vector<node>, Ntk> _fanout;
};

template<class T>
fanout_view(T const&) -> fanout_view<T>;

} // namespace mockturtle
