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
  \file sim_resub.hpp
  \brief Simulation-Guided Resubstitution

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "resubstitution.hpp"
#include "circuit_validator.hpp"
#include "simulation.hpp"
#include "pattern_generation.hpp"
#include "resyn_engines/xag_resyn_engines.hpp"
#include "../io/write_patterns.hpp"
#include "../networks/aig.hpp"
#include "../networks/xag.hpp"
#include "../utils/abc_resub.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"

#include <bill/bill.hpp>
#include <kitty/kitty.hpp>
#include <fmt/format.h>

#include <variant>
#include <algorithm>

namespace mockturtle
{

namespace detail
{

struct abc_resub_functor_stats
{
  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_compute_function{0};

  /*! \brief Time for interfacing with ABC. */
  stopwatch<>::duration time_interface{0};

  /*! \brief Number of found solutions. */
  uint32_t num_success{0};

  /*! \brief Number of times that no solution can be found. */
  uint32_t num_fail{0};

  void report() const
  {
    fmt::print( "[i]     <ResubFn: abc_resub_functor>\n" );
    fmt::print( "[i]         #solution = {:6d}\n", num_success );
    fmt::print( "[i]         #invoke   = {:6d}\n", num_success + num_fail );
    fmt::print( "[i]         ABC time:   {:>5.2f} secs\n", to_seconds( time_compute_function ) );
    fmt::print( "[i]         interface:  {:>5.2f} secs\n", to_seconds( time_interface ) );
  }
};

template<typename Ntk>
class abc_resub_functor
{
public:
  using stats = abc_resub_functor_stats;
  using index_list_t = xag_index_list;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = kitty::partial_truth_table;

  explicit abc_resub_functor( Ntk const& ntk, resubstitution_params const& ps, stats& st, unordered_node_map<TT, Ntk> const& tts, node const& root, std::vector<node> const& divs )
      : ntk( ntk ), ps( ps ), st( st ), tts( tts ), root( root ), divs( divs ), num_blocks( 0 )
  { }

  ~abc_resub_functor()
  {
    call_with_stopwatch( st.time_interface, [&]() {
      abcresub::Abc_ResubPrepareManager( 0 );
    } );
  }

  void check_num_blocks()
  {
    if ( tts[ntk.get_constant( false )].num_blocks() != num_blocks )
    {
      num_blocks = tts[ntk.get_constant( false )].num_blocks();
      call_with_stopwatch( st.time_interface, [&]() {
        abcresub::Abc_ResubPrepareManager( num_blocks );
      });
    }
  }

  std::optional<index_list_t> operator()( TT const& care, uint32_t potential_gain, uint32_t& last_gain )
  {
    auto const num_inserts = std::min( potential_gain - 1, ps.max_inserts );
    check_num_blocks();
    abc_resub rs( 2ul + divs.size(), num_blocks, ps.max_divisors_k );
    call_with_stopwatch( st.time_interface, [&]() {
      rs.add_root( tts[root], care );
      rs.add_divisors( std::begin( divs ), std::end( divs ), tts );
    });

    auto const res = call_with_stopwatch( st.time_compute_function, [&]() {
      if constexpr ( std::is_same<typename Ntk::base_type, xag_network>::value )
      {
        return rs.compute_function( num_inserts, true );
      }
      else
      {
        return rs.compute_function( num_inserts, false );
      }
    } );

    if ( res )
    {
      assert( res->num_gates() <= num_inserts );
      ++st.num_success;
      last_gain = potential_gain - res->num_gates();
      return *res;
    }
    else /* loop until no result can be found by the engine */
    {
      ++st.num_fail;
      return std::nullopt;
    }
  }

private:
  Ntk const& ntk;
  resubstitution_params const& ps;
  stats& st;

  unordered_node_map<TT, Ntk> const& tts;
  node const& root;
  std::vector<node> const& divs;

  uint32_t num_blocks;
};

template<class EngineStat>
struct resyn_functor_stats
{
  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_compute_function{0};

  /*! \brief Number of found solutions. */
  uint32_t num_success{0};

  /*! \brief Number of times that no solution can be found. */
  uint32_t num_fail{0};

  EngineStat engine_st;

