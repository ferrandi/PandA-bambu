#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( aig.size() == 1 );
  CHECK( has_get_constant_v<aig_network> );
  CHECK( has_is_constant_v<aig_network> );
  CHECK( has_get_node_v<aig_network> );
  CHECK( has_is_complemented_v<aig_network> );

  const auto c0 = aig.get_constant( false );
  CHECK( aig.is_constant( aig.get_node( c0 ) ) );
  CHECK( !aig.is_pi( aig.get_node( c0 ) ) );

  CHECK( aig.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, aig_network::signal> );
  CHECK( aig.get_node( c0 ) == 0 );
  CHECK( !aig.is_complemented( c0 ) );

  const auto c1 = aig.get_constant( true );

  CHECK( aig.get_node( c1 ) == 0 );
  CHECK( aig.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "create and use primary inputs in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_create_pi_v<aig_network> );

  auto a = aig.create_pi();
  auto b = aig.create_pi();

  CHECK( aig.size() == 3 ); // constant + two primary inputs
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_gates() == 0 );
  CHECK( aig.is_pi( aig.get_node( a ) ) );
  CHECK( aig.is_pi( aig.get_node( b ) ) );
  CHECK( aig.pi_index( aig.get_node( a ) ) == 0 );
  CHECK( aig.pi_index( aig.get_node( b ) ) == 1 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, aig_network::signal> );

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = !a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );
}

TEST_CASE( "create and use primary outputs in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_create_po_v<aig_network> );

  const auto c0 = aig.get_constant( false );
  const auto x1 = aig.create_pi();

  CHECK( aig.size() == 2 );
  CHECK( aig.num_pis() == 1 );
  CHECK( aig.num_pos() == 0 );

  aig.create_po( c0 );
  aig.create_po( x1 );
  aig.create_po( !x1 );

  CHECK( aig.size() == 2 );
  CHECK( aig.num_pos() == 3 );

  aig.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == c0 );
      break;
    case 1:
      CHECK( s == x1 );
      break;
    case 2:
      CHECK( s == !x1 );
      break;
    }
  } );
}

