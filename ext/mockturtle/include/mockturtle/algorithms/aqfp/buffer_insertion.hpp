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
  \file buffer_insertion.hpp
  \brief Insert buffers and splitters for the AQFP technology

  \author Siang-Yun (Sonia) Lee
  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../../traits.hpp"
#include "../../utils/node_map.hpp"
#include "../../views/fanout_view.hpp"
#include "../../views/topo_view.hpp"
#include "aqfp_assumptions.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace mockturtle
{

/*! \brief Parameters for (AQFP) buffer insertion.
 */
struct buffer_insertion_params
{
  /*! \brief Technology assumptions. */
  aqfp_assumptions assume;

  /*! \brief The scheduling strategy to get the initial depth assignment.
   * - `provided` = An initial level assignment is given in the constructor, thus
   * no scheduling is performed. It is the user's responsibility to ensure that
   * the provided assignment is legal.
   * - `ASAP` = Classical As-Soon-As-Possible scheduling
   * - `ASAP_depth` = As-Soon-As-Possible scheduling with depth optimality
   * - `ALAP` = ASAP (to obtain depth) followed by As-Late-As-Possible scheduling
   * - `ALAP_depth` = As-Late-As-Possible scheduling with depth optimality
   * - `better` = ASAP followed by ALAP, then count buffers for both assignments
   * and choose the better one
   * - `better_depth` = ALAP_depth followed by ASAP_depth, then count buffers for both assignments
   * and choose the better one
   */
  enum
  {
    provided,
    ASAP,
    ASAP_depth,
    ALAP,
    ALAP_depth,
    better,
    better_depth
  } scheduling = ASAP;

  /*! \brief The level of optimization effort.
   * - `none` = No optimization
   * - `one_pass` = Try to form a chunk starting from each gate, once
   * for all gates
   * - `until_sat` = Iterate over all gates until no more beneficial
   * chunk movement can be found
   * - `optimal` = Use an SMT solver to find the global optimal
   */
  enum
  {
    none,
    one_pass,
    until_sat,
    optimal
  } optimization_effort = none;

  /*! \brief The maximum size of a chunk. */
  uint32_t max_chunk_size{ 100u };
};

/*! \brief Insert buffers and splitters for the AQFP technology.
 *
 * In the AQFP technology, (1) logic gates can only have one fanout. If more than one
 * fanout is needed, a splitter has to be inserted in between, which also
 * takes one clocking phase (counted towards the network depth). (2) All fanins of
 * a logic gate have to arrive at the same time (be at the same level). If one
 * fanin path is shorter, buffers have to be inserted to balance it.
 * Buffers and splitters are essentially the same component in this technology.
 *
 * With a given level assignment to all gates in the network, the minimum number of
 * buffers needed is determined. This class implements algorithms to count such
 * "irredundant buffers" and to insert them to obtain a buffered network. Moreover,
 * as buffer optimization is essentially a problem of obtaining a good level assignment,
 * this class also implements algorithms to obtain an initial, legal assignment using
 * scheduling algorithms and to further adjust and optimize it.
 *
 * This class provides two easy-to-use top-level functions which wrap all the above steps
 * together: `run` and `dry_run`. In addition, the following interfaces are kept for
 * more fine-grained usage:
 * - Query the current level assignment (`level`, `depth`)
 * - Count irredundant buffers based on the current level assignment (`count_buffers`,
 * `num_buffers`)
 * - Optimize buffer count by scheduling (`schedule`, `ASAP`, `ALAP`) and by adjusting
 * the level assignment with chunked movement (`optimize`)
 * - Dump the resulting network into a network type which provides representation for
 * buffers (`dump_buffered_network`)
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      mig_network mig = ...

      buffer_insertion_params ps;
      ps.assume.branch_pis = true;
      ps.assume.balance_pis = false;
      ps.assume.balance_pos = true;
      ps.assume.splitter_capacity = 3u;
      ps.scheduling = buffer_insertion_params::ALAP;
      ps.optimization_effort = buffer_insertion_params::one_pass;

      buffer_insertion buffering( mig, ps );
      buffered_mig_network buffered_mig;
      auto const num_buffers = buffering.run( buffered_mig );

      std::cout << num_buffers << std::endl;
      assert( verify_aqfp_buffer( buffered_mig, ps.assume ) );
      write_verilog( buffered_mig, "buffered.v" );
   \endverbatim
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_gate`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_pi`
 * - `is_constant`
 * - `get_node`
 * - `fanout_size`
 * - `size`
 * - `set_visited`
 * - `set_value`
 *
 */
template<typename Ntk>
class buffer_insertion
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit buffer_insertion( Ntk const& ntk, buffer_insertion_params const& ps = {} )
      : _ntk( ntk ), _ps( ps ), _levels( _ntk ), _timeframes( _ntk ), _fanouts( _ntk ), _external_ref_count( _ntk ), _external_ref_count_neg( _ntk ), _num_buffers( _ntk ), _min_level( _ntk ), _max_level( _ntk )
  {
    static_assert( !is_buffered_network_type_v<Ntk>, "Ntk is already buffered" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );

    assert( !( _ps.assume.balance_pis && !_ps.assume.branch_pis ) && "Does not make sense to balance but not branch PIs" );
    assert( _ps.scheduling != buffer_insertion_params::provided );
    initialize_external_ref_counts();
  }

  explicit buffer_insertion( Ntk const& ntk, node_map<uint32_t, Ntk> const& levels, buffer_insertion_params const& ps = {} )
      : _ntk( ntk ), _ps( ps ), _levels( levels ), _timeframes( _ntk ), _fanouts( _ntk ), _external_ref_count( _ntk ), _external_ref_count_neg( _ntk ), _num_buffers( _ntk ), _min_level( _ntk ), _max_level( _ntk )
  {
    static_assert( !is_buffered_network_type_v<Ntk>, "Ntk is already buffered" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );

    assert( !( _ps.assume.balance_pis && !_ps.assume.branch_pis ) && "Does not make sense to balance but not branch PIs" );
    assert( _ps.scheduling == buffer_insertion_params::provided );
    initialize_external_ref_counts();
  }

  /*! \brief Insert buffers and obtain a buffered network.
   * \param bufntk An empty network of an appropriate buffered network type to
   * to store the buffer-insertion result
   * \param pLevels A pointer to a node map which will store the resulting
   * level assignment
   * \return The number of buffers in the resulting network
   */
  template<class BufNtk>
  uint32_t run( BufNtk& bufntk, node_map<uint32_t, Ntk>* pLevels = nullptr )
  {
    dry_run( pLevels );
    dump_buffered_network( bufntk );
    return num_buffers();
  }

  /*! \brief Count the number of buffers without dumping the result into a buffered network.
   *
   * This function saves some runtime for dumping the resulting network and
   * allows users to experiment on the algorithms with new network types whose
   * corresponding buffered_network are not implemented yet.
   *
   * \param pLevels A pointer to a node map which will store the resulting
   * level assignment
   * \return The number of buffers in the resulting network
   */
  uint32_t dry_run( node_map<uint32_t, Ntk>* pLevels = nullptr )
  {
    schedule();
    optimize();
    count_buffers();

    if ( pLevels )
      *pLevels = _levels;

    return num_buffers();
  }

#pragma region Query
  /*! \brief Level of node `n` considering buffer/splitter insertion. */
  uint32_t level( node const& n ) const
  {
    assert( n < _ntk.size() );
    return _levels[n];
  }

  /*! \brief Network depth considering AQFP buffers/splitters.
   *
   * Note that when neither PIs nor POs are balanced, there can be
   * different schedulings for the same buffered network (i.e. having
   * the same number of buffers), thus this number may be different
   * from the depth obtained by dumping the buffered network and wrapping
   * depth_view around it.
   */
  uint32_t depth() const
  {
    return _depth;
  }

  /*! \brief The total number of buffers in the network under the current
   * level assignment. */
  uint32_t num_buffers() const
  {
    assert( !_outdated && "Please call `count_buffers()` first." );

    uint32_t count = 0u;
    if ( _ps.assume.branch_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        count += num_buffers( n );
      } );
    }

    _ntk.foreach_gate( [&]( auto const& n ) {
      count += num_buffers( n );
    } );
    return count;
  }

  /*! \brief The number of buffers between `n` and all of its fanouts under
   * the current level assignment. */
  uint32_t num_buffers( node const& n ) const
  {
    assert( !_outdated && "Please call `count_buffers()` first." );
    return _num_buffers[n];
  }

  /*! \brief The choosen schedule is ASAP */
  uint32_t is_scheduled_ASAP() const
  {
    return _is_scheduled_ASAP;
  }
