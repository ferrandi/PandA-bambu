#include <catch.hpp>

#include <kitty/algorithm.hpp>
#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( xmg.size() == 1 );
  CHECK( has_get_constant_v<xmg_network> );
  CHECK( has_is_constant_v<xmg_network> );
  CHECK( has_get_node_v<xmg_network> );
  CHECK( has_is_complemented_v<xmg_network> );

  const auto c0 = xmg.get_constant( false );
  CHECK( xmg.is_constant( xmg.get_node( c0 ) ) );
  CHECK( !xmg.is_pi( xmg.get_node( c0 ) ) );

  CHECK( xmg.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, xmg_network::signal> );
  CHECK( xmg.get_node( c0 ) == 0 );
  CHECK( !xmg.is_complemented( c0 ) );

  const auto c1 = xmg.get_constant( true );

  CHECK( xmg.get_node( c1 ) == 0 );
  CHECK( xmg.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "create and use primary inputs in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_create_pi_v<xmg_network> );

  auto a = xmg.create_pi();

  CHECK( xmg.size() == 2 );
  CHECK( xmg.num_pis() == 1 );
  CHECK( xmg.num_gates() == 0 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, xmg_network::signal> );

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

TEST_CASE( "create and use primary outputs in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_create_po_v<xmg_network> );

  const auto c0 = xmg.get_constant( false );
  const auto x1 = xmg.create_pi();

  CHECK( xmg.size() == 2 );
  CHECK( xmg.num_pis() == 1 );
  CHECK( xmg.num_pos() == 0 );

  xmg.create_po( c0 );
  xmg.create_po( x1 );
  xmg.create_po( !x1 );

  CHECK( xmg.size() == 2 );
  CHECK( xmg.num_pos() == 3 );

  xmg.foreach_po( [&]( auto s, auto i ) {
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

TEST_CASE( "create and use register in an xmg network", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_foreach_po_v<xmg_network> );
  CHECK( has_create_po_v<xmg_network> );
  CHECK( has_create_pi_v<xmg_network> );
  CHECK( has_create_ro_v<xmg_network> );
  CHECK( has_create_ri_v<xmg_network> );
  CHECK( has_create_maj_v<xmg_network> );

  const auto c0 = xmg.get_constant( false );
  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto x4 = xmg.create_pi();

  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_registers() == 0 );
  CHECK( xmg.num_cis() == 4 );
  CHECK( xmg.num_cos() == 0 );

  const auto f1 = xmg.create_maj( x1, x2, x3 );
  xmg.create_po( f1 );

  CHECK( xmg.num_pos() == 1 );

  const auto s1 = xmg.create_ro(); // ntk. input
  xmg.create_po( s1 );             // po

  const auto f2 = xmg.create_maj( f1, x4, c0 );
  xmg.create_ri( f2 ); // ntk. output

  CHECK( xmg.num_registers() == 1 );
  CHECK( xmg.num_cis() == 4 + 1 );
  CHECK( xmg.num_cos() == 2 + 1 );

  xmg.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( xmg.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xmg.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xmg.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xmg.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xmg.make_signal( node ) == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( xmg.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xmg.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xmg.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xmg.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xmg.make_signal( node ) == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( xmg.make_signal( node ) == s1 ); /* first state-bit */
      CHECK( xmg.is_ci( node ) );
      CHECK( !xmg.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_po( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_co( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    case 2:
      CHECK( node == f2 ); /* first next-state bit */
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "create unary operations in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_create_buf_v<xmg_network> );
  CHECK( has_create_not_v<xmg_network> );

  auto x1 = xmg.create_pi();

  CHECK( xmg.size() == 2 );

  auto f1 = xmg.create_buf( x1 );
  auto f2 = xmg.create_not( x1 );

  CHECK( xmg.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary and ternary operations in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_create_and_v<xmg_network> );
  CHECK( has_create_nand_v<xmg_network> );
  CHECK( has_create_or_v<xmg_network> );
  CHECK( has_create_nor_v<xmg_network> );
  CHECK( has_create_xor_v<xmg_network> );
  CHECK( has_create_maj_v<xmg_network> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();

  CHECK( xmg.size() == 3 );

  const auto f1 = xmg.create_and( x1, x2 );
  CHECK( xmg.size() == 4 );
  CHECK( xmg.num_gates() == 1 );

  const auto f2 = xmg.create_nand( x1, x2 );
  CHECK( xmg.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = xmg.create_or( x1, x2 );
  CHECK( xmg.size() == 5 );

  const auto f4 = xmg.create_nor( x1, x2 );
  CHECK( xmg.size() == 5 );
  CHECK( f3 == !f4 );

  xmg.create_xor( x1, x2 );
  CHECK( xmg.size() == 6 );

  xmg.create_maj( x1, x2, f1 );
  CHECK( xmg.size() == 7 );

  const auto f6 = xmg.create_maj( x1, x2, xmg.get_constant( false ) );
  CHECK( xmg.size() == 7 );
  CHECK( f1 == f6 );

  const auto f7 = xmg.create_maj( x1, x2, xmg.get_constant( true ) );
  CHECK( xmg.size() == 7 );
  CHECK( f3 == f7 );

  const auto x3 = xmg.create_pi();
  CHECK( xmg.size() == 8 );

  const auto f8 = xmg.create_maj( x1, x2, x3 );
  const auto f9 = xmg.create_maj( !x1, !x2, !x3 );
  CHECK( xmg.size() == 9 );
  CHECK( f8 == !f9 );

  xmg.create_xor3( x1, x2, x3 );
  CHECK( xmg.size() == 10 );
}

TEST_CASE( "hash nodes in xmg network", "[xmg]" )
{
  xmg_network xmg;

  auto a = xmg.create_pi();
  auto b = xmg.create_pi();
  auto c = xmg.create_pi();

  auto f = xmg.create_maj( a, b, c );
  auto g = xmg.create_maj( a, b, c );

  CHECK( xmg.size() == 5u );
  CHECK( xmg.num_gates() == 1u );

  CHECK( xmg.get_node( f ) == xmg.get_node( g ) );

  auto f1 = xmg.create_maj( a, !b, c );
  auto g1 = xmg.create_maj( a, !b, c );

  CHECK( xmg.size() == 6u );
  CHECK( xmg.num_gates() == 2u );

  CHECK( xmg.get_node( f1 ) == xmg.get_node( g1 ) );
}

TEST_CASE( "clone a node in xmg network", "[xmg]" )
{
  xmg_network xmg1, xmg2;

  CHECK( has_clone_node_v<xmg_network> );

  auto a1 = xmg1.create_pi();
  auto b1 = xmg1.create_pi();
  auto c1 = xmg1.create_pi();
  auto f1 = xmg1.create_maj( a1, b1, c1 );
  auto g1 = xmg1.create_xor3( a1, b1, c1 );
  CHECK( xmg1.size() == 6 );

  auto a2 = xmg2.create_pi();
  auto b2 = xmg2.create_pi();
  auto c2 = xmg2.create_pi();
  CHECK( xmg2.size() == 4 );

  auto f2 = xmg2.clone_node( xmg1, xmg1.get_node( f1 ), {a2, b2, c2} );
  xmg2.clone_node( xmg1, xmg1.get_node( g1 ), {a2, b2, c2} );
  CHECK( xmg2.size() == 6 );

  xmg2.foreach_fanin( xmg2.get_node( f2 ), [&]( auto const& s ) {
    CHECK( !xmg2.is_complemented( s ) );
  } );

  xmg2.foreach_gate( [&]( auto const& n, auto i ) {
    switch ( i )
    {
    default:
      break;
    case 0:
      CHECK( xmg2.is_maj( n ) );
      break;
    case 1:
      CHECK( xmg2.is_xor3( n ) );
      break;
    }
  } );
}

TEST_CASE( "structural properties of an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_size_v<xmg_network> );
  CHECK( has_num_pis_v<xmg_network> );
  CHECK( has_num_pos_v<xmg_network> );
  CHECK( has_num_gates_v<xmg_network> );
  CHECK( has_fanin_size_v<xmg_network> );
  CHECK( has_fanout_size_v<xmg_network> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();

  const auto f1 = xmg.create_maj( x1, x2, x3 );
  const auto f2 = xmg.create_maj( x1, x2, !x3 );

  xmg.create_po( f1 );
  xmg.create_po( f2 );

  CHECK( xmg.size() == 6 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 2 );
  CHECK( xmg.num_gates() == 2 );
  CHECK( xmg.fanin_size( xmg.get_node( x1 ) ) == 0 );
  CHECK( xmg.fanin_size( xmg.get_node( x2 ) ) == 0 );
  CHECK( xmg.fanin_size( xmg.get_node( x3 ) ) == 0 );
  CHECK( xmg.fanin_size( xmg.get_node( f1 ) ) == 3 );
  CHECK( xmg.fanin_size( xmg.get_node( f2 ) ) == 3 );
  CHECK( xmg.fanout_size( xmg.get_node( x1 ) ) == 2 );
  CHECK( xmg.fanout_size( xmg.get_node( x2 ) ) == 2 );
  CHECK( xmg.fanout_size( xmg.get_node( f1 ) ) == 1 );
  CHECK( xmg.fanout_size( xmg.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in an xmg", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_foreach_node_v<xmg_network> );
  CHECK( has_foreach_pi_v<xmg_network> );
  CHECK( has_foreach_po_v<xmg_network> );
  CHECK( has_foreach_gate_v<xmg_network> );
  CHECK( has_foreach_fanin_v<xmg_network> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto f1 = xmg.create_maj( x1, x2, x3 );
  const auto f2 = xmg.create_maj( x1, x2, !x3 );
  xmg.create_po( f1 );
  xmg.create_po( f2 );

  CHECK( xmg.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{0}, counter{0};
  xmg.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  xmg.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  xmg.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  xmg.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  xmg.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  xmg.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  xmg.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  xmg.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  xmg.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xmg.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  xmg.foreach_po( [&]( auto s ) { mask |= ( 1 << xmg.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  xmg.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xmg.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  xmg.foreach_po( [&]( auto s ) { mask |= ( 1 << xmg.get_node( s ) ); return false; } );
  CHECK( mask == 16 );

  /* iterate over gates */
  mask = counter = 0;
  xmg.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  xmg.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  xmg.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  xmg.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 16 );

  /* iterate over fanins */
  mask = counter = 0;
  xmg.foreach_fanin( xmg.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xmg.get_node( s ) ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  xmg.foreach_fanin( xmg.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xmg.get_node( s ) ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  xmg.foreach_fanin( xmg.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xmg.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  xmg.foreach_fanin( xmg.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xmg.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in xmgs", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_compute_v<xmg_network, bool> );
  CHECK( has_compute_v<xmg_network, kitty::dynamic_truth_table> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto f1 = xmg.create_maj( !x1, x2, x3 );
  const auto f2 = xmg.create_maj( x1, !x2, x3 );
  xmg.create_po( f1 );
  xmg.create_po( f2 );

  std::vector<bool> values{{true, false, true}};

  CHECK( xmg.compute( xmg.get_node( f1 ), values.begin(), values.end() ) == false );
  CHECK( xmg.compute( xmg.get_node( f2 ), values.begin(), values.end() ) == true );

  std::vector<kitty::dynamic_truth_table> xs{3, kitty::dynamic_truth_table( 3 )};
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );

  CHECK( xmg.compute( xmg.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
  CHECK( xmg.compute( xmg.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
}

TEST_CASE( "custom node values in xmgs", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_clear_values_v<xmg_network> );
  CHECK( has_value_v<xmg_network> );
  CHECK( has_set_value_v<xmg_network> );
  CHECK( has_incr_value_v<xmg_network> );
  CHECK( has_decr_value_v<xmg_network> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto f1 = xmg.create_maj( x1, x2, x3 );
  const auto f2 = xmg.create_maj( !x1, x2, x3 );
  xmg.create_po( f1 );
  xmg.create_po( f2 );

  CHECK( xmg.size() == 6 );

  xmg.clear_values();
  xmg.foreach_node( [&]( auto n ) {
    CHECK( xmg.value( n ) == 0 );
    xmg.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( xmg.value( n ) == n );
    CHECK( xmg.incr_value( n ) == n );
    CHECK( xmg.value( n ) == n + 1 );
    CHECK( xmg.decr_value( n ) == n );
    CHECK( xmg.value( n ) == n );
  } );
  xmg.clear_values();
  xmg.foreach_node( [&]( auto n ) {
    CHECK( xmg.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in xmgs", "[xmg]" )
{
  xmg_network xmg;

  CHECK( has_clear_visited_v<xmg_network> );
  CHECK( has_visited_v<xmg_network> );
  CHECK( has_set_visited_v<xmg_network> );

  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto f1 = xmg.create_maj( x1, x2, x3 );
  const auto f2 = xmg.create_and( x1, x2 );
  xmg.create_po( f1 );
  xmg.create_po( f2 );

  CHECK( xmg.size() == 6 );

  xmg.clear_visited();
  xmg.foreach_node( [&]( auto n ) {
    CHECK( xmg.visited( n ) == 0 );
    xmg.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( xmg.visited( n ) == n );
  } );
  xmg.clear_visited();
  xmg.foreach_node( [&]( auto n ) {
    CHECK( xmg.visited( n ) == 0 );
  } );
}

TEST_CASE( "node substitution in xmgs", "[xmg]" )
{
  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto f = xmg.create_and( a, b );

  CHECK( xmg.size() == 4 );

  xmg.foreach_fanin( xmg.get_node( f ), [&]( auto const& s ) {
    CHECK( !xmg.is_complemented( s ) );
  } );

  xmg.substitute_node( xmg.get_node( xmg.get_constant( false ) ), xmg.get_constant( true ) );

  CHECK( xmg.size() == 4 );

  xmg.foreach_fanin( xmg.get_node( f ), [&]( auto const& s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( xmg.is_complemented( s ) );
      break;
    default:
      CHECK( !xmg.is_complemented( s ) );
      break;
    }
  } );
}

TEST_CASE( "create nary functions in XMGs", "[xmg]" )
{
  xmg_network xmg;
  std::vector<xmg_network::signal> pis( 8u );
  std::generate( pis.begin(), pis.end(), [&]() { return xmg.create_pi(); } );
  xmg.create_po( xmg.create_nary_and( pis ) );
  xmg.create_po( xmg.create_nary_or( pis ) );
  xmg.create_po( xmg.create_nary_xor( pis ) );

  CHECK( xmg.num_gates() == 18u );

  auto result = simulate<kitty::dynamic_truth_table>( xmg, default_simulator<kitty::dynamic_truth_table>( 8 ) );

  CHECK( kitty::count_ones( result[0] ) == 1u );
  CHECK( kitty::get_bit( result[0], 255 ) );

  CHECK( kitty::count_ones( result[1] ) == 255u );
  CHECK( !kitty::get_bit( result[1], 0 ) );

  auto copy = result[2].construct();
  kitty::create_parity( copy );
  CHECK( result[2] == copy );
}
