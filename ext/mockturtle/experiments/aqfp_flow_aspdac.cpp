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
  \file aqfp_flow_aspdac.cpp
  \brief AQFP synthesis flow

  This file contains the code to reproduce the experiment (Table I)
  in the following paper:
  "Depth-optimal Buffer and Splitter Insertion and Optimization in AQFP Circuits",
  ASP-DAC 2023, by Alessandro Tempia Calvino and Giovanni De Micheli.

  This version runs on the ISCAS benchmarks. The benchmarks for Table 1 can be
  downloaded at https://github.com/lsils/SCE-benchmarks
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <mockturtle/algorithms/aqfp/aqfp_cleanup.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_rebuild.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_retiming.hpp>
#include <mockturtle/algorithms/aqfp/buffer_insertion.hpp>
#include <mockturtle/algorithms/aqfp/buffer_verification.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aqfp.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/views/depth_view.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double, uint32_t, uint32_t, uint32_t, double, bool> exp(
      "aqfp_retiming", "Bench", "Size_init", "Depth_init", "B/S_sched", "JJs_sched", "Depth_sched", "Time_sched (s)", "B/S_fin", "JJs_fin", "Depth_fin", "Time (s)", "cec" );

  uint32_t total_jjs = 0;
  uint32_t total_bufs = 0;

  double retiming_opt_ratio = 0;
  double num_benchmarks = 0;
  for ( auto const& benchmark : iscas_benchmarks() )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    mig_network mig;
    if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( mig ) ) != lorina::return_code::success )
    {
      continue;
    }

    /* MIG-based logic optimization can be added here */
    auto mig_opt = cleanup_dangling( mig );

    const uint32_t size_before = mig_opt.num_gates();
    const uint32_t depth_before = depth_view( mig_opt ).depth();

    double total_runtime = 0;

    /* convert MIG network to AQFP */
    aqfp_network aqfp = cleanup_dangling<mig_network, aqfp_network>( mig_opt );

    /* Buffer insertion params */
    buffer_insertion_params buf_ps;
    buf_ps.scheduling = buffer_insertion_params::better_depth;
    buf_ps.optimization_effort = buffer_insertion_params::none;
    buf_ps.max_chunk_size = 100;
    buf_ps.assume.splitter_capacity = 4u;
    buf_ps.assume.branch_pis = true;
    buf_ps.assume.balance_pis = true;
    buf_ps.assume.balance_pos = true;

    /* buffer insertion */
    stopwatch<>::duration time_insertion{ 0 };
    buffer_insertion buf_inst( aqfp, buf_ps );
    buffered_aqfp_network buffered_aqfp;
    uint32_t num_bufs = call_with_stopwatch( time_insertion, [&]() { return buf_inst.run( buffered_aqfp ); } );
    uint32_t num_jjs = aqfp.num_gates() * 6 + num_bufs * 2;
    uint32_t jj_depth = buf_inst.depth();
    total_runtime += to_seconds( time_insertion );

    aqfp_assumptions aqfp_ps;
    aqfp_ps.splitter_capacity = buf_ps.assume.splitter_capacity;
    aqfp_ps.branch_pis = buf_ps.assume.branch_pis;
    aqfp_ps.balance_pis = buf_ps.assume.balance_pis;
    aqfp_ps.balance_pos = buf_ps.assume.balance_pos;

    /* retiming params */
    aqfp_retiming_params aps;
    aps.aqfp_assumptions_ps = aqfp_ps;
    aps.backwards_first = buf_inst.is_scheduled_ASAP();
    aps.iterations = 250;
    aps.retime_splitters = true;

    /* chunk movement params */
    buffer_insertion_params buf_ps2 = buf_ps;
    buf_ps2.scheduling = buffer_insertion_params::provided;
    buf_ps2.optimization_effort = buffer_insertion_params::one_pass;

    double retiming_saved = buffered_aqfp.size();

    /* first retiming */
    {
      aqfp_retiming_stats ast;
      auto buf_aqfp_ret = aqfp_retiming( buffered_aqfp, aps, &ast );
      total_runtime += to_seconds( ast.time_total );
      buffered_aqfp = buf_aqfp_ret;
    }

    retiming_saved -= buffered_aqfp.size();

    /* repeat loop */
    uint32_t iterations = 10;
    aps.det_randomization = true;
    while ( iterations-- > 0 )
    {
      uint32_t size_previous = buffered_aqfp.size();

      /* chunk movement */
      aqfp_reconstruct_params reconstruct_ps;
      aqfp_reconstruct_stats reconstruct_st;
      reconstruct_ps.buffer_insertion_ps = buf_ps2;
      auto buf_aqfp_chunk = aqfp_reconstruct( buffered_aqfp, reconstruct_ps, &reconstruct_st );
      total_runtime += to_seconds( reconstruct_st.total_time );
      retiming_saved += buffered_aqfp.size();

      /* retiming */
      aqfp_retiming_stats ast;
      auto buf_aqfp_ret = aqfp_retiming( buf_aqfp_chunk, aps, &ast );
      total_runtime += to_seconds( ast.time_total );
      retiming_saved -= buffered_aqfp.size();

      if ( buf_aqfp_ret.size() >= size_previous )
        break;

      buffered_aqfp = buf_aqfp_ret;
    }

    /* cec */
    auto cec = abc_cec( buffered_aqfp, benchmark );
    cec &= verify_aqfp_buffer( buffered_aqfp, aqfp_ps );

    /* compute final JJ cost */
    uint32_t num_jjs_ret = 0;
    uint32_t num_bufs_ret = 0;
    uint32_t jj_depth_ret = depth_view<buffered_aqfp_network>( buffered_aqfp ).depth();

    buffered_aqfp.foreach_node( [&]( auto const& n ) {
      if ( buffered_aqfp.is_pi( n ) || buffered_aqfp.is_constant( n ) )
        return;
      if ( buffered_aqfp.is_buf( n ) )
      {
        ++num_bufs_ret;
        num_jjs_ret += 2;
      }
      else
      {
        num_jjs_ret += 6;
      }
    } );

    total_bufs += num_bufs_ret;
    total_jjs += num_jjs_ret;

    if ( ( num_bufs - num_bufs_ret ) > 0 )
    {
      retiming_opt_ratio += retiming_saved / static_cast<double>( num_bufs - num_bufs_ret );
      ++num_benchmarks;
    }

    exp( benchmark, size_before, depth_before, num_bufs, num_jjs, jj_depth, to_seconds( time_insertion ), num_bufs_ret, num_jjs_ret, jj_depth_ret, total_runtime, cec );
  }

  exp.save();
  exp.table();

  std::cout << fmt::format( "[i] Total B/S = {} \tTotal JJs = {}\n", total_bufs, total_jjs );
  std::cout << "Ratio: " << retiming_opt_ratio * 100.0 / num_benchmarks << "\n";

  return 0;
}
