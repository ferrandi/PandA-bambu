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

#if defined(BILL_HAS_Z3)

#include "experiments.hpp"

#include <bill/sat/interface/abc_bsat2.hpp>
#include <bill/sat/interface/z3.hpp>
#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <mockturtle/algorithms/detail/minmc_xags.hpp>
#include <mockturtle/algorithms/exact_mc_synthesis.hpp>
#include <mockturtle/algorithms/xag_optimization.hpp>
#include <mockturtle/io/index_list.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/utils/progress_bar.hpp>

#include <cstdint>
#include <string>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, uint32_t, double> exp( "exact_mc_synthesis", "name", "XOR gates", "AND gates", "total runtime" );

  // All spectral classes
  const auto all_spectral = [&]( uint32_t num_vars, bool multiple, bool minimize_xor, bool sat_linear_resyn ) {
    stopwatch<>::duration time{};
    uint32_t xor_gates{}, and_gates{};
    const auto prefix = fmt::format( "exact_mc_synthesis{}{}{}", multiple ? "-multiple" : "", minimize_xor ? "-xor" : "", sat_linear_resyn ? "-resyn" : "" );
    progress_bar pbar( mockturtle::detail::minmc_xags[num_vars].size(), prefix + " |{}| class = {}, function = {}, time so far = {:.2f}", true );
    for ( auto const& [cls, word, list, expr] : mockturtle::detail::minmc_xags[num_vars] )
    {
      (void)expr;
      stopwatch<> t( time );

      if ( word == 0u )
      {
        continue;
      }

      kitty::dynamic_truth_table tt( num_vars );
      kitty::create_from_words( tt, &word, &word + 1 );
      pbar( cls, cls, kitty::to_hex( tt ), to_seconds( time ) );

      exact_mc_synthesis_params ps;
      ps.break_symmetric_variables = true;
      ps.break_subset_symmetries = true;
      ps.break_multi_level_subset_symmetries = false;
      ps.ensure_to_use_gates = true;
      ps.auto_update_xor_bound = minimize_xor;
      ps.conflict_limit = 50000u;
      ps.ignore_conflict_limit_for_first_solution = true;

      xag_network xag;

      if ( multiple )
      {
        const auto xags = exact_mc_synthesis_multiple<xag_network, bill::solvers::z3>( tt, 50u, ps );
        xag = *std::min_element( xags.begin(), xags.end(), [&]( auto const& x1, auto const& x2 ) { return x1.num_gates() < x2.num_gates(); } );
      }
      else
      {
        xag = exact_mc_synthesis<xag_network, bill::solvers::z3>( tt, ps );
      }

      if ( sat_linear_resyn )
      {
        xag = exact_linear_resynthesis_optimization<bill::solvers::z3>( xag, 500000u );
      }

      const auto num_ands = *multiplicative_complexity( xag );
      const auto num_xors = xag.num_gates() - num_ands;

      xor_gates += num_xors;
      and_gates += num_ands;

      /* verify MC */
      const auto xag_db = create_from_binary_index_list<xag_network>( list.begin() );
      if ( *multiplicative_complexity( xag_db ) != *multiplicative_complexity( xag ) )
      {
        fmt::print( "[e] MC mismatch for {}, got {}, expected {}.\n", kitty::to_hex( tt ), *multiplicative_complexity( xag ), *multiplicative_complexity( xag_db ) );
        std::abort();
      }
    }
    const auto name = fmt::format( "all-spectral-{}{}{}{}", num_vars, multiple ? "-multiple" : "", minimize_xor ? "-xor" : "", sat_linear_resyn ? "-resyn" : "" );
    exp( name, xor_gates, and_gates, to_seconds( time ) );
  };

  // Confirm some 6-input functions with MC = 4
  const auto practical6 = [&]( bool multiple, bool minimize_xor, bool sat_linear_resyn ) {
    stopwatch<>::duration time{};
    uint32_t xor_gates{}, and_gates{};
    const auto prefix = fmt::format( "exact_mc_synthesis{}{}{}", multiple ? "-multiple" : "", minimize_xor ? "-xor" : "", sat_linear_resyn ? "-resyn" : "" );

    std::vector<uint64_t> functions = {
        UINT64_C(0x6996966996696996),
        UINT64_C(0x9669699669969669),
        UINT64_C(0x0000000069969669),
        UINT64_C(0x0000000096696996),
        UINT64_C(0x9669699600000000),
        UINT64_C(0x6996966900000000),
        UINT64_C(0x1ee1e11ee11e1ee1),
        UINT64_C(0xe11e1ee11ee1e11e),
        UINT64_C(0x6996699669969669),
        UINT64_C(0x00000000e11e1ee1),
        UINT64_C(0x6969699696969669),
        UINT64_C(0x56a9a956a95656a9),
        UINT64_C(0x000000001ee1e11e),
        UINT64_C(0x9669966996696996),
        UINT64_C(0x9669966996690000),
        UINT64_C(0x9696966969696996),
        UINT64_C(0x00000000953f6ac0),
        UINT64_C(0xa95656a956a9a956),
        UINT64_C(0x0000000000006996),
        UINT64_C(0x00000000ff00807f),
        UINT64_C(0x6669999699966669),
        UINT64_C(0x0000000096969669),
        UINT64_C(0x96665aaa3cccf000),
        UINT64_C(0xe888a000c0000000),
        UINT64_C(0x00000000153f55ff),
        UINT64_C(0xaa00aa0080000000),
        UINT64_C(0xaa00aa00953f55ff),
        UINT64_C(0xc0c00000eac0aa00),
        UINT64_C(0x9996666966699996),
        UINT64_C(0x0000000069696996),
        UINT64_C(0x00000000999a0000),
        UINT64_C(0x0000ffff00006665),
        UINT64_C(0xffff00006665999a),
        UINT64_C(0xe11e1ee100000000),
        UINT64_C(0x0000000000009669),
        UINT64_C(0x6996699669960000),
        UINT64_C(0x00000000ffff6996),
        UINT64_C(0x00000000a95656a9),
        UINT64_C(0x2882822882282882),
        UINT64_C(0x1ee1e11e00000000),
        UINT64_C(0x9696966900000000),
        UINT64_C(0x0000000099966669),
        UINT64_C(0x0000000066699996),
        UINT64_C(0x953f6ac06ac0953f),
        UINT64_C(0x0000000056a9a956),
        UINT64_C(0x6ac0953f00000000),
        UINT64_C(0x0000ffff00007fff),
        UINT64_C(0x6969699600000000),
        UINT64_C(0xff00807f00ff7f80),
        UINT64_C(0x00000000ffff9669),
        UINT64_C(0x9996666900000000),
        UINT64_C(0x0000699669960000),
        UINT64_C(0x00000000aaaa5556),
        UINT64_C(0x00ff7f8000000000),
        UINT64_C(0x6669999600000000),
        UINT64_C(0x9669000000009669),
        UINT64_C(0xfdfdfd5400000000),
        UINT64_C(0x0001111155555555),
        UINT64_C(0x00000000fdfdfd54),
        UINT64_C(0x0000ffff0000f0e1),
        UINT64_C(0xfdfdfd54020202ab),
        UINT64_C(0x020202ab00000000),
        UINT64_C(0xa95656a900000000),
        UINT64_C(0xefeeaaaa00000000),
        UINT64_C(0x00a9000000a900a9),
        UINT64_C(0xa956a9a9a956a956),
        UINT64_C(0x5600565656005600),
        UINT64_C(0xa888a00000000000),
        UINT64_C(0x0000222333333333),
        UINT64_C(0x0000966996690000),
        UINT64_C(0x0000ffff0000f096),
        UINT64_C(0xfff1111100000000),
        UINT64_C(0x000c000ccccf888e),
        UINT64_C(0xccc3ccc300004441),
        UINT64_C(0xffff035703030303),
        UINT64_C(0xccc3ccc3333c6669),
        UINT64_C(0x333c333c00001114),
        UINT64_C(0x888288828882ccc3),
        UINT64_C(0x00000000ccc38882),
        UINT64_C(0xa900a9a9a900a900),
        UINT64_C(0xf1fff1f111111111),
        UINT64_C(0x02ab020202ab02ab),
        UINT64_C(0xffcdcccc00000000),
        UINT64_C(0x00000000566a0000),
        UINT64_C(0x2223000233333333),
        UINT64_C(0x0000ffff0000a995),
        UINT64_C(0x00000000fff11111),
        UINT64_C(0xaaffbeeb00aa2882),
        UINT64_C(0xffff0000a995566a),
        UINT64_C(0x00000000ffcdcccc),
        UINT64_C(0x000eeeee00000000),
        UINT64_C(0x55004114ff00c33c),
        UINT64_C(0x0032333300000000),
        UINT64_C(0xfff11111000eeeee),
        UINT64_C(0xffcdcccc00323333),
        UINT64_C(0x0000ffff00000f69),
        UINT64_C(0x0000ffff00000f1e),
        UINT64_C(0x000000000000007f),
        UINT64_C(0x1115155500010111),
        UINT64_C(0x0000ffff00006566)};

    progress_bar pbar( static_cast<uint32_t>( functions.size() ), prefix + " |{}| function = {:016x}, time so far = {:.2f}", true );

    for ( auto i = 0u; i < functions.size(); ++i )
    {
      stopwatch<> t( time );

      kitty::dynamic_truth_table tt( 6u );
      kitty::create_from_words( tt, &functions[i], &functions[i] + 1 );

      if ( kitty::get_bit( tt, 0u ) )
      {
        tt = ~tt;
      }

      pbar( i, functions[i], to_seconds( time ) );

      exact_mc_synthesis_params ps;
      ps.verbose = true;
      ps.very_verbose = true;
      ps.break_symmetric_variables = true;
      ps.break_subset_symmetries = true;
      ps.break_multi_level_subset_symmetries = false;
      ps.ensure_to_use_gates = true;
      ps.auto_update_xor_bound = minimize_xor;
      ps.conflict_limit = 50000u;

      xag_network xag;

      if ( multiple )
      {
        const auto xags = exact_mc_synthesis_multiple<xag_network, bill::solvers::z3>( tt, 5u, ps );
        xag = *std::min_element( xags.begin(), xags.end(), [&]( auto const& x1, auto const& x2 ) { return x1.num_gates() < x2.num_gates(); } );
      }
      else
      {
        xag = exact_mc_synthesis<xag_network, bill::solvers::z3>( tt, ps );
      }

      if ( sat_linear_resyn )
      {
        fmt::print( "Linear resynthesis\n" );
        xag = exact_linear_resynthesis_optimization( xag, 500000u );
      }

      const auto num_ands = *multiplicative_complexity( xag );
      const auto num_xors = xag.num_gates() - num_ands;

      xor_gates += num_xors;
      and_gates += num_ands;
    }
    const auto name = fmt::format( "practical6{}{}{}", multiple ? "-multiple" : "", minimize_xor ? "-xor" : "", sat_linear_resyn ? "-resyn" : "" );
    exp( name, xor_gates, and_gates, to_seconds( time ) );
  };

  all_spectral( 4u, false, false, false );
  all_spectral( 4u, true, false, false );
  all_spectral( 4u, true, true, false );
  all_spectral( 4u, true, true, true );
  all_spectral( 5u, false, false, false );
  all_spectral( 5u, true, false, false );
  all_spectral( 5u, true, true, false );
  all_spectral( 5u, true, true, true );

  practical6( false, false, false );
  practical6( true, false, false );
  practical6( true, true, false );
  practical6( true, true, true );

  exp.save();
  exp.table();
}

#else

#include <iostream>

int main()
{
  std::cout << "requires Z3" << std::endl;
  return 0;
}

#endif
