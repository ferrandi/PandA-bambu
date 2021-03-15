#include <catch.hpp>

#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

TEST_CASE( "Rebalance AND chain in AIG", "[balancing]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  aig.create_po( aig.create_and( a, aig.create_and( b, aig.create_and( c, d ) ) ) );

  CHECK( depth_view{aig}.depth() == 3u );

  aig = balancing( aig, {sop_rebalancing<aig_network>{}} );
  CHECK( depth_view{aig}.depth() == 2u );
}
