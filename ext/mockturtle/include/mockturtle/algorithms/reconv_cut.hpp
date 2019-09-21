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

  \author Heinz Riener
*/

#pragma once

#include <algorithm>
#include <cassert>

#include "../traits.hpp"
#include "../utils/node_map.hpp"

namespace mockturtle
{

/*! \brief Parameters for reconvergence_driven_cut.
 *
 * The data structure `reconv_cut_params` holds configurable parameters
 * with default arguments for `reconv_cut`.
 */
struct reconv_cut_params
{
  /*! \brief Maximum number of leaves for a cut. */
  uint32_t cut_size{10u};
};

/*! \cond PRIVATE */
namespace detail
{

template<typename Ntk>
class compute_fanin_cut
{
public:
  explicit compute_fanin_cut( Ntk const& ntk, std::vector<node<Ntk>> const& pivots, reconv_cut_params const& ps )
      : _ntk( ntk ), _pivots( pivots ), _ps( ps ), _values( ntk )
  {
    assert( _pivots.size() > 0 );
  }

public:
  std::vector<node<Ntk>> run()
  {
    _values.reset();
    std::vector<node<Ntk>> cut( _pivots );
    for ( const auto& p : _pivots )
    {
      _values[p] = 1;
    }
    compute_cut_recur( cut );
    return cut;
  }

protected:
  void compute_cut_recur( std::vector<node<Ntk>>& cut )
  {
    assert( cut.size() <= _ps.cut_size && "cut-size overflow" );
    std::sort( cut.begin(), cut.end(), [this]( node<Ntk> const& a, node<Ntk> const& b ) { return cost( a ) < cost( b ); } );

    /* find the first non-pi node to extend the cut (because the vector is sorted, the non-pi is cost-minimal) */
    auto const it = std::find_if( cut.begin(), cut.end(), [&]( auto const& node ) { return !_ntk.is_pi( node ); } );
    if ( cut.end() == it )
    {
      /* if all nodes are pis, then the cut cannot be extended */
      return;
    }

    /* the cost is identical to the number of nodes added to the cut if *it is used to expand the cut */
    auto const c = cost( *it );
    if ( cut.size() + c > _ps.cut_size )
    {
      /* if the expansion exceeds the cut_size, then the cut cannot be extended */
      return;
    }

    /* otherwise expand the cut with the children of *it and mark *it visited (by setting a value) */
    const auto n = *it;
    cut.erase( it );
    _ntk.foreach_fanin( n, [&]( signal<Ntk> const& s ) {
      auto const& child = _ntk.get_node( s );
      if ( !_ntk.is_constant( child ) && std::find( cut.begin(), cut.end(), child ) == cut.end() && !_values[child] )
      {
        cut.push_back( child );
        _values[child] = 1;
      }
    } );

    assert( cut.size() <= _ps.cut_size );
    compute_cut_recur( cut );
  }

  inline int32_t cost( node<Ntk> const& n ) const
  {
    int32_t current_cost = -1;
    _ntk.foreach_fanin( n, [&]( signal<Ntk> const& s ) {
      auto const& child = _ntk.get_node( s );
      if ( !_ntk.is_constant( child ) && !_values[child] )
      {
        ++current_cost;
      }
    } );
    return current_cost;
  }

protected:
  Ntk const& _ntk;
  std::vector<node<Ntk>> _pivots;
  reconv_cut_params const& _ps;
  node_map<uint32_t, Ntk> _values;
};

template<typename Ntk>
class compute_fanout_cut
{
public:
  explicit compute_fanout_cut( Ntk const& ntk, std::vector<node<Ntk>> const& pivots, reconv_cut_params const& ps )
      : _ntk( ntk ), _pivots( pivots ), _ps( ps ), _values( ntk )
  {
    assert( _pivots.size() > 0 );
  }

public:
  std::vector<node<Ntk>> run()
  {
    _values.reset();
    std::vector<node<Ntk>> cut( _pivots );
    for ( const auto& p : _pivots )
    {
      _values[p] = 1;
    }
    compute_cut_recur( cut );
    return cut;
  }

protected:
  bool has_internal_fanout( node<Ntk> const& n )
  {
    bool has = false;
    _ntk.foreach_fanout( n, [&]( const auto& ) {
      has = true;
      return false;
    } );
    return has;
  }

