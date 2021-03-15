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
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  std::unordered_map<std::string, double> baseline = {
    {"adder", 0.00},
    {"bar", 0.33},
    {"div", 9.66},
    {"hyp", 25.70},
    {"log2", 9.74},
    {"max", 0.21},
    {"multiplier", 6.27},
    {"sin", 1.97},
    {"sqrt", 5.28},
    {"square", 2.90},
    {"arbiter", 0.01},
    {"cavlc", 0.01},
    {"ctrl", 0.01},
    {"dec", 0.00},
    {"i2c", 0.02},
    {"int2float", 0.01},
    {"mem_ctrl", 4.46},
    {"priority", 0.06},
    {"router", 0.01},
    {"voter", 3.54}
  };

  experiment<std::string, double, double, bool> exp( "equivalence_checking", "benchmark", "abc cec", "runtime", "equivalent" );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    aig_network aig;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) );
    const auto orig = aig;

    xag_npn_resynthesis<aig_network> resyn;

    cut_rewriting_params ps;
    ps.cut_enumeration_ps.cut_size = 4;
    ps.progress = true;

    aig = cut_rewriting( aig, resyn, ps );

    equivalence_checking_stats st;
    auto cec = *equivalence_checking( *miter<aig_network>( orig, aig ), {}, &st );

    exp( benchmark, baseline[benchmark], to_seconds( st.time_total ), cec );
  }

  exp.save();
  exp.table();

  return 0;
}
