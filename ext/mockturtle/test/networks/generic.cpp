#include <catch.hpp>

#include <vector>

#include <mockturtle/networks/generic.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_size_v<generic_network> );
  CHECK( has_get_constant_v<generic_network> );
  CHECK( has_is_constant_v<generic_network> );
  CHECK( has_is_pi_v<generic_network> );
  CHECK( has_is_constant_v<generic_network> );
  CHECK( has_get_node_v<generic_network> );
  CHECK( has_is_complemented_v<generic_network> );

  CHECK( generic_net.size() == 2 );

  auto c0 = generic_net.get_constant( false );
  auto c1 = generic_net.get_constant( true );

  CHECK( generic_net.size() == 2 );
  CHECK( c0 != c1 );
  CHECK( generic_net.get_node( c0 ) == 0 );
  CHECK( generic_net.get_node( c1 ) == 1 );
  CHECK( !generic_net.is_complemented( c0 ) );
  CHECK( !generic_net.is_complemented( c1 ) );
  CHECK( generic_net.is_constant( c0 ) );
  CHECK( generic_net.is_constant( c1 ) );
  CHECK( !generic_net.is_pi( c0 ) );
  CHECK( !generic_net.is_pi( c1 ) );
}

TEST_CASE( "create and use primary inputs in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_create_pi_v<generic_network> );
  CHECK( has_is_constant_v<generic_network> );
  CHECK( has_is_pi_v<generic_network> );
  CHECK( has_num_pis_v<generic_network> );

  CHECK( generic_net.num_pis() == 0 );

  auto x1 = generic_net.create_pi();
  auto x2 = generic_net.create_pi();

  CHECK( generic_net.size() == 4 );
  CHECK( generic_net.num_pis() == 2 );
  CHECK( x1 != x2 );
}

TEST_CASE( "create and use primary outputs in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_create_po_v<generic_network> );
  CHECK( has_num_pos_v<generic_network> );

  auto c0 = generic_net.get_constant( false );
  auto c1 = generic_net.get_constant( true );
  auto x = generic_net.create_pi();

  generic_net.create_po( c0 );
  generic_net.create_po( c1 );
  generic_net.create_po( x );

  CHECK( generic_net.size() == 6 );
  CHECK( generic_net.num_pis() == 1 );
  CHECK( generic_net.num_pos() == 3 );
}

TEST_CASE( "create unary operations in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_create_buf_v<generic_network> );
  CHECK( has_create_not_v<generic_network> );

  auto x1 = generic_net.create_pi();

  CHECK( generic_net.size() == 3 );

  auto f1 = generic_net.create_buf( x1 );
  auto f2 = generic_net.create_not( x1 );

  CHECK( generic_net.size() == 5 );
  CHECK( f1 != x1 );
  CHECK( f2 != x1 );
}

TEST_CASE( "create binary operations in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_create_and_v<generic_network> );

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();

  CHECK( generic_net.size() == 4 );

  generic_net.create_and( x1, x2 );
  CHECK( generic_net.size() == 5 );

  generic_net.create_and( x1, x2 );
  CHECK( generic_net.size() == 6 );

  generic_net.create_and( x2, x1 );
  CHECK( generic_net.size() == 7 );
}

TEST_CASE( "clone a generic network", "[generic_net]" )
{
  CHECK( has_clone_v<generic_network> );

  generic_network ntk1;
  auto a = ntk1.create_pi();
  auto b = ntk1.create_pi();
  auto f1 = ntk1.create_and( a, b );
  ntk1.create_po( f1 );
  CHECK( ntk1.size() == 6 );
  CHECK( ntk1.num_gates() == 1 );
  CHECK( ntk1.num_pos() == 1 );

  auto ntk2 = ntk1;
  auto ntk3 = ntk1.clone();

  auto c = ntk2.create_pi();
  auto f2 = ntk2.create_or( f1, c );
  ntk2.create_po( f2 );
  CHECK( ntk1.size() == 9 );
  CHECK( ntk1.num_gates() == 2 );
  CHECK( ntk1.num_pos() == 2 );

  CHECK( ntk3.size() == 6 );
  CHECK( ntk3.num_gates() == 1 );
  CHECK( ntk3.num_pos() == 1 );
}

