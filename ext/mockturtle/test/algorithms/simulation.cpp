#include <catch.hpp>

#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>

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

  const auto tt = simulate<kitty::static_truth_table<2>>( aig )[0];
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
