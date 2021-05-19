/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
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
  \file aqfp_view.hpp
  \brief Constraints for AQFP technology

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "../networks/detail/foreach.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"
#include "mockturtle/networks/mig.hpp"
#include "mockturtle/views/depth_view.hpp"

#include <cstdint>
#include <stack>
#include <vector>
#include <cmath>
#include <algorithm>

namespace mockturtle
{

struct aqfp_view_params
{
  bool update_on_add{true};
  bool update_on_modified{true};
  bool update_on_delete{true};

  uint32_t splitter_capacity{4u};
  uint32_t max_splitter_levels{2u};
};

/*! \brief Implements `foreach_fanout`, `depth`, `level`
 * `num_buffers`, `num_splitter_levels` methods for MIG network.
 *
 * This view calculates the number of buffers (for path balancing) and 
 * splitters (for multi-fanout) after AQFP technology mapping from an MIG
 * network. The calculation is rather naive without much optimization such
 * as retiming, which can serve as an upper bound on the cost or as a
 * baseline for future works on buffer optimization to be compared to.
 * 
 * In AQFP technology, (1) MAJ gates can only have one fanout. If more than one
 * fanout is needed, a splitter has to be inserted in between, which also 
 * takes one clock cycle (counts toward the network depth). (2) All fanins of
 * a MAJ gate have to arrive at the same time (at the same level). If one
 * fanin path is shorter, buffers have to be inserted to balance it. 
 * Buffers and splitters are essentially the same component in this technology.
 *
 * Some assumptions are made: (1) PIs do not need to be balanced (they are 
 * always available). (2) POs count toward the fanout sizes and have to be
 * balanced.
 *
 * The number of fanouts of each buffer is restricted to `splitter_capacity`.
 * The additional depth of a node introduced by splitters is limited to at
 * most `max_splitter_levels`. These two parameters are defined in
 * `aqfp_view_params`. Following these restrictions, the maximum number of 
 * fanouts of a node in the original MIG network is limited to 
 * `pow(splitter_capacity, max_splitter_levels)`. To ensure this, one should
 * apply `fanout_limit_view` before `aqfp_view` to duplicate the nodes with
 * too many fanouts. The template parameter `CheckFanoutLimit` can be set to
 * `true` to check for this restriction.
 *
 * The network depth and the levels of each node are determined first by the 
 * number of fanouts and adding sufficient levels for splitters assuming all
 * fanouts are at the lowest possible level. This could be suboptimal when some
 * fanouts are only needed at higher levels. Then, the number of buffers (which
 * also serve as splitters) is counted in a way that signals are only splitted 
 * before the level where they are needed (i.e., sharing of buffers among
 * multiple fanouts is maximized).
 *
 * Updating the network with this view is supported (`substitute_node` and 
 * `create_po` are overwritten) but not advised because of efficiency concern.
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool CheckFanoutLimit = false>
class aqfp_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node    = typename Ntk::node;
  using signal  = typename Ntk::signal;

  struct node_depth
  {
    node_depth( aqfp_view* p ): aqfp( p ) {}
    uint32_t operator()( depth_view<Ntk, node_depth> const& ntk, node const& n ) const
    {
      (void)ntk;
      return aqfp->num_splitter_levels( n ) + 1u;
    }
    aqfp_view* aqfp;
  };

  aqfp_view( Ntk const& ntk, aqfp_view_params const& ps = {} )
   : Ntk( ntk ), _fanout( ntk ), _external_ref_count( ntk ), _ps( ps ), _max_fanout( std::pow( ps.splitter_capacity, ps.max_splitter_levels ) ), _node_depth( this ), _depth_view( ntk, _node_depth )
  {
    static_assert( !has_foreach_fanout_v<Ntk> && "Ntk already has fanout interfaces" );
    static_assert( !has_depth_v<Ntk> && !has_level_v<Ntk> && !has_update_levels_v<Ntk>, "Ntk already has depth interfaces" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    if constexpr ( !std::is_same<typename Ntk::base_type, mig_network>::value )
    {
      std::cerr << "[w] base_type of Ntk is not mig_network.\n";
    }

    update_fanout();

    if ( _ps.update_on_add )
    {
      Ntk::events().on_add.push_back( [this]( auto const& n ) {
        _fanout.resize();
        _external_ref_count.resize();
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
          on_update( Ntk::get_node( f ) );
        } );
        _depth_view.update_levels();
      } );
    }

    if ( _ps.update_on_delete )
    {
      Ntk::events().on_delete.push_back( [this]( auto const& n ) {
        _fanout[n].clear();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        } );
        _depth_view.update_levels();
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

  /*! \brief Additional depth caused by the splitters of node `n`. */
  uint32_t num_splitter_levels ( node const& n ) const
  {
    return std::ceil( std::log( fanout_size( n ) ) / std::log( _ps.splitter_capacity ) );
  }

