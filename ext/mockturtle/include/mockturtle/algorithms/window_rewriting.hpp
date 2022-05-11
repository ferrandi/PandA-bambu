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
  \file window_rewriting.hpp
  \brief Window rewriting

  \author Heinz Riener
*/

#include "../utils/index_list.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/window_utils.hpp"
#include "../views/topo_view.hpp"
#include "../views/window_view.hpp"
#include "../utils/debugging_utils.hpp"

#include <abcresub/abcresub2.hpp>
#include <fmt/format.h>
#include <stack>

#pragma once

namespace mockturtle
{

struct window_rewriting_params
{
  uint64_t cut_size{6};
  uint64_t num_levels{5};
}; /* window_rewriting_params */

struct window_rewriting_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{0};

  /*! \brief Total number of calls to the resub. engine. */
  uint64_t num_substitutions{0};
}; /* window_rewriting_stats */

namespace detail
{

template<class Ntk>
class window_rewriting_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit window_rewriting_impl( Ntk& ntk, window_rewriting_params const& ps, window_rewriting_stats& st )
    : ntk( ntk )
    , ps( ps )
    , st( st )
  {
    auto const update_level_of_new_node = [&]( const auto& n ) {
      ntk.resize_levels();
      update_node_level( n );
    };

    auto const update_level_of_existing_node = [&]( node const& n, const auto& old_children ) {
      (void)old_children;
      ntk.resize_levels();
      update_node_level( n );
    };

    auto const update_level_of_deleted_node = [&]( node const& n ) {
      assert( ntk.fanout_size( n ) == 0u );
      ntk.set_level( n, -1 );
    };

    ntk._events->on_add.emplace_back( update_level_of_new_node );
    ntk._events->on_modified.emplace_back( update_level_of_existing_node );
    ntk._events->on_delete.emplace_back( update_level_of_deleted_node );
  }

  void run()
  {
    stopwatch t( st.time_total );

    create_window_impl windowing( ntk );
    uint32_t const size = 3*ntk.size();
    for ( uint32_t n = 0u; n < std::min( size, ntk.size() ); ++n )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) || ntk.is_dead( n ) )
      {
        continue;
      }

      if ( const auto w = windowing.run( n, ps.cut_size, ps.num_levels ) )
      {
        window_view win( ntk, w->inputs, w->outputs, w->nodes );
        topo_view topo_win{win};

        abc_index_list il;
        encode( il, topo_win );

        auto il_opt = optimize( il );
        if ( !il_opt )
        {
          continue;
        }

        std::vector<signal> signals;
        for ( auto const& i : w->inputs )
        {
          signals.push_back( ntk.make_signal( i ) );
        }

        std::vector<signal> outputs;
        topo_win.foreach_co( [&]( signal const& o ){
          outputs.push_back( o );
        });

        std::vector<signal> new_outputs;
        uint32_t counter{0};
        bool substitution_failure = false;

        ++st.num_substitutions;
        insert( ntk, std::begin( signals ), std::end( signals ), *il_opt,
                [&]( signal const& _new )
                {
                  auto const _old = outputs.at( counter++ );
                  if ( substitution_failure )
                  {
                    if ( ntk.fanout_size( ntk.get_node( _new ) ) == 0 )
                    {
                      ntk.take_out_node( ntk.get_node( _new ) );
                    }
                    return true;
                  }
                  if ( _old == _new )
                  {
                    return true;
                  }
                  else if ( ntk.level( ntk.get_node( _old ) ) >= ntk.level( ntk.get_node( _new ) ) )
                  {
                    auto const updates = substitute_node( ntk.get_node( _old ), topo_win.is_complemented( _old ) ? !_new : _new );
                    update_vector( outputs, updates );
                  }
                  else
                  {
                    if ( ntk.fanout_size( ntk.get_node( _new ) ) == 0 )
                    {
                      ntk.take_out_node( ntk.get_node( _new ) );
                    }
                    substitution_failure = true;
                  }
                  return true;
                });

        /* update internal data structures in windowing */
        windowing.resize( ntk.size() );
      }
    }

    assert( count_reachable_dead_nodes( ntk ) == 0u );
  }