#pragma endregion

#pragma region Count buffers
  /*! \brief Count the number of buffers needed at the fanout of each gate
   * according to the current level assignment.
   *
   * This function must be called after level (re-)assignment and before
   * querying `num_buffers`.
   */
  void count_buffers()
  {
    if ( _outdated )
    {
      update_fanout_info();
    }

    if ( _ps.assume.branch_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        assert( !_ps.assume.balance_pis || _levels[n] == 0 );
        _num_buffers[n] = count_buffers( n );
      } );
    }

    _ntk.foreach_gate( [&]( auto const& n ) {
      _num_buffers[n] = count_buffers( n );
    } );
  }

private:
  uint32_t count_buffers( node const& n )
  {
    assert( !_outdated && "Please call `update_fanout_info()` first." );
    auto const& fo_infos = _fanouts[n];

    if ( _ntk.fanout_size( n ) == 0u ) /* dangling */
    {
      if ( !_ntk.is_pi( n ) )
        std::cerr << "[w] node " << n << " (which is not a PI) is dangling.\n";
      return 0u;
    }

    if ( _ntk.fanout_size( n ) == 1u ) /* single fanout */
    {
      if ( _external_ref_count[n] == 1u && !_ps.assume.balance_pos )
        return 0u;
      else
        return fo_infos.front().relative_depth - 1u;
    }

    /* special case: don't balance POs; multiple PO refs but no gate fanout */
    if ( !_ps.assume.balance_pos && _ntk.fanout_size( n ) == _external_ref_count[n] )
    {
      /* have both polarities */
      if ( _external_ref_count_neg[n] > 0 && _external_ref_count[n] > _external_ref_count_neg[n] )
        return std::ceil( float( _external_ref_count[n] - _external_ref_count_neg[n] - 1 ) / float( _ps.assume.splitter_capacity - 1 ) ) + std::ceil( float( _external_ref_count_neg[n] - 1 ) / float( _ps.assume.splitter_capacity - 1 ) ) + 1;
      else
        return std::ceil( float( _external_ref_count[n] - 1 ) / float( _ps.assume.splitter_capacity - 1 ) );
    }

    assert( fo_infos.size() > 1u );
    uint32_t count{ 0u };

    /* special case: don't balance POs; have both gate fanout(s) and PO ref(s) */
    if ( !_ps.assume.balance_pos && _external_ref_count[n] > 0u )
    {
      /* hacky (rare?) case */
      /*fmt::print( "[w] hacky case: node {} has {} fanouts, including {} gates, {} positive PO refs and {} negative PO refs.\n",
                  n, _ntk.fanout_size( n ), _ntk.fanout_size( n ) - _external_ref_count[n],
                  _external_ref_count[n] - _external_ref_count_neg[n], _external_ref_count_neg[n] );*/

      /* count ignoring POs */
      auto rit = fo_infos.rbegin();
      assert( rit->fanouts.size() == 0 );
      while ( rit->fanouts.size() == 0 )
        ++rit;
      auto nedges = rit->fanouts.size();
      auto prev_rd = rit->relative_depth;
      for ( ++rit; rit != fo_infos.rend(); ++rit )
      {
        nedges = num_splitters( nedges );
        count += nedges + prev_rd - rit->relative_depth - 1;
        nedges += rit->fanouts.size();
        prev_rd = rit->relative_depth;
      }
      assert( nedges == 1 );

      /* check if available slots in the remaining buffers are enough for POs */
      auto slots = count * ( _ps.assume.splitter_capacity - 1 ) + 1;
      int32_t needed_pos = _ntk.fanout_size( n ) - _external_ref_count_neg[n] - slots;
      if ( _external_ref_count_neg[n] > 0 )
        ++needed_pos;
      if ( needed_pos > 0 )
      {
        count += std::ceil( float( needed_pos ) / float( _ps.assume.splitter_capacity - 1 ) );
      }
      if ( _external_ref_count_neg[n] > 0 )
      {
        count += std::max<uint32_t>( std::ceil( float( _external_ref_count_neg[n] - 1 ) / float( _ps.assume.splitter_capacity - 1 ) ), 1 );
      }

      return count;
    }

    /* main counting */
    auto it = fo_infos.begin();
    count = it->num_edges;
    auto rd = it->relative_depth;
    for ( ++it; it != fo_infos.end(); ++it )
    {
      count += it->num_edges - it->fanouts.size() + it->relative_depth - rd - 1;
      rd = it->relative_depth;
    }

    /* PO refs were added as num_edges and counted as buffers */
    count -= _external_ref_count[n];
    return count;
  }

  /* (Upper bound on) the additional depth caused by a balanced splitter tree at the output of node `n`. */
  uint32_t num_splitter_levels( node const& n ) const
  {
    assert( n < _ntk.size() );
    return std::ceil( std::log( _ntk.fanout_size( n ) ) / std::log( _ps.assume.splitter_capacity ) );
  }

  uint32_t num_splitter_levels_po( node const& n ) const
  {
    assert( n < _ntk.size() );
    if ( _ntk.fanout_size( n ) == 1 )
      return 0;
    if ( _external_ref_count_neg[n] == 0 || _external_ref_count_neg[n] == _external_ref_count[n] )
      return std::ceil( std::log( _ntk.fanout_size( n ) ) / std::log( _ps.assume.splitter_capacity ) );
    return std::max( std::ceil( std::log( _ntk.fanout_size( n ) - _external_ref_count_neg[n] ) / std::log( _ps.assume.splitter_capacity ) ),
                     std::ceil( std::log( _external_ref_count_neg[n] ) / std::log( _ps.assume.splitter_capacity ) ) ) +
           1; // suboptimal
  }
#pragma endregion

