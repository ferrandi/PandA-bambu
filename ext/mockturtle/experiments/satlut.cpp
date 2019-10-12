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

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/satlut_mapping.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  std::unordered_map<std::string, uint32_t> baseline = {
    {"adder", 192},
    {"bar", 512},
    {"div", 15640},
    {"log2", 6607},
    {"max", 714},
    {"multiplier", 4966},
    {"sin", 1255},
    {"sqrt", 4498},
    {"square", 3353},
    {"arbiter", 2599},
    {"cavlc", 116},
    {"ctrl", 28},
    {"dec", 287},
    {"i2c", 338},
    {"int2float", 47},
    {"mem_ctrl", 11164},
    {"priority", 217},
    {"router", 46},
    {"voter", 2158}
  };

  experiment<std::string, uint32_t, uint32_t, uint32_t, float, bool> exp( "satlut", "benchmark", "cells_baseline", "cells_init", "cells_final", "runtime", "equivalent" );

  for ( auto const& benchmark : epfl_benchmarks( ~hyp & ~experiments::div ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );
    aig_network aig;
    lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) );

    mapping_view<aig_network, true> mapped_aig{aig};
    lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 6;
    ps.cut_enumeration_ps.cut_limit = 16;
    lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );
    const auto cells_init = mapped_aig.num_cells();

    satlut_mapping_params slps;
    slps.cut_enumeration_ps.cut_size = 6;
    slps.cut_enumeration_ps.cut_limit = 16;
    slps.conflict_limit = 100;
    slps.progress = true;
    satlut_mapping_stats st;

    satlut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, 32u, slps, &st );

    auto cec = abc_cec( aig, benchmark );

    exp( benchmark, baseline[benchmark], cells_init, mapped_aig.num_cells(), to_seconds( st.time_total ), cec );
  }

  exp.save();
  exp.table();

  return 0;
}
