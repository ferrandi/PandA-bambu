#include <catch.hpp>

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/xmgcost.hpp>

using namespace mockturtle;

TEST_CASE( "profile gates in XMG", "[xmgcost]" )
{
  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto f1 = xmg.create_xor3( a, b, c );
  const auto f2 = xmg.create_xor3( a, b, xmg.get_constant( 0 ) );
  const auto f3 = xmg.create_and( a, b );
  const auto f4 = xmg.create_maj( f1, f2, f3 );
  xmg.create_po( f4 );

  xmg_gate_stats stats;
  xmg_profile_gates( xmg, stats );
  CHECK( stats.total_xor3 == 2u );
  CHECK( stats.xor3 == 1u );
  CHECK( stats.xor2 == 1u );

  CHECK( stats.total_maj == 2u );
  CHECK( stats.maj == 1u );
  CHECK( stats.and_or == 1u );
}
