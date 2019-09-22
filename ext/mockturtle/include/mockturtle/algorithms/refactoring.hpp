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
  \file refactoring.hpp
  \brief Refactoring

  \author Mathias Soeken
  \author Eleonora Testa 
*/
#pragma once

#include <iostream>

#include "../networks/mig.hpp"
#include "../traits.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/cut_view.hpp"
#include "../views/mffc_view.hpp"
#include "../views/topo_view.hpp"
#include "cleanup.hpp"
#include "cut_rewriting.hpp"
#include "detail/mffc_utils.hpp"
#include "dont_cares.hpp"
#include "simulation.hpp"

#include <fmt/format.h>
#include <kitty/dynamic_truth_table.hpp>

namespace mockturtle
{

/*! \brief Parameters for refactoring.
 *
 * The data structure `refactoring_params` holds configurable parameters with
 * default arguments for `refactoring`.
 */
struct refactoring_params
{
  /*! \brief Maximum number of PIs in MFFCs. */
  uint32_t max_pis{6};

  /*! \brief Allow zero-gain substitutions */
  bool allow_zero_gain{false};

  /*! \brief Use don't cares for optimization. */
  bool use_dont_cares{false};

  /*! \brief Show progress. */
  bool progress{false};

  /*! \brief Be verbose. */
  bool verbose{false};
};

/*! \brief Statistics for refactoring.
 *
 * The data structure `refactoring_stats` provides data collected by running
 * `refactoring`.
 */
struct refactoring_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{0};

  /*! \brief Accumulated runtime for computing MFFCs. */
  stopwatch<>::duration time_mffc{0};

  /*! \brief Accumulated runtime for rewriting. */
  stopwatch<>::duration time_refactoring{0};

  /*! \brief Accumulated runtime for simulating MFFCs. */
  stopwatch<>::duration time_simulation{0};

  void report() const
  {
    std::cout << fmt::format( "[i] total time       = {:>5.2f} secs\n", to_seconds( time_total ) );
    std::cout << fmt::format( "[i] MFFC time        = {:>5.2f} secs\n", to_seconds( time_mffc ) );
    std::cout << fmt::format( "[i] refactoring time = {:>5.2f} secs\n", to_seconds( time_refactoring ) );
    std::cout << fmt::format( "[i] simulation time  = {:>5.2f} secs\n", to_seconds( time_simulation ) );
  }
};

namespace detail
{

template<class Ntk, class RefactoringFn, class Iterator, class = void>
struct has_refactoring_with_dont_cares : std::false_type
{
};

template<class Ntk, class RefactoringFn, class Iterator>
struct has_refactoring_with_dont_cares<Ntk,
                                       RefactoringFn, Iterator,
                                       std::void_t<decltype( std::declval<RefactoringFn>()( std::declval<Ntk&>(),
                                                                                            std::declval<kitty::dynamic_truth_table>(),
                                                                                            std::declval<kitty::dynamic_truth_table>(),
                                                                                            std::declval<Iterator const&>(),
                                                                                            std::declval<Iterator const&>(),
                                                                                            std::declval<void( signal<Ntk> )>() ) )>> : std::true_type
{
};

template<class Ntk, class RefactoringFn, class Iterator>
inline constexpr bool has_refactoring_with_dont_cares_v = has_refactoring_with_dont_cares<Ntk, RefactoringFn, Iterator>::value;

template<class Ntk, class RefactoringFn, class NodeCostFn>
class refactoring_impl
{
public:
  refactoring_impl( Ntk& ntk, RefactoringFn&& refactoring_fn, refactoring_params const& ps, refactoring_stats& st, NodeCostFn const& cost_fn )
      : ntk( ntk ), refactoring_fn( refactoring_fn ), ps( ps ), st( st ), cost_fn( cost_fn ) {}

