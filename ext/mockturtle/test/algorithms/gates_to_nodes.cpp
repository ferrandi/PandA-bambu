#include <catch.hpp>

#include <mockturtle/algorithms/gates_to_nodes.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/klut.hpp>

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
