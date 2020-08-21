#include <catch.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <mockturtle/algorithms/detail/minmc_xags.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_minmc.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/xag_optimization.hpp>
#include <mockturtle/io/index_list.hpp>
#include <mockturtle/io/write_dot.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/traits.hpp>

#include <kitty/constructors.hpp>
#include <kitty/operators.hpp>
#include <kitty/spectral.hpp>
#include <lorina/verilog.hpp>

using namespace mockturtle;

template<int NumVars>
static void check_minmc_xags()
{
  for ( auto const& [cls, tt, repr, expr] : detail::minmc_xags[NumVars] ) {
    xag_network xag;
    std::vector<xag_network::signal> pis( NumVars );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); });

    for ( auto const& po : create_from_binary_index_list( xag, repr.begin(), pis.begin() ) )
    {
      xag.create_po( po );
    }

    const auto f = simulate<kitty::static_truth_table<NumVars>>( xag )[0];
    auto f_tt = f.construct(), f_expr = f.construct();
    kitty::create_from_words( f_tt, &tt, &tt + 1 );
    kitty::create_from_expression( f_expr, expr );
    CHECK( kitty::get_spectral_class( f ) == cls );
    CHECK( f == f_tt );
    CHECK( f == f_expr );
  }
}

TEST_CASE( "create MC-optumum XAGs from binary index list", "[index_list]" )
{
  check_minmc_xags<0>();
  check_minmc_xags<1>();
  check_minmc_xags<2>();
  check_minmc_xags<3>();
  check_minmc_xags<4>();
  check_minmc_xags<5>();
}

template<int NumVars>
static void check_repr_match()
{
  for ( auto const& [cls, tt, repr, expr] : detail::minmc_xags[NumVars] ) {
    (void)cls;
    (void)repr;
    (void)expr;

    kitty::dynamic_truth_table f_tt( NumVars );
    kitty::create_from_words( f_tt, &tt, &tt + 1 );
    const auto func = kitty::spectral_representative( f_tt );
    const auto func2 = kitty::hybrid_exact_spectral_canonization( f_tt );
    CHECK( func == f_tt );
    CHECK( func2 == f_tt );
  }
}

TEST_CASE( "check representatives for database functions", "[index_list]" )
{
  check_repr_match<0>();
  check_repr_match<1>();
  check_repr_match<2>();
  check_repr_match<3>();
  check_repr_match<4>();
}
