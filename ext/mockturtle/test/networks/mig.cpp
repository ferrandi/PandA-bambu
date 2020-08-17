#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( mig.size() == 1 );
  CHECK( has_get_constant_v<mig_network> );
  CHECK( has_is_constant_v<mig_network> );
  CHECK( has_get_node_v<mig_network> );
  CHECK( has_is_complemented_v<mig_network> );

  const auto c0 = mig.get_constant( false );
  CHECK( mig.is_constant( mig.get_node( c0 ) ) );
  CHECK( !mig.is_pi( mig.get_node( c0 ) ) );

  CHECK( mig.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, mig_network::signal> );
  CHECK( mig.get_node( c0 ) == 0 );
  CHECK( !mig.is_complemented( c0 ) );

  const auto c1 = mig.get_constant( true );

  CHECK( mig.get_node( c1 ) == 0 );
  CHECK( mig.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "create and use primary inputs in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_create_pi_v<mig_network> );

  auto a = mig.create_pi();

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_gates() == 0 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, mig_network::signal> );

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

TEST_CASE( "create and use primary outputs in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_create_po_v<mig_network> );

  const auto c0 = mig.get_constant( false );
  const auto x1 = mig.create_pi();

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 0 );

  mig.create_po( c0 );
  mig.create_po( x1 );
  mig.create_po( !x1 );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pos() == 3 );

  mig.foreach_po( [&]( auto s, auto i ) {
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

TEST_CASE( "create and use register in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_foreach_po_v<mig_network> );
  CHECK( has_create_po_v<mig_network> );
  CHECK( has_create_pi_v<mig_network> );
  CHECK( has_create_ro_v<mig_network> );
  CHECK( has_create_ri_v<mig_network> );
  CHECK( has_create_maj_v<mig_network> );

  const auto c0 = mig.get_constant( false );
  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto x4 = mig.create_pi();

  CHECK( mig.size() == 5 );
  CHECK( mig.num_registers() == 0 );
  CHECK( mig.num_pis() == 4 );
  CHECK( mig.num_pos() == 0 );
  CHECK( mig.is_combinational() );

  const auto f1 = mig.create_maj( x1, x2, x3 );
  mig.create_po( f1 );
  mig.create_po( !f1 );

  const auto f2 = mig.create_maj( f1, x4, c0 );
  mig.create_ri( f2 );

  const auto ro = mig.create_ro();
  mig.create_po( ro );

  CHECK( mig.num_pos() == 3 );
  CHECK( mig.num_registers() == 1 );
  CHECK( !mig.is_combinational() );

  mig.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == f1 );
      break;
    case 1:
      CHECK( s == !f1 );
      break;
    case 2:
      // Check if the output (connected to the register) data is the same as the node data being registered.
      CHECK( f2.data == mig.po_at( i ).data );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
}

TEST_CASE( "create unary operations in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_create_buf_v<mig_network> );
  CHECK( has_create_not_v<mig_network> );

  auto x1 = mig.create_pi();

  CHECK( mig.size() == 2 );

  auto f1 = mig.create_buf( x1 );
  auto f2 = mig.create_not( x1 );

  CHECK( mig.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary and ternary operations in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_create_and_v<mig_network> );
  CHECK( has_create_nand_v<mig_network> );
  CHECK( has_create_or_v<mig_network> );
  CHECK( has_create_nor_v<mig_network> );
  CHECK( has_create_xor_v<mig_network> );
  CHECK( has_create_maj_v<mig_network> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();

  CHECK( mig.size() == 3 );

  const auto f1 = mig.create_and( x1, x2 );
  CHECK( mig.size() == 4 );
  CHECK( mig.num_gates() == 1 );

  const auto f2 = mig.create_nand( x1, x2 );
  CHECK( mig.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = mig.create_or( x1, x2 );
  CHECK( mig.size() == 5 );

  const auto f4 = mig.create_nor( x1, x2 );
  CHECK( mig.size() == 5 );
  CHECK( f3 == !f4 );

  mig.create_xor( x1, x2 );
  CHECK( mig.size() == 8 );

  mig.create_maj( x1, x2, f1 );
  CHECK( mig.size() == 9 );

  const auto f6 = mig.create_maj( x1, x2, mig.get_constant( false ) );
  CHECK( mig.size() == 9 );
  CHECK( f1 == f6 );

  const auto f7 = mig.create_maj( x1, x2, mig.get_constant( true ) );
  CHECK( mig.size() == 9 );
  CHECK( f3 == f7 );

  const auto x3 = mig.create_pi();

  const auto f8 = mig.create_maj( x1, x2, x3 );
  const auto f9 = mig.create_maj( !x1, !x2, !x3 );
  CHECK( f8 == !f9 );
}

TEST_CASE( "hash nodes in MIG network", "[mig]" )
{
  mig_network mig;

  auto a = mig.create_pi();
  auto b = mig.create_pi();
  auto c = mig.create_pi();

  auto f = mig.create_maj( a, b, c );
  auto g = mig.create_maj( a, b, c );

  CHECK( mig.size() == 5u );
  CHECK( mig.num_gates() == 1u );

  CHECK( mig.get_node( f ) == mig.get_node( g ) );

  auto f1 = mig.create_maj( a, !b, c );
  auto g1 = mig.create_maj( a, !b, c );

  CHECK( mig.size() == 6u );
  CHECK( mig.num_gates() == 2u );

  CHECK( mig.get_node( f1 ) == mig.get_node( g1 ) );
}

TEST_CASE( "clone a node in MIG network", "[mig]" )
{
  mig_network mig1, mig2;

  CHECK( has_clone_node_v<mig_network> );

  auto a1 = mig1.create_pi();
  auto b1 = mig1.create_pi();
  auto c1 = mig1.create_pi();
  auto f1 = mig1.create_maj( a1, b1, c1 );
  CHECK( mig1.size() == 5 );

  auto a2 = mig2.create_pi();
  auto b2 = mig2.create_pi();
  auto c2 = mig2.create_pi();
  CHECK( mig2.size() == 4 );

  auto f2 = mig2.clone_node( mig1, mig1.get_node( f1 ), {a2, b2, c2} );
  CHECK( mig2.size() == 5 );

  mig2.foreach_fanin( mig2.get_node( f2 ), [&]( auto const& s, auto ) {
    CHECK( !mig2.is_complemented( s ) );
  } );
}

TEST_CASE( "structural properties of an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_size_v<mig_network> );
  CHECK( has_num_pis_v<mig_network> );
  CHECK( has_num_pos_v<mig_network> );
  CHECK( has_num_gates_v<mig_network> );
  CHECK( has_fanin_size_v<mig_network> );
  CHECK( has_fanout_size_v<mig_network> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();

  const auto f1 = mig.create_maj( x1, x2, x3 );
  const auto f2 = mig.create_maj( x1, x2, !x3 );

  mig.create_po( f1 );
  mig.create_po( f2 );

  CHECK( mig.size() == 6 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 2 );
  CHECK( mig.num_gates() == 2 );
  CHECK( mig.fanin_size( mig.get_node( x1 ) ) == 0 );
  CHECK( mig.fanin_size( mig.get_node( x2 ) ) == 0 );
  CHECK( mig.fanin_size( mig.get_node( x3 ) ) == 0 );
  CHECK( mig.fanin_size( mig.get_node( f1 ) ) == 3 );
  CHECK( mig.fanin_size( mig.get_node( f2 ) ) == 3 );
  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 2 );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 2 );
  CHECK( mig.fanout_size( mig.get_node( f1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in an MIG", "[mig]" )
{
  mig_network mig;

  CHECK( has_foreach_node_v<mig_network> );
  CHECK( has_foreach_pi_v<mig_network> );
  CHECK( has_foreach_po_v<mig_network> );
  CHECK( has_foreach_gate_v<mig_network> );
  CHECK( has_foreach_fanin_v<mig_network> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto f1 = mig.create_maj( x1, x2, x3 );
  const auto f2 = mig.create_maj( x1, x2, !x3 );
  mig.create_po( f1 );
  mig.create_po( f2 );

  CHECK( mig.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{0}, counter{0};
  mig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  mig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  mig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  mig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  mig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  mig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  mig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  mig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  mig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << mig.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  mig.foreach_po( [&]( auto s ) { mask |= ( 1 << mig.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  mig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << mig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  mig.foreach_po( [&]( auto s ) { mask |= ( 1 << mig.get_node( s ) ); return false; } );
  CHECK( mask == 16 );

  /* iterate over gates */
  mask = counter = 0;
  mig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  mig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  mig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  mig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 16 );

  /* iterate over fanins */
  mask = counter = 0;
  mig.foreach_fanin( mig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << mig.get_node( s ) ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  mig.foreach_fanin( mig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << mig.get_node( s ) ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  mig.foreach_fanin( mig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << mig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  mig.foreach_fanin( mig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << mig.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in MIGs", "[mig]" )
{
  mig_network mig;

  CHECK( has_compute_v<mig_network, bool> );
  CHECK( has_compute_v<mig_network, kitty::dynamic_truth_table> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto f1 = mig.create_maj( !x1, x2, x3 );
  const auto f2 = mig.create_maj( x1, !x2, x3 );
  mig.create_po( f1 );
  mig.create_po( f2 );

  std::vector<bool> values{{true, false, true}};

  CHECK( mig.compute( mig.get_node( f1 ), values.begin(), values.end() ) == false );
  CHECK( mig.compute( mig.get_node( f2 ), values.begin(), values.end() ) == true );

  std::vector<kitty::dynamic_truth_table> xs{3, kitty::dynamic_truth_table( 3 )};
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );

  CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
  CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
}

TEST_CASE( "custom node values in MIGs", "[mig]" )
{
  mig_network mig;

  CHECK( has_clear_values_v<mig_network> );
  CHECK( has_value_v<mig_network> );
  CHECK( has_set_value_v<mig_network> );
  CHECK( has_incr_value_v<mig_network> );
  CHECK( has_decr_value_v<mig_network> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto f1 = mig.create_maj( x1, x2, x3 );
  const auto f2 = mig.create_maj( !x1, x2, x3 );
  mig.create_po( f1 );
  mig.create_po( f2 );

  CHECK( mig.size() == 6 );

  mig.clear_values();
  mig.foreach_node( [&]( auto n ) {
    CHECK( mig.value( n ) == 0 );
    mig.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( mig.value( n ) == n );
    CHECK( mig.incr_value( n ) == n );
    CHECK( mig.value( n ) == n + 1 );
    CHECK( mig.decr_value( n ) == n );
    CHECK( mig.value( n ) == n );
  } );
  mig.clear_values();
  mig.foreach_node( [&]( auto n ) {
    CHECK( mig.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in MIGs", "[mig]" )
{
  mig_network mig;

  CHECK( has_clear_visited_v<mig_network> );
  CHECK( has_visited_v<mig_network> );
  CHECK( has_set_visited_v<mig_network> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto f1 = mig.create_maj( x1, x2, x3 );
  const auto f2 = mig.create_and( x1, x2 );
  mig.create_po( f1 );
  mig.create_po( f2 );

  CHECK( mig.size() == 6 );

  mig.clear_visited();
  mig.foreach_node( [&]( auto n ) {
    CHECK( mig.visited( n ) == 0 );
    mig.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( mig.visited( n ) == n );
  } );
  mig.clear_visited();
  mig.foreach_node( [&]( auto n ) {
    CHECK( mig.visited( n ) == 0 );
  } );
}

TEST_CASE( "node substitution in MIGs", "[mig]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto f = mig.create_and( a, b );

  CHECK( mig.size() == 4 );

  mig.foreach_fanin( mig.get_node( f ), [&]( auto const& s ) {
    CHECK( !mig.is_complemented( s ) );
  } );

  mig.substitute_node( mig.get_node( mig.get_constant( false ) ), mig.get_constant( true ) );

  CHECK( mig.size() == 4 );

  mig.foreach_fanin( mig.get_node( f ), [&]( auto const& s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( mig.is_complemented( s ) );
      break;
    default:
      CHECK( !mig.is_complemented( s ) );
      break;
    }
  } );
}
