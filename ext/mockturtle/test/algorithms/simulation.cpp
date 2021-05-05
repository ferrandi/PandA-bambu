#include <catch.hpp>

#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

#include <kitty/static_truth_table.hpp>

using namespace mockturtle;

TEST_CASE( "Simulate XOR AIG circuit with Booleans", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( !simulate<bool>( aig, default_simulator<bool>( {false, false} ) )[0] );
  CHECK( simulate<bool>( aig, default_simulator<bool>( {false, true} ) )[0] );
  CHECK( simulate<bool>( aig, default_simulator<bool>( {true, false} ) )[0] );
  CHECK( !simulate<bool>( aig, default_simulator<bool>( {false, false} ) )[0] );
}

TEST_CASE( "Simulate XOR AIG circuit with static truth table", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x6 );
}


TEST_CASE( "Simulate XOR AIG circuit with dynamic truth table", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  default_simulator<kitty::dynamic_truth_table> sim( 2 );
  const auto tt = simulate<kitty::dynamic_truth_table>( aig, sim )[0];
  CHECK( tt._bits[0] == 0x6 );
}

TEST_CASE( "Simulate XOR AIG circuit with pre-defined values", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  default_simulator<kitty::dynamic_truth_table> sim( 2 );

  unordered_node_map<kitty::dynamic_truth_table, aig_network> node_to_value( aig );
  simulate_nodes<kitty::dynamic_truth_table>( aig, node_to_value, sim );

  CHECK( ( aig.is_complemented( f4 ) ? ~node_to_value[f4] : node_to_value[f4] )._bits[0] == 0x6 );

  node_to_value.reset();

  /* set node f1 to false, such that function f1 becomes true */
  node_to_value[ aig.get_node( f1 ) ] = kitty::dynamic_truth_table( 2 );

  /* re-simulated with the fixed value for f1 */
  simulate_nodes<kitty::dynamic_truth_table>( aig, node_to_value, sim );
  CHECK( ( aig.is_complemented( f1 ) ? ~node_to_value[f1] : node_to_value[f1] )._bits[0] == 0xf );
  CHECK( ( aig.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[0] == 0x5 );
  CHECK( ( aig.is_complemented( f3 ) ? ~node_to_value[f3] : node_to_value[f3] )._bits[0] == 0x3 );
  CHECK( ( aig.is_complemented( f4 ) ? ~node_to_value[f4] : node_to_value[f4] )._bits[0] == 0xe );
}

TEST_CASE( "Partial simulator", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  std::vector<kitty::partial_truth_table> pats( 2 );
  pats[0].add_bits( 0x0a, 5 ); /* a = 01010 */
  pats[1].add_bits( 0x13, 5 ); /* b = 10011 */
  partial_simulator sim( pats );

  unordered_node_map<kitty::partial_truth_table, aig_network> node_to_value( aig );
  simulate_nodes( aig, node_to_value, sim );

  CHECK( ( aig.is_complemented( f1 ) ? ~node_to_value[f1] : node_to_value[f1] )._bits[0] == 0x1d ); /* f1 = 11101 */
  CHECK( ( aig.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[0] == 0x17 ); /* f2 = 10111 */
  CHECK( ( aig.is_complemented( f3 ) ? ~node_to_value[f3] : node_to_value[f3] )._bits[0] == 0x0e ); /* f3 = 01110 */
  CHECK( ( aig.is_complemented( f4 ) ? ~node_to_value[f4] : node_to_value[f4] )._bits[0] == 0x19 ); /* f4 = 11001 */
}

TEST_CASE( "Add pattern and re-simulate with partial_simulator", "[simulation]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto f1 = xag.create_xor( a, b );
  const auto f2 = xag.create_xor( f1, c );
  xag.create_po( f2 ); /* f2 = a ^ b ^ c */

  std::vector<kitty::partial_truth_table> pats( 3 );
  pats[0].add_bits( 0x1, 2 ); /* a = 01 */
  pats[1].add_bits( 0x1, 2 ); /* b = 01 */
  pats[2].add_bits( 0x1, 2 ); /* c = 01 */
  partial_simulator sim( pats );

  unordered_node_map<kitty::partial_truth_table, xag_network> node_to_value( xag );
  simulate_nodes( xag, node_to_value, sim );
  CHECK( sim.num_bits() == 2u );
  CHECK( ( xag.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[0] == 0x1 ); /* f2 = 01 */

  std::vector<bool> pattern( 3 );
  pattern[0] = 0; pattern[1] = 1; pattern[2] = 0;
  sim.add_pattern( pattern );
  pattern[0] = 1; pattern[1] = 0; pattern[2] = 0;
  sim.add_pattern( pattern );
  /* a = 1001, b = 0101, c = 0001 */

  simulate_nodes( xag, node_to_value, sim, false );
  CHECK( sim.num_bits() == 4u );
  CHECK( ( xag.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[0] == 0xd ); /* f2 = 1101 */
}

TEST_CASE( "Incremental simulation with partial_simulator", "[simulation]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto f1 = aig.create_xor( a, b );
  const auto f2 = aig.create_xor( f1, c );
  aig.create_po( f2 ); /* f2 = a ^ b ^ c */
  const auto f3 = aig.create_xor( b, !c );
  const auto f4 = aig.create_xor( f3, a );
  aig.create_po( f4 ); /* f4 = !(a ^ b ^ c) = !f2 */

  std::vector<kitty::partial_truth_table> pats( 3 );
  pats[0].add_bits( 0, 64 );
  pats[1].add_bits( 0, 64 );
  pats[2].add_bits( 0, 64 );
  partial_simulator sim( pats );

  unordered_node_map<kitty::partial_truth_table, aig_network> node_to_value( aig );
  simulate_nodes( aig, node_to_value, sim );
  CHECK( ( aig.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[0] == 0 );
  CHECK( ( aig.is_complemented( f4 ) ? ~node_to_value[f4] : node_to_value[f4] )._bits[0] == ~0 );

  std::vector<bool> pattern( 3 );
  pattern[0] = 0; pattern[1] = 1; pattern[2] = 0;
  sim.add_pattern( pattern );

  simulate_node( aig, aig.get_node( f2 ), node_to_value, sim );
  CHECK( node_to_value[f2].num_bits() == 65 );
  CHECK( ( aig.is_complemented( f2 ) ? ~node_to_value[f2] : node_to_value[f2] )._bits[1] == 1 );
  CHECK( node_to_value[f1].num_bits() == 65 ); /* f1 is in f2's fanin cone hence it is re-simulated. */
  CHECK( node_to_value[f4].num_bits() == 64 ); /* f4 is not in f2's fanin cone hence it is not re-simulated. */

  const auto f5 = aig.create_and( f2, f4 );
  simulate_node( aig, aig.get_node( f5 ), node_to_value, sim );
  CHECK( ( aig.is_complemented( f5 ) ? ~node_to_value[f5] : node_to_value[f5] ) == kitty::partial_truth_table( 65 ) );
}

TEST_CASE( "Bit packing", "[simulation]" )
{
  std::vector<kitty::partial_truth_table> pats( 5 );
  pats[0].add_bits( 0x1, 2 ); /* x0 = 01 */
  pats[1].add_bits( 0x1, 2 ); /* x1 = 01 */
  pats[2].add_bits( 0x1, 2 ); /* x2 = 01 */
  pats[3].add_bits( 0x1, 2 ); /* x3 = 01 */
  pats[4].add_bits( 0x1, 2 ); /* x4 = 01 */
  bit_packed_simulator sim( pats ); /* initial patterns are not packable (all bits are cared) */

  std::vector<bool> p( 5 ); /* pattern */
  std::vector<bool> c( 5 ); /* care */

  /* first pattern */
  p[0] = 0; p[1] = 1; p[2] = 0; p[3] = 1; p[4] = 1;
  c[0] = 0; c[1] = 0; c[2] = 0; c[3] = 1; c[4] = 1;
  sim.add_pattern( p, c );

  /* second pattern */
  p[0] = 1; p[1] = 0; p[2] = 1; p[3] = 0; p[4] = 1;
  c[0] = 0; c[1] = 0; c[2] = 1; c[3] = 0; c[4] = 1;
  sim.add_pattern( p, c );

  CHECK( sim.pack_bits() == false );
  CHECK( sim.num_bits() == 4u );

  /* third pattern: can be packed into the third bit */
  p[0] = 1; p[1] = 0; p[2] = 1; p[3] = 0; p[4] = 0;
  c[0] = 0; c[1] = 1; c[2] = 1; c[3] = 0; c[4] = 0;
  sim.add_pattern( p, c );

  CHECK( sim.pack_bits() == true );
  CHECK( sim.num_bits() == 4u );
  CHECK( ( sim.compute_pi( 0 )._bits[0] & 0x3 ) == 0x1 ); /* x0 = xxx01 -> xx01 */
  CHECK( ( sim.compute_pi( 1 )._bits[0] & 0x7 ) == 0x1 ); /* x1 = 0xx01 -> x001 */
  CHECK( ( sim.compute_pi( 2 )._bits[0] & 0xf ) == 0xd ); /* x2 = 11x01 -> 1101 */
  CHECK( ( sim.compute_pi( 3 )._bits[0] & 0x7 ) == 0x5 ); /* x3 = xx101 -> x101 */
  CHECK( ( sim.compute_pi( 4 )._bits[0] & 0xf ) == 0xd ); /* x4 = x1101 -> 1101 */

  /* fourth pattern: can be packed into the fourth bit */
  p[0] = 1; p[1] = 0; p[2] = 0; p[3] = 1; p[4] = 0;
  c[0] = 1; c[1] = 1; c[2] = 0; c[3] = 1; c[4] = 0;
  sim.add_pattern( p, c );

  /* fifth pattern */
  p[0] = 0; p[1] = 1; p[2] = 0; p[3] = 1; p[4] = 1;
  c[0] = 1; c[1] = 0; c[2] = 0; c[3] = 0; c[4] = 1;
  sim.add_pattern( p, c );

  /* sixth pattern: can be packed into the third bit */
  p[0] = 1; p[1] = 0; p[2] = 0; p[3] = 1; p[4] = 0;
  c[0] = 1; c[1] = 0; c[2] = 0; c[3] = 0; c[4] = 0;
  sim.add_pattern( p, c );

  CHECK( sim.pack_bits() == true );
  CHECK( sim.num_bits() == 5u );
  CHECK( ( sim.compute_pi( 0 )._bits[0] & 0x1f ) == 0x0d ); /* x0 = 101xx01 -> 01101 */
  CHECK( ( sim.compute_pi( 1 )._bits[0] & 0x0f ) == 0x01 ); /* x1 = xx0x001 -> x0001 */
  CHECK( ( sim.compute_pi( 2 )._bits[0] & 0x0f ) == 0x0d ); /* x2 = xxx1101 -> x1101 */
  CHECK( ( sim.compute_pi( 3 )._bits[0] & 0x0f ) == 0x0d ); /* x3 = xx1x101 -> x1101 */
  CHECK( ( sim.compute_pi( 4 )._bits[0] & 0x1f ) == 0x1d ); /* x4 = x1x1101 -> 11101 */
}
