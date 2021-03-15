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
#include <algorithm>
#include <vector>

#include <fmt/format.h>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/cut_enumeration/cnf_cut.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <experiments.hpp>

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  experiment<std::string, double, bool, double, double, bool> exp( "cnf_map", "benchmark", "time_tseytin", "eq_tseytin", "time_mapping", "time_cnfmap", "eq_cnfmap" );

  for ( auto i = 4u; i < 6u; ++i )
  {
    const auto benchmark = fmt::format( "assoc-{}", i );
    fmt::print( "[i] processing {}\n", benchmark );

    aig_network aig;
    std::vector<aig_network::signal> as( i ), bs( i ), cs( i );
    std::generate( as.begin(), as.end(), [&]() { return aig.create_pi(); } );
    std::generate( bs.begin(), bs.end(), [&]() { return aig.create_pi(); } );
    std::generate( cs.begin(), cs.end(), [&]() { return aig.create_pi(); } );

    auto o1 = carry_ripple_multiplier( aig, carry_ripple_multiplier( aig, as, bs ), cs );
    auto o2 = carry_ripple_multiplier( aig, as, carry_ripple_multiplier( aig, bs, cs ) );
    std::vector<aig_network::signal> xors( o1.size() );
    std::transform( o1.begin(), o1.end(), o2.begin(),
                    xors.begin(),
                    [&]( auto const& a, auto const& b ) { return aig.create_xor( a, b ); } );
    aig.create_po( aig.create_nary_or( xors ) );

    equivalence_checking_stats st;
    const auto result = *equivalence_checking( aig, {}, &st );

    lut_mapping_params lmps;
    lmps.cut_enumeration_ps.cut_size = 8;
    lut_mapping_stats lmst;
    mapping_view<aig_network, true> mapped_aig{aig};
    lut_mapping<decltype( mapped_aig ), true, cut_enumeration_mf_cut>( mapped_aig, lmps, &lmst );
    const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

    equivalence_checking_stats st2;
    const auto result2 = *equivalence_checking( klut, {}, &st2 );

    exp( benchmark, to_seconds( st.time_total ), result, to_seconds( lmst.time_total ), to_seconds( st2.time_total ), result2 );
  }

  exp.save();
  exp.table();

  return 0;
}
