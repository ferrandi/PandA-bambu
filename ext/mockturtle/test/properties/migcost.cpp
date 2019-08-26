#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/properties/migcost.hpp>

using namespace mockturtle;

TEST_CASE( "count inverters in AIG", "[migcost]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( num_inverters( aig ) == 4u );
}

TEST_CASE( "count dangling inputs in AIG", "[migcost]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( num_dangling_inputs( aig ) == 4u );
}