private:
#pragma region Update fanout info
  void initialize_external_ref_counts()
  {
    _ntk.foreach_po( [&]( auto const& f ) {
      if ( !_ntk.is_constant( _ntk.get_node( f ) ) )
      {
        _external_ref_count[f]++;
        if ( _ntk.is_complemented( f ) )
        {
          _external_ref_count_neg[f]++;
        }
      }
    } );
  }

  /* Update fanout_information of all nodes */
  void update_fanout_info()
  {
    _fanouts.reset();
    _ntk.foreach_gate( [&]( auto const& n ) {
      _ntk.foreach_fanin( n, [&]( auto const& fi ) {
        auto const ni = _ntk.get_node( fi );
        if ( !_ntk.is_constant( ni ) )
        {
          insert_fanout( ni, n );
        }
      } );
    } );

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( !_ps.assume.branch_pis && _ntk.is_pi( n ) )
        return true;
      if ( _external_ref_count[n] > 0u )
        _fanouts[n].push_back( { _depth + 1 - _levels[n], {}, _external_ref_count[n] } );
      return true;
    } );

    /* //debugging checks
    if ( !_ps.assume.branch_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) { assert( _fanouts[n].size() == 0 ); });
      _ntk.foreach_gate( [&]( auto const& n ) {
        if ( _ntk.fanout_size( n ) == 1 ) assert( _fanouts[n].size() == 1 );
        else assert( _fanouts[n].front().relative_depth > 1 );
      });
    }
    else
    {
      _ntk.foreach_node( [&]( auto const& n ) {
        if ( _ntk.is_constant( n ) || _ntk.fanout_size( n ) == 0 ) return true;
        if ( _ntk.fanout_size( n ) == 1 ) assert( _fanouts[n].size() == 1 );
        else assert( _fanouts[n].front().relative_depth > 1 );
        return true;
      });
    }
    */

    _ntk.foreach_gate( [&]( auto const& n ) {
      count_edges( n );
    } );

    if ( _ps.assume.branch_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        count_edges( n );
      } );
    }

    _outdated = false;
  }

  /* Update the fanout_information of a node */
  template<bool verify = false>
  bool update_fanout_info( node const& n )
  {
    std::vector<node> fos;
    for ( auto it = _fanouts[n].begin(); it != _fanouts[n].end(); ++it )
    {
      if ( it->fanouts.size() )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
          fos.push_back( *it2 );
      }
    }

    _fanouts[n].clear();
    for ( auto& fo : fos )
      insert_fanout( n, fo );

    if ( _external_ref_count[n] > 0u )
      _fanouts[n].push_back( { _depth + 1 - _levels[n], {}, _external_ref_count[n] } );

    return count_edges<verify>( n );
  }

  void insert_fanout( node const& n, node const& fanout )
  {
    if ( !_ps.assume.branch_pis && _ntk.is_pi( n ) )
      return;
    auto const rd = _levels[fanout] - _levels[n];
    assert( rd > 0 );
    auto& fo_infos = _fanouts[n];
    for ( auto it = fo_infos.begin(); it != fo_infos.end(); ++it )
    {
      if ( it->relative_depth == rd )
      {
        it->fanouts.push_back( fanout );
        ++it->num_edges;
        return;
      }
      else if ( it->relative_depth > rd )
      {
        fo_infos.insert( it, { rd, { fanout }, 1u } );
        return;
      }
    }
    fo_infos.push_back( { rd, { fanout }, 1u } );
  }

  template<bool verify = false>
  bool count_edges( node const& n )
  {
    auto& fo_infos = _fanouts[n];

    if ( fo_infos.size() == 0u || ( fo_infos.size() == 1u && fo_infos.front().num_edges == 1u ) )
    {
      return true;
    }
    assert( fo_infos.front().relative_depth > 1u );
    fo_infos.push_front( { 1u, {}, 0u } );

    auto it = fo_infos.end();
    --it;
    uint32_t splitters;
    if ( _external_ref_count_neg[n] > 0 )
      splitters = num_splitters_po( _external_ref_count[n] - _external_ref_count_neg[n], _external_ref_count_neg[n] );
    else
      splitters = num_splitters( it->num_edges );

    while ( it != fo_infos.begin() )
    {
      auto rd = it->relative_depth;
      --it;
      if ( it->relative_depth < rd - 1 && splitters > 1 )
      {
        ++it;
        it = fo_infos.insert( it, { rd - 1, {}, splitters } );
      }
      else
      {
        it->num_edges += splitters;
      }
      splitters = num_splitters( it->num_edges );
    }

    assert( fo_infos.front().relative_depth == 1u );
    if constexpr ( verify )
    {
      return fo_infos.front().num_edges == 1u;
    }
    else
    {
      assert( fo_infos.front().num_edges == 1u );
      return true;
    }
  }

  /* Return the number of splitters needed in one level lower */
  uint32_t num_splitters( uint32_t const& num_fanouts ) const
  {
    return std::ceil( float( num_fanouts ) / float( _ps.assume.splitter_capacity ) );
  }

  uint32_t num_splitters_po( uint32_t num_positive, uint32_t num_negative ) const
  {
    return std::ceil( float( num_positive ) / float( _ps.assume.splitter_capacity ) ) + std::ceil( float( num_negative ) / float( _ps.assume.splitter_capacity ) );
  }
#pragma endregion

#pragma region Level assignment
public:
  /*! \brief Obtain the initial level assignment using the specified scheduling policy */
  void schedule()
  {
    if ( _ps.scheduling == buffer_insertion_params::provided )
    {
      _ntk.foreach_po( [&]( auto const& f ) {
        _depth = std::max( _depth, _levels[f] + num_splitter_levels( _ntk.get_node( f ) ) );
      } );
    }
    else if ( _ps.scheduling == buffer_insertion_params::better_depth || _ps.scheduling == buffer_insertion_params::ASAP_depth || _ps.scheduling == buffer_insertion_params::ALAP_depth )
    {
      fanout_view<Ntk> f_ntk{ _ntk };
      depth_optimal_schedule( f_ntk );
    }
    else
    {
      ASAP();
    }

    if ( _ps.scheduling == buffer_insertion_params::ALAP )
    {
      ALAP();
    }
    else if ( _ps.scheduling == buffer_insertion_params::better )
    {
      count_buffers();
      auto num_buf_ASAP = num_buffers();
      ALAP();
      count_buffers();
      if ( num_buffers() > num_buf_ASAP )
      {
        ASAP();
      }
    }
  }

  /*! \brief ASAP scheduling */
  void ASAP()
  {
    _depth = 0;
    _levels.reset( 0 );
    _ntk.incr_trav_id();

    _ntk.foreach_po( [&]( auto const& f ) {
      auto const no = _ntk.get_node( f );
      auto clevel = compute_levels_ASAP( no ) + num_splitter_levels_po( no );
      _depth = std::max( _depth, clevel );
    } );

    _outdated = true;
    _is_scheduled_ASAP = true;
  }

  /*! \brief ASAP optimal-depth scheduling
   *
   * ASAP_depth should follow right after ALAP_depth (i.e., initialization).
   *
   * \param try_regular tries to insert balanced trees when sufficient slack.
   */
  void ASAP_depth( fanout_view<Ntk> const& f_ntk, bool try_regular )
  {
    node_map<uint32_t, Ntk> mobility( _ntk, UINT32_MAX );

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
      {
        mobility[n] = _levels[n];
      }

      if ( !_ntk.is_constant( n ) )
      {
        compute_mobility_ASAP( f_ntk, n, mobility, try_regular );
      }

      _min_level[n] = _levels[n];
    } );

    _outdated = true;
    _is_scheduled_ASAP = true;
  }

  /*! \brief ALAP scheduling.
   *
   * ALAP should follow right after ASAP (i.e., initialization) without other optimization in between.
   */
  void ALAP()
  {
    _levels.reset( 0 );
    _ntk.incr_trav_id();

    _ntk.foreach_po( [&]( auto const& f ) {
      const auto n = _ntk.get_node( f );
      if ( !_ntk.is_constant( n ) && _ntk.visited( n ) != _ntk.trav_id() && ( !_ps.assume.balance_pis || !_ntk.is_pi( n ) ) )
      {
        _levels[n] = _depth - num_splitter_levels_po( n );
        compute_levels_ALAP( n );
      }
    } );

    _outdated = true;
    _is_scheduled_ASAP = false;
  }

  /*! \brief ALAP depth-optimal sheduling */
  void ALAP_depth( fanout_view<Ntk> const& f_ntk )
  {
    _levels.reset( 0 );
    topo_view<Ntk> topo_ntk{ _ntk };

    /* compute ALAP */
    _depth = UINT32_MAX - 1;
    uint32_t min_level = UINT32_MAX - 1;
    topo_ntk.foreach_node_reverse( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) && ( _ps.assume.branch_pis || !_ntk.is_pi( n ) ) )
      {
        compute_levels_ALAP_depth( f_ntk, n );
        min_level = std::min( min_level, _levels[n] );
      }
    } );

    if ( !_ps.assume.branch_pis && min_level != 0 )
      --min_level;

    /* normalize level */
    _ntk.foreach_node( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) )
      {
        if ( _ps.assume.balance_pis && _ntk.is_pi( n ) )
        {
          _levels[n] = 0;
        }
        else if ( !_ps.assume.balance_pis || !_ntk.is_pi( n ) )
        {
          _levels[n] = _levels[n] - min_level;
        }
        _max_level[n] = _levels[n];
      }
    } );

    _depth -= min_level;
    _outdated = true;
    _is_scheduled_ASAP = false;
  }

  void depth_optimal_schedule( fanout_view<Ntk> const& f_ntk )
  {
    /* Optimum-depth ALAP scheduling */
    ALAP_depth( f_ntk );
    count_buffers();
    auto const num_buf_ALAP_depth = num_buffers();

    if ( _ps.scheduling == buffer_insertion_params::ALAP_depth )
      return;

    /* Optimum-depth ALAP scheduling: no balanced trees */
    ASAP_depth( f_ntk, false );
    count_buffers();
    auto const num_buf_ASAP_depth = num_buffers();

    if ( _ps.scheduling == buffer_insertion_params::ASAP_depth )
      return;

    /* Revert to optimum-depth ALAP scheduling if better */
    if ( num_buf_ALAP_depth < num_buf_ASAP_depth )
    {
      ALAP_depth( f_ntk );
    }
  }