  void report() const
  {
    fmt::print( "[i]     <ResubFn: resyn_functor>\n" );
    fmt::print( "[i]         #solution = {:6d}\n", num_success );
    fmt::print( "[i]         #invoke   = {:6d}\n", num_success + num_fail );
    fmt::print( "[i]         engine time:{:>5.2f} secs\n", to_seconds( time_compute_function ) );
    engine_st.report();
  }
};

/*! \brief Interfacing resubstitution functor with various resynthesis engines for `simulation_based_resub_engine`.
 * 
 * The resynthesis engine `ResynEngine` should provide the following interfaces:
 * - Constructor: `ResynEngine( kitty::partial_truth_table const& target,`
 * `kitty::partial_truth_table const& care, ResynEngine::stats& st, ResynEngine::params const& ps )`
 * - `std::optional<ResynEngine::index_list_t> operator()( std::vector<Ntk::node>::iterator begin,`
 * `std::vector<Ntk::node>::iterator end, unordered_node_map<kitty::partial_truth_table, Ntk> const& tts )`
 * - `ResynEngine::params` should have at least one member `uint32_t max_size` defining
 * the maximum size of the dependency circuit.
 */
template<typename Ntk, typename ResynEngine>
class resyn_functor
{
public:
  using stats = resyn_functor_stats<typename ResynEngine::stats>;
  using index_list_t = typename ResynEngine::index_list_t;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = kitty::partial_truth_table;
  static_assert( std::is_same_v<TT, typename ResynEngine::truth_table_t>, "truth table type of ResynEngine is not kitty::partial_truth_table" );

  explicit resyn_functor( Ntk const& ntk, resubstitution_params const& ps, stats& st, unordered_node_map<TT, Ntk> const& tts, node const& root, std::vector<node> const& divs )
      : ntk( ntk ), ps( ps ), st( st ), tts( tts ), root( root ), divs( divs )
  { }

  std::optional<index_list_t> operator()( TT const& care, uint32_t potential_gain, uint32_t& last_gain )
  {
    typename ResynEngine::params ps_resyn;
    ps_resyn.max_size = std::min( potential_gain - 1, ps.max_inserts );
    if ( std::is_same_v<ResynEngine, xag_resyn_engine<TT>> )
    {
      ps_resyn.use_xor = std::is_same_v<typename Ntk::base_type, xag_network>;
      ps_resyn.max_binates = ps.max_divisors_k;
    }
    ResynEngine engine( tts[root], care, st.engine_st, ps_resyn );

    auto const res = call_with_stopwatch( st.time_compute_function, [&]() {
      return engine( std::begin( divs ), std::end( divs ), tts );
    } );

    if ( res )
    {
      ++st.num_success;
      last_gain = potential_gain - res->num_gates();
      return *res;
    }
    else /* loop until no result can be found by the engine */
    {
      ++st.num_fail;
      return std::nullopt;
    }
  }

private:
  Ntk const& ntk;
  resubstitution_params const& ps;
  stats& st;

  unordered_node_map<TT, Ntk> const& tts;
  node const& root;
  std::vector<node> const& divs;
};

template<typename ResubFnSt>
struct sim_resub_stats
{
  /*! \brief Time for pattern generation. */
  stopwatch<>::duration time_patgen{0};

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{0};

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_sat{0};
  stopwatch<>::duration time_sat_restart{0};

  /*! \brief Time for computing ODCs. */
  stopwatch<>::duration time_odc{0};

  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_functor{0};

  /*! \brief Number of patterns used. */
  uint32_t num_pats{0};

  /*! \brief Number of counter-examples. */
  uint32_t num_cex{0};

  /*! \brief Number of successful resubstitutions. */
  uint32_t num_resub{0};

  /*! \brief Number of SAT solver timeout. */
  uint32_t num_timeout{0};

  ResubFnSt functor_st;

  void report() const
  {
    fmt::print( "[i] <ResubEngine: simulation_based_resub_engine>\n" );
    fmt::print( "[i]     ========  Stats  ========\n" );
    fmt::print( "[i]     #pat     = {:6d}\n", num_pats );
    fmt::print( "[i]     #resub   = {:6d}\n", num_resub );
    fmt::print( "[i]     #CEX     = {:6d}\n", num_cex );
    fmt::print( "[i]     #timeout = {:6d}\n", num_timeout );
    fmt::print( "[i]     ======== Runtime ========\n" );
    fmt::print( "[i]     generate pattern: {:>5.2f} secs\n", to_seconds( time_patgen ) );
    fmt::print( "[i]     simulation:       {:>5.2f} secs\n", to_seconds( time_sim ) );
    fmt::print( "[i]     SAT solve:        {:>5.2f} secs\n", to_seconds( time_sat ) );
    fmt::print( "[i]     SAT restart:      {:>5.2f} secs\n", to_seconds( time_sat_restart ) );
    fmt::print( "[i]     compute ODCs:     {:>5.2f} secs\n", to_seconds( time_odc ) );
    fmt::print( "[i]     compute function: {:>5.2f} secs\n", to_seconds( time_functor ) );
    fmt::print( "[i]     ======== Details ========\n" );
    functor_st.report();
    fmt::print( "[i]     =========================\n\n" );
  }
};

/*! \brief Simulation-based resubstitution engine.
 * 
 * This engine simulates in the whole network and uses partial truth tables
 * to find potential resubstitutions. It then formally verifies the resubstitution
 * candidates given by the resubstitution functor. If the validation fails,
 * a counter-example will be added to the simulation patterns, and the functor
 * will be invoked again with updated truth tables, looping until it returns
 * `std::nullopt`. This engine only requires the divisor collector to prepare `divs`.
 *
 * Please refer to the following paper for further details.
 *
 * [1] Simulation-Guided Boolean Resubstitution. IWLS 2020 (arXiv:2007.02579).
 *
 * Interfaces of the resubstitution functor:
 * - Constructor: `resub_fn( Ntk const& ntk, resubstitution_params const& ps, ResubFnSt& st,`
 * `unordered_node_map<TT, Ntk> const& tts, node const& root, std::vector<node> const& divs )`
 * - A public `operator()`: `std::optional<index_list_t> operator()`
 * `( TT const& care, MffcRes potential_gain, uint32_t& last_gain )`
 *
 * Compatible resubstitution functors implemented:
 * - `abc_resub_functor`: interfacing functor with `abcresub`, ported from ABC (deprecated).
 * - `resyn_functor`: interfacing functor with various resynthesis engines defined in `resyn_engines`.
 *
 * \param validator_t Specialization of `circuit_validator`.
 * \param ResubFn Resubstitution functor to compute the resubstitution.
 * \param MffcRes Typename of `potential_gain` needed by the resubstitution functor.
 */
template<class Ntk, typename validator_t = circuit_validator<Ntk, bill::solvers::bsat2, false, true, false>, class ResubFn = resyn_functor<Ntk, xag_resyn_engine<kitty::partial_truth_table>>, typename MffcRes = uint32_t>
class simulation_based_resub_engine
{
public:
  static constexpr bool require_leaves_and_mffc = false;
  using stats = sim_resub_stats<typename ResubFn::stats>;
  using mffc_result_t = MffcRes;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = kitty::partial_truth_table;

  explicit simulation_based_resub_engine( Ntk& ntk, resubstitution_params const& ps, stats& st )
      : ntk( ntk ), ps( ps ), st( st ), tts( ntk ), validator( ntk, vps )
  {
    if constexpr ( !validator_t::use_odc_ )
    {
      assert( ps.odc_levels == 0 && "to consider ODCs, circuit_validator::use_odc (the last template parameter) has to be turned on" );
    }
    else
    {
      vps.odc_levels = ps.odc_levels;
    }

    vps.conflict_limit = ps.conflict_limit;
    vps.random_seed = ps.random_seed;

    ntk._events->on_add.emplace_back( [&]( const auto& n ) {
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_node<Ntk>( ntk, n, tts, sim );
      });
    } );

    /* prepare simulation patterns */
    call_with_stopwatch( st.time_patgen, [&]() {
      if ( ps.pattern_filename )
      {
        sim = partial_simulator( *ps.pattern_filename );
      }
      else
      {
        sim = partial_simulator( ntk.num_pis(), 1024 );
        pattern_generation( ntk, sim );
      }
    });
    st.num_pats = sim.num_bits();