TEST_CASE( "clone a node in a generic network", "[generic_net]" )
{
  generic_network generic_net1, generic_net2;

  CHECK( has_clone_node_v<generic_network> );

  auto a1 = generic_net1.create_pi();
  auto b1 = generic_net1.create_pi();
  auto f1 = generic_net1.create_and( a1, b1 );
  CHECK( generic_net1.size() == 5 );

  auto a2 = generic_net2.create_pi();
  auto b2 = generic_net2.create_pi();
  CHECK( generic_net2.size() == 4 );

  auto f2 = generic_net2.clone_node( generic_net1, generic_net1.get_node( f1 ), { a2, b2 } );
  CHECK( generic_net2.size() == 5 );

  generic_net2.foreach_fanin( generic_net2.get_node( f2 ), [&]( auto const& s ) {
    CHECK( !generic_net2.is_complemented( s ) );
  } );
}

TEST_CASE( "compute functions from AND and NOT gates in generic networks", "[generic_net]" )
{
  generic_network generic_net;

  const auto a = generic_net.create_pi();
  const auto b = generic_net.create_pi();

  const auto f1 = generic_net.create_not( a );
  const auto f2 = generic_net.create_and( a, b );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );

  const auto sim_f1 = generic_net.compute( generic_net.get_node( f1 ), xs.begin(), xs.begin() + 1 );
  const auto sim_f2 = generic_net.compute( generic_net.get_node( f2 ), xs.begin(), xs.end() );

  CHECK( sim_f1 == ~xs[0] );
  CHECK( sim_f2 == ( xs[0] & xs[1] ) );
}

TEST_CASE( "create nodes and compute a function in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_create_node_v<generic_network> );
  CHECK( has_compute_v<generic_network, kitty::dynamic_truth_table> );

  const auto a = generic_net.create_pi();
  const auto b = generic_net.create_pi();
  const auto c = generic_net.create_pi();

  kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u ), tt_const0( 0u );
  kitty::create_from_hex_string( tt_maj, "e8" );
  kitty::create_from_hex_string( tt_xor, "96" );

  CHECK( generic_net.size() == 5 );

  const auto _const0 = generic_net.create_node( {}, tt_const0 );
  const auto _const1 = generic_net.create_node( {}, ~tt_const0 );
  CHECK( _const0 == generic_net.get_constant( false ) );
  CHECK( _const1 == generic_net.get_constant( true ) );

  const auto _maj = generic_net.create_node( { a, b, c }, tt_maj );
  const auto _xor = generic_net.create_node( { a, b, c }, tt_xor );

  CHECK( generic_net.size() == 7 );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );

  const auto sim_maj = generic_net.compute( generic_net.get_node( _maj ), xs.begin(), xs.end() );
  const auto sim_xor = generic_net.compute( generic_net.get_node( _xor ), xs.begin(), xs.end() );

  CHECK( sim_maj == kitty::ternary_majority( xs[0], xs[1], xs[2] ) );
  CHECK( sim_xor == ( xs[0] ^ xs[1] ^ xs[2] ) );
}

TEST_CASE( "No hash nodes in generic network", "[generic_net]" )
{
  generic_network generic_net;

  const auto a = generic_net.create_pi();
  const auto b = generic_net.create_pi();
  const auto c = generic_net.create_pi();

  kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u );
  kitty::create_from_hex_string( tt_maj, "e8" );
  kitty::create_from_hex_string( tt_xor, "96" );

  generic_net.create_node( { a, b, c }, tt_maj );
  generic_net.create_node( { a, b, c }, tt_xor );

  CHECK( generic_net.size() == 7 );

  generic_net.create_node( { a, b, c }, tt_maj );

  CHECK( generic_net.size() == 8 );
}