private:
  uint32_t compute_levels_ASAP( node const& n )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
    {
      return _levels[n];
    }
    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
    {
      return _levels[n] = 0;
    }

    uint32_t level{ 0 };
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        auto fi_level = compute_levels_ASAP( ni );
        if ( _ps.assume.branch_pis || !_ntk.is_pi( ni ) )
        {
          fi_level += num_splitter_levels( ni );
        }
        level = std::max( level, fi_level );
      }
    } );

    return _levels[n] = level + 1;
  }

  void compute_levels_ALAP( node const& n )
  {
    _ntk.set_visited( n, _ntk.trav_id() );

    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        if ( _ps.assume.balance_pis && _ntk.is_pi( ni ) )
        {
          assert( _levels[n] > 0 );
          _levels[ni] = 0;
        }
        else if ( _ps.assume.branch_pis || !_ntk.is_pi( ni ) )
        {
          assert( _levels[n] > num_splitter_levels( ni ) );
          auto fi_level = _levels[n] - num_splitter_levels( ni ) - 1;
          if ( _ntk.visited( ni ) != _ntk.trav_id() || _levels[ni] > fi_level )
          {
            _levels[ni] = fi_level;
            compute_levels_ALAP( ni );
          }
        }
      }
    } );
  }

  void compute_levels_ALAP_depth( fanout_view<Ntk> const& ntk, node const& n )
  {
    std::vector<uint32_t> level_assignment;
    level_assignment.reserve( ntk.fanout_size( n ) );

    /* if node is a PO, add levels */
    for ( auto i = ntk.fanout( n ).size(); i < ntk.fanout_size( n ); ++i )
      level_assignment.push_back( _depth + 1 );

    /* get fanout levels */
    ntk.foreach_fanout( n, [&]( auto const& f ) {
      level_assignment.push_back( _levels[f] );
    } );

    /* dangling PI */
    if ( level_assignment.empty() )
    {
      _levels[n] = _depth;
      return;
    }

    /* sort by descending order of levels */
    std::sort( level_assignment.begin(), level_assignment.end(), std::greater<uint32_t>() );

    /* simulate splitter tree reconstruction */
    uint32_t nodes_in_level = 0;
    uint32_t last_level = level_assignment.front();
    for ( int const l : level_assignment )
    {
      if ( l == last_level )
      {
        ++nodes_in_level;
      }
      else
      {
        /* update splitters */
        for ( auto i = 0; ( i < last_level - l ) && ( nodes_in_level != 1 ); ++i )
          nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );

        ++nodes_in_level;
        last_level = l;
      }
    }

    /* search for a feasible level for node n */
    --last_level;
    while ( nodes_in_level > 1 )
    {
      nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
      --last_level;
    }

    _levels[n] = last_level;
  }

  void compute_mobility_ASAP( fanout_view<Ntk> const& ntk, node const& n, node_map<uint32_t, Ntk>& mobility, bool try_regular )
  {
    /* commit ASAP scheduling */
    uint32_t level_n = _levels[n] - mobility[n];
    _levels[n] = level_n;

    if ( !_ps.assume.branch_pis && _ntk.is_pi( n ) )
    {
      ntk.foreach_fanout( n, [&]( auto const& f ) {
        mobility[f] = std::min( mobility[f], _levels[f] - level_n - 1 );
      } );
      return;
    }

    /* try to fit a balanced tree */
    if ( try_regular )
    {
      uint32_t fo_level = num_splitter_levels( n );
      bool valid = true;
      ntk.foreach_fanout( n, [&]( auto const& f ) {
        if ( level_n + fo_level + 1 > _levels[f] )
          valid = false;
        return valid;
      } );

      if ( valid )
      {
        ntk.foreach_fanout( n, [&]( auto const& f ) {
          mobility[f] = std::min( mobility[f], _levels[f] - level_n - fo_level - 1 );
        } );
        return;
      }
    }

    /* keep current splitter structure, selecting the mobility based on the buffers */
    std::vector<std::array<uint32_t, 3>> level_assignment;
    level_assignment.reserve( _ntk.fanout_size( n ) );

    /* if node is a PO, add levels */
    for ( auto i = ntk.fanout( n ).size(); i < ntk.fanout_size( n ); ++i )
      level_assignment.push_back( { 0, _depth + 1, 0 } );

    /* get fanout levels */
    ntk.foreach_fanout( n, [&]( auto const& f ) {
      level_assignment.push_back( { ntk.node_to_index( f ), _levels[f], 0 } );
    } );

    /* dangling PI */
    if ( level_assignment.empty() )
    {
      return;
    }

    /* sort by descending order of levels */
    std::sort( level_assignment.begin(), level_assignment.end(), []( auto const& a, auto const& b ) {
      return a[1] > b[1];
    } );

    /* simulate splitter tree reconstruction */
    uint32_t nodes_in_level = 0;
    uint32_t nodes_in_last_level = _ps.assume.splitter_capacity;
    uint32_t last_level = level_assignment.front()[1];
    for ( auto i = 0; i < level_assignment.size(); ++i )
    {
      uint32_t l = level_assignment[i][1];
      if ( l == last_level )
      {
        ++nodes_in_level;
      }
      else
      {
        /* update splitters */
        uint32_t mobility_update = 0;
        for ( auto j = 0; j < last_level - l; ++j )
        {
          if ( nodes_in_level == 1 )
          {
            ++mobility_update;
          }
          nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
        }

        if ( mobility_update )
        {
          for ( auto j = 0; j < i; ++j )
            level_assignment[j][2] += mobility_update;
        }

        ++nodes_in_level;
        last_level = l;
      }
    }

    /* search a feasible level for node n */
    uint32_t mobility_update = 0;
    for ( auto i = level_n + 1; i < last_level; ++i )
    {
      if ( nodes_in_level == 1 )
        ++mobility_update;
      nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
    }

    /* update mobilities */
    for ( auto const& v : level_assignment )
    {
      if ( v[0] != 0 )
      {
        mobility[v[0]] = std::min( mobility[v[0]], v[2] + mobility_update );
      }
    }
  }
