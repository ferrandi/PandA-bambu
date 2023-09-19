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

#include "experiments.hpp"

#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/window_rewriting.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>

#include <fmt/format.h>

using namespace mockturtle;

struct stats
{
  uint32_t estimated_gain{ 0 };
  uint32_t real_gain{ 0 };
  uint32_t num_substitutions{ 0 };
  uint32_t num_iterations{ 0 };
  stopwatch<>::duration time_total{ 0 };
};

aig_network optimize( aig_network const& aig, window_rewriting_params const& ps, window_rewriting_stats& st )
{
  window_rewriting( aig, ps, &st );
  return cleanup_dangling( aig );
}

int main()
{
  using namespace experiments;
  experiment<std::string, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, double, bool>
      exp( "window_rewriting", "benchmark", "size_before", "size_after", "est. gain", "real gain", "resubs", "iterations", "runtime", "equivalent" );

  for ( auto const& benchmark : all_benchmarks( iscas | epfl ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    /* read the benchmark */
    aig_network aig;
    if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) ) != lorina::return_code::success )
    {
      fmt::print( "[e] could not read {}\n", benchmark );
      continue;
    }

    window_rewriting_params ps;
    ps.cut_size = 6u;
    ps.num_levels = 5u;
    ps.filter_cyclic_substitutions = benchmark == "c432" ? true : false;

    stats st{};

    /* optimize benchmark until convergence */
    uint64_t const size_before{ aig.num_gates() };
    uint64_t size_current{};
    do
    {
      size_current = aig.num_gates();

      window_rewriting_stats win_st;
      aig = optimize( aig, ps, win_st );

      /* add up statistics from each iteration */
      st.real_gain += size_before - aig.num_gates();
      st.estimated_gain += win_st.gain;
      st.num_substitutions += win_st.num_substitutions;
      ++st.num_iterations;
      st.time_total += win_st.time_total;

      // st.report();
    } while ( aig.num_gates() < size_current );

    auto const cec = benchmark != "hyp" ? abc_cec( aig, benchmark ) : true;

    exp( benchmark, size_before, aig.num_gates(),
         st.estimated_gain, st.real_gain, st.num_substitutions, st.num_iterations,
         to_seconds( st.time_total ), cec );
  }

  exp.save();
  exp.table();

  return 0;
}