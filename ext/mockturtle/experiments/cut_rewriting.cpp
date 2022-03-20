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
#include <vector>

#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <lorina/verilog.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/networks/aig.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, uint32_t, uint32_t, float, float, bool, bool> exp( "cut_rewriting", "benchmark", "size_before", "size_after", "size after 2", "runtime", "runtime 2", "equivalent", "equivalent 2" );
  xag_npn_resynthesis<aig_network> resyn;

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    aig_network aig, aig2;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) );
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig2 ) );

    cut_rewriting_params ps;
    ps.cut_enumeration_ps.cut_size = 4;
    ps.progress = true;

    uint32_t size_before = aig.num_gates();
    cut_rewriting_stats st;
    cut_rewriting_with_compatibility_graph( aig, resyn, ps, &st );
    aig = cleanup_dangling( aig );

    auto cec = abc_cec( aig, benchmark );

    cut_rewriting_stats st2;
    aig2 = cut_rewriting( aig2, resyn, ps, &st2 );
    auto cec2 = abc_cec( aig2, benchmark );

    exp( benchmark, size_before, aig.num_gates(), aig2.num_gates(), to_seconds( st.time_total ), to_seconds( st2.time_total ), cec, cec2 );
  }

  exp.save();
  exp.table();

  return 0;
}
