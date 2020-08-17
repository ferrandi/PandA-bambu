#include <catch.hpp>

#include <vector>

#include <mockturtle/networks/klut.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_size_v<klut_network> );
  CHECK( has_get_constant_v<klut_network> );
  CHECK( has_is_constant_v<klut_network> );
  CHECK( has_is_pi_v<klut_network> );
  CHECK( has_is_constant_v<klut_network> );
  CHECK( has_get_node_v<klut_network> );
  CHECK( has_is_complemented_v<klut_network> );

  CHECK( klut.size() == 2 );

  auto c0 = klut.get_constant( false );
  auto c1 = klut.get_constant( true );

  CHECK( klut.size() == 2 );
  CHECK( c0 != c1 );
  CHECK( klut.get_node( c0 ) == 0 );
  CHECK( klut.get_node( c1 ) == 1 );
  CHECK( !klut.is_complemented( c0 ) );
  CHECK( !klut.is_complemented( c1 ) );
  CHECK( klut.is_constant( c0 ) );
  CHECK( klut.is_constant( c1 ) );
  CHECK( !klut.is_pi( c0 ) );
  CHECK( !klut.is_pi( c1 ) );
}

TEST_CASE( "create and use primary inputs in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_create_pi_v<klut_network> );
  CHECK( has_is_constant_v<klut_network> );
  CHECK( has_is_pi_v<klut_network> );
  CHECK( has_num_pis_v<klut_network> );

  CHECK( klut.num_pis() == 0 );

  auto x1 = klut.create_pi();
  auto x2 = klut.create_pi();

  CHECK( klut.size() == 4 );
  CHECK( klut.num_pis() == 2 );
  CHECK( x1 != x2 );
}

TEST_CASE( "create and use primary outputs in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_create_po_v<klut_network> );
  CHECK( has_num_pos_v<klut_network> );

  auto c0 = klut.get_constant( false );
  auto c1 = klut.get_constant( true );
  auto x = klut.create_pi();

  klut.create_po( c0 );
  klut.create_po( c1 );
  klut.create_po( x );

  CHECK( klut.size() == 3 );
  CHECK( klut.num_pis() == 1 );
  CHECK( klut.num_pos() == 3 );
}