#pragma endregion

#pragma region Compute timeframe
  /*! \brief Compute the earliest and latest possible timeframe by eager ASAP and ALAP */
  uint32_t compute_timeframe( uint32_t max_depth )
  {
    _timeframes.reset( std::make_pair( 0, 0 ) );
    uint32_t min_depth{ 0 };

    _ntk.incr_trav_id();
    _ntk.foreach_po( [&]( auto const& f ) {
      auto const no = _ntk.get_node( f );
      auto clevel = compute_levels_ASAP_eager( no ) + ( _ntk.fanout_size( no ) > 1 ? 1 : 0 );
      min_depth = std::max( min_depth, clevel );
    } );

    _ntk.incr_trav_id();
    _ntk.foreach_po( [&]( auto const& f ) {
      const auto n = _ntk.get_node( f );
      if ( !_ntk.is_constant( n ) && _ntk.visited( n ) != _ntk.trav_id() && ( !_ps.assume.balance_pis || !_ntk.is_pi( n ) ) )
      {
        _timeframes[n].second = max_depth - ( _ntk.fanout_size( n ) > 1 ? 1 : 0 );
        compute_levels_ALAP_eager( n );
      }
    } );

    return min_depth;
  }

  uint32_t compute_levels_ASAP_eager( node const& n )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
    {
      return _timeframes[n].first;
    }
    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
    {
      return _timeframes[n].first = 0;
    }

    uint32_t level{ 0 };
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        auto fi_level = compute_levels_ASAP_eager( ni );
        if ( _ps.assume.branch_pis || !_ntk.is_pi( ni ) )
        {
          fi_level += _ntk.fanout_size( ni ) > 1 ? 1 : 0;
        }
        level = std::max( level, fi_level );
      }
    } );

    return _timeframes[n].first = level + 1;
  }

  void compute_levels_ALAP_eager( node const& n )
  {
    _ntk.set_visited( n, _ntk.trav_id() );

    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        if ( _ps.assume.balance_pis && _ntk.is_pi( ni ) )
        {
          assert( _timeframes[n].second > 0 );
          _timeframes[ni].second = 0;
        }
        else if ( _ps.assume.branch_pis || !_ntk.is_pi( ni ) )
        {
          assert( _timeframes[n].second > num_splitter_levels( ni ) );
          auto fi_level = _timeframes[n].second - ( _ntk.fanout_size( ni ) > 1 ? 2 : 1 );
          if ( _ntk.visited( ni ) != _ntk.trav_id() || _timeframes[ni].second > fi_level )
          {
            _timeframes[ni].second = fi_level;
            compute_levels_ALAP_eager( ni );
          }
        }
      }
    } );
  }
#pragma

