#include <catch.hpp>

#include <mockturtle/io/serialize.hpp>

using namespace mockturtle;

TEST_CASE( "serialize aig_network into a file", "[serialize]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( a, f1 );
  const auto f5 = aig.create_nand( f4, f3 );
  aig.create_po( f5 );

  /* serialize */
  serialize_network( aig, "aig.dmp" );

  /* deserialize */
  aig_network aig2 = deserialize_network( "aig.dmp" );

  CHECK( aig.size() == aig2.size() );
  CHECK( aig.num_cis() == aig2.num_cis() );
  CHECK( aig.num_cos() == aig2.num_cos() );
  CHECK( aig.num_gates() == aig2.num_gates() );

  CHECK( aig._storage->nodes == aig2._storage->nodes );
  CHECK( aig._storage->inputs == aig2._storage->inputs );
  CHECK( aig._storage->outputs == aig2._storage->outputs );
  CHECK( aig._storage->hash == aig2._storage->hash );

  CHECK( aig2._storage->hash.size() == 4u );
  CHECK( aig2._storage->nodes[f1.index].children[0u].index == a.index );
  CHECK( aig2._storage->nodes[f1.index].children[1u].index == b.index );
  CHECK( aig2._storage->nodes[f2.index].children[0u].index == a.index );
  CHECK( aig2._storage->nodes[f2.index].children[1u].index == f1.index );
  CHECK( aig2._storage->nodes[f3.index].children[0u].index == b.index );
  CHECK( aig2._storage->nodes[f3.index].children[1u].index == f1.index );
  CHECK( aig2._storage->nodes[f4.index].children[0u].index == a.index );
  CHECK( aig2._storage->nodes[f4.index].children[1u].index == f1.index );
  CHECK( aig2._storage->nodes[f5.index].children[0u].index == f4.index );
  CHECK( aig2._storage->nodes[f5.index].children[1u].index == f3.index );
}
