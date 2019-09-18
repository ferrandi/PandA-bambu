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
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, double, double, bool> exp( "lut_mapping", "benchmark", "luts", "runtime", "runtime cec", "equivalent" );

  for ( auto const& benchmark : epfl_benchmarks( ( adder | bar | max | experiments::random ) & ~mem_ctrl & ~voter ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    aig_network aig;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) );

    lut_mapping_stats st;
    mapping_view<aig_network, true> mapped_aig{aig};
    lut_mapping<decltype( mapped_aig ), true>( mapped_aig, {}, &st );
    const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

    equivalence_checking_stats ecst;
    auto cec = *equivalence_checking( *miter<klut_network>( aig, klut ), {}, &ecst );

    exp( benchmark, klut.num_gates(), to_seconds( st.time_total ), to_seconds( ecst.time_total ), cec );
  }

  exp.save();
  exp.table();

  return 0;
}