#pragma region Dump buffered network
public:
  /*! \brief Dump buffered network
   *
   * After level assignment, (optimization), and buffer counting, this method
   * can be called to dump the resulting buffered network.
   */
  template<class BufNtk>
  void dump_buffered_network( BufNtk& bufntk ) const
  {
    static_assert( is_buffered_network_type_v<BufNtk>, "BufNtk is not a buffered network type" );
    static_assert( has_is_buf_v<BufNtk>, "BufNtk does not implement the is_buf method" );
    static_assert( has_create_buf_v<BufNtk>, "BufNtk does not implement the create_buf method" );
    assert( !_outdated && "Please call `count_buffers()` first." );

    using buf_signal = typename BufNtk::signal;
    using fanout_tree = std::vector<std::list<buf_signal>>;

    node_map<buf_signal, Ntk> node_to_signal( _ntk );
    node_map<fanout_tree, Ntk> buffers( _ntk );

    /* constants */
    node_to_signal[_ntk.get_constant( false )] = bufntk.get_constant( false );
    buffers[_ntk.get_constant( false )].emplace_back( 1, bufntk.get_constant( false ) );
    if ( _ntk.get_node( _ntk.get_constant( false ) ) != _ntk.get_node( _ntk.get_constant( true ) ) )
    {
      std::cerr << "[w] ntk has different nodes for const0 and const1 -- poorly tested case, might be buggy.\n";
      node_to_signal[_ntk.get_constant( true )] = bufntk.get_constant( true );
      buffers[_ntk.get_constant( true )].emplace_back( 1, bufntk.get_constant( true ) );
    }

    /* PIs */
    _ntk.foreach_pi( [&]( auto const& n ) {
      node_to_signal[n] = bufntk.create_pi();
    } );
    if ( _ps.assume.branch_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        create_buffer_chain( bufntk, buffers, n, node_to_signal[n] );
      } );
    }
    else
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        buffers[n].emplace_back( 1, node_to_signal[n] );
      } );
    }

    /* gates: assume topological order */
    _ntk.foreach_gate( [&]( auto const& n ) {
      std::vector<buf_signal> children;
      _ntk.foreach_fanin( n, [&]( auto const& fi ) {
        auto ni = _ntk.get_node( fi );
        buf_signal s;
        if ( _ntk.is_constant( ni ) || ( !_ps.assume.branch_pis && _ntk.is_pi( ni ) ) )
          s = node_to_signal[ni];
        else
          s = get_buffer_at_relative_depth( bufntk, buffers[fi], _levels[n] - _levels[fi] - 1 );
        children.push_back( _ntk.is_complemented( fi ) ? !s : s );
      } );
      node_to_signal[n] = bufntk.clone_node( _ntk, n, children );
      create_buffer_chain( bufntk, buffers, n, node_to_signal[n] );
    } );

    /* POs */
    if ( _ps.assume.balance_pos )
    {
      _ntk.foreach_po( [&]( auto const& f, uint32_t i ) {
        auto n = _ntk.get_node( f );
        if ( _ntk.is_constant( n ) || ( !_ps.assume.branch_pis && _ntk.is_pi( n ) ) ) // not branch => not balance
        {
          if ( _ntk.is_pi( n ) && _ntk.is_complemented( f ) )
            std::cerr << "[w] an explicit inverter between non-branched PI " << n << " and PO " << i << " is neglected.\n";
          bufntk.create_po( _ntk.is_complemented( f ) ? !node_to_signal[f] : node_to_signal[f] );
        }
        else if ( _ntk.fanout_size( n ) == 1 && _depth == _levels[f] )
        {
          if ( _ntk.is_complemented( f ) )
            bufntk.invert( bufntk.get_node( node_to_signal[f] ) );
          bufntk.create_po( node_to_signal[f] );
        }
        else
        {
          buf_signal s = get_buffer_or_inverter( bufntk, buffers[f], _depth - _levels[f], _ntk.is_complemented( f ) );
          bufntk.create_po( s );
        }
      } );
    }
    else // !_ps.assume.balance_pos
    {
      std::set<node> checked;
      unordered_node_map<std::list<buf_signal>, Ntk> inverted_buffers( _ntk );
      _ntk.foreach_po( [&]( auto const& f ) {
        auto n = _ntk.get_node( f );
        if ( !_ntk.is_constant( n ) && !( _ntk.is_pi( n ) && !_ps.assume.branch_pis ) && _ntk.fanout_size( n ) > 1 )
        {
          if ( checked.find( n ) == checked.end() )
          {
            checked.insert( n );
            /* count available slots in buffers[n] */
            uint32_t slots{ 0u };
            for ( auto const& bufs : buffers[n] )
            {
              slots += _ps.assume.splitter_capacity - bufntk.fanout_size( bufntk.get_node( bufs.back() ) );
            }
            slots -= _ps.assume.splitter_capacity - 1; // buffers[n][0] is n itself

            /* add enough buffers */
            if ( _external_ref_count[n] > _external_ref_count_neg[n] ) /* there are pos POs */
            {
              if ( buffers[n].size() == 1 )
              {
                buffers[n].emplace_back( 1, bufntk.create_buf( buffers[n][0].back() ) );
                slots += _ps.assume.splitter_capacity - 1;
              }
              uint32_t needed_slots = _external_ref_count[n] - _external_ref_count_neg[n];
              if ( _external_ref_count_neg[n] > 0 )
                ++needed_slots;
              while ( slots < needed_slots )
              {
                auto p = get_lowest_spot( bufntk, buffers[n] );
                add_splitter( bufntk, buffers[n], p.first, p.second );
                slots += _ps.assume.splitter_capacity - 1;
              }
            }

            /* add inverted buffer tree */
            if ( _external_ref_count_neg[n] > 0 )
            {
              auto p = get_lowest_spot( bufntk, buffers[n] );
              buf_signal const& s = p.first;
              uint32_t const& rd = p.second;
              if ( _external_ref_count_neg[n] == _ntk.fanout_size( n ) )
              {
                bufntk.invert( bufntk.get_node( s ) );
                buffers[n][rd].remove( s );
                inverted_buffers[n].push_back( s );
              }
              else
              {
                inverted_buffers[n].push_back( bufntk.create_buf( !s ) );
              }
              uint32_t inverted_slots{ _ps.assume.splitter_capacity };
              while ( inverted_slots < _external_ref_count_neg[n] )
              {
                buf_signal s = get_first_spot( bufntk, inverted_buffers[n] );
                inverted_buffers[n].push_back( bufntk.create_buf( s ) );
                inverted_slots += _ps.assume.splitter_capacity - 1;
              }
            }

            /* check */
            uint32_t nbufs = 0;
            for ( auto l : buffers[n] )
              nbufs += l.size();
            assert( nbufs - 1 + inverted_buffers[n].size() == _num_buffers[n] );
          }
        }
      } );

      _ntk.foreach_po( [&]( auto const& f, uint32_t i ) {
        auto n = _ntk.get_node( f );
        if ( _ntk.is_constant( n ) || ( _ntk.is_pi( n ) && ( !_ps.assume.branch_pis || _ntk.fanout_size( n ) == 1 ) ) )
        {
          if ( _ntk.is_pi( n ) && _ntk.is_complemented( f ) )
            std::cerr << "[w] an explicit inverter between non-branched PI " << n << " and PO " << i << " is neglected.\n";
          bufntk.create_po( _ntk.is_complemented( f ) ? !node_to_signal[f] : node_to_signal[f] );
        }
        else if ( _ntk.fanout_size( n ) == 1 )
        {
          if ( _ntk.is_complemented( f ) )
            bufntk.invert( bufntk.get_node( node_to_signal[f] ) );
          bufntk.create_po( node_to_signal[f] );
        }
        else
        {
          buf_signal s = _ntk.is_complemented( f ) ? get_first_spot( bufntk, inverted_buffers[n] ) : get_lowest_spot( bufntk, buffers[n] ).first;
          assert( bufntk.is_buf( bufntk.get_node( s ) ) );
          bufntk.create_po( s );
        }
      } );
    }

    // assert( bufntk.size() - bufntk.num_pis() - bufntk.num_gates() - 1 == num_buffers() );
    if ( bufntk.size() - bufntk.num_pis() - bufntk.num_gates() - 1 != num_buffers() )
    {
      std::cerr << "[w] actual #bufs = " << ( bufntk.size() - bufntk.num_pis() - bufntk.num_gates() - 1 ) << ", counted = " << num_buffers() << "\n";
    }
  }

private:
  template<class BufNtk, typename Buffers>
  void create_buffer_chain( BufNtk& bufntk, Buffers& buffers, node const& n, typename BufNtk::signal const& s ) const
  {
    if ( _ntk.fanout_size( n ) == 0 )
      return; /* dangling */

    auto const& fanout_info = _fanouts[n];
    assert( fanout_info.size() > 0u );

    if ( _external_ref_count[n] > 0u && !_ps.assume.balance_pos )
    {
      if ( _ntk.fanout_size( n ) == _external_ref_count[n] )
      {
        if ( _external_ref_count[n] > _external_ref_count_neg[n] )
          buffers[n].resize( std::ceil( std::log( _external_ref_count[n] - _external_ref_count_neg[n] ) / std::log( _ps.assume.splitter_capacity ) ) + 1 );
        else
          buffers[n].resize( _external_ref_count_neg[n] > 1 ? 2 : 1 );
      }
      else
      {
        auto it = fanout_info.rbegin();
        while ( it->fanouts.size() == 0u )
          ++it;
        buffers[n].resize( it->relative_depth );
      }
    }
    else
    {
      buffers[n].resize( fanout_info.back().relative_depth );
    }
    auto& fot = buffers[n];

    typename BufNtk::signal fi = s;
    fot[0].push_back( fi );
    for ( auto i = 1u; i < fot.size(); ++i )
    {
      fi = bufntk.create_buf( fi );
      fot[i].push_back( fi );
    }
  }

  template<class BufNtk, typename FOT>
  typename BufNtk::signal get_buffer_or_inverter( BufNtk& bufntk, FOT& fot, uint32_t rd, bool inverted ) const
  {
    assert( rd == fot.size() - 1 ); // must be at the highest level
    for ( auto it = fot[rd].begin(); it != fot[rd].end(); ++it )
    {
      auto b = bufntk.get_node( *it );
      if ( bufntk.fanout_size( b ) < _ps.assume.splitter_capacity )
      {
        if ( bufntk.is_not( b ) != inverted )
        {
          if ( bufntk.fanout_size( b ) == 0 )
            bufntk.invert( b );
          else
            continue;
        }
        return *it;
      }
    }
    typename BufNtk::signal b_lower = get_buffer_at_relative_depth( bufntk, fot, rd - 1 );
    typename BufNtk::signal b = bufntk.create_buf( inverted ? !b_lower : b_lower );
    fot[rd].push_back( b );
    return b;
  }

  template<class BufNtk, typename FOT>
  typename BufNtk::signal get_buffer_at_relative_depth( BufNtk& bufntk, FOT& fot, uint32_t rd ) const
  {
    typename BufNtk::signal b = fot[rd].back();
    if ( bufntk.fanout_size( bufntk.get_node( b ) ) == _ps.assume.splitter_capacity )
    {
      assert( rd > 0 );
      typename BufNtk::signal b_lower = get_buffer_at_relative_depth( bufntk, fot, rd - 1 );
      b = bufntk.create_buf( b_lower );
      fot[rd].push_back( b );
    }
    return b;
  }

  template<class BufNtk, typename FOT>
  std::pair<typename BufNtk::signal, uint32_t> get_lowest_spot( BufNtk& bufntk, FOT& fot ) const
  {
    for ( auto rd = 1u; rd < fot.size(); ++rd )
    {
      for ( auto it = fot[rd].begin(); it != fot[rd].end(); ++it )
      {
        typename BufNtk::signal& b = *it;
        if ( bufntk.fanout_size( bufntk.get_node( b ) ) < _ps.assume.splitter_capacity )
        {
          return { b, rd };
        }
      }
    }
    assert( false );
  }

  template<class BufNtk, typename BufSig = typename BufNtk::signal>
  BufSig get_first_spot( BufNtk const& bufntk, std::list<BufSig> const& bufs ) const
  {
    auto it = bufs.begin();
    while ( it != bufs.end() )
    {
      if ( bufntk.fanout_size( bufntk.get_node( *it ) ) < _ps.assume.splitter_capacity )
        return *it;
      ++it;
    }
    assert( false );
  }

  template<class BufNtk, typename FOT>
  void add_splitter( BufNtk& bufntk, FOT& fot, typename BufNtk::signal b, uint32_t rd ) const
  {
    if ( rd == fot.size() - 1 )
      fot.emplace_back( 1, bufntk.create_buf( b ) );
    else
      fot[rd + 1].push_back( bufntk.create_buf( b ) );
  }
