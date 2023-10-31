#include <catch.hpp>

#include <vector>

#include <mockturtle/algorithms/dont_cares.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>

using namespace mockturtle;

TEST_CASE( "SDCs in simple AIG", "[dont_cares]" )
{
  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto f1 = aig.create_and( a, b );
  auto f2 = aig.create_and( a, !b );
  auto f3 = aig.create_and( f1, f2 );
  aig.create_po( f3 );

  std::vector<node<aig_network>> leaves{ { aig.get_node( f1 ), aig.get_node( f2 ) } };
  const auto tt = satisfiability_dont_cares( aig, leaves );

  CHECK( tt._bits[0] == 0x8u );
}

TEST_CASE( "ODCs in simple AIG", "[dont_cares]" )
{
  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto f1 = aig.create_and( a, b );
  auto f2 = aig.create_and( a, !b );
  auto f3 = aig.create_and( f1, f2 );
  aig.create_po( f3 );

  std::vector<node<aig_network>> leaves{ { aig.get_node( a ), aig.get_node( b ) } };
  const auto f1_odc = observability_dont_cares( aig, aig.get_node( f1 ), leaves, { aig.get_node( f3 ) } );
  CHECK( f1_odc._bits[0] == 0xd );

  const auto f2_odc = observability_dont_cares( aig, aig.get_node( f2 ), leaves, { aig.get_node( f3 ) } );
  CHECK( f2_odc._bits[0] == 0x7 );
}

TEST_CASE( "SDCs in simple AIG using satisfiability checker", "[dont_cares]" )
{
  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto f1 = aig.create_and( a, b );
  auto f2 = aig.create_and( a, !b );
  auto f3 = aig.create_and( f1, f2 );
  aig.create_po( f3 );

  satisfiability_dont_cares_checker<aig_network> checker( aig );
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{ { false, false } } ) );
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{ { false, true } } ) );
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{ { true, false } } ) );
  CHECK( checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{ { true, true } } ) );
}

TEST_CASE( "ODCs with partial simulation", "[dont_cares]" )
{
  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto c = aig.create_pi();
  auto d = aig.create_pi();
  auto f1 = aig.create_and( b, c );
  auto f2 = aig.create_and( a, f1 );
  auto f3 = aig.create_and( f1, d );
  auto f4 = aig.create_and( f2, f3 );
  aig.create_po( f4 );

  partial_simulator sim( 4, 0 );
  sim.add_pattern( std::vector<bool>( { 1, 1, 1, 1 } ) );
  sim.add_pattern( std::vector<bool>( { 0, 0, 1, 0 } ) );
  sim.add_pattern( std::vector<bool>( { 1, 0, 0, 0 } ) );

  fanout_view<aig_network> ntk( aig );
  unordered_node_map<kitty::partial_truth_table, fanout_view<aig_network>> tts( ntk );

  const auto odc_1_lev = observability_dont_cares( ntk, ntk.get_node( f1 ), sim, tts, 1 );
  CHECK( odc_1_lev._bits[0] == 0x2 );

  const auto odc_glob = observability_dont_cares( ntk, ntk.get_node( f1 ), sim, tts, -1 );
  CHECK( odc_glob._bits[0] == 0x6 );
}
