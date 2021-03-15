#include <catch.hpp>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/xag_optimization.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <lorina/verilog.hpp>

#include <algorithm>
#include <vector>

using namespace mockturtle;

TEST_CASE( "Edge cases for linear resynthesis", "[xag_optimization]" )
{
  {
    xag_network xag;
    std::vector<xag_network::signal> pis( 4u );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );
    xag.create_po( xag.create_nary_and( pis ) );

    const auto opt = exact_linear_resynthesis_optimization( xag );
    CHECK( simulate<kitty::static_truth_table<4>>( xag ) == simulate<kitty::static_truth_table<4u>>( opt ) );
  }

  {
    xag_network xag;
    std::vector<xag_network::signal> pis( 4u );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );
    xag.create_po( xag.create_xor( xag.create_and( pis[0u], pis[1u] ), xag.create_and( pis[2u], pis[3u] ) ) );

    const auto opt = exact_linear_resynthesis_optimization( xag );
    CHECK( simulate<kitty::static_truth_table<4>>( xag ) == simulate<kitty::static_truth_table<4u>>( opt ) );
  }
}

TEST_CASE( "Test XAG constant fanin optimization", "[xag_optimization]" )
{
  /* regression test that leads to a segmentation violation */
  std::string const test_case =
    "module top( x0 , x1 , y0 );\n"
    "  input x0 , x1 ;\n"
    "  output y0 ;\n"
    "  wire n2 , n3 , n4 , n5 , n6 , n7 ;\n"
    "  assign n2 = x1 ^ x0 ;\n"
    "  assign n3 = n2 ^ x1 ;\n"
    "  assign n4 = n3 ^ n2 ;\n"
    "  assign n5 = n2 & n4 ;\n"
    "  assign n6 = n5 ^ n2 ;\n"
    "  assign n7 = n6 ^ n2 ;\n"
    "  assign y0 = n7 ;\n"
    "endmodule\n" ;

  std::stringstream ss( test_case );
  mockturtle::xag_network xag;
  CHECK( lorina::read_verilog( ss, mockturtle::verilog_reader( xag ) ) == lorina::return_code::success );
  xag_constant_fanin_optimization( xag );
}