#pragma endregion

public:
  /*! \brief Optimize with chunked movement using the specified optimization policy.
   *
   * For more information, please refer to [1].
   *
   * [1] Irredundant Buffer and Splitter Insertion and Scheduling-Based Optimization for AQFP Circuits.
   * Siang-Yun Lee et. al. IWLS 2021. */
  void optimize()
  {
    if ( _ps.optimization_effort == buffer_insertion_params::none )
    {
      return;
    }
    else if ( _ps.optimization_effort == buffer_insertion_params::optimal )
    {
      if constexpr ( has_get_network_name_v<Ntk> )
        optimize_with_smt( _ntk.get_network_name() );
      else
        optimize_with_smt( "" );
      return;
    }

    if ( _outdated )
    {
      update_fanout_info();
    }

    bool updated;
    do
    {
      updated = find_and_move_chunks();
    } while ( updated && _ps.optimization_effort == buffer_insertion_params::until_sat );

    adjust_depth();
  }

#pragma region Chunked movement
private:
  struct io_interface
  {
    node c; // chunk node
    node o; // outside node
  };

  struct chunk
  {
    uint32_t id;
    std::vector<node> members{};
    std::vector<io_interface> input_interfaces{};
    std::vector<io_interface> output_interfaces{};
    int32_t slack{ std::numeric_limits<int32_t>::max() };
    int32_t benefits{ 0 };
  };

  bool is_ignored( node const& n ) const
  {
    return _ntk.is_constant( n ) || ( !_ps.assume.branch_pis && _ntk.is_pi( n ) );
  }

  bool is_fixed( node const& n ) const
  {
    if ( _ps.assume.balance_pis )
      return _ntk.is_pi( n );
    return false;
  }

  bool find_and_move_chunks()
  {
    bool updated = false;
    count_buffers();
    uint32_t num_buffers_before = num_buffers();
    _start_id = _ntk.trav_id();

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( is_ignored( n ) || is_fixed( n ) || _ntk.visited( n ) > _start_id /* belongs to a chunk */ )
      {
        return true;
      }

      _ntk.incr_trav_id();
      chunk c{ _ntk.trav_id() };
      recruit( n, c );
      if ( c.members.size() > _ps.max_chunk_size )
      {
        return true; /* skip */
      }
      cleanup_interfaces( c );

      auto moved = analyze_chunk_down( c );
      if ( !moved )
        moved = analyze_chunk_up( c );
      updated |= moved;
      return true;
    } );

    count_buffers();
    // assert( num_buffers() <= num_buffers_before );
    return updated && num_buffers() < num_buffers_before;
  }

  void recruit( node const& n, chunk& c )
  {
    if ( _ntk.visited( n ) == c.id )
      return;
    // if ( c.members.size() > _ps.max_chunk_size ) // TODO: Directly returning might be problematic
    //   return;

    assert( _ntk.visited( n ) <= _start_id );
    assert( !is_fixed( n ) );
    assert( !is_ignored( n ) );

    _ntk.set_visited( n, c.id );
    c.members.emplace_back( n );
    recruit_fanins( n, c );
    recruit_fanouts( n, c );
  }

  void recruit_fanins( node const& n, chunk& c )
  {
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !is_ignored( ni ) && _ntk.visited( ni ) != c.id )
      {
        if ( is_fixed( ni ) )
          c.input_interfaces.push_back( { n, ni } );
        else if ( are_close( ni, n ) )
          recruit( ni, c );
        else
          c.input_interfaces.push_back( { n, ni } );
      }
    } );
  }

  void recruit_fanouts( node const& n, chunk& c )
  {
    auto const& fanout_info = _fanouts[n];
    if ( fanout_info.size() == 0 )
      return;

    if ( _ntk.fanout_size( n ) == _external_ref_count[n] ) // only POs
    {
      c.output_interfaces.push_back( { n, n } ); // PO interface
    }
    else if ( fanout_info.size() == 1 ) // single gate fanout
    {
      auto const& no = fanout_info.front().fanouts.front();
      if ( is_fixed( no ) )
        c.output_interfaces.push_back( { n, no } );
      else if ( fanout_info.front().relative_depth == 1 )
        recruit( no, c );
      else
        c.output_interfaces.push_back( { n, no } );
    }
    else
    {
      for ( auto it = fanout_info.begin(); it != fanout_info.end(); ++it )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
        {
          if ( is_fixed( *it2 ) )
            c.output_interfaces.push_back( { n, *it2 } );
          else if ( it->relative_depth == 2 )
            recruit( *it2, c );
          else if ( _ntk.visited( *it2 ) != c.id )
            c.output_interfaces.push_back( { n, *it2 } );
        }
      }
    }
  }

  bool are_close( node const& ni, node const& n )
  {
    auto const& fanout_info = _fanouts[ni];
    if ( fanout_info.size() == 1 && fanout_info.front().relative_depth == 1 )
    {
      assert( fanout_info.front().fanouts.front() == n );
      return true;
    }
    if ( fanout_info.size() > 1 )
    {
      auto it = fanout_info.begin();
      it++;
      if ( it->relative_depth > 2 )
        return false;
      for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
      {
        if ( *it2 == n )
          return true;
      }
    }
    return false;
  }

  void cleanup_interfaces( chunk& c )
  {
    for ( int i = 0; i < c.input_interfaces.size(); ++i )
    {
      if ( _ntk.visited( c.input_interfaces[i].o ) == c.id )
      {
        c.input_interfaces.erase( c.input_interfaces.begin() + i );
        --i;
      }
    }
    for ( int i = 0; i < c.output_interfaces.size(); ++i )
    {
      if ( _ntk.visited( c.output_interfaces[i].o ) == c.id && c.output_interfaces[i].o != c.output_interfaces[i].c )
      {
        c.output_interfaces.erase( c.output_interfaces.begin() + i );
        --i;
      }
    }
  }

  bool analyze_chunk_down( chunk c )
  {
    std::set<node> marked_oi;
    for ( auto oi : c.output_interfaces )
    {
      if ( marked_oi.find( oi.c ) == marked_oi.end() )
      {
        marked_oi.insert( oi.c );
        --c.benefits;
      }
    }

    for ( auto ii : c.input_interfaces )
    {
      auto const rd = _levels[ii.c] - _levels[ii.o];
      auto const lowest = lowest_spot( ii.o );
      if ( rd <= lowest )
      {
        c.slack = 0;
        break;
      }
      c.slack = std::min( c.slack, int32_t( rd - lowest ) );
      if ( c.slack == rd - lowest )
        mark_occupied( ii.o, lowest );                                                          // TODO: may be inaccurate
      if ( _fanouts[ii.o].back().relative_depth == rd && _fanouts[ii.o].back().num_edges == 1 ) // is the only highest fanout
      {
        ++c.benefits;
      }
    }

    for ( auto m : c.members )
      c.slack = std::min( c.slack, int32_t( _ntk.is_pi( m ) ? _levels[m] : _levels[m] - 1 ) );

    if ( c.benefits > 0 && c.slack > 0 )
    {
      count_buffers();
      bool legal = true;
      auto buffers_before = num_buffers();

      for ( auto m : c.members )
        _levels[m] -= c.slack;
      for ( auto m : c.members )
        update_fanout_info( m );
      for ( auto ii : c.input_interfaces )
        legal &= update_fanout_info<true>( ii.o );

      _outdated = true;
      if ( legal )
        count_buffers();
      if ( !legal || num_buffers() >= buffers_before )
      {
        /* UNDO */
        for ( auto m : c.members )
          _levels[m] += c.slack;
        for ( auto m : c.members )
          update_fanout_info( m );
        for ( auto ii : c.input_interfaces )
          update_fanout_info( ii.o );
        _outdated = true;
        return false;
      }

      _start_id = _ntk.trav_id();
      return true;
    }
    else
    {
      /* reset fanout_infos of input_interfaces because num_edges may be modified by mark_occupied */
      for ( auto ii : c.input_interfaces )
        update_fanout_info( ii.o );
      return false;
    }
  }

  /* relative_depth of the lowest available spot in the fanout tree of n */
  uint32_t lowest_spot( node const& n ) const
  {
    auto const& fanout_info = _fanouts[n];
    assert( fanout_info.size() );
    assert( _ntk.fanout_size( n ) != _external_ref_count[n] );
    if ( fanout_info.size() == 1 )
    {
      assert( fanout_info.front().fanouts.size() == 1 );
      return 1;
    }
    auto it = fanout_info.begin();
    ++it;
    while ( it != fanout_info.end() && it->num_edges == _ps.assume.splitter_capacity )
      ++it;
    if ( it == fanout_info.end() ) // full fanout tree
      return fanout_info.back().relative_depth + 1;
    --it; // the last full layer
    return it->relative_depth + 1;
  }

  void mark_occupied( node const& n, uint32_t rd )
  {
    auto& fanout_info = _fanouts[n];
    for ( auto it = fanout_info.begin(); it != fanout_info.end(); ++it )
    {
      if ( it->relative_depth == rd )
      {
        ++it->num_edges;
        return;
      }
    }
  }

  bool analyze_chunk_up( chunk c )
  {
    for ( auto ii : c.input_interfaces )
    {
      if ( _fanouts[ii.o].back().relative_depth == _levels[ii.c] - _levels[ii.o] ) // is highest fanout
        --c.benefits;
    }

    std::set<node> marked_oi;
    for ( auto oi : c.output_interfaces )
    {
      if ( marked_oi.find( oi.c ) == marked_oi.end() )
      {
        marked_oi.insert( oi.c );
        ++c.benefits;
      }
      auto const& fanout_info = _fanouts[oi.c];
      if ( _ntk.fanout_size( oi.c ) == _external_ref_count[oi.c] ) // only POs
        c.slack = std::min( c.slack, int32_t( _depth - _levels[oi.c] - num_splitter_levels( oi.c ) ) );
      else if ( fanout_info.size() == 1 ) // single fanout
        c.slack = std::min( c.slack, int32_t( fanout_info.front().relative_depth - 1 ) );
      else
        c.slack = std::min( c.slack, int32_t( _levels[oi.o] - _levels[oi.c] - 2 ) );
    }

    if ( c.benefits > 0 && c.slack > 0 )
    {
      count_buffers();
      bool legal = true;
      auto buffers_before = num_buffers();

      for ( auto m : c.members )
        _levels[m] += c.slack;
      for ( auto m : c.members )
      {
        legal &= update_fanout_info<true>( m );
        if ( !legal )
          break;
      }
      if ( legal )
      {
        for ( auto ii : c.input_interfaces )
          update_fanout_info( ii.o );
      }

      _outdated = true;
      if ( legal )
        count_buffers();
      if ( !legal || num_buffers() >= buffers_before )
      {
        /* UNDO */
        for ( auto m : c.members )
          _levels[m] -= c.slack;
        for ( auto m : c.members )
          update_fanout_info( m );
        for ( auto ii : c.input_interfaces )
          update_fanout_info( ii.o );
        _outdated = true;
        return false;
      }

      _start_id = _ntk.trav_id();
      return true;
    }
    else
    {
      return false;
    }
  }

  void adjust_depth()
  {
    if ( !_ps.assume.balance_pis )
    {
      auto min_level = std::numeric_limits<uint32_t>::max();
      if ( _ps.assume.branch_pis )
      {
        _ntk.foreach_pi( [&]( auto n ) {
          min_level = std::min( min_level, _levels[n] );
        } );

        if ( min_level != 0 )
        {
          _ntk.foreach_node( [&]( auto n ) {
            if ( !_ntk.is_constant( n ) )
              _levels[n] -= min_level;
          } );
        }
      }
      else
      {
        _ntk.foreach_gate( [&]( auto n ) {
          min_level = std::min( min_level, _levels[n] );
        } );

        if ( min_level > 1 )
        {
          _ntk.foreach_gate( [&]( auto n ) {
            _levels[n] -= min_level - 1;
          } );
        }
      }
    }

    _depth = 0;
    _ntk.foreach_po( [&]( auto f ) {
      _depth = std::max( _depth, _levels[_ntk.get_node( f )] + num_splitter_levels( _ntk.get_node( f ) ) );
    } );

    _outdated = true;
  }
