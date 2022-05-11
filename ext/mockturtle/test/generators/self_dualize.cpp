#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/self_dualize.hpp>
#include <mockturtle/generators/arithmetic.hpp>

#include <kitty/kitty.hpp>

using namespace mockturtle;

TEST_CASE( "self-dualize 4-bit substractor", "[self_dualize]" )
{
  /* create carry ripple subtractor */
  aig_network aig;
  {
    std::vector<typename aig_network::signal> a( 4 ), b( 4 );
    std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
    std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
    auto carry = aig.get_constant( true );

    carry_ripple_subtractor_inplace( aig, a, b, carry );

    std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
    aig.create_po( aig.create_not( carry ) );
  }

  default_simulator<kitty::dynamic_truth_table> sim0( aig.num_pis() );
  CHECK( !kitty::is_selfdual( simulate<kitty::dynamic_truth_table>( aig, sim0 )[0] ) );

  auto const self_dual_aig = self_dualize_aig( aig );
  default_simulator<kitty::dynamic_truth_table> sim1( self_dual_aig.num_pis() );
  CHECK( kitty::is_selfdual( simulate<kitty::dynamic_truth_table>( self_dual_aig, sim1 )[0] ) );
}
