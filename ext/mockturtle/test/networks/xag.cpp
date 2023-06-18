#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <kitty/algorithm.hpp>
#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( xag.size() == 1 );
  CHECK( has_get_constant_v<xag_network> );
  CHECK( has_is_constant_v<xag_network> );
  CHECK( has_get_node_v<xag_network> );
  CHECK( has_is_complemented_v<xag_network> );

  const auto c0 = xag.get_constant( false );
  CHECK( xag.is_constant( xag.get_node( c0 ) ) );
  CHECK( !xag.is_pi( xag.get_node( c0 ) ) );

  CHECK( xag.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, xag_network::signal> );
  CHECK( xag.get_node( c0 ) == 0 );
  CHECK( !xag.is_complemented( c0 ) );

  const auto c1 = xag.get_constant( true );

  CHECK( xag.get_node( c1 ) == 0 );
  CHECK( xag.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "special cases in XAGs", "[xag]" )
{
  xag_network xag;
  auto x = xag.create_pi();

  CHECK( xag.create_xor( xag.get_constant( false ), xag.get_constant( false ) ) == xag.get_constant( false ) );
  CHECK( xag.create_xor( xag.get_constant( false ), xag.get_constant( true ) ) == xag.get_constant( true ) );
  CHECK( xag.create_xor( xag.get_constant( true ), xag.get_constant( false ) ) == xag.get_constant( true ) );
  CHECK( xag.create_xor( xag.get_constant( true ), xag.get_constant( true ) ) == xag.get_constant( false ) );

  CHECK( xag.create_and( xag.get_constant( false ), xag.get_constant( false ) ) == xag.get_constant( false ) );
  CHECK( xag.create_and( xag.get_constant( false ), xag.get_constant( true ) ) == xag.get_constant( false ) );
  CHECK( xag.create_and( xag.get_constant( true ), xag.get_constant( false ) ) == xag.get_constant( false ) );
  CHECK( xag.create_and( xag.get_constant( true ), xag.get_constant( true ) ) == xag.get_constant( true ) );

  CHECK( xag.create_xor( !x, xag.get_constant( false ) ) == !x );
  CHECK( xag.create_xor( !x, xag.get_constant( true ) ) == x );
  CHECK( xag.create_xor( x, xag.get_constant( false ) ) == x );
  CHECK( xag.create_xor( x, xag.get_constant( true ) ) == !x );

  CHECK( xag.create_and( !x, xag.get_constant( false ) ) == xag.get_constant( false ) );
  CHECK( xag.create_and( !x, xag.get_constant( true ) ) == !x );
  CHECK( xag.create_and( x, xag.get_constant( false ) ) == xag.get_constant( false ) );
  CHECK( xag.create_and( x, xag.get_constant( true ) ) == x );

  CHECK( xag.create_xor( x, x ) == xag.get_constant( false ) );
  CHECK( xag.create_xor( !x, x ) == xag.get_constant( true ) );
  CHECK( xag.create_xor( x, !x ) == xag.get_constant( true ) );
  CHECK( xag.create_xor( !x, !x ) == xag.get_constant( false ) );

  CHECK( xag.create_and( x, x ) == x );
  CHECK( xag.create_and( !x, x ) == xag.get_constant( false ) );
  CHECK( xag.create_and( x, !x ) == xag.get_constant( false ) );
  CHECK( xag.create_and( !x, !x ) == !x );
}

