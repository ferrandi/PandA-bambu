#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/pattern_generation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

#include <kitty/bit_operations.hpp>

using namespace mockturtle;

TEST_CASE( "Stuck-at pattern generation", "[pattern_generation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  const auto f = aig.create_and( aig.create_and( a, b ), !aig.create_and( c, d ) );
  aig.create_po( f );

  partial_simulator sim( aig.num_pis(), 0 );
  sim.add_pattern( {0, 0, 0, 0} );
  sim.add_pattern( {1, 1, 1, 1} );

  pattern_generation( aig, sim );

  CHECK( sim.num_bits() == 3 );
  CHECK( ( kitty::get_bit( sim.compute_pi( 0 ), 2 ) && kitty::get_bit( sim.compute_pi( 1 ), 2 ) ) == true );
  CHECK( ( kitty::get_bit( sim.compute_pi( 2 ), 2 ) && kitty::get_bit( sim.compute_pi( 3 ), 2 ) ) == false );
}

TEST_CASE( "Constant node removal", "[pattern_generation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_and( !a, b );
  const auto f2 = aig.create_and( a, !b );
  const auto f3 = aig.create_or( f1, f2 );
  aig.create_po( f3 );

  const auto g1 = aig.create_and( a, b );
  const auto g2 = aig.create_and( !a, !b );
  const auto g3 = aig.create_or( g1, g2 );
  aig.create_po( g3 );

  const auto h = aig.create_and( f3, g3 );
  aig.create_po( h );

  CHECK( aig.num_gates() == 7 );

  partial_simulator sim( aig.num_pis(), 0 );
  pattern_generation_params ps;
  ps.substitute_const = true;
  pattern_generation( aig, sim, ps );

  CHECK( aig.num_gates() == 6 );
}

TEST_CASE( "Multiple stuck-at pattern generation", "[pattern_generation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  const auto f = aig.create_and( aig.create_and( a, b ), !aig.create_and( c, d ) );
  aig.create_po( f );

  partial_simulator sim( aig.num_pis(), 0 );
  sim.add_pattern( {0, 0, 0, 0} );
  sim.add_pattern( {1, 1, 1, 1} );
  sim.add_pattern( {1, 1, 0, 1} );
  sim.add_pattern( {0, 1, 1, 1} );

  pattern_generation_params ps;
  ps.num_stuck_at = 2;

  pattern_generation( aig, sim, ps );

  CHECK( sim.num_bits() == 5 );
  CHECK( ( kitty::get_bit( sim.compute_pi( 0 ), 4 ) && kitty::get_bit( sim.compute_pi( 1 ), 4 ) ) == true );
  CHECK( ( kitty::get_bit( sim.compute_pi( 2 ), 4 ) && kitty::get_bit( sim.compute_pi( 3 ), 4 ) ) == false );
}

TEST_CASE( "With observability awareness", "[pattern_generation]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( f1, c );
  const auto f3 = xag.create_and( !f1, !a );
  const auto f4 = xag.create_xor( f2, f3 );
  xag.create_po( f4 );

  partial_simulator sim( xag.num_pis(), 0 );
  sim.add_pattern( {0, 1, 1} ); // this is the only pattern making f1 = 0, but it's not observable
  sim.add_pattern( {1, 1, 0} );
  sim.add_pattern( {1, 1, 1} );

  pattern_generation_params ps;
  ps.odc_levels = -1;

  pattern_generation( xag, sim, ps );

  CHECK( sim.num_bits() == 4 );
  /* the generated pattern should be either 000, 010, or 101 */
  CHECK( ( ( !kitty::get_bit( sim.compute_pi( 0 ), 3 ) && !kitty::get_bit( sim.compute_pi( 2 ), 3 ) ) || ( kitty::get_bit( sim.compute_pi( 0 ), 3 ) && !kitty::get_bit( sim.compute_pi( 1 ), 3 ) && kitty::get_bit( sim.compute_pi( 2 ), 3 ) ) ) == true );
}