  void compute_cut_recur( std::vector<node<Ntk>>& cut )
  {
    assert( cut.size() <= _ps.cut_size && "cut-size overflow" );
    std::sort( cut.begin(), cut.end(), [this]( node<Ntk> const& a, node<Ntk> const& b ) { return cost( a ) < cost( b ); } );

    /* find the first non-po node to extend the cut (because the vector is sorted, the non-po is cost-minimal) */
    auto const it = std::find_if( cut.begin(), cut.end(), [&]( auto const& node ) { return has_internal_fanout( node ); } );
    if ( cut.end() == it )
    {
      /* if all nodes are pos, then the cut cannot be extended */
      return;
    }

    /* the cost is identical to the number of nodes added to the cut if *it is used to expand the cut */
    auto const c = cost( *it );
    if ( cut.size() + c > _ps.cut_size )
    {
      /* if the expansion exceeds the cut_size, then the cut cannot be extended */
      return;
    }

    /* otherwise expand the cut with the parents of *it and mark *it visited (by setting a value) */
    const auto n = *it;
    cut.erase( it );
    _ntk.foreach_fanout( n, [&]( node<Ntk> const& parent ) {
      if ( std::find( cut.begin(), cut.end(), parent ) == cut.end() && !_values[parent] )
      {
        cut.push_back( parent );
        _values[parent] = 1;
      }
    } );

    assert( cut.size() <= _ps.cut_size );
    compute_cut_recur( cut );
  }

  inline int32_t cost( node<Ntk> const& n ) const
  {
    int32_t current_cost = -1;
    _ntk.foreach_fanout( n, [&]( node<Ntk> const& parent ) {
      if ( !_values[parent] )
      {
        ++current_cost;
      }
    } );
    return current_cost;
  }

protected:
  Ntk const& _ntk;
  std::vector<node<Ntk>> _pivots;
  reconv_cut_params const& _ps;
  node_map<uint32_t, Ntk> _values;
};

} /* namespace detail */
/*! \endcond */

/*! \brief Reconvergence-driven cuts towards inputs.
 *
 * This class implements a generation algorithm for
 * reconvergence-driven cuts.  The cut grows towards the primary
 * inputs starting from a pivot node.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `get_node`
 * - `foreach_fanin`
 *
 */
struct reconv_cut
{
public:
  explicit reconv_cut( reconv_cut_params const& ps = {} )
      : _ps( ps )
  {
  }

  template<typename Ntk>
  std::vector<node<Ntk>> operator()( Ntk const& ntk, node<Ntk> const& pivot )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the is_pi method" );

    detail::compute_fanin_cut cut_generator( ntk, {pivot}, _ps );

    return cut_generator.run();
  }

  template<typename Ntk>
  std::vector<node<Ntk>> operator()( Ntk const& ntk, std::vector<node<Ntk>> const& pivots )
  {
    assert( pivots.size() > 0u && "pivots must not be empty" );

    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the is_pi method" );

    detail::compute_fanin_cut cut_generator( ntk, pivots, _ps );

    return cut_generator.run();
  }

private:
  reconv_cut_params _ps;
};

/*! \brief Reconvergence-driven cuts towards outputs.
 *
 * This class implements a generation algorithm for
 * reconvergence-driven cuts.  The cut grows towards the primary
 * outputs starting from a pivot node.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `get_node`
 * - `foreach_fanout`
 *
 */
struct reconv_fanout_cut
{
public:
  explicit reconv_fanout_cut( reconv_cut_params const& ps = {} )
      : _ps( ps )
  {
  }

  template<typename Ntk>
  std::vector<node<Ntk>> operator()( Ntk const& ntk, node<Ntk> const& pivot )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );

    detail::compute_fanout_cut cut_generator( ntk, {pivot}, _ps );
    return cut_generator.run();
  }

  template<typename Ntk>
  std::vector<node<Ntk>> operator()( Ntk const& ntk, std::vector<node<Ntk>> const& pivots )
  {
    assert( pivots.size() > 0u && "pivots must not be empty" );

    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );

    detail::compute_fanout_cut cut_generator( ntk, pivots, _ps );
    return cut_generator.run();
  }

private:
  reconv_cut_params _ps;
};

} /* namespace mockturtle */