TEST_CASE( "create and use register in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_foreach_po_v<klut_network> );
  CHECK( has_create_po_v<klut_network> );
  CHECK( has_create_pi_v<klut_network> );
  CHECK( has_create_ro_v<klut_network> );
  CHECK( has_create_ri_v<klut_network> );
  CHECK( has_create_maj_v<klut_network> );

  const auto c0 = klut.get_constant( false );
  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();
  const auto x3 = klut.create_pi();
  const auto x4 = klut.create_pi();

  CHECK( klut.size() == 6 );
  CHECK( klut.num_registers() == 0 );
  CHECK( klut.num_cis() == 4 );
  CHECK( klut.num_cos() == 0 );

  const auto f1 = klut.create_maj( x1, x2, x3 );
  klut.create_po( f1 );

  CHECK( klut.num_pos() == 1 );

  const auto s1 = klut.create_ro(); // ntk. input
  klut.create_po( s1 );             // po

  const auto f2 = klut.create_maj( f1, x4, c0 );
  klut.create_ri( f2 ); // ntk. output

  CHECK( klut.num_registers() == 1 );
  CHECK( klut.num_cis() == 4 + 1 );
  CHECK( klut.num_cos() == 2 + 1 );

  klut.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( klut.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  klut.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( klut.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( node == s1 ); /* first state-bit */
      CHECK( klut.is_ci( node ) );
      CHECK( !klut.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  klut.foreach_po( [&]( auto const& node, auto index ) {
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

  klut.foreach_co( [&]( auto const& node, auto index ) {
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

TEST_CASE( "create unary operations in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_create_buf_v<klut_network> );
  CHECK( has_create_not_v<klut_network> );

  auto x1 = klut.create_pi();

  CHECK( klut.size() == 3 );

  auto f1 = klut.create_buf( x1 );
  auto f2 = klut.create_not( x1 );

  CHECK( klut.size() == 4 );
  CHECK( f1 == x1 );
  CHECK( f2 != x1 );
}

TEST_CASE( "create binary operations in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_create_and_v<klut_network> );

  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();

  CHECK( klut.size() == 4 );

  klut.create_and( x1, x2 );
  CHECK( klut.size() == 5 );

  klut.create_and( x1, x2 );
  CHECK( klut.size() == 5 );

  klut.create_and( x2, x1 );
  CHECK( klut.size() == 6 );
}

TEST_CASE( "clone a node in a k-LUT network", "[klut]" )
{
  klut_network klut1, klut2;

  CHECK( has_clone_node_v<klut_network> );

  auto a1 = klut1.create_pi();
  auto b1 = klut1.create_pi();
  auto f1 = klut1.create_and( a1, b1 );
  CHECK( klut1.size() == 5 );

  auto a2 = klut2.create_pi();
  auto b2 = klut2.create_pi();
  CHECK( klut2.size() == 4 );

  auto f2 = klut2.clone_node( klut1, klut1.get_node( f1 ), {a2, b2} );
  CHECK( klut2.size() == 5 );

  klut2.foreach_fanin( klut2.get_node( f2 ), [&]( auto const& s ) {
    CHECK( !klut2.is_complemented( s ) );
  } );
}

TEST_CASE( "compute functions from AND and NOT gates in k-LUT networks", "[klut]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  const auto f1 = klut.create_not( a );
  const auto f2 = klut.create_and( a, b );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );

  const auto sim_f1 = klut.compute( klut.get_node( f1 ), xs.begin(), xs.begin() + 1 );
  const auto sim_f2 = klut.compute( klut.get_node( f2 ), xs.begin(), xs.end() );

  CHECK( sim_f1 == ~xs[0] );
  CHECK( sim_f2 == ( xs[0] & xs[1] ) );
}

TEST_CASE( "create nodes and compute a function in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_create_node_v<klut_network> );
  CHECK( has_compute_v<klut_network, kitty::dynamic_truth_table> );

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u ), tt_const0( 0u );
  kitty::create_from_hex_string( tt_maj, "e8" );
  kitty::create_from_hex_string( tt_xor, "96" );

  CHECK( klut.size() == 5 );

  const auto _const0 = klut.create_node( {}, tt_const0 );
  const auto _const1 = klut.create_node( {}, ~tt_const0 );
  CHECK( _const0 == klut.get_constant( false ) );
  CHECK( _const1 == klut.get_constant( true ) );

  const auto _maj = klut.create_node( {a, b, c}, tt_maj );
  const auto _xor = klut.create_node( {a, b, c}, tt_xor );

  CHECK( klut.size() == 7 );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );

  const auto sim_maj = klut.compute( klut.get_node( _maj ), xs.begin(), xs.end() );
  const auto sim_xor = klut.compute( klut.get_node( _xor ), xs.begin(), xs.end() );

  CHECK( sim_maj == kitty::ternary_majority( xs[0], xs[1], xs[2] ) );
  CHECK( sim_xor == ( xs[0] ^ xs[1] ^ xs[2] ) );
}

TEST_CASE( "hash nodes in K-LUT network", "[klut]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u );
  kitty::create_from_hex_string( tt_maj, "e8" );
  kitty::create_from_hex_string( tt_xor, "96" );

  klut.create_node( {a, b, c}, tt_maj );
  klut.create_node( {a, b, c}, tt_xor );

  CHECK( klut.size() == 7 );

  klut.create_node( {a, b, c}, tt_maj );

  CHECK( klut.size() == 7 );
}

TEST_CASE( "subsitute node by another", "[klut]" )
{
  klut_network klut;

  const auto c0 = klut.get_node( klut.get_constant( false ) );
  const auto c1 = klut.get_node( klut.get_constant( true ) );
  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  kitty::dynamic_truth_table tt_nand( 2u ), tt_le( 2u ), tt_ge( 2u ), tt_or( 2u );
  kitty::create_from_hex_string( tt_nand, "7" );
  kitty::create_from_hex_string( tt_le, "2" );
  kitty::create_from_hex_string( tt_ge, "4" );
  kitty::create_from_hex_string( tt_or, "e" );

  // XOR with NAND
  const auto n1 = klut.create_node( {a, b}, tt_nand );
  const auto n2 = klut.create_node( {a, n1}, tt_nand );
  const auto n3 = klut.create_node( {b, n1}, tt_nand );
  const auto n4 = klut.create_node( {n2, n3}, tt_nand );
  klut.create_po( n4 );

  std::vector<node<klut_network>> nodes;
  klut.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector<klut_network::node>{c0, c1, a, b, n1, n2, n3, n4} );
  CHECK( klut.fanout_size( n4 ) == 1 );
  klut.foreach_po( [&]( auto f ) {
    CHECK( f == n4 );
    return false;
  } );

  // XOR with AND and OR
  const auto n5 = klut.create_node( {a, b}, tt_le );
  const auto n6 = klut.create_node( {a, b}, tt_ge );
  const auto n7 = klut.create_node( {n5, n6}, tt_or );

  nodes.clear();
  klut.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector{c0, c1, a, b, n1, n2, n3, n4, n5, n6, n7} );
  CHECK( klut.fanout_size( n7 ) == 0 );

  // substitute nodes
  klut.substitute_node( n4, n7 );

  CHECK( klut.size() == 11 );
  CHECK( klut.fanout_size( n4 ) == 0 );
  CHECK( klut.fanout_size( n7 ) == 1 );
  klut.foreach_po( [&]( auto f ) {
    CHECK( f == n7 );
    return false;
  } );
}

TEST_CASE( "structural properties of a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_size_v<klut_network> );
  CHECK( has_num_pis_v<klut_network> );
  CHECK( has_num_pos_v<klut_network> );
  CHECK( has_num_gates_v<klut_network> );
  CHECK( has_fanin_size_v<klut_network> );
  CHECK( has_fanout_size_v<klut_network> );

  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();

  const auto f1 = klut.create_and( x1, x2 );
  const auto f2 = klut.create_and( x2, x1 );

  klut.create_po( f1 );
  klut.create_po( f2 );

  CHECK( klut.size() == 6 );
  CHECK( klut.num_pis() == 2 );
  CHECK( klut.num_pos() == 2 );
  CHECK( klut.num_gates() == 2 );
  CHECK( klut.fanin_size( klut.get_node( x1 ) ) == 0 );
  CHECK( klut.fanin_size( klut.get_node( x2 ) ) == 0 );
  CHECK( klut.fanin_size( klut.get_node( f1 ) ) == 2 );
  CHECK( klut.fanin_size( klut.get_node( f2 ) ) == 2 );
  CHECK( klut.fanout_size( klut.get_node( x1 ) ) == 2 );
  CHECK( klut.fanout_size( klut.get_node( x2 ) ) == 2 );
  CHECK( klut.fanout_size( klut.get_node( f1 ) ) == 1 );
  CHECK( klut.fanout_size( klut.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in a k-LUT network", "[klut]" )
{
  klut_network klut;

  CHECK( has_foreach_node_v<klut_network> );
  CHECK( has_foreach_pi_v<klut_network> );
  CHECK( has_foreach_po_v<klut_network> );
  CHECK( has_foreach_fanin_v<klut_network> );

  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();
  const auto f1 = klut.create_and( x1, x2 );
  const auto f2 = klut.create_and( x2, x1 );
  klut.create_po( f1 );
  klut.create_po( f2 );

  CHECK( klut.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{0}, counter{0};
  klut.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  klut.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  klut.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  klut.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  klut.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 12 );
  CHECK( counter == 1 );

  mask = 0;
  klut.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 12 );

  mask = counter = 0;
  klut.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 4 );
  CHECK( counter == 0 );

  mask = 0;
  klut.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 4 );

  /* iterate over POs */
  mask = counter = 0;
  klut.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << klut.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  klut.foreach_po( [&]( auto s ) { mask |= ( 1 << klut.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  klut.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << klut.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  klut.foreach_po( [&]( auto s ) { mask |= ( 1 << klut.get_node( s ) ); return false; } );
  CHECK( mask == 16 );
}

TEST_CASE( "custom node values in k-LUT networks", "[klut]" )
{
  klut_network klut;

  CHECK( has_clear_values_v<klut_network> );
  CHECK( has_value_v<klut_network> );
  CHECK( has_set_value_v<klut_network> );
  CHECK( has_incr_value_v<klut_network> );
  CHECK( has_decr_value_v<klut_network> );

  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();
  const auto f1 = klut.create_and( x1, x2 );
  const auto f2 = klut.create_and( x2, x1 );
  klut.create_po( f1 );
  klut.create_po( f2 );

  CHECK( klut.size() == 6 );

  klut.clear_values();
  klut.foreach_node( [&]( auto n ) {
    CHECK( klut.value( n ) == 0 );
    klut.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( klut.value( n ) == n );
    CHECK( klut.incr_value( n ) == n );
    CHECK( klut.value( n ) == n + 1 );
    CHECK( klut.decr_value( n ) == n );
    CHECK( klut.value( n ) == n );
  } );
  klut.clear_values();
  klut.foreach_node( [&]( auto n ) {
    CHECK( klut.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in k-LUT networks", "[klut]" )
{
  klut_network klut;

  CHECK( has_clear_visited_v<klut_network> );
  CHECK( has_visited_v<klut_network> );
  CHECK( has_set_visited_v<klut_network> );

  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();
  const auto f1 = klut.create_and( x1, x2 );
  const auto f2 = klut.create_and( x2, x1 );
  klut.create_po( f1 );
  klut.create_po( f2 );

  CHECK( klut.size() == 6 );

  klut.clear_visited();
  klut.foreach_node( [&]( auto n ) {
    CHECK( klut.visited( n ) == 0 );
    klut.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( klut.visited( n ) == n );
  } );
  klut.clear_visited();
  klut.foreach_node( [&]( auto n ) {
    CHECK( klut.visited( n ) == 0 );
  } );
}
