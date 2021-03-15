#include <catch.hpp>

#include <mockturtle/algorithms/gates_to_nodes.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/klut.hpp>

#include <kitty/dynamic_truth_table.hpp>

using namespace mockturtle;

TEST_CASE( "Gates to nodes from NAND XOR", "[gates_to_nodes]" )
{
  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto f1 = aig.create_nand( a, b );
  auto f2 = aig.create_nand( a, f1 );
  auto f3 = aig.create_nand( b, f1 );
  auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  const auto klut = gates_to_nodes<klut_network>( aig );
  CHECK( klut.size() == aig.size() + 2 );
  CHECK( klut.num_pis() == aig.num_pis() );
  CHECK( klut.num_pos() == aig.num_pos() );
  CHECK( klut.num_gates() == aig.num_gates() + 1 );
}

TEST_CASE( "Single node network from AIG", "[gates_to_nodes]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();
  aig.create_po( aig.create_maj( a, b, c ) );
  aig.create_po( aig.create_ite( b, c, d ) );
  aig.create_po( aig.create_xor3( a, c, d ) );
  aig.create_po( aig.create_nary_or( {a, b, c, d} ) );

  const auto klut = single_node_network<klut_network>( aig );

  klut.foreach_gate( [&]( auto n, auto i ) {
    switch ( i )
    {
    case 0:
    case 1:
    case 2:
      CHECK( klut.fanin_size( n ) == 3 );
      break;
    case 3:
      CHECK( klut.fanin_size( n ) == 4 );
      break;
    default:
      CHECK( false );
      break;
    }
  } );

  default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim ) == simulate<kitty::dynamic_truth_table>( klut, sim ) );
}