  void run()
  {
    const auto size = ntk.size();
    progress_bar pbar{ntk.size(), "refactoring |{0}| node = {1:>4}   cand = {2:>4}   est. reduction = {3:>5}", ps.progress};

    stopwatch t( st.time_total );

    ntk.clear_visited();
    ntk.clear_values();
    ntk.foreach_node( [&]( auto const& n ) {
      ntk.set_value( n, ntk.fanout_size( n ) );
    } );

    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      if ( i >= size )
      {
        return false;
      }
      if ( ntk.fanout_size( n ) == 0u )
      {
        return true;
      }
      const auto mffc = make_with_stopwatch<mffc_view<Ntk>>( st.time_mffc, ntk, n );

      pbar( i, i, _candidates, _estimated_gain );

      if ( mffc.num_pos() == 0 || mffc.num_pis() > ps.max_pis || mffc.size() < 4 )
      {
        return true;
      }

      std::vector<signal<Ntk>> leaves( mffc.num_pis() );
      mffc.foreach_pi( [&]( auto const& n, auto j ) {
        leaves[j] = ntk.make_signal( n );
      } );

      default_simulator<kitty::dynamic_truth_table> sim( mffc.num_pis() );
      const auto tt = call_with_stopwatch( st.time_simulation,
                                           [&]() { return simulate<kitty::dynamic_truth_table>( mffc, sim )[0]; } );

      signal<Ntk> new_f;
      {
        if ( ps.use_dont_cares )
        {
          if constexpr ( has_refactoring_with_dont_cares_v<Ntk, RefactoringFn, decltype( leaves.begin() )> )
          {
            std::vector<node<Ntk>> pivots;
            for ( auto const& c : leaves )
            {
              pivots.push_back( ntk.get_node( c ) );
            }
            stopwatch t( st.time_refactoring );

            refactoring_fn( ntk, tt, satisfiability_dont_cares( ntk, pivots, 16u ), leaves.begin(), leaves.end(), [&]( auto const& f ) { new_f = f; return false; } );
          }
          else
          {
            stopwatch t( st.time_refactoring );
            refactoring_fn( ntk, tt, leaves.begin(), leaves.end(), [&]( auto const& f ) { new_f = f; return false; } );
          }
        }
        else
        {
          stopwatch t( st.time_refactoring );
          refactoring_fn( ntk, tt, leaves.begin(), leaves.end(), [&]( auto const& f ) { new_f = f; return false; } );
        }
      }

      if ( n == ntk.get_node( new_f ) )
      {
        return true;
      }

      int32_t gain = recursive_deref( n );
      gain -= recursive_ref( ntk.get_node( new_f ) );

      if ( gain > 0 || ( ps.allow_zero_gain && gain == 0 ) )
      {
        ++_candidates;
        _estimated_gain += gain;
        ntk.substitute_node( n, new_f );
        ntk.set_value( n, 0 );
        ntk.set_value( ntk.get_node( new_f ), ntk.fanout_size( ntk.get_node( new_f ) ) );
        for ( auto i = 0u; i < leaves.size(); i++ )
        {
          ntk.set_value( ntk.get_node( leaves[i] ), ntk.fanout_size( ntk.get_node( leaves[i] ) ) );
        }
      }
      else
      {
        recursive_deref( ntk.get_node( new_f ) );
        recursive_ref( n );
      }
      return true;
    } );
  }

private:
  uint32_t recursive_deref( node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value{cost_fn( ntk, n )};
    ntk.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk.decr_value( ntk.get_node( s ) ) == 0 )
      {
        value += recursive_deref( ntk.get_node( s ) );
      }
    } );
    return value;
  }

  uint32_t recursive_ref( node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value{cost_fn( ntk, n )};
    ntk.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk.incr_value( ntk.get_node( s ) ) == 0 )
      {
        value += recursive_ref( ntk.get_node( s ) );
      }
    } );
    return value;
  }

private:
  Ntk& ntk;
  RefactoringFn&& refactoring_fn;
  refactoring_params const& ps;
  refactoring_stats& st;
  NodeCostFn cost_fn;

  uint32_t _candidates{0};
  uint32_t _estimated_gain{0};
};

} /* namespace detail */

/*! \brief Boolean refactoring.
 *
 * This algorithm performs refactoring by collapsing maximal fanout-free cones
 * (MFFCs) into truth tables and recreate a new network structure from it.
 * The algorithm performs changes directly in the input network and keeps the
 * substituted structures dangling in the network.  They can be cleaned up using
 * the `cleanup_dangling` algorithm.
 *
 * The refactoring function must be of type `NtkDest::signal(NtkDest&,
 * kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)` where
 * `LeavesIterator` can be dereferenced to a `NtkDest::signal`.  The last two
 * parameters compose an iterator pair where the distance matches the number of
 * variables of the truth table that is passed as second parameter.  There are
 * some refactoring algorithms in the folder
 * `mockturtle/algorithms/node_resyntesis`, since the resynthesis functions
 * have the same signature.
 *
 * **Required network functions:**
 * - `get_node`
 * - `size`
 * - `make_signal`
 * - `foreach_gate`
 * - `substitute_node`
 * - `clear_visited`
 * - `clear_values`
 * - `fanout_size`
 * - `set_value`
 * - `foreach_node`
 *
 * \param ntk Input network (will be changed in-place)
 * \param refactoring_fn Refactoring function
 * \param ps Refactoring params
 * \param pst Refactoring statistics
 * \param cost_fn Node cost function (a functor with signature `uint32_t(Ntk const&, node<Ntk> const&)`)
 */
template<class Ntk, class RefactoringFn, class NodeCostFn = detail::unit_cost<Ntk>>
void refactoring( Ntk& ntk, RefactoringFn&& refactoring_fn, refactoring_params const& ps = {}, refactoring_stats* pst = nullptr, NodeCostFn const& cost_fn = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_clear_visited_v<Ntk>, "Ntk does not implement the clear_visited method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );

  refactoring_stats st;
  detail::refactoring_impl<Ntk, RefactoringFn, NodeCostFn> p( ntk, refactoring_fn, ps, st, cost_fn );
  p.run();
  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */
