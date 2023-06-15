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

/*
  \file aqfp_flow_date.cpp
  \brief AQFP synthesis flow

  This file contains the code to reproduce the experiment (Table I)
  in the following paper:
  "Majority-based Design Flow for AQFP Superconducting Family", DATE 2022,
  by Giulia Meuli, Vinicius Possani, Rajinder Singh, Siang-Yun Lee,
  Alessandro Tempia Calvino, Dewmini Sudara Marakkalage, Patrick Vuillod,
  Luca Amaru, Scott Chase, Jamil Kawa, and Giovanni De Micheli.
 */

#include <mockturtle/algorithms/aqfp/aqfp_db.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_fanout_resyn.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_node_resyn.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_resynthesis.hpp>
#include <mockturtle/algorithms/aqfp/buffer_insertion.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/networks/aqfp.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/properties/aqfpcost.hpp>
#include <mockturtle/views/depth_view.hpp>

#include "experiments.hpp"

#include <fmt/format.h>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace experiments;
using namespace mockturtle;

static const std::string benchmark_repo_path = "SCE-benchmarks";

/* AQFP benchmarks */
std::vector<std::string> aqfp_benchmarks = {
    "5xp1", "c1908", "c432", "c5315", "c880", "chkn", "count", "dist", "in5", "in6", "k2",
    "m3", "max512", "misex3", "mlp4", "prom2", "sqr6", "x1dn" };

std::string benchmark_aqfp_path( std::string const& benchmark_name )
{
  return fmt::format( "{}/MCNC/original/{}.v", benchmark_repo_path, benchmark_name );
}

template<class Ntk>
inline bool abc_cec_aqfp( Ntk const& ntk, std::string const& benchmark )
{
  return abc_cec_impl( ntk, benchmark_aqfp_path( benchmark ) );
}

/* Supplementary functions for AQFP resynthesis */
template<typename Result>
bool has_better_cost( Result& current, Result& previous )
{
  if ( current.first < previous.first )
    return true;

  if ( current.first > previous.first )
    return false;

  return current.second < previous.second;
}

template<typename Result>
bool has_better_level( Result& current, Result& previous )
{
  if ( current.second < previous.second )
    return true;

  if ( current.second > previous.second )
    return false;

  return current.first < previous.first;
}

template<typename Ntk>
mockturtle::klut_network lut_map_abc( Ntk const& ntk, uint32_t k = 4, std::string name = {} )
{
  std::string tempfile1 = "temp1_" + name + ".blif";
  std::string tempfile2 = "temp2_" + name + ".blif";

  mockturtle::write_blif( ntk, tempfile1 );

  system( fmt::format( "abc -q \"{}; &get; &if -K {}; &put; write_blif {}\" >> /dev/null 2>&1", tempfile1, k, tempfile2 ).c_str() );

  mockturtle::klut_network klut;
  if ( lorina::read_blif( tempfile2, mockturtle::blif_reader( klut ) ) != lorina::return_code::success )
  {
    std::cout << "FATAL NEW LUT MAP - Reading mapped network failed! " << tempfile1 << " " << tempfile2 << std::endl;
    std::abort();
    return klut;
  }

  system( fmt::format( "rm {}", tempfile1 ).c_str() );
  system( fmt::format( "rm {}", tempfile2 ).c_str() );
  return klut;
}

template<typename T>
auto count_majorities( T& ntk )
{
  std::unordered_map<uint32_t, uint32_t> counts;
  ntk.foreach_gate( [&]( auto n ) { counts[ntk.fanin_size( n )]++; } );
  return counts;
}

struct opt_params_t
{
  uint32_t optimization_rounds{ 1 };
  uint32_t max_remapping_rounds{ 3 };
  uint32_t max_resynthesis_rounds{ 10 };
  std::unordered_map<uint32_t, double> gate_costs{ { 3u, 6.0 }, { 5u, 10.0 } };
  std::unordered_map<uint32_t, double> splitters{ { 1u, 2.0 }, { 4u, 2.0 } };
  mutable mockturtle::aqfp_db<> db{ gate_costs, splitters };
  mutable mockturtle::aqfp_db<> db_last{ gate_costs, splitters };
  mockturtle::aqfp_node_resyn_strategy strategy{ mockturtle::aqfp_node_resyn_strategy::area };
  std::string lutmap{ "abc" };
  mockturtle::aqfp_assumptions assume{ false, false, true, 4u };
};

struct opt_stats_t
{
  uint32_t maj3_after_remapping;
  uint32_t level_after_remapping;
  uint32_t maj3_after_exact;
  uint32_t maj5_after_exact;
  uint32_t jj_after_exact;
  uint32_t jj_level_after_exact;
};

mig_network remapping_round( mig_network const& ntk, exact_library<mig_network, mig_npn_resynthesis> const& exact_lib, opt_params_t const& opt_params, opt_stats_t& stats )
{
  map_params psm;
  psm.skip_delay_round = false;
  map_stats stm;

  mig_network mig = cleanup_dangling( ntk );

  /* initial mig mapping, depth-oriented */
  for ( auto i = 0u; i < opt_params.max_remapping_rounds; ++i )
  {
    uint32_t old_mig_depth = depth_view( ntk ).depth();
    uint32_t old_mig_size = ntk.num_gates();

    mig_network mig_map = map( mig, exact_lib, psm, &stm );

    if ( depth_view( mig_map ).depth() > old_mig_depth ||
         ( depth_view( mig_map ).depth() == old_mig_depth && mig_map.num_gates() >= old_mig_size ) )
    {
      break;
    }
    mig = cleanup_dangling( mig_map );
  }

  stats.maj3_after_remapping = mig.num_gates();
  stats.level_after_remapping = depth_view( mig ).depth();

  return mig;
}

template<typename Ntk>
aqfp_network aqfp_exact_resynthesis( Ntk& ntk, opt_params_t const& params, opt_stats_t& stats )
{
  const uint32_t max_branching_factor = std::max_element( params.splitters.begin(), params.splitters.end(), [&]( auto s1, auto s2 ) { return s1.first < s2.first; } )
                                            ->first;
  assert( max_branching_factor == params.assume.splitter_capacity );

  mockturtle::aqfp_network_cost cost_fn( params.assume, params.gate_costs, params.splitters );
  mockturtle::aqfp_node_resyn n_resyn( params.db, { params.assume, params.splitters, params.strategy } );
  mockturtle::aqfp_node_resyn n_resyn_last( params.db_last, { params.assume, params.splitters, params.strategy } );
  mockturtle::aqfp_fanout_resyn fo_resyn( params.assume );

  mockturtle::klut_network klut;

  if ( params.lutmap == "abc" )
  {
    klut = lut_map_abc( ntk, 4 );
  }
  else
  {
    assert( false );
  }

  mockturtle::aqfp_network aqfp;
  mockturtle::aqfp_network aqfp_last;

  auto res = mockturtle::aqfp_resynthesis( aqfp, klut, n_resyn, fo_resyn );
  auto res_last = mockturtle::aqfp_resynthesis( aqfp_last, klut, n_resyn_last, fo_resyn );
  std::pair<double, uint32_t> cost_level = { cost_fn( aqfp_last, res_last.node_level, res_last.po_level ), res_last.critical_po_level() };

  mockturtle::aqfp_network best_aqfp = aqfp_last;
  auto best_res = res_last;
  auto best_cost_level = cost_level;

  for ( auto i = 2u; i <= params.max_resynthesis_rounds; i++ )
  {

    if ( params.lutmap == "abc" )
    {
      klut = lut_map_abc( aqfp, 4 );
    }
    else
    {
      assert( false );
    }

    aqfp = mockturtle::aqfp_network();
    aqfp_last = mockturtle::aqfp_network();
    res = mockturtle::aqfp_resynthesis( aqfp, klut, n_resyn, fo_resyn );
    res_last = mockturtle::aqfp_resynthesis( aqfp_last, klut, n_resyn_last, fo_resyn );
    cost_level = { cost_fn( aqfp_last, res_last.node_level, res_last.po_level ), res_last.critical_po_level() };

    if ( params.strategy == mockturtle::aqfp_node_resyn_strategy::area )
    {
      if ( has_better_cost( cost_level, best_cost_level ) )
      {
        best_aqfp = aqfp_last;
        best_res = res_last;
        best_cost_level = cost_level;
      }
    }
    else
    {
      assert( params.strategy == mockturtle::aqfp_node_resyn_strategy::delay );
      if ( has_better_level( cost_level, best_cost_level ) )
      {
        best_aqfp = aqfp_last;
        best_res = res_last;
        best_cost_level = cost_level;
      }
    }
  }

  auto maj_counts = count_majorities( best_aqfp );
  stats.maj3_after_exact = maj_counts[3];
  stats.maj5_after_exact = maj_counts[5];
  stats.jj_after_exact = static_cast<uint32_t>( best_cost_level.first );
  stats.jj_level_after_exact = best_cost_level.second;

  return best_aqfp;
}

/* `database_type` is either 'db3' (only maj-3 gates) or 'db5' (maj-3 and maj-5 gates). */
std::ifstream get_database( std::string database_type )
{
  const std::string db_path = fmt::format( "aqfp_database/{}.txt", database_type );
  if ( !std::filesystem::exists( db_path ) )
  {
    fmt::print( "Cloning the aqfp database repository to working directory...\n" );
    system( "git clone https://github.com/mdsudara/AQFP-Database.git aqfp_database" );
  }

  std::ifstream db_file( db_path );
  return db_file;
}

int main( int argc, char** argv )
{
  opt_params_t opt_params;

  std::string exact_syn_db_cfg = "db5";
  auto i = 1;

  while ( i < argc )
  {
    std::string arg( argv[i] );
    if ( arg[0] == '-' )
    {
      if ( arg == "-balance_pis" )
        opt_params.assume.balance_pis = true;
      else if ( arg == "-no-balance_pis" )
        opt_params.assume.balance_pis = false;
      else if ( arg == "-branch_pis" )
        opt_params.assume.branch_pis = true;
      else if ( arg == "-no-branch_pis" )
        opt_params.assume.branch_pis = false;
      else if ( arg == "-balance_pos" )
        opt_params.assume.balance_pos = true;
      else if ( arg == "-no-balance_pos" )
        opt_params.assume.balance_pos = false;
      else
      {
        i++;
        std::string val( i < argc ? argv[i] : "" );
        if ( arg == "-opt_rounds" )
          opt_params.optimization_rounds = std::stoul( val );
        else if ( arg == "-remap_rounds" )
          opt_params.max_remapping_rounds = std::stoul( val );
        else if ( arg == "-resyn_rounds" )
          opt_params.max_resynthesis_rounds = std::stoul( val );
        else if ( arg == "-db_cfg" )
        {
          exact_syn_db_cfg = val;
        }
        else if ( arg == "-exact_resyn_strategy" )
          opt_params.strategy = ( val == "cost" )
                                    ? mockturtle::aqfp_node_resyn_strategy::area
                                    : mockturtle::aqfp_node_resyn_strategy::delay;
        else if ( arg == "-lutmap" )
          opt_params.lutmap = val;
        else
        {
          fmt::print( "Unrecognized argument `{}` with value `{}` \n", arg, val );
          return 0;
        }
      }
      i++;
    }
    else
    {
      fmt::print( "Unrecognized argument `{}` with value `{}` \n", arg );
      return 0;
    }
  }

  /* library to map to MIGs */
  mig_npn_resynthesis resyn{ true };
  exact_library_params eps;
  exact_library<mig_network, mig_npn_resynthesis> exact_lib( resyn, eps );

  /* database loading for aqfp resynthesis*/
  auto db3_str = get_database( "db3" );
  auto db5_str( exact_syn_db_cfg == "all3" ? get_database( "db3" ) : get_database( "db5" ) );

  opt_params.db.load_db( db3_str );
  opt_params.db_last.load_db( db5_str );

  /* download benchmarks */
  if ( !std::filesystem::exists( benchmark_repo_path ) )
  {
    fmt::print( "Cloning the SCE benchmark repository to {}...\n", benchmark_repo_path );
    system( fmt::format( "git clone https://github.com/lsils/SCE-benchmarks.git {}", benchmark_repo_path ).c_str() );
  }

  experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, bool> exp(
      "aqfp_flow_date", "bench", "size_init", "dep_init", "size_remap", "dep_remap", "maj3_exact", "maj5_exact", "JJ_exact", "JJ_dep_exact", "JJ_fin", "JJ_dep_fin", "cec" );

  for ( auto const& benchmark : aqfp_benchmarks )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    opt_stats_t opt_stats;

    mig_network mig;
    if ( lorina::read_verilog( benchmark_aqfp_path( benchmark ), verilog_reader( mig ) ) != lorina::return_code::success )
    {
      continue;
    }

    const uint32_t size_before = mig.num_gates();
    const uint32_t depth_before = depth_view( mig ).depth();

    aqfp_network aqfp;
    /* main optimization loop */
    for ( auto i = 0u; i < opt_params.optimization_rounds; ++i )
    {
      auto mig_opt = remapping_round( mig, exact_lib, opt_params, opt_stats );
      aqfp = aqfp_exact_resynthesis( mig_opt, opt_params, opt_stats );
    }

    /* buffer insertion */
    buffer_insertion_params buf_ps;
    buf_ps.scheduling = buffer_insertion_params::better;
    buf_ps.optimization_effort = buffer_insertion_params::until_sat;
    buf_ps.max_chunk_size = std::numeric_limits<uint32_t>::max();
    buf_ps.assume.splitter_capacity = 4u;
    buf_ps.assume.branch_pis = false;
    buf_ps.assume.balance_pis = false;
    buf_ps.assume.balance_pos = true;
    buffer_insertion buf_inst( aqfp, buf_ps );
    uint32_t num_bufs = buf_inst.dry_run();
    uint32_t num_jjs = opt_stats.maj3_after_exact * 6 + opt_stats.maj5_after_exact * 10 + num_bufs * 2;
    uint32_t jj_depth = buf_inst.depth();

    const auto cec = abc_cec_aqfp( aqfp, benchmark );

    exp( benchmark, size_before, depth_before,
         opt_stats.maj3_after_remapping, opt_stats.level_after_remapping,
         opt_stats.maj3_after_exact, opt_stats.maj5_after_exact, opt_stats.jj_after_exact, opt_stats.jj_level_after_exact,
         num_jjs, jj_depth, cec );
  }

  exp.save();
  exp.table();

  return 0;
}