    /* first simulation: the whole circuit; from 0 bits. */
    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    });
  }

  ~simulation_based_resub_engine()
  {
    if ( ps.save_patterns )
    {
      write_patterns( sim, *ps.save_patterns );
    }
  }

  std::optional<signal> run( node const& n, std::vector<node> const& divs, mffc_result_t potential_gain, uint32_t& last_gain )
  {
    ResubFn resub_fn( ntk, ps, st.functor_st, tts, n, divs );
    for ( auto j = 0u; j < ps.max_trials; ++j )
    {
      check_tts( n );
      for ( auto const& d : divs )
      {
        check_tts( d );
      }

      TT const care = call_with_stopwatch( st.time_odc, [&]() {
        return ( ps.odc_levels == 0 ) ? sim.compute_constant( true ) : ~observability_dont_cares( ntk, n, sim, tts, ps.odc_levels );
      });
      const auto res = call_with_stopwatch( st.time_functor, [&]() {
        return resub_fn( care, potential_gain, last_gain );
      });
      if ( res )
      {
        auto const& id_list = *res;
        assert( id_list.num_pos() == 1u );
        auto valid = call_with_stopwatch( st.time_sat, [&]() {
          return validator.validate( n, divs, id_list );
        });
        if ( valid )
        {
          if ( *valid )
          {
            ++st.num_resub;
            signal out_sig;
            std::vector<signal> divs_sig( divs.size() );
            std::transform( divs.begin(), divs.end(), divs_sig.begin(), [&]( const node n ){
              return ntk.make_signal( n );
            });
            insert( ntk, divs_sig.begin(), divs_sig.end(), id_list, [&]( signal const& s ){
              out_sig = s;
            });
            if constexpr ( validator_t::use_odc_ )
            {
              call_with_stopwatch( st.time_sat_restart, [&]() {
                validator.update();
              });
            }
            return out_sig;
          }
          else
          {
            found_cex();
            continue;
          }
        }
        else /* timeout */
        {
          return std::nullopt;
        }
      }
      else /* functor can not find any potential resubstitution */
      {
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

  void found_cex()
  {
    ++st.num_cex;
    call_with_stopwatch( st.time_sim, [&]() {
      sim.add_pattern( validator.cex );
    });

    /* re-simulate the whole circuit (for the last block) when a block is full */
    if ( sim.num_bits() % 64 == 0 )
    {
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_nodes<Ntk>( ntk, tts, sim, false );
      } );
    }
  }

  void check_tts( node const& n )
  {
    if ( tts[n].num_bits() != sim.num_bits() )
    {
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_node<Ntk>( ntk, n, tts, sim );
      } );
    }
  }

private:
  Ntk& ntk;
  resubstitution_params const& ps;
  stats& st;

  unordered_node_map<TT, Ntk> tts;
  partial_simulator sim;

  validator_params vps;
  validator_t validator;
}; /* simulation_based_resub_engine */

} /* namespace detail */

template<class Ntk>
void sim_resubstitution( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( std::is_same<typename Ntk::base_type, aig_network>::value || std::is_same<typename Ntk::base_type, xag_network>::value, "Currently only supports AIG and XAG" );

  using resub_view_t = fanout_view<depth_view<Ntk>>;
  depth_view<Ntk> depth_view{ntk};
  resub_view_t resub_view{depth_view};

  if ( ps.odc_levels != 0 )
  {
    using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, true>;
    using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t>>;

    resubstitution_stats st;
    typename resub_impl_t::engine_st_t engine_st;
    typename resub_impl_t::collector_st_t collector_st;

    resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
    p.run();

    if ( ps.verbose )
    {
      st.report();
      collector_st.report();
      engine_st.report();
    }

    if ( pst )
    {
      *pst = st;
    }
  }
  else
  {
    using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t>>;

    resubstitution_stats st;
    typename resub_impl_t::engine_st_t engine_st;
    typename resub_impl_t::collector_st_t collector_st;

    resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
    p.run();

    if ( ps.verbose )
    {
      st.report();
      collector_st.report();
      engine_st.report();
    }

    if ( pst )
    {
      *pst = st;
    }
  }
}

} /* namespace mockturtle */