#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>
#include <mockturtle/algorithms/balancing/esop_balancing.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
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

TEST_CASE( "Rebalance XAG adder using ESOP balancing", "[balancing]" )
{
  xag_network xag;
  std::vector<xag_network::signal> as( 8u ), bs( 8u );
  std::generate( as.begin(), as.end(), [&]() { return xag.create_pi(); });
  std::generate( bs.begin(), bs.end(), [&]() { return xag.create_pi(); });
  auto carry = xag.get_constant( false );
  carry_ripple_adder_inplace( xag, as, bs, carry );
  std::for_each( as.begin(), as.end(), [&]( auto const& f ) { xag.create_po( f ); });

  CHECK( depth_view{xag}.depth() == 22u );

  xag = balancing( xag, {esop_rebalancing<xag_network>{}} );
  CHECK( depth_view{xag}.depth() == 22u );
}
