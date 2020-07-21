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

#include <string>
#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, double, bool, uint32_t, uint32_t, double, bool> exp( "sop_balancing", "benchmark", "size", "depth", "size 4", "depth 4", "RT 4", "cec 4", "size 6", "depth 6", "RT 6", "cec 6" );

  sop_rebalancing<aig_network> sop_balancing;

  for ( auto const& benchmark : epfl_benchmarks( ~experiments::hyp ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    aig_network aig;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) );

    balancing_params ps;
    balancing_stats st4, st6;

    ps.progress = true;
    ps.cut_enumeration_ps.cut_size = 4u;
    const auto aig4 = balancing( aig, {sop_balancing}, ps, &st4 );

    ps.cut_enumeration_ps.cut_size = 6u;
    const auto aig6 = balancing( aig, {sop_balancing}, ps, &st6 );

    depth_view daig{aig};
    depth_view daig4{aig4};
    depth_view daig6{aig6};

    const auto cec4 = abc_cec( aig4, benchmark );
    const auto cec6 = abc_cec( aig6, benchmark );

    exp( benchmark,
         aig.num_gates(), daig.depth(),
         aig4.num_gates(), daig4.depth(),
         to_seconds( st4.time_total ), cec4,
         aig6.num_gates(), daig6.depth(),
         to_seconds( st6.time_total ), cec6 );
  }

  exp.save();
  exp.table();

  return 0;
}
