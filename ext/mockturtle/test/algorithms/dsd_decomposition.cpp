#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/dsd_decomposition.hpp>
#include <mockturtle/algorithms/decomposition.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Full DSD decomposition on some 4-input functions into AIGs", "[dsd_decomposition]" )
{
  std::vector<std::string> functions = {"b0bb", "00b0", "0804", "090f"};

  for ( auto const& func : functions )
  {
    kitty::dynamic_truth_table table( 4u );
    kitty::create_from_hex_string( table, func );

    aig_network aig;
    const auto x1 = aig.create_pi();
    const auto x2 = aig.create_pi();
    const auto x3 = aig.create_pi();
    const auto x4 = aig.create_pi();

    auto fn = [&]( kitty::dynamic_truth_table const&, std::vector<aig_network::signal> const& ) {
      CHECK( false );
      return aig.get_constant( false );
    };
    aig.create_po( dsd_decomposition( aig, table, {x1, x2, x3, x4}, fn ) );

    default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
    CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  }
}

TEST_CASE( "Full DSD decomposition on some 10-input functions into XAGs", "[dsd_decomposition]" )
{
  std::vector<std::string> functions = {"0080004000080004ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                                        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003333bbbbf3f3fbfbff33ffbbfff3fffb",
                                        "000000000000000000000000000000003333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb3333bbbbf3f3fbfbff33ffbbfff3fffb"};

  for ( auto const& func : functions )
  {
    kitty::dynamic_truth_table table( 10u );
    kitty::create_from_hex_string( table, func );

    xag_network xag;
    std::vector<xag_network::signal> pis( 10u );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

    auto fn = [&]( kitty::dynamic_truth_table const&, std::vector<xag_network::signal> const& ) {
      CHECK( false );
      return xag.get_constant( false );
    };
    xag.create_po( dsd_decomposition( xag, table, pis, fn ) );

    default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
    CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  }
}

TEST_CASE( "Partial DSD decomposition into k-LUT network", "[dsd_decomposition]" )
{
  kitty::dynamic_truth_table table( 5u );
  kitty::create_from_expression( table, "{a<(bc)de>}" );

  klut_network ntk;
  const auto x1 = ntk.create_pi();
  const auto x2 = ntk.create_pi();
  const auto x3 = ntk.create_pi();
  const auto x4 = ntk.create_pi();
  const auto x5 = ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return ntk.create_node( children, remainder );
  };

  ntk.create_po( dsd_decomposition( ntk, table, {x1, x2, x3, x4, x5}, fn ) );

  CHECK( ntk.num_gates() == 3u );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == table );
}

TEST_CASE( "DSD decomposition on random functions of different size", "[dsd_decomposition]" )
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

      auto on_prime = [&]( kitty::dynamic_truth_table const& func, std::vector<aig_network::signal> const& pis ) {
        std::vector<uint32_t> vars( func.num_vars() );
        std::iota( vars.begin(), vars.end(), 0u );
        return shannon_decomposition( ntk, func, vars, pis );
      };

      ntk.create_po( dsd_decomposition( ntk, func, pis, on_prime ) );

      default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );

      CHECK( simulate<kitty::dynamic_truth_table>( ntk, sim )[0] == func );
    }
  }
}
