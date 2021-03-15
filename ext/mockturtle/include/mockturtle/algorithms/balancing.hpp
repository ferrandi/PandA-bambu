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
  \file balancing.hpp
  \brief Cut-based depth-optimization

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <kitty/dynamic_truth_table.hpp>

#include "../utils/cost_functions.hpp"
#include "../utils/node_map.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/depth_view.hpp"
#include "../views/topo_view.hpp"
#include "cleanup.hpp"
#include "cut_enumeration.hpp"

namespace mockturtle
{

/*! \brief Parameters for balancing.
 */
struct balancing_params
{
  /*! \brief Cut enumeration params. */
  cut_enumeration_params cut_enumeration_ps;

  /*! \brief Optimize only on critical path. */
  bool only_on_critical_path{false};

  /*! \brief Show progress. */
  bool progress{false};

  /*! \brief Be verbose. */
  bool verbose{false};
};

/*! \brief Statistics for balancing.
 */
struct balancing_stats
{
  /*! \brief Total run-time. */
  stopwatch<>::duration time_total{};

  /*! \brief Cut enumeration run-time. */
  cut_enumeration_stats cut_enumeration_st;

  /*! \brief Prints report. */
  void report() const
  {
    fmt::print( "[i] total time             = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] Cut enumeration stats\n" );
    cut_enumeration_st.report();
  }
};

template<class Ntk>
struct arrival_time_pair
{
  signal<Ntk> f;
  uint32_t level;
};

/*! \brief Callback function for `rebalancing_function_t`.
 *
 * This callback is used in the rebalancing function to announce a new candidate that
 * could be used for replacement in the main balancing algorithm.  Using a callback
 * makes it possible to account for situations in which none, a single, or multiple
 * candidates are generated.
 * 
 * The callback returns a pair composed of the output signal of the replacement
 * candidate and the level of the new candidate.  Ideally, the rebalancing function
 * should not call the callback with candidates that a worse level.
 */
template<class Ntk>
using rebalancing_function_callback_t = std::function<void(arrival_time_pair<Ntk> const&, uint32_t)>;

template<class Ntk>
using rebalancing_function_t = std::function<void(Ntk&, kitty::dynamic_truth_table const&, std::vector<arrival_time_pair<Ntk>> const&, uint32_t, uint32_t, rebalancing_function_callback_t<Ntk> const&)>;

namespace detail
{

template<class Ntk, class CostFn>
struct balancing_impl
{
  balancing_impl( Ntk const& ntk, rebalancing_function_t<Ntk> const& rebalancing_fn, balancing_params const& ps, balancing_stats& st )
      : ntk_( ntk ),
        rebalancing_fn_( rebalancing_fn ),
        ps_( ps ),
        st_( st )
  {
  }

  Ntk run()
  {
    Ntk dest;
    node_map<arrival_time_pair<Ntk>, Ntk> old_to_new( ntk_ );

    /* input arrival times and mapping */
    old_to_new[ntk_.get_constant( false )] = {dest.get_constant( false ), 0u};
    if ( ntk_.get_node( ntk_.get_constant( false ) ) != ntk_.get_node( ntk_.get_constant( true ) ) )
    {
      old_to_new[ntk_.get_constant( true )] = {dest.get_constant( true ), 0u};
    }
    ntk_.foreach_pi( [&]( auto const& n ) {
      old_to_new[n] = {dest.create_pi(), 0u};
    } );

    std::shared_ptr<depth_view<Ntk, CostFn>> depth_ntk;
    if ( ps_.only_on_critical_path )
    {
      depth_ntk = std::make_shared<depth_view<Ntk, CostFn>>( ntk_ );
    }

    stopwatch<> t( st_.time_total );
    const auto cuts = cut_enumeration<Ntk, true>( ntk_, ps_.cut_enumeration_ps, &st_.cut_enumeration_st );

    uint32_t current_level{};
    const auto size = ntk_.size();
    progress_bar pbar{ntk_.size(), "balancing |{0}| node = {1:>4} / " + std::to_string( size ) + "   current level = {2}", ps_.progress};
    topo_view<Ntk>{ntk_}.foreach_node( [&]( auto const& n, auto index ) {
      pbar( index, index, current_level );

      if ( ntk_.is_constant( n ) || ntk_.is_pi( n ) )
      {
        return;
      }

      if ( ps_.only_on_critical_path && !depth_ntk->is_on_critical_path( n ) )
      {
        std::vector<signal<Ntk>> children;
        ntk_.foreach_fanin( n, [&]( auto const& f ) {
          const auto f_best = old_to_new[f].f;
          children.push_back( ntk_.is_complemented( f ) ? dest.create_not( f_best ) : f_best );
        });
        old_to_new[n] = {dest.clone_node( ntk_, n, children ), depth_ntk->level( n )};
        return;
      }

      arrival_time_pair<Ntk> best{{}, std::numeric_limits<uint32_t>::max()};
      uint32_t best_size{};
      for ( auto& cut : cuts.cuts( ntk_.node_to_index( n ) ) )
      {
        if ( cut->size() == 1u || kitty::is_const0( cuts.truth_table( *cut ) ) )
        {
          continue;
        }

        std::vector<arrival_time_pair<Ntk>> arrival_times( cut->size() );
        std::transform( cut->begin(), cut->end(), arrival_times.begin(), [&]( auto leaf ) { return old_to_new[ntk_.index_to_node( leaf )]; });

        rebalancing_fn_( dest, cuts.truth_table( *cut ), arrival_times, best.level, best_size, [&]( arrival_time_pair<Ntk> const& cand, uint32_t cand_size ) {
          if ( cand.level < best.level || ( cand.level == best.level && cand_size < best_size ) )
          {
            best = cand;
            best_size = cand_size;
          }
        });
      }
      old_to_new[n] = best;
      current_level = std::max( current_level, best.level );
    } );

    ntk_.foreach_po( [&]( auto const& f ) {
      const auto s = old_to_new[f].f;
      dest.create_po( ntk_.is_complemented( f ) ? dest.create_not( s ) : s );
    } );

    return cleanup_dangling( dest );
  }

private:
  Ntk const& ntk_;
  rebalancing_function_t<Ntk> const& rebalancing_fn_;
  balancing_params const& ps_;
  balancing_stats& st_;
};

} // namespace detail

/*! Balancing of a logic network
 *
 * This function implements a dynamic-programming and cut-enumeration based
 * balancing algorithm.  It returns a new network of the same type and performs
 * generic balancing by providing a rebalancing function.
 *
 * The template parameter `CostFn` is only used to compute the critical paths,
 * when the `only_on_critical_path` parameter is assigned true.  Note that the
 * size for rewriting candidates is computed by the rebalancing function and
 * may not correspond to the cost given by CostFn.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const auto aig = ...;

      sop_balancing<aig_network> balance_fn;
      balance_params ps;
      ps.cut_enumeration_ps.cut_size = 6u;
      const auto balanced_aig = balance( aig, {balance_fn}, ps );
   \endverbatim
 */
template<class Ntk, class CostFn = unit_cost<Ntk>>
Ntk balancing( Ntk const& ntk, rebalancing_function_t<Ntk> const& rebalancing_fn = {}, balancing_params const& ps = {}, balancing_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );

  balancing_stats st;
  const auto dest = detail::balancing_impl<Ntk, CostFn>{ntk, rebalancing_fn, ps, st}.run();

  if ( pst )
  {
    *pst = st;
  }
  if ( ps.verbose )
  {
    st.report();
  }

  return dest;
}

} // namespace mockturtle