TEST_CASE( "substitute node by another in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  const auto c0 = generic_net.get_node( generic_net.get_constant( false ) );
  const auto c1 = generic_net.get_node( generic_net.get_constant( true ) );
  const auto a = generic_net.create_pi();
  const auto b = generic_net.create_pi();

  kitty::dynamic_truth_table tt_nand( 2u ), tt_le( 2u ), tt_ge( 2u ), tt_or( 2u );
  kitty::create_from_hex_string( tt_nand, "7" );
  kitty::create_from_hex_string( tt_le, "2" );
  kitty::create_from_hex_string( tt_ge, "4" );
  kitty::create_from_hex_string( tt_or, "e" );

  // XOR with NAND
  const auto n1 = generic_net.create_node( { a, b }, tt_nand );
  const auto n2 = generic_net.create_node( { a, n1 }, tt_nand );
  const auto n3 = generic_net.create_node( { b, n1 }, tt_nand );
  const auto n4 = generic_net.create_node( { n2, n3 }, tt_nand );
  const auto po = generic_net.po_at( generic_net.create_po( n4 ) );

  std::vector<node<generic_network>> nodes;
  generic_net.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector<generic_network::node>{ c0, c1, a, b, n1, n2, n3, n4, po } );
  CHECK( generic_net.fanout_size( n4 ) == 1 );
  generic_net.foreach_po( [&]( auto f ) {
    CHECK( generic_net.get_fanin0( f ) == n4 );
    return false;
  } );

  // XOR with AND and OR
  const auto n5 = generic_net.create_node( { a, b }, tt_le );
  const auto n6 = generic_net.create_node( { a, b }, tt_ge );
  const auto n7 = generic_net.create_node( { n5, n6 }, tt_or );

  nodes.clear();
  generic_net.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector{ c0, c1, a, b, n1, n2, n3, n4, po, n5, n6, n7 } );
  CHECK( generic_net.fanout_size( n7 ) == 0 );

  // substitute nodes
  generic_net.substitute_node( n4, n7 );

  CHECK( generic_net.size() == 12 );
  CHECK( generic_net.fanout_size( n4 ) == 0 );
  CHECK( generic_net.fanout_size( n7 ) == 1 );
  generic_net.foreach_po( [&]( auto f ) {
    CHECK( generic_net.get_fanin0( f ) == n7 );
    return false;
  } );
}

TEST_CASE( "structural properties of a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_size_v<generic_network> );
  CHECK( has_num_pis_v<generic_network> );
  CHECK( has_num_pos_v<generic_network> );
  CHECK( has_num_gates_v<generic_network> );
  CHECK( has_fanin_size_v<generic_network> );
  CHECK( has_fanout_size_v<generic_network> );

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();

  const auto f1 = generic_net.create_and( x1, x2 );
  const auto f2 = generic_net.create_and( x2, x1 );

  generic_net.create_po( f1 );
  generic_net.create_po( f2 );

  CHECK( generic_net.size() == 8 );
  CHECK( generic_net.num_pis() == 2 );
  CHECK( generic_net.num_pos() == 2 );
  CHECK( generic_net.num_gates() == 2 );
  CHECK( generic_net.fanin_size( generic_net.get_node( x1 ) ) == 0 );
  CHECK( generic_net.fanin_size( generic_net.get_node( x2 ) ) == 0 );
  CHECK( generic_net.fanin_size( generic_net.get_node( f1 ) ) == 2 );
  CHECK( generic_net.fanin_size( generic_net.get_node( f2 ) ) == 2 );
  CHECK( generic_net.fanout_size( generic_net.get_node( x1 ) ) == 2 );
  CHECK( generic_net.fanout_size( generic_net.get_node( x2 ) ) == 2 );
  CHECK( generic_net.fanout_size( generic_net.get_node( f1 ) ) == 1 );
  CHECK( generic_net.fanout_size( generic_net.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in a generic network", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_foreach_node_v<generic_network> );
  CHECK( has_foreach_pi_v<generic_network> );
  CHECK( has_foreach_po_v<generic_network> );
  CHECK( has_foreach_fanin_v<generic_network> );

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();
  const auto f1 = generic_net.create_and( x1, x2 );
  const auto f2 = generic_net.create_and( x2, x1 );
  generic_net.create_po( f1 );
  generic_net.create_po( f2 );

  CHECK( generic_net.size() == 8 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  generic_net.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 255 );
  CHECK( counter == 28 );

  mask = 0;
  generic_net.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 255 );

  mask = counter = 0;
  generic_net.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  generic_net.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  generic_net.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 12 );
  CHECK( counter == 1 );

  mask = 0;
  generic_net.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 12 );

  mask = counter = 0;
  generic_net.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 4 );
  CHECK( counter == 0 );

  mask = 0;
  generic_net.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 4 );

  /* iterate over POs */
  mask = counter = 0;
  generic_net.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << generic_net.get_node( s ) ); counter += i; } );
  CHECK( mask == 192 );
  CHECK( counter == 1 );

  mask = 0;
  generic_net.foreach_po( [&]( auto s ) { mask |= ( 1 << generic_net.get_node( s ) ); } );
  CHECK( mask == 192 );

  mask = counter = 0;
  generic_net.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << generic_net.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 64 );
  CHECK( counter == 0 );

  mask = 0;
  generic_net.foreach_po( [&]( auto s ) { mask |= ( 1 << generic_net.get_node( s ) ); return false; } );
  CHECK( mask == 64 );
}

