#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/decomposition.hpp>
#include <mockturtle/algorithms/node_resynthesis/davio.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Shannon decomposition on random functions of different size", "[decomposition]" )
{
  for ( uint32_t var = 0u; var <= 6u; ++var )
  {
    for ( auto i = 0u; i < 100u; ++i )
    {
      kitty::dynamic_truth_table func( var );
      kitty::create_random( func );

      std::vector<uint32_t> vars( var );
      std::iota( vars.begin(), vars.end(), 0u );

      aig_network ntk;
      std::vector<aig_network::signal> pis( var );
      std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
      ntk.create_po( shannon_decomposition( ntk, func, vars, pis ) );

      default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
      CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
    }
  }
}

TEST_CASE( "Partial Shannon decomposition on random 6-input functions on two variables with Davio resynthesis", "[decomposition]" )
{
  positive_davio_resynthesis<xag_network> resyn;

  for ( uint32_t i = 0u; i < 100u; ++i )
  {
    kitty::dynamic_truth_table func( 6u );
    kitty::create_random( func );

    xag_network ntk;
    std::vector<xag_network::signal> pis( 6u );
    std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
    ntk.create_po( shannon_decomposition( ntk, func, {0u, 3u}, pis, resyn ) );

    default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
    CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
  }
}

TEST_CASE( "Complete Davio decomposition on random functions of different size", "[decomposition]" )
{
  for ( uint32_t var = 0u; var <= 6u; ++var )
  {
    for ( auto i = 0u; i < 100u; ++i )
    {
      kitty::dynamic_truth_table func( var );
      kitty::create_random( func );

      std::vector<uint32_t> vars( var );
      std::iota( vars.begin(), vars.end(), 0u );

      xag_network ntk;
      std::vector<xag_network::signal> pis( var );
      std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
      ntk.create_po( positive_davio_decomposition( ntk, func, vars, pis ) );
      ntk.create_po( negative_davio_decomposition( ntk, func, vars, pis ) );

      default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
      CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
      CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[1] == func );
    }
  }
}

TEST_CASE( "Partial Davio decomposition on random 6-input functions on two variables with Shannon resynthesis", "[decomposition]" )
{
  shannon_resynthesis<xag_network> resyn;

  for ( uint32_t i = 0u; i < 100u; ++i )
  {
    kitty::dynamic_truth_table func( 6u );
    kitty::create_random( func );

    xag_network ntk;
    std::vector<xag_network::signal> pis( 6u );
    std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
    ntk.create_po( positive_davio_decomposition( ntk, func, {0u, 3u}, pis, resyn ) );
    ntk.create_po( negative_davio_decomposition( ntk, func, {0u, 3u}, pis, resyn ) );

    default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
    CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
    CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[1] == func );
  }
}
