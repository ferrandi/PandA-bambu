#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/shannon_decomposition.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>

using namespace mockturtle;

TEST_CASE( "Shannon decomposition on random functions of different size", "[shannon_decomposition]" )
{
  for ( uint32_t var = 0u; var <= 6u; ++var )
  {
    for ( auto i = 0u; i < 100u; ++i )
    {
      kitty::dynamic_truth_table func( var );
      kitty::create_random( func );

      aig_network ntk;
      std::vector<aig_network::signal> pis( var );
      std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
      ntk.create_po( shannon_decomposition( ntk, func, pis ) );

      default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
      CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
    }
  }
}
