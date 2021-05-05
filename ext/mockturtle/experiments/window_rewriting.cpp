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

#include "experiments.hpp"

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/window_rewriting.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/color_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <lorina/aiger.hpp>

#include <fmt/format.h>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, uint64_t, uint64_t, double, uint64_t, bool>
    exp( "window_rewriting", "benchmark", "size_before", "size_after", "runtime", "resubs", "equivalent" );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    /* read the benchmark */
    aig_network ntk;
    if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( ntk ) ) != lorina::return_code::success )
    {
      fmt::print( "[e] could not read {}\n", benchmark );
      continue;
    }

    /* optimize benchmark */
    uint64_t const size_before{ntk.num_gates()};

    fanout_view fntk{ntk};
    depth_view dntk{fntk};
    color_view aig{dntk};

    window_rewriting_params ps;
    ps.cut_size = 6u;
    ps.num_levels = 5u;

    window_rewriting_stats st;
    window_rewriting( aig, ps, &st );
    ntk = cleanup_dangling( ntk );

    auto const cec = benchmark != "hyp" ? abc_cec( ntk, benchmark ) : true;

    exp( benchmark, size_before, ntk.num_gates(), to_seconds( st.time_total ),
         st.num_substitutions, cec );
  }

  exp.save();
  exp.table();

  return 0;
}
