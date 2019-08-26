#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/cut_view.hpp>

using namespace mockturtle;

TEST_CASE( "create cut view on AIG for XOR", "[cut_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.size() == 7 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 4 );

  cut_view cut1{aig, {aig.get_node( a ), aig.get_node( b )}, aig.get_node( f4 )};

  CHECK( is_network_type_v<decltype( cut1 )> );
  CHECK( cut1.size() == 7 );
  CHECK( cut1.num_pis() == 2 );
  CHECK( cut1.num_pos() == 1 );
  CHECK( cut1.num_gates() == 4 );

  cut_view cut2{aig, {aig.get_node( a ), aig.get_node( b ), aig.get_node( a )}, aig.get_node( f4 )};

  CHECK( is_network_type_v<decltype( cut1 )> );
  CHECK( cut2.size() == 7 );
  CHECK( cut2.num_pis() == 2 );
  CHECK( cut2.num_pos() == 1 );
  CHECK( cut2.num_gates() == 4 );

  cut_view cut3{aig, {aig.get_node( f2 ), aig.get_node( f3 )}, aig.get_node( f4 )};
  CHECK( cut3.size() == 4 );
  CHECK( cut3.num_pis() == 2 );
  CHECK( cut3.num_pos() == 1 );
  CHECK( cut3.num_gates() == 1 );

  CHECK( cut3.index_to_node( 0 ) == aig.get_node( aig.get_constant( false ) ) );
  CHECK( cut3.index_to_node( 1 ) == aig.get_node( f2 ) );
  CHECK( cut3.index_to_node( 2 ) == aig.get_node( f3 ) );
  CHECK( cut3.index_to_node( 3 ) == aig.get_node( f4 ) );

  CHECK( cut3.node_to_index( aig.get_node( aig.get_constant( false ) ) ) == 0 );
  CHECK( cut3.node_to_index( aig.get_node( f2 ) ) == 1 );
  CHECK( cut3.node_to_index( aig.get_node( f3 ) ) == 2 );
  CHECK( cut3.node_to_index( aig.get_node( f4 ) ) == 3 );
}