  /*! \brief Level of node `n` itself. Not the highest level of its splitters */
  uint32_t level ( node const& n ) const
  {
    return _depth_view.level( n ) - num_splitter_levels( n );
  }

  /*! \brief Circuit depth */
  uint32_t depth() const
  {
    return _depth_view.depth();
  }

  /*! \brief Get the number of buffers/splitters in the whole circuit */
  uint32_t num_buffers() const
  {
    uint32_t count = 0u;
    this->foreach_gate( [&]( auto const& n ){
      count += num_buffers( n );
    });
    return count;
  }

  /*! \brief Get the number of buffers/splitters between `n` and all its fanouts */
  uint32_t num_buffers( node const& n ) const
  {
    if ( num_splitter_levels( n ) == 0u )
    {
      /* single fanout */
      if ( fanout_size( n ) > 0u )
      {
        assert( fanout_size( n ) == 1u );
        return _external_ref_count[n] > 0u ? depth() - level( n ) : level( _fanout[n][0] ) - level( n ) - 1u;
      }
      /* dangling */
      return 0u;
    }

    /* fanout sizes at each level (pair<level, size>) */
    std::vector<std::pair<uint32_t, uint32_t>> fanout_sizes;
    uint32_t nlevel = level( n ) + 1u;
    fanout_sizes.emplace_back( std::make_pair( nlevel, 0u ) );
    for ( auto fo : _fanout[n] )
    {
      assert( level( fo ) >= nlevel );
      if ( level( fo ) == nlevel )
      {
        fanout_sizes.back().second++;
      }
      else
      {
        nlevel = level( fo );
        fanout_sizes.emplace_back( std::make_pair( nlevel, 1u ) );
      }
    }
    if ( _external_ref_count[n] > 0u )
    {
      fanout_sizes.emplace_back( std::make_pair( depth() + 1u, _external_ref_count[n] ) );
    }
    assert( fanout_sizes.size() > 1u );
    assert( fanout_sizes[0].second == 0u );

    /* count buffers from the highest level */
    uint32_t count = 0u;
    for ( auto i = fanout_sizes.size() - 1; i > 0; --i )
    {
      auto l = fanout_sizes[i].first - 1;
      /* number of splitters needed in level `l` */
      auto s = num_splitters( fanout_sizes[i].second );
      count += s;

      if ( fanout_sizes[i-1].first == l )
      {
        fanout_sizes[i-1].second += s;
      }
      else /* there is no other fanouts in level `l` */
      {
        if ( s == 1 )
        {
          count += l - fanout_sizes[i-1].first;
          fanout_sizes[i-1].second++;
        }
        else
        {
          fanout_sizes.insert( fanout_sizes.begin() + i, std::make_pair( l, s ) );
          ++i;
        }
      }
    }
    assert( fanout_sizes[0].second == 1 );

    return count;
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
    compute_fanout();
  }

  uint32_t create_po( signal const& f, std::string const& name = std::string() )
  {
    auto const ret = Ntk::create_po( f, name );
    _external_ref_count[f]++;
    return ret;
  }

private:
  uint32_t fanout_size( node const& n ) const
  {
    return _fanout[n].size() + _external_ref_count[n];
  }

  /* Return the number of splitters needed in one level lower */
  uint32_t num_splitters( uint32_t const& num_fanouts ) const
  {
    return std::ceil( float( num_fanouts ) / float( _ps.splitter_capacity ) );
  }

  void compute_fanout()
  {
    _fanout.reset();
    _external_ref_count.reset();

    this->foreach_gate( [&]( auto const& n ){
        this->foreach_fanin( n, [&]( auto const& c ){
            auto& fanout = _fanout[c];
            if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
            {
              fanout.push_back( n );
            }
          });
      });

    this->foreach_po( [&]( auto const& f ){
        _external_ref_count[f]++;
      });

    _depth_view.update_levels();

    this->foreach_gate( [&]( auto const& n ){
        on_update( n );
      });
  }

  void on_update( node const& n )
  {
    if constexpr ( CheckFanoutLimit )
    {
      if ( fanout_size( n ) > _max_fanout )
      {
        std::cerr << "[e] node " << n << " has too many (" << fanout_size( n ) << ") fanouts!\n";
      }
    }
    /* sort the fanouts by their level */
    auto& fanout = _fanout[n];
    std::sort( fanout.begin(), fanout.end(), [&](node a, node b){
      return level( a ) < level( b );
    });
  }

private:
  node_map<std::vector<node>, Ntk> _fanout;
  node_map<uint32_t, Ntk> _external_ref_count;
  aqfp_view_params _ps;
  uint64_t _max_fanout;

  node_depth _node_depth;
  depth_view<Ntk, node_depth> _depth_view;
};

template<class T>
aqfp_view( T const&, aqfp_view_params const& ps = {} ) -> aqfp_view<T>;

} // namespace mockturtle