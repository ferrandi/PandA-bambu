#include <catch.hpp>

#include <bill/sat/interface/z3.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/exact_mc_synthesis.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/xag_optimization.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/properties/mccost.hpp>

using namespace mockturtle;

TEST_CASE( "Find some simple functions", "[exact_mc_synthesis]" )
{
  auto const test_one = [&]( uint32_t num_vars, const std::string& expression ) {
    kitty::dynamic_truth_table func( num_vars );
    kitty::create_from_expression( func, expression );
    const auto xag = exact_mc_synthesis<xag_network>( func );
    CHECK( simulate<kitty::dynamic_truth_table>( xag, {num_vars} )[0] == func );
  };

  test_one( 3u, "<abc>" );
  test_one( 3u, "!<abc>" );
  test_one( 3u, "(abc)" );
  test_one( 4u, "(abcd)" );
  test_one( 3u, "[(ab)(!ac)]" );
}

TEST_CASE( "Find multiple MAJ with exact MC synthesis", "[exact_mc_synthesis]" )
{
  kitty::dynamic_truth_table func( 3 );
  kitty::create_majority( func );
  const auto xags = exact_mc_synthesis_multiple<xag_network>( func, 3u );

  CHECK( xags.size() == 2u );
  for ( auto const& xag : xags )
  {
    CHECK( simulate<kitty::dynamic_truth_table>( xag, {3u} )[0] == func );
  }
}

TEST_CASE( "Find multiple ITE with exact MC synthesis", "[exact_mc_synthesis]" )
{
  kitty::dynamic_truth_table func( 3 );
  kitty::create_from_expression( func, "[(ab)(!ac)]" );
  const auto xags = exact_mc_synthesis_multiple<xag_network>( func, 10u );
  CHECK( xags.size() == 1u );
  for ( auto const& xag : xags )
  {
    CHECK( simulate<kitty::dynamic_truth_table>( xag, {3u} )[0] == func );
  }
}
