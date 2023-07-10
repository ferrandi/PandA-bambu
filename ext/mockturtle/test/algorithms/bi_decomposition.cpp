#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <mockturtle/algorithms/bi_decomposition.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Bi-decomposition on some 4-input functions into AIGs", "[bi_decomposition]" )
{
  std::vector<std::string> functions = { "b0bb", "00b0", "0804", "090f", "abcd", "3ah6" };

  for ( auto const& func : functions )
  {
    kitty::dynamic_truth_table table( 4u );
    kitty::dynamic_truth_table care( 4u );
    kitty::create_from_hex_string( table, func );
    kitty::create_from_hex_string( care, "ffef" );

    aig_network aig;
    const auto x1 = aig.create_pi();
    const auto x2 = aig.create_pi();
    const auto x3 = aig.create_pi();
    const auto x4 = aig.create_pi();

    aig.create_po( bi_decomposition( aig, table, care, { x1, x2, x3, x4 } ) );

    default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
    CHECK( binary_and( simulate<kitty::dynamic_truth_table>( aig, sim )[0], care ) == binary_and( table, care ) );
  }
}

TEST_CASE( "Bi-decomposition on some 10-input functions into XAGs", "[bi_decomposition]" )
{
  std::vector<std::string> functions = { "0080004000080004ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                                         "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003333bbbbf3f3fbfbff33ffbbfff3fffb",
                                         "000000000000000000000000000000003333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb" };

  for ( auto const& func : functions )
  {
    kitty::dynamic_truth_table table( 10u );
    kitty::dynamic_truth_table care( 10u );
    kitty::create_from_hex_string( table, func );
    kitty::create_from_hex_string( care, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" );

    xag_network xag;
    std::vector<xag_network::signal> pis( 10u );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

    xag.create_po( bi_decomposition( xag, table, care, pis ) );

    default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
    CHECK( binary_and( simulate<kitty::dynamic_truth_table>( xag, sim )[0], care ) == binary_and( table, care ) );
  }
}

TEST_CASE( "Bi-decomposition on random functions of different size into XAGs", "[bi_decomposition]" )
{
  for ( uint32_t var = 0u; var <= 6u; ++var )
  {
    for ( auto i = 0u; i < 100u; ++i )
    {
      kitty::dynamic_truth_table func( var ), care( var );
      kitty::create_random( func );
      kitty::create_random( care );
      care = ~func.construct();

      xag_network ntk;
      std::vector<xag_network::signal> pis( var );
      std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
      ntk.create_po( bi_decomposition( ntk, func, care, pis ) );

      default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );

      CHECK( kitty::binary_and( care, simulate<kitty::dynamic_truth_table>( ntk, sim )[0] ) == kitty::binary_and( care, func ) );
    }
  }
}
