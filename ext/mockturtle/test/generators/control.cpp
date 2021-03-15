#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <kitty/bit_operations.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/control.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "Create n-to-2^n binary decoder using XAGs", "[control]" )
{
  for ( auto n = 0u; n <= 6u; ++n )
  {
    xag_network xag;
    std::vector<xag_network::signal> xs( n );
    std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
    const auto ds = binary_decoder( xag, xs );
    std::for_each( ds.begin(), ds.end(), [&]( auto const& d ) { xag.create_po( d ); } );

    const auto sim = simulate<kitty::dynamic_truth_table>( xag, {n} );

    for ( auto i = 0u; i < sim.size(); ++i )
    {
      const auto& s = sim[i];

      CHECK( kitty::count_ones( s ) == 1u );
      CHECK( static_cast<bool>( kitty::get_bit( s, i ) ) );
    }
  }
}

TEST_CASE( "Create a 2^k-way MUX in XAGs", "[control]" )
{
  for ( auto k = 1u; k <= 4u; ++k )
  {
    xag_network xag;
    std::vector<xag_network::signal> sel( k ), data( 1u << k );

    std::generate( sel.begin(), sel.end(), [&]() { return xag.create_pi(); } );
    std::generate( data.begin(), data.end(), [&]() { return xag.create_pi(); } );

    xag.create_po( binary_mux( xag, sel, data ) );

    CHECK( xag.num_gates() == 3 * ( data.size() - 1u ) );
  }
}

TEST_CASE( "Create a Klein-Paterson 2^k-way MUX in XAGs", "[control]" )
{
  for ( auto k = 1u; k <= 4u; ++k )
  {
    xag_network xag;
    std::vector<xag_network::signal> sel( k ), data( 1u << k );

    std::generate( sel.begin(), sel.end(), [&]() { return xag.create_pi(); } );
    std::generate( data.begin(), data.end(), [&]() { return xag.create_pi(); } );

    xag.create_po( binary_mux_klein_paterson( xag, sel, data ) );

    CHECK( xag.num_gates() <= 3 * ( data.size() - 1u ) );
  }
}