TEST_CASE( "create unary operations in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_create_buf_v<aig_network> );
  CHECK( has_create_not_v<aig_network> );

  auto x1 = aig.create_pi();

  CHECK( aig.size() == 2 );

  auto f1 = aig.create_buf( x1 );
  auto f2 = aig.create_not( x1 );

  CHECK( aig.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary operations in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_create_and_v<aig_network> );
  CHECK( has_create_nand_v<aig_network> );
  CHECK( has_create_or_v<aig_network> );
  CHECK( has_create_nor_v<aig_network> );
  CHECK( has_create_xor_v<aig_network> );
  CHECK( has_create_xnor_v<aig_network> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  CHECK( aig.size() == 3 );

  const auto f1 = aig.create_and( x1, x2 );
  CHECK( aig.size() == 4 );

  const auto f2 = aig.create_nand( x1, x2 );
  CHECK( aig.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = aig.create_or( x1, x2 );
  CHECK( aig.size() == 5 );

  const auto f4 = aig.create_nor( x1, x2 );
  CHECK( aig.size() == 5 );
  CHECK( f3 == !f4 );

  const auto f5 = aig.create_xor( x1, x2 );
  CHECK( aig.size() == 8 );

  const auto f6 = aig.create_xnor( x1, x2 );
  CHECK( aig.size() == 8 );
  CHECK( f5 == !f6 );
}

TEST_CASE( "hash nodes in AIG network", "[aig]" )
{
  aig_network aig;

  auto a = aig.create_pi();
  auto b = aig.create_pi();

  auto f = aig.create_and( a, b );
  auto g = aig.create_and( a, b );

  CHECK( aig.size() == 4u );
  CHECK( aig.num_gates() == 1u );

  CHECK( aig.get_node( f ) == aig.get_node( g ) );
}

TEST_CASE( "clone a AIG network", "[aig]" )
{
  CHECK( has_clone_v<aig_network> );

  aig_network aig0;
  auto a = aig0.create_pi();
  auto b = aig0.create_pi();
  auto f0 = aig0.create_and( a, b );
  CHECK( aig0.size() == 4 );
  CHECK( aig0.num_gates() == 1 );

  auto aig1 = aig0;
  auto aig_clone = aig0.clone();

  auto c = aig1.create_pi();
  aig1.create_and( f0, c );
  CHECK( aig0.size() == 6 );
  CHECK( aig0.num_gates() == 2 );

  CHECK( aig_clone.size() == 4 );
  CHECK( aig_clone.num_gates() == 1 );
}

TEST_CASE( "clone a node in AIG network", "[aig]" )
{
  aig_network aig1, aig2;

  CHECK( has_clone_node_v<aig_network> );

  auto a1 = aig1.create_pi();
  auto b1 = aig1.create_pi();
  auto f1 = aig1.create_and( a1, b1 );
  CHECK( aig1.size() == 4 );

  auto a2 = aig2.create_pi();
  auto b2 = aig2.create_pi();
  CHECK( aig2.size() == 3 );

  auto f2 = aig2.clone_node( aig1, aig1.get_node( f1 ), { a2, b2 } );
  CHECK( aig2.size() == 4 );

  aig2.foreach_fanin( aig2.get_node( f2 ), [&]( auto const& s, auto ) {
    CHECK( !aig2.is_complemented( s ) );
  } );
}

TEST_CASE( "structural properties of an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_size_v<aig_network> );
  CHECK( has_num_pis_v<aig_network> );
  CHECK( has_num_pos_v<aig_network> );
  CHECK( has_num_gates_v<aig_network> );
  CHECK( has_fanin_size_v<aig_network> );
  CHECK( has_fanout_size_v<aig_network> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );

  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 2 );
  CHECK( aig.fanin_size( aig.get_node( x1 ) ) == 0 );
  CHECK( aig.fanin_size( aig.get_node( x2 ) ) == 0 );
  CHECK( aig.fanin_size( aig.get_node( f1 ) ) == 2 );
  CHECK( aig.fanin_size( aig.get_node( f2 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );
}

TEST_CASE( "check has_and in AIG", "[aig]" )
{
  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();

  auto const n4 = aig.create_and( !x1, x2 );
  auto const n5 = aig.create_and( x1, n4 );
  auto const n6 = aig.create_and( x3, n5 );
  auto const n7 = aig.create_and( n4, x2 );
  auto const n8 = aig.create_and( !n5, !n7 );
  auto const n9 = aig.create_and( !n8, n4 );

  aig.create_po( n6 );
  aig.create_po( n9 );

  CHECK( aig.has_and( !x1, x2 ).has_value() == true );
  CHECK( *aig.has_and( !x1, x2 ) == n4 );
  CHECK( aig.has_and( !x1, x3 ).has_value() == false );
  CHECK( aig.has_and( !n7, !n5 ).has_value() == true );
  CHECK( *aig.has_and( !n7, !n5 ) == n8 );
}

TEST_CASE( "node and signal iteration in an AIG", "[aig]" )
{
  aig_network aig;

  CHECK( has_foreach_node_v<aig_network> );
  CHECK( has_foreach_pi_v<aig_network> );
  CHECK( has_foreach_po_v<aig_network> );
  CHECK( has_foreach_gate_v<aig_network> );
  CHECK( has_foreach_fanin_v<aig_network> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  aig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 31 );
  CHECK( counter == 10 );

  mask = 0;
  aig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 31 );

  mask = counter = 0;
  aig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  aig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  aig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  aig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_po( [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  aig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_po( [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); return false; } );
  CHECK( mask == 8 );

  /* iterate over gates */
  mask = counter = 0;
  aig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  aig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 8 );

  /* iterate over fanins */
  mask = counter = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in AIGs", "[aig]" )
{
  aig_network aig;

  CHECK( has_compute_v<aig_network, bool> );
  CHECK( has_compute_v<aig_network, kitty::dynamic_truth_table> );
  CHECK( has_compute_v<aig_network, kitty::partial_truth_table> );
  CHECK( has_compute_inplace_v<aig_network, kitty::partial_truth_table> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_and( !x1, x2 );
  const auto f2 = aig.create_and( x1, !x2 );
  aig.create_po( f1 );
  aig.create_po( f2 );

  {
    std::vector<bool> values{ { true, false } };

    CHECK( aig.compute( aig.get_node( f1 ), values.begin(), values.end() ) == false );
    CHECK( aig.compute( aig.get_node( f2 ), values.begin(), values.end() ) == true );
  }

  {
    std::vector<kitty::dynamic_truth_table> xs{ 2, kitty::dynamic_truth_table( 2 ) };
    kitty::create_nth_var( xs[0], 0 );
    kitty::create_nth_var( xs[1], 1 );

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 2 };

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );

    CHECK( aig.compute( aig.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( aig.compute( aig.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 2 };
    kitty::partial_truth_table result;

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );

    aig.compute( aig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    aig.compute( aig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );

    aig.compute( aig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    aig.compute( aig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );

    aig.compute( aig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    aig.compute( aig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );

    aig.compute( aig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    aig.compute( aig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );
  }
}

TEST_CASE( "custom node values in AIGs", "[aig]" )
{
  aig_network aig;

  CHECK( has_clear_values_v<aig_network> );
  CHECK( has_value_v<aig_network> );
  CHECK( has_set_value_v<aig_network> );
  CHECK( has_incr_value_v<aig_network> );
  CHECK( has_decr_value_v<aig_network> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );

  aig.clear_values();
  aig.foreach_node( [&]( auto n ) {
    CHECK( aig.value( n ) == 0 );
    aig.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( aig.value( n ) == n );
    CHECK( aig.incr_value( n ) == n );
    CHECK( aig.value( n ) == n + 1 );
    CHECK( aig.decr_value( n ) == n );
    CHECK( aig.value( n ) == n );
  } );
  aig.clear_values();
  aig.foreach_node( [&]( auto n ) {
    CHECK( aig.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in AIGs", "[aig]" )
{
  aig_network aig;

  CHECK( has_clear_visited_v<aig_network> );
  CHECK( has_visited_v<aig_network> );
  CHECK( has_set_visited_v<aig_network> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );

  aig.clear_visited();
  aig.foreach_node( [&]( auto n ) {
    CHECK( aig.visited( n ) == 0 );
    aig.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( aig.visited( n ) == static_cast<uint32_t>( n ) );
  } );
  aig.clear_visited();
  aig.foreach_node( [&]( auto n ) {
    CHECK( aig.visited( n ) == 0 );
  } );
}

TEST_CASE( "simulate some special functions in AIGs", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();

  const auto f1 = aig.create_maj( x1, x2, x3 );
  const auto f2 = aig.create_ite( x1, x2, x3 );

  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.num_gates() == 6u );

  auto result = simulate<kitty::dynamic_truth_table>( aig, default_simulator<kitty::dynamic_truth_table>( 3 ) );

  CHECK( result[0]._bits[0] == 0xe8u );
  CHECK( result[1]._bits[0] == 0xd8u );
}

TEST_CASE( "substitute nodes with propagation in AIGs (test case 1)", "[aig]" )
{
  CHECK( has_substitute_node_v<aig_network> );
  CHECK( has_replace_in_node_v<aig_network> );

  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();
  const auto x4 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_and( x3, x4 );
  const auto f3 = aig.create_and( x1, x3 );
  const auto f4 = aig.create_and( f1, f2 );
  const auto f5 = aig.create_and( f3, f4 );

  aig.create_po( f5 );

  CHECK( aig.size() == 10u );
  CHECK( aig.num_gates() == 5u );
  CHECK( aig._storage->hash.size() == 5u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );

  CHECK( aig._storage->nodes[f5.index].children[0u].index == f3.index );
  CHECK( aig._storage->nodes[f5.index].children[1u].index == f4.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 1u );
  CHECK( !aig.is_dead( aig.get_node( f1 ) ) );

  aig.substitute_node( aig.get_node( x2 ), x3 );

  // Node of signal f1 is now relabelled
  CHECK( aig.size() == 10u );
  CHECK( aig.num_gates() == 4u );
  CHECK( aig._storage->hash.size() == 4u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );

  CHECK( aig._storage->nodes[f5.index].children[0u].index == f3.index );
  CHECK( aig._storage->nodes[f5.index].children[1u].index == f4.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 2u );
  CHECK( aig.is_dead( aig.get_node( f1 ) ) );

  aig = cleanup_dangling( aig );

  CHECK( aig.num_gates() == 4u );
}

TEST_CASE( "substitute nodes with propagation in AIGs (test case 2)", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_and( x1, x3 );
  const auto f3 = aig.create_and( f1, f2 );

  aig.create_po( f3 );

  CHECK( aig.num_gates() == 3u );
  CHECK( aig._storage->hash.size() == 3u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );
  CHECK( aig._storage->nodes[f2.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f2.index].children[1u].index == x3.index );
  CHECK( aig._storage->nodes[f3.index].children[0u].index == f1.index );
  CHECK( aig._storage->nodes[f3.index].children[1u].index == f2.index );
  CHECK( aig._storage->outputs[0].index == f3.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 1u );

  aig.substitute_node( aig.get_node( x2 ), x3 );

  // Node of signal f1 is now relabelled
  CHECK( aig.num_gates() == 1u );
  CHECK( aig._storage->hash.size() == 1u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );
  CHECK( aig._storage->nodes[f2.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f2.index].children[1u].index == x3.index );
  CHECK( aig._storage->nodes[f3.index].children[0u].index == f1.index );
  CHECK( aig._storage->nodes[f3.index].children[1u].index == f2.index );
  CHECK( aig._storage->outputs[0].index == f2.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );

  aig = cleanup_dangling( aig );

  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "substitute input by constant in NAND-based XOR circuit", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.num_gates() == 4u );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x6 );

  aig.substitute_node( aig.get_node( x1 ), aig.get_constant( true ) );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x3 );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f4 ) ) == 0u );
}

TEST_CASE( "substitute node by constant in NAND-based XOR circuit", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.num_gates() == 4u );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x6 );

  aig.substitute_node( aig.get_node( f3 ), aig.get_constant( false ) );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x2 );

  CHECK( aig.num_gates() == 2u );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f4 ) ) == 0u );
  CHECK( !aig.is_dead( aig.get_node( f1 ) ) );
  CHECK( !aig.is_dead( aig.get_node( f2 ) ) );
  CHECK( aig.is_dead( aig.get_node( f3 ) ) );
  CHECK( aig.is_dead( aig.get_node( f4 ) ) );
}