private:
  /* optimize an index_list and return the new list */
  std::optional<abc_index_list> optimize( abc_index_list const& il, bool verbose = false )
  {
    int *raw = ABC_CALLOC( int, il.size() + 1u );
    uint64_t i = 0;
    for ( auto const& v : il.raw() )
    {
      raw[i++] = v;
    }
    raw[1] = 0; /* fix encoding */

    abcresub::Abc_ResubPrepareManager( 1 );
    int *new_raw = nullptr;
    int num_resubs = 0;
    uint64_t new_entries = abcresub::Abc_ResubComputeWindow( raw, ( il.size() / 2u ), 1000, -1, 0, 0, 0, 0, &new_raw, &num_resubs );
    abcresub::Abc_ResubPrepareManager( 0 );

    if ( verbose )
    {
      fmt::print( "Performed resub {} times.  Reduced {} nodes.\n",
                  num_resubs, new_entries > 0 ? ( ( il.size() / 2u ) - new_entries ) : 0 );
    }

    if ( raw )
    {
      ABC_FREE( raw );
    }

    if ( new_entries > 0 )
    {
      std::vector<uint32_t> values;
      for ( uint32_t i = 0; i < 2*new_entries; ++i )
      {
        values.push_back( new_raw[i] );
      }
      values[1u] = 1; /* fix encoding */
      if ( new_raw )
      {
        ABC_FREE( new_raw );
      }
      return abc_index_list( values, il.num_pis() );
    }
    else
    {
      assert( new_raw == nullptr );
      return std::nullopt;
    }
  }

  /* substitute the node with a signal and return all strashing updates */
  std::vector<std::pair<node, signal>> substitute_node( node const& old_node, signal const& new_signal )
  {
    std::vector<std::pair<node, signal>> updates;
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( {old_node, new_signal} );
    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      auto const p = std::make_pair( _old, _new );
      if ( std::find( std::begin( updates ), std::end( updates ), p ) == std::end( updates ) )
      {
        updates.push_back( p );
      }

      for ( auto idx = 1u; idx < ntk._storage->nodes.size(); ++idx )
      {
        if ( ntk.is_ci( idx ) || ntk.is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = ntk.replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      ntk.replace_in_outputs( _old, _new );

      /* reset fan-in of old node */
      ntk.take_out_node( _old );
    }

    return updates;
  }

  void update_vector( std::vector<signal>& vs, std::vector<std::pair<node, signal>> const& updates )
  {
    for ( auto it = std::begin( vs ); it != std::end( vs ); ++it )
    {
      for ( auto it2 = std::begin( updates ); it2 != std::end( updates ); ++it2 )
      {
        if ( ntk.get_node( *it ) == it2->first )
        {
          *it = ntk.is_complemented( *it ) ? !it2->second : it2->second;
        }
      }
    }
  }

  /* recursively update the node levels and the depth of the critical path */
  void update_node_level( node const& n, bool top_most = true )
  {
    uint32_t curr_level = ntk.level( n );

    uint32_t max_level = 0;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const p = ntk.get_node( f );
      auto const fanin_level = ntk.level( p );
      if ( fanin_level > max_level )
      {
        max_level = fanin_level;
      }
    } );
    ++max_level;

    if ( curr_level != max_level )
    {
      ntk.set_level( n, max_level );
      if ( max_level >= ntk.depth() )
      {
        ntk.set_depth( max_level + 1 );
      }

      /* update only one more level */
      if ( top_most )
      {
        ntk.foreach_fanout( n, [&]( const auto& p ) {
          update_node_level( p, false );
        } );
      }
    }
  }

private:
  Ntk& ntk;
  window_rewriting_params ps;
  window_rewriting_stats& st;
}; /* window_rewriting_impl */

} /* detail */

template<class Ntk>
void window_rewriting( Ntk& ntk, window_rewriting_params const& ps = {}, window_rewriting_stats* pst = nullptr )
{
  window_rewriting_stats st;
  detail::window_rewriting_impl<Ntk>( ntk, ps, st ).run();
  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */