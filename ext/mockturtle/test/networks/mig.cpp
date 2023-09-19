#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
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
  auto b = mig.create_pi();

  CHECK( mig.size() == 3 ); // constant + two primary inputs
  CHECK( mig.num_pis() == 2 );
  CHECK( mig.num_gates() == 0 );
  CHECK( mig.is_pi( mig.get_node( a ) ) );
  CHECK( mig.is_pi( mig.get_node( b ) ) );
  CHECK( mig.pi_index( mig.get_node( a ) ) == 0 );
  CHECK( mig.pi_index( mig.get_node( b ) ) == 1 );

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

TEST_CASE( "clone a MIG network", "[mig]" )
{
  CHECK( has_clone_v<mig_network> );

  mig_network mig0;
  auto a = mig0.create_pi();
  auto b = mig0.create_pi();
  auto c = mig0.create_pi();
  auto f0 = mig0.create_maj( a, b, c );
  CHECK( mig0.size() == 5 );
  CHECK( mig0.num_gates() == 1 );

  auto mig1 = mig0;
  auto mig_clone = mig0.clone();

  auto d = mig0.create_pi();
  auto e = mig0.create_pi();
  mig1.create_maj( f0, d, e );
  CHECK( mig0.size() == 8 );
  CHECK( mig0.num_gates() == 2 );

  CHECK( mig_clone.size() == 5 );
  CHECK( mig_clone.num_gates() == 1 );
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

  auto f2 = mig2.clone_node( mig1, mig1.get_node( f1 ), { a2, b2, c2 } );
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

TEST_CASE( "check has_maj in MIG", "[mig]" )
{
  mig_network mig;

  auto a = mig.create_pi();
  auto b = mig.create_pi();
  auto c = mig.create_pi();
  auto d = mig.create_pi();

  auto f = mig.create_maj( a, b, c );
  auto g = mig.create_maj( a, c, d );

  CHECK( mig.has_maj( a, b, c ).has_value() == true );
  CHECK( *mig.has_maj( a, b, c ) == f );
  CHECK( mig.has_maj( a, b, d ).has_value() == false );
  CHECK( mig.has_maj( !a, !c, !d ).has_value() == true );
  CHECK( *mig.has_maj( !a, !c, !d ) == !g );
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
  uint32_t mask{ 0 }, counter{ 0 };
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
  CHECK( has_compute_v<mig_network, kitty::partial_truth_table> );
  CHECK( has_compute_inplace_v<mig_network, kitty::partial_truth_table> );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto f1 = mig.create_maj( !x1, x2, x3 );
  const auto f2 = mig.create_maj( x1, !x2, x3 );
  mig.create_po( f1 );
  mig.create_po( f2 );

  {
    std::vector<bool> values{ { true, false, true } };

    CHECK( mig.compute( mig.get_node( f1 ), values.begin(), values.end() ) == false );
    CHECK( mig.compute( mig.get_node( f2 ), values.begin(), values.end() ) == true );
  }

  {
    std::vector<kitty::dynamic_truth_table> xs{ 3, kitty::dynamic_truth_table( 3 ) };
    kitty::create_nth_var( xs[0], 0 );
    kitty::create_nth_var( xs[1], 1 );
    kitty::create_nth_var( xs[2], 2 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( mig.compute( mig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    CHECK( mig.compute( mig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };
    kitty::partial_truth_table result;

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    mig.compute( mig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) | ( xs[2] & xs[1] ) ) );
    mig.compute( mig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & ~xs[1] ) | ( xs[0] & xs[2] ) | ( xs[2] & ~xs[1] ) ) );
  }
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

TEST_CASE( "invoke take_out_node two times on the same node in MIG", "[mig]" )
{
  mig_network mig;
  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();

  const auto f1 = mig.create_and( x1, x2 );
  const auto f2 = mig.create_or( x1, x2 );
  (void)f2;

  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 2u );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 2u );

  /* delete node */
  CHECK( !mig.is_dead( mig.get_node( f1 ) ) );
  mig.take_out_node( mig.get_node( f1 ) );
  CHECK( mig.is_dead( mig.get_node( f1 ) ) );
  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 1u );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 1u );

  /* ensure that double-deletion has no effect on the fanout-size of x1 and x2 */
  CHECK( mig.is_dead( mig.get_node( f1 ) ) );
  mig.take_out_node( mig.get_node( f1 ) );
  CHECK( mig.is_dead( mig.get_node( f1 ) ) );
  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 1u );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 1u );
}

TEST_CASE( "substitute node and restrash in MIG", "[mig]" )
{
  mig_network mig;
  auto const x1 = mig.create_pi();
  auto const x2 = mig.create_pi();

  auto const f1 = mig.create_and( x1, x2 );
  auto const f2 = mig.create_and( f1, x2 );
  mig.create_po( f2 );

  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 2 );
  CHECK( mig.fanout_size( mig.get_node( f1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( mig )[0]._bits == 0x8 );

  /* substitute f1 with x1
   *
   * this is a very interesting test case because replacing f1 with x1
   * in f2 makes f2 and f1 equal.  a correct implementation will
   * create a new entry in the hash, although (x1, x2) is already
   * there, because (x1, x2) will be deleted in the next step.
   */
  mig.substitute_node( mig.get_node( f1 ), x1 );
  CHECK( simulate<kitty::static_truth_table<2u>>( mig )[0]._bits == 0x8 );

  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f1 ) ) == 0 );
  CHECK( mig.fanout_size( mig.get_node( f2 ) ) == 1 );
}

TEST_CASE( "substitute node with complemented node in mig_network", "[mig]" )
{
  mig_network mig;
  auto const x1 = mig.create_pi();
  auto const x2 = mig.create_pi();

  auto const f1 = mig.create_and( x1, x2 );
  auto const f2 = mig.create_and( x1, f1 );
  mig.create_po( f2 );

  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 2 );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( mig )[0]._bits == 0x8 );

  mig.substitute_node( mig.get_node( f2 ), !f2 );

  CHECK( mig.fanout_size( mig.get_node( x1 ) ) == 2 );
  CHECK( mig.fanout_size( mig.get_node( x2 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f1 ) ) == 1 );
  CHECK( mig.fanout_size( mig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( mig )[0]._bits == 0x7 );
}

TEST_CASE( "substitute node with dependency in mig_network", "[mig]" )
{
  mig_network mig{};

  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();          /* place holder */
  auto const tmp = mig.create_and( b, c ); /* place holder */
  auto const f1 = mig.create_and( a, b );
  auto const f2 = mig.create_and( f1, tmp );
  auto const f3 = mig.create_and( f1, a );
  mig.create_po( f2 );
  mig.substitute_node( mig.get_node( tmp ), f3 );

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

  mig.substitute_node( mig.get_node( f1 ), mig.get_constant( 1 ) /* constant 1 */ );

  CHECK( mig.is_dead( mig.get_node( f1 ) ) );
  CHECK( mig.is_dead( mig.get_node( f2 ) ) );
  CHECK( mig.is_dead( mig.get_node( f3 ) ) );
  mig.foreach_po( [&]( auto s ) {
    CHECK( mig.is_dead( mig.get_node( s ) ) == false );
  } );
}