TEST_CASE( "substitute node by constant in NAND-based XOR circuit (test case 2)", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.num_gates() == 4u );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x6 );

  aig.substitute_node( aig.get_node( f1 ), aig.get_constant( false ) );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0xe );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f4 ) ) == 1u );
}

TEST_CASE( "invoke take_out_node two times on the same node", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );
  (void)f2;

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2u );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 2u );

  /* delete node */
  CHECK( !aig.is_dead( aig.get_node( f1 ) ) );
  aig.take_out_node( aig.get_node( f1 ) );
  CHECK( aig.is_dead( aig.get_node( f1 ) ) );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1u );

  /* ensure that double-deletion has no effect on the fanout-size of x1 and x2 */
  CHECK( aig.is_dead( aig.get_node( f1 ) ) );
  aig.take_out_node( aig.get_node( f1 ) );
  CHECK( aig.is_dead( aig.get_node( f1 ) ) );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1u );
}

TEST_CASE( "substitute node and restrash", "[aig]" )
{
  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();

  auto const f1 = aig.create_and( x1, x2 );
  auto const f2 = aig.create_and( f1, x2 );
  aig.create_po( f2 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x8 );

  /* substitute f1 with x1
   *
   * this is a very interesting test case because replacing f1 with x1
   * in f2 makes f2 and f1 equal.  a correct implementation will
   * create a new entry in the hash, although (x1, x2) is already
   * there, because (x1, x2) will be deleted in the next step.
   */
  aig.substitute_node( aig.get_node( f1 ), x1 );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x8 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );
}

TEST_CASE( "substitute node with complemented node in aig_network", "[aig]" )
{
  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();

  auto const f1 = aig.create_and( x1, x2 );
  auto const f2 = aig.create_and( x1, f1 );
  aig.create_po( f2 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x8 );

  aig.substitute_node( aig.get_node( f2 ), !f2 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x7 );
}

TEST_CASE( "substitute multiple nodes", "[aig]" )
{
  using node = aig_network::node;
  using signal = aig_network::signal;

  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();

  auto const n4 = aig.create_and( !x1, x2 );
  auto const n5 = aig.create_and( x1, n4 );
  auto const n6 = aig.create_and( x3, n5 );
  auto const n7 = aig.create_and( n4, x2 );
  auto const n8 = aig.create_and( !n5, !n7 );
  auto const n9 = aig.create_and( !n8, n4 );

  aig.create_po( n6 );
  aig.create_po( n9 );

  aig.substitute_nodes( std::list<std::pair<node, signal>>{
      { aig.get_node( n5 ), aig.get_constant( false ) },
      { aig.get_node( n9 ), n4 } } );

  CHECK( !aig.is_dead( aig.get_node( aig.get_constant( false ) ) ) );
  CHECK( !aig.is_dead( aig.get_node( x1 ) ) );
  CHECK( !aig.is_dead( aig.get_node( x2 ) ) );
  CHECK( !aig.is_dead( aig.get_node( x3 ) ) );
  CHECK( !aig.is_dead( aig.get_node( n4 ) ) );
  CHECK( aig.is_dead( aig.get_node( n5 ) ) );
  CHECK( aig.is_dead( aig.get_node( n6 ) ) );
  CHECK( aig.is_dead( aig.get_node( n7 ) ) );
  CHECK( aig.is_dead( aig.get_node( n8 ) ) );
  CHECK( aig.is_dead( aig.get_node( n9 ) ) );

  CHECK( aig.fanout_size( aig.get_node( aig.get_constant( false ) ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n4 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( n5 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n6 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n7 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n8 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n9 ) ) == 0u );

  aig.foreach_po( [&]( signal const o, uint32_t index ) {
    switch ( index )
    {
    case 0:
      CHECK( o == aig.get_constant( false ) );
      break;
    case 1:
      CHECK( o == n4 );
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "substitute node with dependency in aig_network", "[aig]" )
{
  aig_network aig{};

  auto const a = aig.create_pi();
  auto const b = aig.create_pi();
  auto const c = aig.create_pi();          /* place holder */
  auto const tmp = aig.create_and( b, c ); /* place holder */
  auto const f1 = aig.create_and( a, b );
  auto const f2 = aig.create_and( f1, tmp );
  auto const f3 = aig.create_and( f1, a );
  aig.create_po( f2 );
  aig.substitute_node( aig.get_node( tmp ), f3 );

  /**
   * issue #545
   *
   *      f2
   *     /  \
   *    /   f3
   *    \  /  \
   *  1->f1    a
   *
   * stack:
   * 1. push (f2->f3)
   * 2. push (f3->a)
   * 3. pop (f3->a)
   * 4. pop (f2->f3) but, f3 is dead !!!
   */

  aig.substitute_node( aig.get_node( f1 ), aig.get_constant( 1 ) /* constant 1 */ );

  CHECK( aig.is_dead( aig.get_node( f1 ) ) );
  CHECK( aig.is_dead( aig.get_node( f2 ) ) );
  CHECK( aig.is_dead( aig.get_node( f3 ) ) );
  aig.foreach_po( [&]( auto s ) {
    CHECK( aig.is_dead( aig.get_node( s ) ) == false );
  } );
}

TEST_CASE( "substitute node and re-strash case 2", "[aig]" )
{
  aig_network aig;

  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();
  auto const n4 = aig.create_and( x2, x3 );
  auto const n5 = aig.create_and( x1, n4 );
  auto const n6 = aig.create_and( n5, x3 );
  auto const n7 = aig.create_and( x1, n6 );
  aig.create_po( n7 );

  aig.substitute_node( aig.get_node( n6 ), n4 );
  /* replace in node n7: n6 <- n4 => re-strash with fanins (x1, n4) => n7 <- n5
   * take out node n6 => take out node n5 => take out node n4 (MFFC)
   * execute n7 <- n5, but n5 is dead => revive n5 and n4 */

  CHECK( !aig.is_dead( aig.get_node( n4 ) ) );
  CHECK( !aig.is_dead( aig.get_node( n5 ) ) );
  CHECK( aig.is_dead( aig.get_node( n6 ) ) );
  CHECK( aig.is_dead( aig.get_node( n7 ) ) );
  aig.foreach_fanin( aig.get_node( aig.po_at( 0 ) ), [&]( auto f, auto i ){
    switch ( i )
    {
    case 0:
      CHECK( f == x1 );
      break;
    case 1:
      CHECK( f == n4 );
      break;
    default:
      CHECK( false );
    }
  } );
  CHECK( aig.fanout_size( aig.get_node( n4 ) ) == 1 );
}

TEST_CASE( "substitute node without re-strashing case 1", "[aig]" )
{
  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const f1 = aig.create_and( x1, x2 );
  auto const f2 = aig.create_and( f1, x2 );
  aig.create_po( f2 );

  aig.substitute_node_no_restrash( aig.get_node( f1 ), x1 );
  aig = cleanup_dangling( aig );
  CHECK( aig.num_gates() == 1 );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x8 );
}

TEST_CASE( "substitute node without re-strashing case 2", "[aig]" )
{
  aig_network aig;

  auto const a = aig.create_pi();
  auto const b = aig.create_pi();
  auto const c = aig.create_pi();
  auto const tmp = aig.create_and( b, c );
  auto const f1 = aig.create_and( a, b );
  auto const f2 = aig.create_and( f1, tmp );
  auto const f3 = aig.create_and( f1, a );
  aig.create_po( f2 );

  aig.substitute_node_no_restrash( aig.get_node( tmp ), f3 );
  aig.substitute_node_no_restrash( aig.get_node( f1 ), aig.get_constant( 1 ) );
  aig = cleanup_dangling( aig );

  CHECK( aig.num_gates() == 0 );
  CHECK( !aig.is_dead( aig.get_node( aig.po_at( 0 ) ) ) );
  CHECK( aig.get_node( aig.po_at( 0 ) ) == aig.pi_at( 0 ) );
}

TEST_CASE( "substitute node without re-strashing case 3", "[aig]" )
{
  aig_network aig;

  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();
  auto const n4 = aig.create_and( x2, x3 );
  auto const n5 = aig.create_and( x1, n4 );
  auto const n6 = aig.create_and( n5, x3 );
  auto const n7 = aig.create_and( x1, n6 );
  aig.create_po( n7 );

  aig.substitute_node_no_restrash( aig.get_node( n6 ), n4 );
  aig = cleanup_dangling( aig );
  CHECK( aig.num_gates() == 2 );
  CHECK( simulate<kitty::static_truth_table<3u>>( aig )[0]._bits == 0x80 );
}
