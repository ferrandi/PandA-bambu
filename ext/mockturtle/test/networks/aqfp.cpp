#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/networks/aqfp.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( aqfp.size() == 1 );
  CHECK( has_get_constant_v<aqfp_network> );
  CHECK( has_is_constant_v<aqfp_network> );
  CHECK( has_get_node_v<aqfp_network> );
  CHECK( has_is_complemented_v<aqfp_network> );

  const auto c0 = aqfp.get_constant( false );
  CHECK( aqfp.is_constant( aqfp.get_node( c0 ) ) );
  CHECK( !aqfp.is_pi( aqfp.get_node( c0 ) ) );

  CHECK( aqfp.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, aqfp_network::signal> );
  CHECK( aqfp.get_node( c0 ) == 0 );
  CHECK( !aqfp.is_complemented( c0 ) );

  const auto c1 = aqfp.get_constant( true );

  CHECK( aqfp.get_node( c1 ) == 0 );
  CHECK( aqfp.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "create and use primary inputs in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_create_pi_v<aqfp_network> );

  auto a = aqfp.create_pi();

  CHECK( aqfp.size() == 2 );
  CHECK( aqfp.num_pis() == 1 );
  CHECK( aqfp.num_gates() == 0 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, aqfp_network::signal> );

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

TEST_CASE( "create and use primary outputs in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_create_po_v<aqfp_network> );

  const auto c0 = aqfp.get_constant( false );
  const auto x1 = aqfp.create_pi();

  CHECK( aqfp.size() == 2 );
  CHECK( aqfp.num_pis() == 1 );
  CHECK( aqfp.num_pos() == 0 );

  aqfp.create_po( c0 );
  aqfp.create_po( x1 );
  aqfp.create_po( !x1 );

  CHECK( aqfp.size() == 2 );
  CHECK( aqfp.num_pos() == 3 );

  aqfp.foreach_po( [&]( auto s, auto i ) {
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

TEST_CASE( "create unary operations in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_create_buf_v<aqfp_network> );
  CHECK( has_create_not_v<aqfp_network> );

  auto x1 = aqfp.create_pi();

  CHECK( aqfp.size() == 2 );

  auto f1 = aqfp.create_buf( x1 );
  auto f2 = aqfp.create_not( x1 );
  (void)f1;
  (void)f2;

  CHECK( aqfp.size() == 2 );
}

TEST_CASE( "create binary and ternary operations in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_create_and_v<aqfp_network> );
  CHECK( has_create_nand_v<aqfp_network> );
  CHECK( has_create_or_v<aqfp_network> );
  CHECK( has_create_nor_v<aqfp_network> );
  CHECK( has_create_xor_v<aqfp_network> );
  CHECK( has_create_maj_v<aqfp_network> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();

  CHECK( aqfp.size() == 3 );

  const auto f1 = aqfp.create_and( x1, x2 );
  CHECK( aqfp.size() == 4 );
  CHECK( aqfp.num_gates() == 1 );
  (void)f1;

  const auto f2 = aqfp.create_nand( x1, x2 );
  CHECK( aqfp.size() == 5 );
  (void)f2;

  const auto f3 = aqfp.create_or( x1, x2 );
  CHECK( aqfp.size() == 6 );
  (void)f3;

  const auto f4 = aqfp.create_nor( x1, x2 );
  CHECK( aqfp.size() == 7 );
  (void)f4;

  aqfp.create_xor( x1, x2 );
  CHECK( aqfp.size() == 10 );

  aqfp.create_maj( x1, x2, f1 );
  CHECK( aqfp.size() == 11 );

  const auto f6 = aqfp.create_maj( x1, x2, aqfp.get_constant( false ) );
  CHECK( aqfp.size() == 12 );
  (void)f6;

  const auto f7 = aqfp.create_maj( x1, x2, aqfp.get_constant( true ) );
  CHECK( aqfp.size() == 13 );
  (void)f7;

  const auto x3 = aqfp.create_pi();

  const auto f8 = aqfp.create_maj( x1, x2, x3 );
  const auto f9 = aqfp.create_maj( !x1, !x2, !x3 );
  (void)f8;
  (void)f9;

  CHECK( aqfp.size() == 16 );
}

TEST_CASE( "hash nodes in AQFP network", "[aqfp]" )
{
  aqfp_network aqfp;

  auto a = aqfp.create_pi();
  auto b = aqfp.create_pi();
  auto c = aqfp.create_pi();

  auto f = aqfp.create_maj( a, b, c );
  auto g = aqfp.create_maj( a, b, c );

  CHECK( aqfp.size() == 6u );
  CHECK( aqfp.num_gates() == 2u );

  CHECK( aqfp.get_node( f ) != aqfp.get_node( g ) );

  auto f1 = aqfp.create_maj( a, !b, c );
  auto g1 = aqfp.create_maj( a, !b, c );

  CHECK( aqfp.size() == 8u );
  CHECK( aqfp.num_gates() == 4u );

  CHECK( aqfp.get_node( f1 ) != aqfp.get_node( g1 ) );
}

TEST_CASE( "clone a node in AQFP network", "[aqfp]" )
{
  aqfp_network aqfp1, aqfp2;

  CHECK( has_clone_node_v<aqfp_network> );

  auto a1 = aqfp1.create_pi();
  auto b1 = aqfp1.create_pi();
  auto c1 = aqfp1.create_pi();
  auto f1 = aqfp1.create_maj( a1, b1, c1 );
  CHECK( aqfp1.size() == 5 );

  auto a2 = aqfp2.create_pi();
  auto b2 = aqfp2.create_pi();
  auto c2 = aqfp2.create_pi();
  CHECK( aqfp2.size() == 4 );

  auto f2 = aqfp2.clone_node( aqfp1, aqfp1.get_node( f1 ), { a2, b2, c2 } );
  CHECK( aqfp2.size() == 5 );

  aqfp2.foreach_fanin( aqfp2.get_node( f2 ), [&]( auto const& s, auto ) {
    CHECK( !aqfp2.is_complemented( s ) );
  } );
}

TEST_CASE( "structural properties of an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_size_v<aqfp_network> );
  CHECK( has_num_pis_v<aqfp_network> );
  CHECK( has_num_pos_v<aqfp_network> );
  CHECK( has_num_gates_v<aqfp_network> );
  CHECK( has_fanin_size_v<aqfp_network> );
  CHECK( has_fanout_size_v<aqfp_network> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();

  const auto f1 = aqfp.create_maj( x1, x2, x3 );
  const auto f2 = aqfp.create_maj( x1, x2, !x3 );

  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  CHECK( aqfp.size() == 6 );
  CHECK( aqfp.num_pis() == 3 );
  CHECK( aqfp.num_pos() == 2 );
  CHECK( aqfp.num_gates() == 2 );
  CHECK( aqfp.fanin_size( aqfp.get_node( x1 ) ) == 0 );
  CHECK( aqfp.fanin_size( aqfp.get_node( x2 ) ) == 0 );
  CHECK( aqfp.fanin_size( aqfp.get_node( x3 ) ) == 0 );
  CHECK( aqfp.fanin_size( aqfp.get_node( f1 ) ) == 3 );
  CHECK( aqfp.fanin_size( aqfp.get_node( f2 ) ) == 3 );
  CHECK( aqfp.fanout_size( aqfp.get_node( x1 ) ) == 2 );
  CHECK( aqfp.fanout_size( aqfp.get_node( x2 ) ) == 2 );
  CHECK( aqfp.fanout_size( aqfp.get_node( f1 ) ) == 1 );
  CHECK( aqfp.fanout_size( aqfp.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in an AQFP", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_foreach_node_v<aqfp_network> );
  CHECK( has_foreach_pi_v<aqfp_network> );
  CHECK( has_foreach_po_v<aqfp_network> );
  CHECK( has_foreach_gate_v<aqfp_network> );
  CHECK( has_foreach_fanin_v<aqfp_network> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();
  const auto f1 = aqfp.create_maj( x1, x2, x3 );
  const auto f2 = aqfp.create_maj( x1, x2, !x3 );
  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  CHECK( aqfp.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  aqfp.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  aqfp.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  aqfp.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  aqfp.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  aqfp.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  aqfp.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  aqfp.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aqfp.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  aqfp.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aqfp.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  aqfp.foreach_po( [&]( auto s ) { mask |= ( 1 << aqfp.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  aqfp.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aqfp.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  aqfp.foreach_po( [&]( auto s ) { mask |= ( 1 << aqfp.get_node( s ) ); return false; } );
  CHECK( mask == 16 );

  /* iterate over gates */
  mask = counter = 0;
  aqfp.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  aqfp.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  aqfp.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  aqfp.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 16 );

  /* iterate over fanins */
  mask = counter = 0;
  aqfp.foreach_fanin( aqfp.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aqfp.get_node( s ) ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  aqfp.foreach_fanin( aqfp.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aqfp.get_node( s ) ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  aqfp.foreach_fanin( aqfp.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aqfp.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aqfp.foreach_fanin( aqfp.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aqfp.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in AQFPs", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_compute_v<aqfp_network, bool> );
  CHECK( has_compute_v<aqfp_network, kitty::dynamic_truth_table> );
  CHECK( has_compute_v<aqfp_network, kitty::partial_truth_table> );
  CHECK( has_compute_inplace_v<aqfp_network, kitty::partial_truth_table> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();
  const auto f1 = aqfp.create_maj( !x1, x2, x3 );
  const auto f2 = aqfp.create_maj( x1, !x2, x3 );
  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  {
    std::vector<bool> values{ { true, false, true } };

    CHECK( aqfp.compute( aqfp.get_node( f1 ), values.begin(), values.end() ) == false );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), values.begin(), values.end() ) == true );
  }

  {
    std::vector<kitty::dynamic_truth_table> xs{ 3, kitty::dynamic_truth_table( 3 ) };
    kitty::create_nth_var( xs[0], 0 );
    kitty::create_nth_var( xs[1], 1 );
    kitty::create_nth_var( xs[2], 2 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( aqfp.compute( aqfp.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( aqfp.compute( aqfp.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };
    kitty::partial_truth_table result;

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    aqfp.compute( aqfp.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    aqfp.compute( aqfp.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }
}

TEST_CASE( "custom node values in AQFPs", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_clear_values_v<aqfp_network> );
  CHECK( has_value_v<aqfp_network> );
  CHECK( has_set_value_v<aqfp_network> );
  CHECK( has_incr_value_v<aqfp_network> );
  CHECK( has_decr_value_v<aqfp_network> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();
  const auto f1 = aqfp.create_maj( x1, x2, x3 );
  const auto f2 = aqfp.create_maj( !x1, x2, x3 );
  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  CHECK( aqfp.size() == 6 );

  aqfp.clear_values();
  aqfp.foreach_node( [&]( auto n ) {
    CHECK( aqfp.value( n ) == 0 );
    aqfp.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( aqfp.value( n ) == n );
    CHECK( aqfp.incr_value( n ) == n );
    CHECK( aqfp.value( n ) == n + 1 );
    CHECK( aqfp.decr_value( n ) == n );
    CHECK( aqfp.value( n ) == n );
  } );
  aqfp.clear_values();
  aqfp.foreach_node( [&]( auto n ) {
    CHECK( aqfp.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in AQFPs", "[aqfp]" )
{
  aqfp_network aqfp;

  CHECK( has_clear_visited_v<aqfp_network> );
  CHECK( has_visited_v<aqfp_network> );
  CHECK( has_set_visited_v<aqfp_network> );

  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();
  const auto f1 = aqfp.create_maj( x1, x2, x3 );
  const auto f2 = aqfp.create_and( x1, x2 );
  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  CHECK( aqfp.size() == 6 );

  aqfp.clear_visited();
  aqfp.foreach_node( [&]( auto n ) {
    CHECK( aqfp.visited( n ) == 0 );
    aqfp.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( aqfp.visited( n ) == n );
  } );
  aqfp.clear_visited();
  aqfp.foreach_node( [&]( auto n ) {
    CHECK( aqfp.visited( n ) == 0 );
  } );
}

TEST_CASE( "node substitution in AQFPs", "[aqfp]" )
{
  aqfp_network aqfp;
  const auto a = aqfp.create_pi();
  const auto b = aqfp.create_pi();
  const auto f = aqfp.create_and( a, b );

  CHECK( aqfp.size() == 4 );

  aqfp.foreach_fanin( aqfp.get_node( f ), [&]( auto const& s ) {
    CHECK( !aqfp.is_complemented( s ) );
  } );

  aqfp.substitute_node( aqfp.get_node( aqfp.get_constant( false ) ), aqfp.get_constant( true ) );

  CHECK( aqfp.size() == 4 );

  aqfp.foreach_fanin( aqfp.get_node( f ), [&]( auto const& s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( aqfp.is_complemented( s ) );
      break;
    default:
      CHECK( !aqfp.is_complemented( s ) );
      break;
    }
  } );
}
