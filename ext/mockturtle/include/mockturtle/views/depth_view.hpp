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
  \file depth_view.hpp
  \brief Implements depth and level for a network

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

/*! \brief Implements `depth` and `level` methods for networks.
 *
 * This view computes the level of each node and also the depth of
 * the network.  It implements the network interface methods
 * `level` and `depth`.  The levels are computed at construction
 * and can be recomputed by calling the `update_levels` method.
 *
 * **Required network functions:**
 * - `size`
 * - `get_node`
 * - `visited`
 * - `set_visited`
 * - `foreach_fanin`
 * - `foreach_po`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow
      aig_network aig = ...;

      // create a depth view on the network
      depth_view aig_depth{aig};

      // print depth
      std::cout << "Depth: " << aig_depth.depth() << "\n";
   \endverbatim
 */
template<typename Ntk, bool has_depth_interface = has_depth_v<Ntk>&& has_level_v<Ntk>&& has_update_levels_v<Ntk>>
class depth_view
{
};

template<typename Ntk>
class depth_view<Ntk, true> : public Ntk
{
public:
  depth_view( Ntk const& ntk ) : Ntk( ntk )
  {
  }
};

template<typename Ntk>
class depth_view<Ntk, false> : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /*! \brief Standard constructor.
   *
   * \param ntk Base network
   * \param count_complements Count inverters as 1
   */
  explicit depth_view( Ntk const& ntk, bool count_complements = false )
      : Ntk( ntk ),
        _count_complements( count_complements ),
        _levels( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_levels();
  }

  uint32_t depth() const
  {
    return _depth;
  }

  uint32_t level( node const& n ) const
  {
    return _levels[n];
  }

  void set_level( node const& n, uint32_t level )
  {
    _levels[n] = level;
  }

  void update_levels()
  {
    _levels.reset( 0 );

    this->incr_trav_id();
    compute_levels();
  }

  void resize_levels()
  {
    _levels.resize();
  }

private:
  uint32_t compute_levels( node const& n )
  {
    if ( this->visited( n ) == this->trav_id() )
    {
      return _levels[n];
    }
    this->set_visited( n, this->trav_id() );

    if ( this->is_constant( n ) || this->is_pi( n ) )
    {
      return _levels[n] = 0;
    }

    uint32_t level{0};
    this->foreach_fanin( n, [&]( auto const& f ) {
      auto clevel = compute_levels( this->get_node( f ) );
      if ( _count_complements && this->is_complemented( f ) )
      {
        clevel++;
      }
      level = std::max( level, clevel );
    } );

    return _levels[n] = level + 1;
  }

  void compute_levels()
  {
    _depth = 0;
    this->foreach_po( [&]( auto const& f ) {
      auto clevel = compute_levels( this->get_node( f ) );
      if ( _count_complements && this->is_complemented( f ) )
      {
        clevel++;
      }
      _depth = std::max( _depth, clevel );
    } );
  }

  bool _count_complements{false};
  node_map<uint32_t, Ntk> _levels;
  uint32_t _depth;
};

template<class T>
depth_view( T const& ) -> depth_view<T>;

template<class T>
depth_view( T const&, bool ) -> depth_view<T>;

} // namespace mockturtle