#pragma endregion

#pragma region Global optimal by SMT
private:
#include "optimal_buffer_insertion.hpp"
#pragma endregion

private:
  struct fanout_information
  {
    uint32_t relative_depth{ 0u };
    std::list<node> fanouts;
    uint32_t num_edges{ 0u };
  };
  using fanouts_by_level = std::list<fanout_information>;

  Ntk const& _ntk;
  buffer_insertion_params const _ps;
  bool _outdated{ true };
  bool _is_scheduled_ASAP{ true };

  node_map<uint32_t, Ntk> _levels;
  node_map<std::pair<uint32_t, uint32_t>, Ntk> _timeframes;
  uint32_t _depth{ 0u };

  /* Guarantees on `_fanouts` (when not `_outdated`):
   * - If not `branch_pis`: `_fanouts[PI]` is empty.
   * - PO ref count is added to `num_edges` of the last element.
   * - If having only one fanout: `_fanouts[n].size() == 1`.
   * - If having multiple fanouts: `_fanouts[n]` must have at least two elements,
   *   and the first element must have `relative_depth == 1` and `num_edges == 1`.
   */
  node_map<fanouts_by_level, Ntk> _fanouts;
  node_map<uint32_t, Ntk> _external_ref_count;     // total refs
  node_map<uint32_t, Ntk> _external_ref_count_neg; // negated refs
  node_map<uint32_t, Ntk> _num_buffers;
  node_map<uint32_t, Ntk> _min_level;
  node_map<uint32_t, Ntk> _max_level;

  uint32_t _start_id; // for chunked movement
};                    /* buffer_insertion */

} // namespace mockturtle
