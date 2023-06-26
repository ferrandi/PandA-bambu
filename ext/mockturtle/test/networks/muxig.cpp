#include <catch.hpp>

#include <unordered_map>
#include <unordered_set>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/muxig.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( muxig.size() == 1 );
  CHECK( has_get_constant_v<muxig_network> );
  CHECK( has_is_constant_v<muxig_network> );
  CHECK( has_get_node_v<muxig_network> );
  CHECK( has_is_complemented_v<muxig_network> );

  const auto c0 = muxig.get_constant( false );
  CHECK( muxig.is_constant( muxig.get_node( c0 ) ) );
  CHECK( !muxig.is_pi( muxig.get_node( c0 ) ) );

  CHECK( muxig.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, muxig_network::signal> );
  CHECK( muxig.get_node( c0 ) == 0 );
  CHECK( !muxig.is_complemented( c0 ) );

  const auto c1 = muxig.get_constant( true );

  CHECK( muxig.get_node( c1 ) == 0 );
  CHECK( muxig.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "create and use primary inputs in an MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_create_pi_v<muxig_network> );

  auto a = muxig.create_pi();
  auto b = muxig.create_pi();

  CHECK( muxig.size() == 3 ); // constant + two primary inputs
  CHECK( muxig.num_pis() == 2 );
  CHECK( muxig.num_gates() == 0 );
  CHECK( muxig.is_pi( muxig.get_node( a ) ) );
  CHECK( muxig.is_pi( muxig.get_node( b ) ) );
  CHECK( muxig.pi_index( muxig.get_node( a ) ) == 0 );
  CHECK( muxig.pi_index( muxig.get_node( b ) ) == 1 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, muxig_network::signal> );

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

TEST_CASE( "create and use primary outputs in an MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_create_po_v<muxig_network> );

  const auto c0 = muxig.get_constant( false );
  const auto x1 = muxig.create_pi();

  CHECK( muxig.size() == 2 );
  CHECK( muxig.num_pis() == 1 );
  CHECK( muxig.num_pos() == 0 );

  muxig.create_po( c0 );
  muxig.create_po( x1 );
  muxig.create_po( !x1 );

  CHECK( muxig.size() == 2 );
  CHECK( muxig.num_pos() == 3 );

  muxig.foreach_po( [&]( auto s, auto i ) {
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

TEST_CASE( "create unary operations in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_create_buf_v<muxig_network> );
  CHECK( has_create_not_v<muxig_network> );

  auto x1 = muxig.create_pi();

  CHECK( muxig.size() == 2 );

  auto f1 = muxig.create_buf( x1 );
  auto f2 = muxig.create_not( x1 );

  CHECK( muxig.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary and ternary operations in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_create_and_v<muxig_network> );
  CHECK( has_create_nand_v<muxig_network> );
  CHECK( has_create_or_v<muxig_network> );
  CHECK( has_create_nor_v<muxig_network> );
  CHECK( has_create_xor_v<muxig_network> );
  CHECK( has_create_maj_v<muxig_network> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();

  CHECK( muxig.size() == 3 );

  const auto f1 = muxig.create_and( x1, x2 );
  CHECK( muxig.size() == 4 );
  CHECK( muxig.num_gates() == 1 );

  const auto f2 = muxig.create_nand( x1, x2 );
  CHECK( muxig.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = muxig.create_or( x1, x2 );
  CHECK( muxig.size() == 5 );

  const auto f4 = muxig.create_nor( x1, x2 );
  CHECK( muxig.size() == 5 );
  CHECK( f3 == !f4 );

  muxig.create_xor( x1, x2 );
  CHECK( muxig.size() == 6 );

  muxig.create_maj( x1, x2, f1 );
  CHECK( muxig.size() == 7 );

  const auto f6 = muxig.create_gate( x1, x2, muxig.get_constant( false ) );
  CHECK( muxig.size() == 7 );
  CHECK( f1 == f6 );

  muxig.create_gate( x1, x2, muxig.get_constant( true ) );
  CHECK( muxig.size() == 8 );
}

TEST_CASE( "hash nodes in MUXIG network", "[muxig]" )
{
  muxig_network muxig;

  auto a = muxig.create_pi();
  auto b = muxig.create_pi();
  auto c = muxig.create_pi();

  auto f = muxig.create_gate( a, b, c );
  auto g = muxig.create_gate( a, b, c );

  CHECK( muxig.size() == 5u );
  CHECK( muxig.num_gates() == 1u );

  CHECK( muxig.get_node( f ) == muxig.get_node( g ) );

  auto f1 = muxig.create_gate( a, !b, c );
  auto g1 = muxig.create_gate( a, !b, c );

  CHECK( muxig.size() == 6u );
  CHECK( muxig.num_gates() == 2u );

  CHECK( muxig.get_node( f1 ) == muxig.get_node( g1 ) );
}

TEST_CASE( "normalizations in creating gates in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  const auto x0 = muxig.get_constant( false );
  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();

  CHECK( x0.index == 0 );
  CHECK( x0.complement == 0 );
  CHECK( x1.index == 1 );
  CHECK( x1.complement == 0 );
  CHECK( x2.index == 2 );
  CHECK( x2.complement == 0 );
  CHECK( x3.index == 3 );
  CHECK( x3.complement == 0 );

  std::vector<uint8_t> base_tts = { 0, 0xaa, 0xcc, 0xf0 };
  auto compute_mux = []( auto a, auto b, auto c ) { return ( ( a & b ) | ( ( ~a ) & c ) ); };

  std::unordered_map<uint32_t, std::vector<std::vector<muxig_signal>>> ttmap;
  for ( auto i = 0u; i < 8; i++ )
  {
    for ( auto j = 0u; j < 8; j++ )
    {
      for ( auto k = 0u; k < 8; k++ )
      {
        auto ind_i = i / 2;
        auto inv_i = i & 1;

        auto ind_j = j / 2;
        auto inv_j = j & 1;

        auto ind_k = k / 2;
        auto inv_k = k & 1;

        auto tti = 0xff & ( inv_i ? ~base_tts[ind_i] : base_tts[ind_i] );
        auto ttj = 0xff & ( inv_j ? ~base_tts[ind_j] : base_tts[ind_j] );
        auto ttk = 0xff & ( inv_k ? ~base_tts[ind_k] : base_tts[ind_k] );

        auto tto = compute_mux( tti, ttj, ttk );
        if ( tto & 1 )
          tto = 0xff & ( ~tto );

        ttmap[tto].push_back( { { ind_i, inv_i }, { ind_j, inv_j }, { ind_k, inv_k } } );
      }
    }
  }

  for ( auto& [normalized_tt, fanin_combinations] : ttmap )
  {
    const auto size_before = muxig.size();
    if ( normalized_tt == 0u || normalized_tt == 0xaa || normalized_tt == 0xcc || normalized_tt == 0xf0 )
    {
      for ( auto& fanins : fanin_combinations )
      {
        auto some_gate = muxig.create_gate( fanins[0], fanins[1], fanins[2] );
        CHECK( size_before == muxig.size() );
        CHECK( normalized_tt == base_tts[some_gate.index] );
      }
    }
    else
    {
      const auto expected_size_after = size_before + 1;
      std::unordered_set<muxig_network::node> new_gate_ids;
      for ( auto& fanins : fanin_combinations )
      {
        new_gate_ids.insert( muxig.get_node( muxig.create_gate( fanins[0], fanins[1], fanins[2] ) ) );
        CHECK( expected_size_after == muxig.size() );
      }
      CHECK( 1 == new_gate_ids.size() );
    }
  }
}

TEST_CASE( "clone a MUXIG network", "[muxig]" )
{
  CHECK( has_clone_v<muxig_network> );

  muxig_network muxig0;
  auto a = muxig0.create_pi();
  auto b = muxig0.create_pi();
  auto c = muxig0.create_pi();
  auto f0 = muxig0.create_gate( a, b, c );
  CHECK( muxig0.size() == 5 );
  CHECK( muxig0.num_gates() == 1 );

  auto muxig1 = muxig0;
  auto muxig_clone = muxig0.clone();

  auto d = muxig0.create_pi();
  auto e = muxig0.create_pi();
  muxig1.create_gate( f0, d, e );
  CHECK( muxig0.size() == 8 );
  CHECK( muxig0.num_gates() == 2 );

  CHECK( muxig_clone.size() == 5 );
  CHECK( muxig_clone.num_gates() == 1 );
}

TEST_CASE( "clone a node in a MUXIG network", "[muxig]" )
{
  muxig_network muxig1, muxig2;

  CHECK( has_clone_node_v<muxig_network> );

  auto a1 = muxig1.create_pi();
  auto b1 = muxig1.create_pi();
  auto c1 = muxig1.create_pi();
  auto f1 = muxig1.create_gate( a1, b1, c1 );
  CHECK( muxig1.size() == 5 );

  auto a2 = muxig2.create_pi();
  auto b2 = muxig2.create_pi();
  auto c2 = muxig2.create_pi();
  CHECK( muxig2.size() == 4 );

  auto f2 = muxig2.clone_node( muxig1, muxig1.get_node( f1 ), { a2, b2, c2 } );
  CHECK( muxig2.size() == 5 );

  muxig2.foreach_fanin( muxig2.get_node( f2 ), [&]( auto const& s, auto ) {
    CHECK( !muxig2.is_complemented( s ) );
  } );
}

TEST_CASE( "structural properties of a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_size_v<muxig_network> );
  CHECK( has_num_pis_v<muxig_network> );
  CHECK( has_num_pos_v<muxig_network> );
  CHECK( has_num_gates_v<muxig_network> );
  CHECK( has_fanin_size_v<muxig_network> );
  CHECK( has_fanout_size_v<muxig_network> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();

  const auto f1 = muxig.create_gate( x1, x2, x3 );
  const auto f2 = muxig.create_gate( x1, x2, !x3 );

  muxig.create_po( f1 );
  muxig.create_po( f2 );

  CHECK( muxig.size() == 6 );
  CHECK( muxig.num_pis() == 3 );
  CHECK( muxig.num_pos() == 2 );
  CHECK( muxig.num_gates() == 2 );
  CHECK( muxig.fanin_size( muxig.get_node( x1 ) ) == 0 );
  CHECK( muxig.fanin_size( muxig.get_node( x2 ) ) == 0 );
  CHECK( muxig.fanin_size( muxig.get_node( x3 ) ) == 0 );
  CHECK( muxig.fanin_size( muxig.get_node( f1 ) ) == 3 );
  CHECK( muxig.fanin_size( muxig.get_node( f2 ) ) == 3 );
  CHECK( muxig.fanout_size( muxig.get_node( x1 ) ) == 2 );
  CHECK( muxig.fanout_size( muxig.get_node( x2 ) ) == 2 );
  CHECK( muxig.fanout_size( muxig.get_node( f1 ) ) == 1 );
  CHECK( muxig.fanout_size( muxig.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_foreach_node_v<muxig_network> );
  CHECK( has_foreach_pi_v<muxig_network> );
  CHECK( has_foreach_po_v<muxig_network> );
  CHECK( has_foreach_gate_v<muxig_network> );
  CHECK( has_foreach_fanin_v<muxig_network> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();
  const auto f1 = muxig.create_gate( x1, x2, x3 );
  const auto f2 = muxig.create_gate( x1, x2, !x3 );
  muxig.create_po( f1 );
  muxig.create_po( f2 );

  CHECK( muxig.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  muxig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  muxig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  muxig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  muxig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  muxig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  muxig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  muxig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  muxig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  muxig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << muxig.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  muxig.foreach_po( [&]( auto s ) { mask |= ( 1 << muxig.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  muxig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << muxig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  muxig.foreach_po( [&]( auto s ) { mask |= ( 1 << muxig.get_node( s ) ); return false; } );
  CHECK( mask == 16 );

  /* iterate over gates */
  mask = counter = 0;
  muxig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  muxig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  muxig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  muxig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 16 );

  /* iterate over fanins */
  mask = counter = 0;
  muxig.foreach_fanin( muxig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << muxig.get_node( s ) ); counter += i; } );
  CHECK( mask == 14 );
  CHECK( counter == 3 );

  mask = 0;
  muxig.foreach_fanin( muxig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << muxig.get_node( s ) ); } );
  CHECK( mask == 14 );

  mask = counter = 0;
  muxig.foreach_fanin( muxig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << muxig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  muxig.foreach_fanin( muxig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << muxig.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}

TEST_CASE( "compute values in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_compute_v<muxig_network, bool> );
  CHECK( has_compute_v<muxig_network, kitty::dynamic_truth_table> );
  CHECK( has_compute_v<muxig_network, kitty::partial_truth_table> );
  CHECK( has_compute_inplace_v<muxig_network, kitty::partial_truth_table> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();
  const auto f1 = muxig.create_gate( !x1, x2, x3 );
  const auto f2 = muxig.create_gate( x1, x2, x3 );
  muxig.create_po( f1 );
  muxig.create_po( f2 );

  {
    std::vector<bool> values{ { true, false, true } };

    CHECK( muxig.compute( muxig.get_node( f1 ), values.begin(), values.end() ) == true );
    CHECK( muxig.compute( muxig.get_node( f2 ), values.begin(), values.end() ) == false );
  }

  {
    std::vector<kitty::dynamic_truth_table> xs{ 3, kitty::dynamic_truth_table( 3 ) };
    kitty::create_nth_var( xs[0], 0 );
    kitty::create_nth_var( xs[1], 1 );
    kitty::create_nth_var( xs[2], 2 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    CHECK( muxig.compute( muxig.get_node( f1 ), xs.begin(), xs.end() ) == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    CHECK( muxig.compute( muxig.get_node( f2 ), xs.begin(), xs.end() ) == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );
  }

  {
    std::vector<kitty::partial_truth_table> xs{ 3 };
    kitty::partial_truth_table result;

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 0 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 0 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 0 );
    xs[2].add_bit( 1 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 0 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );

    xs[0].add_bit( 1 );
    xs[1].add_bit( 1 );
    xs[2].add_bit( 1 );

    muxig.compute( muxig.get_node( f1 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( ~xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ) );
    muxig.compute( muxig.get_node( f2 ), result, xs.begin(), xs.end() );
    CHECK( result == ( ( xs[0] & xs[1] ) | ( ~xs[0] & xs[2] ) ) );
  }
}

TEST_CASE( "custom node values in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_clear_values_v<muxig_network> );
  CHECK( has_value_v<muxig_network> );
  CHECK( has_set_value_v<muxig_network> );
  CHECK( has_incr_value_v<muxig_network> );
  CHECK( has_decr_value_v<muxig_network> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();
  const auto f1 = muxig.create_gate( x1, x2, x3 );
  const auto f2 = muxig.create_gate( !x1, x2, x3 );
  muxig.create_po( f1 );
  muxig.create_po( f2 );

  CHECK( muxig.size() == 6 );

  muxig.clear_values();
  muxig.foreach_node( [&]( auto n ) {
    CHECK( muxig.value( n ) == 0 );
    muxig.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( muxig.value( n ) == n );
    CHECK( muxig.incr_value( n ) == n );
    CHECK( muxig.value( n ) == n + 1 );
    CHECK( muxig.decr_value( n ) == n );
    CHECK( muxig.value( n ) == n );
  } );
  muxig.clear_values();
  muxig.foreach_node( [&]( auto n ) {
    CHECK( muxig.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in a MUXIG", "[muxig]" )
{
  muxig_network muxig;

  CHECK( has_clear_visited_v<muxig_network> );
  CHECK( has_visited_v<muxig_network> );
  CHECK( has_set_visited_v<muxig_network> );

  const auto x1 = muxig.create_pi();
  const auto x2 = muxig.create_pi();
  const auto x3 = muxig.create_pi();
  const auto f1 = muxig.create_gate( x1, x2, x3 );
  const auto f2 = muxig.create_and( x1, x2 );
  muxig.create_po( f1 );
  muxig.create_po( f2 );

  CHECK( muxig.size() == 6 );

  muxig.clear_visited();
  muxig.foreach_node( [&]( auto n ) {
    CHECK( muxig.visited( n ) == 0 );
    muxig.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( muxig.visited( n ) == n );
  } );
  muxig.clear_visited();
  muxig.foreach_node( [&]( auto n ) {
    CHECK( muxig.visited( n ) == 0 );
  } );
}

TEST_CASE( "node substitution in a MUXIG: case 1 -- gate normalized into a signal", "[muxig]" )
{
  muxig_network muxig;
  const auto a = muxig.create_pi();
  const auto b = muxig.create_pi();
  const auto f = muxig.create_and( !a, b );
  muxig.create_po( f );

  CHECK( muxig.size() == 4 );
  CHECK( !muxig.is_complemented( muxig.po_at( 0 ) ) );

  muxig.foreach_fanin( muxig.get_node( f ), [&]( auto const& s ) {
    CHECK( !muxig.is_complemented( s ) );
  } );

  muxig.substitute_node( muxig.get_node( a ), b );

  CHECK( muxig.num_gates() == 0 );
  CHECK( muxig.po_at( 0 ) == muxig.get_constant( false ) );
}

TEST_CASE( "node substitution in a MUXIG: case 2 -- general case, no normalization", "[muxig]" )
{
  muxig_network muxig;
  const auto a = muxig.create_pi();
  const auto b = muxig.create_pi();
  const auto c = muxig.create_pi();
  const auto f = muxig.create_and( !a, b );
  muxig.create_po( f );

  muxig.foreach_fanin( muxig.get_node( f ), [&]( auto const& s, auto i ) {
    CHECK( !muxig.is_complemented( s ) );
    switch ( i )
    {
    case 0:
      CHECK( muxig.get_node( s ) == muxig.get_node( a ) );
      break;
    case 1:
      CHECK( muxig.get_node( s ) == muxig.get_node( muxig.get_constant( false ) ) );
      break;
    case 2:
      CHECK( muxig.get_node( s ) == muxig.get_node( b ) );
      break;
    default:
      break;
    }
  } );

  muxig.substitute_node( muxig.get_node( a ), c );

  muxig.foreach_fanin( muxig.get_node( f ), [&]( auto const& s, auto i ) {
    CHECK( !muxig.is_complemented( s ) );
    switch ( i )
    {
    case 0:
      CHECK( muxig.get_node( s ) == muxig.get_node( c ) );
      break;
    case 1:
      CHECK( muxig.get_node( s ) == muxig.get_node( muxig.get_constant( false ) ) );
      break;
    case 2:
      CHECK( muxig.get_node( s ) == muxig.get_node( b ) );
      break;
    default:
      break;
    }
  } );
}

TEST_CASE( "substitute node with complemented node in a muxig_network", "[muxig]" )
{
  muxig_network muxig;
  auto const x1 = muxig.create_pi();
  auto const x2 = muxig.create_pi();

  auto const f1 = muxig.create_and( x1, x2 );
  auto const f2 = muxig.create_and( x1, f1 );
  muxig.create_po( f2 );

  CHECK( muxig.fanout_size( muxig.get_node( x1 ) ) == 2 );
  CHECK( muxig.fanout_size( muxig.get_node( x2 ) ) == 1 );
  CHECK( muxig.fanout_size( muxig.get_node( f1 ) ) == 1 );
  CHECK( muxig.fanout_size( muxig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( muxig )[0]._bits == 0x8 );

  muxig.substitute_node( muxig.get_node( f2 ), !f2 );

  CHECK( muxig.fanout_size( muxig.get_node( x1 ) ) == 2 );
  CHECK( muxig.fanout_size( muxig.get_node( x2 ) ) == 1 );
  CHECK( muxig.fanout_size( muxig.get_node( f1 ) ) == 1 );
  CHECK( muxig.fanout_size( muxig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( muxig )[0]._bits == 0x7 );
}

TEST_CASE( "substitute node with dependency in a muxig_network", "[muxig]" )
{
  muxig_network muxig{};

  auto const a = muxig.create_pi();
  auto const b = muxig.create_pi();
  auto const c = muxig.create_pi();          /* place holder */
  auto const tmp = muxig.create_and( b, c ); /* place holder */
  auto const f1 = muxig.create_and( a, b );
  auto const f2 = muxig.create_and( f1, tmp );
  auto const f3 = muxig.create_and( f1, a );
  muxig.create_po( f2 );
  muxig.substitute_node( muxig.get_node( tmp ), f3 );

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

  muxig.substitute_node( muxig.get_node( f1 ), muxig.get_constant( 1 ) /* constant 1 */ );

  CHECK( muxig.is_dead( muxig.get_node( f1 ) ) );
  CHECK( muxig.is_dead( muxig.get_node( f2 ) ) );
  CHECK( muxig.is_dead( muxig.get_node( f3 ) ) );
  muxig.foreach_po( [&]( auto s ) {
    CHECK( muxig.is_dead( muxig.get_node( s ) ) == false );
  } );
}