TEST_CASE( "custom node values in generic networks", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_clear_values_v<generic_network> );
  CHECK( has_value_v<generic_network> );
  CHECK( has_set_value_v<generic_network> );
  CHECK( has_incr_value_v<generic_network> );
  CHECK( has_decr_value_v<generic_network> );

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();
  const auto f1 = generic_net.create_and( x1, x2 );
  const auto f2 = generic_net.create_and( x2, x1 );
  generic_net.create_po( f1 );
  generic_net.create_po( f2 );

  CHECK( generic_net.size() == 8 );

  generic_net.clear_values();
  generic_net.foreach_node( [&]( auto n ) {
    CHECK( generic_net.value( n ) == 0 );
    generic_net.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( generic_net.value( n ) == n );
    CHECK( generic_net.incr_value( n ) == n );
    CHECK( generic_net.value( n ) == n + 1 );
    CHECK( generic_net.decr_value( n ) == n );
    CHECK( generic_net.value( n ) == n );
  } );
  generic_net.clear_values();
  generic_net.foreach_node( [&]( auto n ) {
    CHECK( generic_net.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in generic networks", "[generic_net]" )
{
  generic_network generic_net;

  CHECK( has_clear_visited_v<generic_network> );
  CHECK( has_visited_v<generic_network> );
  CHECK( has_set_visited_v<generic_network> );

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();
  const auto f1 = generic_net.create_and( x1, x2 );
  const auto f2 = generic_net.create_and( x2, x1 );
  generic_net.create_po( f1 );
  generic_net.create_po( f2 );

  CHECK( generic_net.size() == 8 );

  generic_net.clear_visited();
  generic_net.foreach_node( [&]( auto n ) {
    CHECK( generic_net.visited( n ) == 0 );
    generic_net.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( generic_net.visited( n ) == n );
  } );
  generic_net.clear_visited();
  generic_net.foreach_node( [&]( auto n ) {
    CHECK( generic_net.visited( n ) == 0 );
  } );
}

TEST_CASE( "create register with source and sink", "[generic_net]" )
{
  generic_network generic_net;

  const auto x1 = generic_net.create_pi();
  const auto x2 = generic_net.create_pi();
  const auto f1 = generic_net.create_and( x1, x2 );
  auto const in_register = generic_net.create_box_input( f1 );
  auto const node_register = generic_net.create_register( in_register );
  auto const node_register_out = generic_net.create_box_output( node_register );
  generic_net.create_po( node_register_out );

  CHECK( generic_net.size() == 9 );
  CHECK( generic_net.num_gates() == 4 );
  CHECK( generic_net.is_register( node_register ) );
  CHECK( generic_net.is_box_input( in_register ) );
  CHECK( generic_net.is_box_output( node_register_out ) );
}
