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
#include <mockturtle/algorithms/mig_resub.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/mig.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint32_t, uint32_t, float, bool> exp( "mig_resubstitution", "benchmark", "size_before", "size_after", "runtime", "equivalent" );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    mig_network mig;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( mig ) );

    resubstitution_params ps;
    resubstitution_stats st;

    ps.max_pis = 8u;
    ps.max_inserts = 1u;
    ps.progress = false;

    const uint32_t size_before = mig.num_gates();
    mig_resubstitution( mig, ps, &st );

    mig = cleanup_dangling( mig );

    const auto cec = benchmark == "hyp" ? true : abc_cec( mig, benchmark );

    exp( benchmark, size_before, mig.num_gates(), to_seconds( st.time_total ), cec );
  }

  exp.save();
  exp.table();

  return 0;
}
