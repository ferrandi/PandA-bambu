#include <catch.hpp>

#include <vector>

#include <mockturtle/algorithms/dont_cares.hpp>
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

  std::vector<node<aig_network>> leaves{{aig.get_node( f1 ), aig.get_node( f2 )}};
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

  std::vector<node<aig_network>> leaves{{aig.get_node( a ), aig.get_node( b )}};
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
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{{false, false}} ) );
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{{false, true}} ) );
  CHECK( !checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{{true, false}} ) );
  CHECK( checker.is_dont_care( aig.get_node( f3 ), std::vector<bool>{{true, true}} ) );
}