TEST_CASE( "create and use primary inputs in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_create_pi_v<xag_network> );

  auto a = xag.create_pi();
  auto b = xag.create_pi();

  CHECK( xag.size() == 3 ); // constant + two primary inputs
  CHECK( xag.num_pis() == 2 );
  CHECK( xag.num_gates() == 0 );
  CHECK( xag.is_pi( xag.get_node( a ) ) );
  CHECK( xag.is_pi( xag.get_node( b ) ) );
  CHECK( xag.pi_index( xag.get_node( a ) ) == 0 );
  CHECK( xag.pi_index( xag.get_node( b ) ) == 1 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, xag_network::signal> );

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

TEST_CASE( "create and use primary outputs in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_create_po_v<xag_network> );

  const auto c0 = xag.get_constant( false );
  const auto x1 = xag.create_pi();

  CHECK( xag.size() == 2 );
  CHECK( xag.num_pis() == 1 );
  CHECK( xag.num_pos() == 0 );

  xag.create_po( c0 );
  xag.create_po( x1 );
  xag.create_po( !x1 );

  CHECK( xag.size() == 2 );
  CHECK( xag.num_pos() == 3 );

  xag.foreach_po( [&]( auto s, auto i ) {
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

TEST_CASE( "create unary operations in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_create_buf_v<xag_network> );
  CHECK( has_create_not_v<xag_network> );

  auto x1 = xag.create_pi();

  CHECK( xag.size() == 2 );

  auto f1 = xag.create_buf( x1 );
  auto f2 = xag.create_not( x1 );

  CHECK( xag.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary operations in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_create_and_v<xag_network> );
  CHECK( has_create_nand_v<xag_network> );
  CHECK( has_create_or_v<xag_network> );
  CHECK( has_create_nor_v<xag_network> );
  CHECK( has_create_xor_v<xag_network> );
  CHECK( has_create_xnor_v<xag_network> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();

  CHECK( xag.size() == 3 );

  const auto f1 = xag.create_and( x1, x2 );
  CHECK( xag.size() == 4 );

  const auto f2 = xag.create_nand( x1, x2 );
  CHECK( xag.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = xag.create_or( x1, x2 );
  CHECK( xag.size() == 5 );

  const auto f4 = xag.create_nor( x1, x2 );
  CHECK( xag.size() == 5 );
  CHECK( f3 == !f4 );

  const auto f5 = xag.create_xor( x1, x2 );
  CHECK( xag.size() == 6 );

  const auto f6 = xag.create_xnor( x1, x2 );
  CHECK( xag.size() == 6 );
  CHECK( f5 == !f6 );
}

TEST_CASE( "hash nodes in xag network", "[xag]" )
{
  xag_network xag;

  auto a = xag.create_pi();
  auto b = xag.create_pi();

  auto f = xag.create_and( a, b );
  auto g = xag.create_and( a, b );

  CHECK( xag.size() == 4u );
  CHECK( xag.num_gates() == 1u );

  CHECK( xag.get_node( f ) == xag.get_node( g ) );
}

TEST_CASE( "clone a XAG network", "[xag]" )
{
  CHECK( has_clone_v<xag_network> );

  xag_network xag0;
  auto a = xag0.create_pi();
  auto b = xag0.create_pi();
  auto f0 = xag0.create_and( a, b );
  CHECK( xag0.size() == 4 );
  CHECK( xag0.num_gates() == 1 );

  auto xag1 = xag0;
  auto xag_clone = xag0.clone();

  auto c = xag1.create_pi();
  xag1.create_xor( f0, c );
  CHECK( xag0.size() == 6 );
  CHECK( xag0.num_gates() == 2 );

  CHECK( xag_clone.size() == 4 );
  CHECK( xag_clone.num_gates() == 1 );
}

TEST_CASE( "clone a node in xag network", "[xag]" )
{
  xag_network xag1, xag2;

  CHECK( has_clone_node_v<xag_network> );

  auto a1 = xag1.create_pi();
  auto b1 = xag1.create_pi();
  auto f1 = xag1.create_and( a1, b1 );
  CHECK( xag1.size() == 4 );

  auto a2 = xag2.create_pi();
  auto b2 = xag2.create_pi();
  CHECK( xag2.size() == 3 );

  auto f2 = xag2.clone_node( xag1, xag1.get_node( f1 ), { a2, b2 } );
  CHECK( xag2.size() == 4 );

  xag2.foreach_fanin( xag2.get_node( f2 ), [&]( auto const& s, auto ) {
    CHECK( !xag2.is_complemented( s ) );
  } );
}

TEST_CASE( "structural properties of an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_size_v<xag_network> );
  CHECK( has_num_pis_v<xag_network> );
  CHECK( has_num_pos_v<xag_network> );
  CHECK( has_num_gates_v<xag_network> );
  CHECK( has_fanin_size_v<xag_network> );
  CHECK( has_fanout_size_v<xag_network> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();

  const auto f1 = xag.create_and( x1, x2 );
  const auto f2 = xag.create_xor( x1, x2 );

  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.size() == 5 );
  CHECK( xag.is_and( xag.get_node( f1 ) ) == true );
  CHECK( xag.is_xor( xag.get_node( f1 ) ) == false );
  CHECK( xag.num_pis() == 2 );
  CHECK( xag.num_pos() == 2 );
  CHECK( xag.num_gates() == 2 );
  CHECK( xag.fanin_size( xag.get_node( x1 ) ) == 0 );
  CHECK( xag.fanin_size( xag.get_node( x2 ) ) == 0 );
  CHECK( xag.fanin_size( xag.get_node( f1 ) ) == 2 );
  CHECK( xag.fanin_size( xag.get_node( f2 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in an xag", "[xag]" )
{
  xag_network xag;

  CHECK( has_foreach_node_v<xag_network> );
  CHECK( has_foreach_pi_v<xag_network> );
  CHECK( has_foreach_po_v<xag_network> );
  CHECK( has_foreach_gate_v<xag_network> );
  CHECK( has_foreach_fanin_v<xag_network> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto f1 = xag.create_and( x1, x2 );
  const auto f2 = xag.create_or( x1, x2 );
  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.size() == 5 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  xag.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 31 );
  CHECK( counter == 10 );

  mask = 0;
  xag.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 31 );

  mask = counter = 0;
  xag.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  xag.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  xag.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  xag.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  xag.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  xag.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  xag.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xag.get_node( s ) ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  xag.foreach_po( [&]( auto s ) { mask |= ( 1 << xag.get_node( s ) ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  xag.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xag.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  xag.foreach_po( [&]( auto s ) { mask |= ( 1 << xag.get_node( s ) ); return false; } );
  CHECK( mask == 8 );

  /* iterate over gates */
  mask = counter = 0;
  xag.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  xag.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  xag.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  xag.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 8 );

  /* iterate over fanins */
  mask = counter = 0;
  xag.foreach_fanin( xag.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xag.get_node( s ) ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  xag.foreach_fanin( xag.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xag.get_node( s ) ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  xag.foreach_fanin( xag.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xag.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  xag.foreach_fanin( xag.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xag.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in XAGs", "[xag]" )
{
  xag_network xag;

  CHECK( has_compute_v<xag_network, bool> );
  CHECK( has_compute_v<xag_network, kitty::dynamic_truth_table> );
  CHECK( has_compute_v<xag_network, kitty::partial_truth_table> );
  CHECK( has_compute_inplace_v<xag_network, kitty::partial_truth_table> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto f1 = xag.create_and( !x1, x2 );
  const auto f2 = xag.create_and( x1, !x2 );
  xag.create_po( f1 );
  xag.create_po( f2 );

  {
    std::vector<bool> values{ { true, false } };

    CHECK( xag.compute( xag.get_node( f1 ), values.begin(), values.end() ) == false );
    CHECK( xag.compute( xag.get_node( f2 ), values.begin(), values.end() ) == true );
  }

  {
    std::vector<kitty::dynamic_truth_table> xs{ 2, kitty::dynamic_truth_table( 2 ) };
    kitty::create_nth_var( xs[0], 0 );
    kitty::create_nth_var( xs[1], 1 );

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 2 };

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );

    CHECK( xag.compute( xag.get_node( f1 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
    CHECK( xag.compute( xag.get_node( f2 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 2 };
    kitty::partial_truth_table result;

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );

    xag.compute( xag.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    xag.compute( xag.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );

    xag.compute( xag.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    xag.compute( xag.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );

    xag.compute( xag.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    xag.compute( xag.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );

    xag.compute( xag.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ~xs[0] & xs[1] ) );
    xag.compute( xag.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( xs[0] & ~xs[1] ) );
  }
}

TEST_CASE( "custom node values in xags", "[xag]" )
{
  xag_network xag;

  CHECK( has_clear_values_v<xag_network> );
  CHECK( has_value_v<xag_network> );
  CHECK( has_set_value_v<xag_network> );
  CHECK( has_incr_value_v<xag_network> );
  CHECK( has_decr_value_v<xag_network> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto f1 = xag.create_and( x1, x2 );
  const auto f2 = xag.create_or( x1, x2 );
  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.size() == 5 );

  xag.clear_values();
  xag.foreach_node( [&]( auto n ) {
    CHECK( xag.value( n ) == 0 );
    xag.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( xag.value( n ) == n );
    CHECK( xag.incr_value( n ) == n );
    CHECK( xag.value( n ) == n + 1 );
    CHECK( xag.decr_value( n ) == n );
    CHECK( xag.value( n ) == n );
  } );
  xag.clear_values();
  xag.foreach_node( [&]( auto n ) {
    CHECK( xag.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in xags", "[xag]" )
{
  xag_network xag;

  CHECK( has_clear_visited_v<xag_network> );
  CHECK( has_visited_v<xag_network> );
  CHECK( has_set_visited_v<xag_network> );

  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto f1 = xag.create_and( x1, x2 );
  const auto f2 = xag.create_or( x1, x2 );
  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.size() == 5 );

  xag.clear_visited();
  xag.foreach_node( [&]( auto n ) {
    CHECK( xag.visited( n ) == 0 );
    xag.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( xag.visited( n ) == static_cast<uint32_t>( n ) );
  } );
  xag.clear_visited();
  xag.foreach_node( [&]( auto n ) {
    CHECK( xag.visited( n ) == 0 );
  } );
}

TEST_CASE( "check has_and and has_xor in XAG", "[xag]" )
{
  xag_network xag;
  auto const x1 = xag.create_pi();
  auto const x2 = xag.create_pi();
  auto const x3 = xag.create_pi();

  auto const n4 = xag.create_and( !x1, x2 );
  auto const n5 = xag.create_and( x1, n4 );
  auto const n6 = xag.create_xor( x3, n5 );
  auto const n7 = xag.create_and( n4, x2 );
  auto const n8 = xag.create_and( !n5, !n7 );
  auto const n9 = xag.create_xor( !n8, n4 );

  xag.create_po( n6 );
  xag.create_po( n9 );

  CHECK( xag.has_and( !x1, x2 ).has_value() == true );
  CHECK( *xag.has_and( !x1, x2 ) == n4 );
  CHECK( xag.has_xor( !x1, x2 ).has_value() == false );
  CHECK( xag.has_and( !x1, x3 ).has_value() == false );
  CHECK( xag.has_xor( !x1, x3 ).has_value() == false );
  CHECK( xag.has_xor( n5, x3 ).has_value() == true );
  CHECK( *xag.has_xor( n5, x3 ) == n6 );
  CHECK( xag.has_xor( !n5, !x3 ).has_value() == true );
  CHECK( *xag.has_xor( !n5, !x3 ) == n6 );
  CHECK( xag.has_xor( !n5, x3 ).has_value() == true );
  CHECK( *xag.has_xor( !n5, x3 ) == !n6 );
  CHECK( xag.has_and( !n7, !n5 ).has_value() == true );
  CHECK( *xag.has_and( !n7, !n5 ) == n8 );
}

TEST_CASE( "simulate some special functions in XAGs", "[xag]" )
{
  xag_network xag;
  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto x3 = xag.create_pi();

  const auto f1 = xag.create_maj( x1, x2, x3 );
  const auto f2 = xag.create_ite( x1, x2, x3 );

  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.num_gates() == 7u );

  auto result = simulate<kitty::dynamic_truth_table>( xag, default_simulator<kitty::dynamic_truth_table>( 3 ) );

  CHECK( result[0]._bits[0] == 0xe8u );
  CHECK( result[1]._bits[0] == 0xd8u );
}

TEST_CASE( "create nary functions in XAGs", "[xag]" )
{
  xag_network xag;
  std::vector<xag_network::signal> pis( 8u );
  std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );
  xag.create_po( xag.create_nary_and( pis ) );
  xag.create_po( xag.create_nary_or( pis ) );
  xag.create_po( xag.create_nary_xor( pis ) );

  CHECK( xag.num_gates() == 21u );

  auto result = simulate<kitty::dynamic_truth_table>( xag, default_simulator<kitty::dynamic_truth_table>( 8 ) );

  CHECK( kitty::count_ones( result[0] ) == 1u );
  CHECK( kitty::get_bit( result[0], 255 ) );

  CHECK( kitty::count_ones( result[1] ) == 255u );
  CHECK( !kitty::get_bit( result[1], 0 ) );

  auto copy = result[2].construct();
  kitty::create_parity( copy );
  CHECK( result[2] == copy );
}

TEST_CASE( "invoke take_out_node two times on the same node in XAG", "[xag]" )
{
  xag_network xag;
  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();

  const auto f1 = xag.create_and( x1, x2 );
  const auto f2 = xag.create_or( x1, x2 );
  (void)f2;

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 2u );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 2u );

  /* delete node */
  CHECK( !xag.is_dead( xag.get_node( f1 ) ) );
  xag.take_out_node( xag.get_node( f1 ) );
  CHECK( xag.is_dead( xag.get_node( f1 ) ) );
  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 1u );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 1u );

  /* ensure that double-deletion has no effect on the fanout-size of x1 and x2 */
  CHECK( xag.is_dead( xag.get_node( f1 ) ) );
  xag.take_out_node( xag.get_node( f1 ) );
  CHECK( xag.is_dead( xag.get_node( f1 ) ) );
  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 1u );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 1u );
}

TEST_CASE( "substitute node and restrash in XAG", "[xag]" )
{
  xag_network xag;
  auto const x1 = xag.create_pi();
  auto const x2 = xag.create_pi();

  auto const f1 = xag.create_and( x1, x2 );
  auto const f2 = xag.create_and( f1, x2 );
  xag.create_po( f2 );

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( xag )[0]._bits == 0x8 );

  /* substitute f1 with x1
   *
   * this is a very interesting test case because replacing f1 with x1
   * in f2 makes f2 and f1 equal.  a correct implementation will
   * create a new entry in the hash, although (x1, x2) is already
   * there, because (x1, x2) will be deleted in the next step.
   */
  xag.substitute_node( xag.get_node( f1 ), x1 );
  CHECK( simulate<kitty::static_truth_table<2u>>( xag )[0]._bits == 0x8 );

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 0 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );
}

TEST_CASE( "trivial case (constant) detection in replace_in_node of xag_network", "[xag]" )
{
  xag_network xag;
  auto const x1 = xag.create_pi();
  auto const x2 = xag.create_pi();

  auto const f1 = xag.create_xor( x1, x2 );
  auto const f2 = xag.create_and( x1, x2 );
  xag.create_po( f1 );
  xag.create_po( f2 );

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );

  xag.substitute_node( xag.get_node( x1 ), xag.get_constant( true ) );

  CHECK( xag.is_dead( xag.get_node( f1 ) ) );
  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 0 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 2 );
  CHECK( xag.get_node( xag.po_at( 0 ) ) == xag.get_node( x2 ) );
  CHECK( xag.get_node( xag.po_at( 1 ) ) == xag.get_node( x2 ) );
}

TEST_CASE( "substitute node with complemented node in xag_network", "[xag]" )
{
  xag_network xag;
  auto const x1 = xag.create_pi();
  auto const x2 = xag.create_pi();

  auto const f1 = xag.create_and( x1, x2 );
  auto const f2 = xag.create_and( x1, f1 );
  xag.create_po( f2 );

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( xag )[0]._bits == 0x8 );

  xag.substitute_node( xag.get_node( f2 ), !f2 );

  CHECK( xag.fanout_size( xag.get_node( x1 ) ) == 2 );
  CHECK( xag.fanout_size( xag.get_node( x2 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f1 ) ) == 1 );
  CHECK( xag.fanout_size( xag.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( xag )[0]._bits == 0x7 );
}

TEST_CASE( "substitute node with dependency in xag_network", "[xag]" )
{
  xag_network xag{};

  auto const a = xag.create_pi();
  auto const b = xag.create_pi();
  auto const c = xag.create_pi();          /* place holder */
  auto const tmp = xag.create_and( b, c ); /* place holder */
  auto const f1 = xag.create_and( a, b );
  auto const f2 = xag.create_and( f1, tmp );
  auto const f3 = xag.create_and( f1, a );
  xag.create_po( f2 );
  xag.substitute_node( xag.get_node( tmp ), f3 );

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

  xag.substitute_node( xag.get_node( f1 ), xag.get_constant( 1 ) /* constant 1 */ );

  CHECK( xag.is_dead( xag.get_node( f1 ) ) );
  CHECK( xag.is_dead( xag.get_node( f2 ) ) );
  CHECK( xag.is_dead( xag.get_node( f3 ) ) );
  xag.foreach_po( [&]( auto s ) {
    CHECK( xag.is_dead( xag.get_node( s ) ) == false );
  } );
}
