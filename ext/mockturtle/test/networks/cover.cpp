#include <catch.hpp>

#include <vector>

#include <mockturtle/networks/cover.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
using namespace mockturtle;

TEST_CASE( "create and use constants in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_size_v<cover_network> );
  CHECK( has_get_constant_v<cover_network> );
  CHECK( has_is_constant_v<cover_network> );
  CHECK( has_is_pi_v<cover_network> );
  CHECK( has_is_constant_v<cover_network> );
  CHECK( has_get_node_v<cover_network> );
  CHECK( has_is_complemented_v<cover_network> );

  CHECK( cover.size() == 2 );

  auto c0 = cover.get_constant( false );
  auto c1 = cover.get_constant( true );

  CHECK( cover.size() == 2 );
  CHECK( c0 != c1 );
  CHECK( cover.get_node( c0 ) == 0 );
  CHECK( cover.get_node( c1 ) == 1 );
  CHECK( !cover.is_complemented( c0 ) );
  CHECK( !cover.is_complemented( c1 ) );
  CHECK( cover.is_constant( c0 ) );
  CHECK( cover.is_constant( c1 ) );
  CHECK( !cover.is_pi( c0 ) );
  CHECK( !cover.is_pi( c1 ) );
}

TEST_CASE( "create and use primary inputs in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_pi_v<cover_network> );
  CHECK( has_is_constant_v<cover_network> );
  CHECK( has_is_pi_v<cover_network> );
  CHECK( has_num_pis_v<cover_network> );

  CHECK( cover.num_pis() == 0 );

  auto x1 = cover.create_pi();
  auto x2 = cover.create_pi();

  CHECK( cover.size() == 4 );
  CHECK( cover.num_pis() == 2 );
  CHECK( x1 != x2 );
}

TEST_CASE( "create and use primary outputs in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_po_v<cover_network> );
  CHECK( has_num_pos_v<cover_network> );

  auto c0 = cover.get_constant( false );
  auto c1 = cover.get_constant( true );
  auto x = cover.create_pi();

  cover.create_po( c0 );
  cover.create_po( c1 );
  cover.create_po( x );

  CHECK( cover.size() == 3 );
  CHECK( cover.num_pis() == 1 );
  CHECK( cover.num_pos() == 3 );
}

TEST_CASE( "create unary operations in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_buf_v<cover_network> );
  CHECK( has_create_not_v<cover_network> );

  auto x1 = cover.create_pi();

  CHECK( cover.size() == 3 );

  auto f1 = cover.create_buf( x1 );
  auto f2 = cover.create_not( x1 );

  CHECK( cover.size() == 4 );
  CHECK( f1 == x1 );
  CHECK( f2 != x1 );
}

TEST_CASE( "create binary operations in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_and_v<cover_network> );

  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();

  CHECK( cover.size() == 4 );

  cover.create_and( x1, x2 );
  CHECK( cover.size() == 5 );

  cover.create_and( x1, x2 );
  CHECK( cover.size() == 6 ); /* differently from the k-LUT case here we store redundantly */

  cover.create_and( x2, x1 );
  CHECK( cover.size() == 7 ); /* differently from the k-LUT case here we store redundantly */
}

TEST_CASE( "clone a node in a cover network", "[cover]" )
{
  cover_network cover1, cover2;

  CHECK( has_clone_node_v<cover_network> );

  auto a1 = cover1.create_pi();
  auto b1 = cover1.create_pi();
  auto f1 = cover1.create_and( a1, b1 );
  CHECK( cover1.size() == 5 );

  auto a2 = cover2.create_pi();
  auto b2 = cover2.create_pi();
  CHECK( cover2.size() == 4 );

  auto f2 = cover2.clone_node( cover1, cover1.get_node( f1 ), { a2, b2 } );
  CHECK( cover2.size() == 5 );

  cover2.foreach_fanin( cover2.get_node( f2 ), [&]( auto const& s ) {
    CHECK( !cover2.is_complemented( s ) );
  } );
}

TEST_CASE( "compute functions from AND and NOT gates in cover networks", "[cover]" )
{
  cover_network cover;

  const auto a = cover.create_pi();
  const auto b = cover.create_pi();

  const auto f1 = cover.create_not( a );
  const auto f2 = cover.create_and( a, b );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );

  const auto sim_f1 = cover.compute( cover.get_node( f1 ), xs.begin(), xs.begin() + 1 );
  const auto sim_f2 = cover.compute( cover.get_node( f2 ), xs.begin(), xs.end() );

  CHECK( sim_f1 == ~xs[0] );
  CHECK( sim_f2 == ( xs[0] & xs[1] ) );
}

TEST_CASE( "create nodes and compute a function in a cover network from truth tables", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_node_v<cover_network> );
  CHECK( has_compute_v<cover_network, kitty::dynamic_truth_table> );

  const auto a = cover.create_pi();
  const auto b = cover.create_pi();
  const auto c = cover.create_pi();

  kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u ), tt_const0( 0u );
  kitty::create_from_hex_string( tt_maj, "e8" );
  kitty::create_from_hex_string( tt_xor, "96" );

  CHECK( cover.size() == 5 );

  const auto _const0 = cover.create_node( {}, tt_const0 );
  const auto _const1 = cover.create_node( {}, ~tt_const0 );
  CHECK( _const0 == cover.get_constant( false ) );
  CHECK( _const1 == cover.get_constant( true ) );

  const auto _maj = cover.create_node( { a, b, c }, tt_maj );
  const auto _xor = cover.create_node( { a, b, c }, tt_xor );

  CHECK( cover.size() == 7 );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  xs.emplace_back( 3u );
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );

  const auto sim_maj = cover.compute( cover.get_node( _maj ), xs.begin(), xs.end() );
  const auto sim_xor = cover.compute( cover.get_node( _xor ), xs.begin(), xs.end() );

  CHECK( sim_maj == kitty::ternary_majority( xs[0], xs[1], xs[2] ) );
  CHECK( sim_xor == ( xs[0] ^ xs[1] ^ xs[2] ) );
}

TEST_CASE( "create nodes and compute a function in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_create_cover_node_v<cover_network> );

  const auto a = cover.create_pi();
  const auto b = cover.create_pi();
  const auto c = cover.create_pi();

  std::vector<kitty::cube> v_cube0 = { kitty::cube() };
  std::pair<std::vector<kitty::cube>, bool> cb_const0 = std::make_pair( v_cube0, false );
  std::pair<std::vector<kitty::cube>, bool> cb_const1 = std::make_pair( v_cube0, true );

  CHECK( cover.size() == 5 );

  const auto _const0 = cover.create_cover_node( {}, cb_const0 );
  const auto _const1 = cover.create_cover_node( {}, cb_const1 );
  CHECK( _const0 == cover.get_constant( false ) );
  CHECK( _const1 == cover.get_constant( true ) );

  kitty::cube _X11 = kitty::cube( "-11" );
  kitty::cube _1X1 = kitty::cube( "1-1" );
  kitty::cube _11X = kitty::cube( "11-" );

  std::vector<kitty::dynamic_truth_table> xs;
  xs.emplace_back( 2u );
  xs.emplace_back( 2u );
  xs.emplace_back( 2u );
  kitty::create_nth_var( xs[0], 1 ); // 1100
  kitty::create_nth_var( xs[1], 0 ); // 1010
  kitty::create_nth_var( xs[2], 1 ); // 1100

  std::vector<kitty::cube> cubes_maj = { _X11, _1X1, _11X };
  std::pair<std::vector<kitty::cube>, bool> cover_maj3 = std::make_pair( cubes_maj, true );
  const auto _maj = cover.create_cover_node( { a, b, c }, cover_maj3 );

  const auto sim_maj = cover.compute( cover.get_node( _maj ), xs.begin(), xs.end() );
  kitty::dynamic_truth_table answer( 2u );
  kitty::create_nth_var( answer, 1 );

  CHECK( sim_maj == answer );
}

TEST_CASE( "hash nodes in cover network", "[cover]" )
{
  cover_network cover;

  const auto a = cover.create_pi();
  const auto b = cover.create_pi();
  const auto c = cover.create_pi();

  kitty::cube _X11 = kitty::cube( "-11" );
  kitty::cube _1X1 = kitty::cube( "1-1" );
  kitty::cube _11X = kitty::cube( "11-" );
  kitty::cube _X00 = kitty::cube( "-00" );
  kitty::cube _0X0 = kitty::cube( "0-0" );
  kitty::cube _00X = kitty::cube( "00-" );

  kitty::cube _100 = kitty::cube( "100" );
  kitty::cube _010 = kitty::cube( "010" );
  kitty::cube _001 = kitty::cube( "001" );
  kitty::cube _111 = kitty::cube( "111" );

  std::vector<kitty::cube> cubes_maj = { _X11, _1X1, _11X };
  std::pair<std::vector<kitty::cube>, bool> cover_maj = std::make_pair( cubes_maj, true );

  std::vector<kitty::cube> cubes_maj_pos = { _X00, _0X0, _00X };
  std::pair<std::vector<kitty::cube>, bool> cover_maj_pos = std::make_pair( cubes_maj_pos, false );

  std::vector<kitty::cube> cubes_xor = { _001, _010, _100, _111 };
  std::pair<std::vector<kitty::cube>, bool> cover_xor = std::make_pair( cubes_xor, true );

  cover.create_cover_node( { a, b, c }, cover_maj_pos );
  cover.create_cover_node( { a, b, c }, cover_maj );
  cover.create_cover_node( { a, b, c }, cover_xor );

  CHECK( cover.size() == 8 );

  cover.create_cover_node( { a, b, c }, cover_maj );

  CHECK( cover.size() == 9 );
}

TEST_CASE( "substitute cover node by another", "[cover]" )
{
  cover_network cover;

  const auto c0 = cover.get_node( cover.get_constant( false ) );
  const auto c1 = cover.get_node( cover.get_constant( true ) );
  const auto a = cover.create_pi();
  const auto b = cover.create_pi();

  kitty::cube _00 = kitty::cube( "00" );
  kitty::cube _01 = kitty::cube( "01" );
  kitty::cube _10 = kitty::cube( "10" );
  kitty::cube _11 = kitty::cube( "11" );

  std::vector<kitty::cube> _nand{ _00, _01, _11 };
  std::vector<kitty::cube> _le{ _00, _01, _11 };
  std::vector<kitty::cube> _ge{ _00, _10, _11 };
  std::vector<kitty::cube> _or{ _10, _01, _11 };

  // XOR with NAND
  const auto n1 = cover.create_cover_node( { a, b }, std::make_pair( _nand, true ) );
  const auto n2 = cover.create_cover_node( { a, n1 }, std::make_pair( _nand, true ) );
  const auto n3 = cover.create_cover_node( { b, n1 }, std::make_pair( _nand, true ) );
  const auto n4 = cover.create_cover_node( { n2, n3 }, std::make_pair( _nand, true ) );
  cover.create_po( n4 );

  std::vector<node<cover_network>> nodes;
  cover.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector<cover_network::node>{ c0, c1, a, b, n1, n2, n3, n4 } );
  CHECK( cover.fanout_size( n4 ) == 1 );
  cover.foreach_po( [&]( auto f ) {
    CHECK( f == n4 );
    return false;
  } );

  // XOR with AND and OR
  const auto n5 = cover.create_cover_node( { a, b }, std::make_pair( _le, true ) );
  const auto n6 = cover.create_cover_node( { a, b }, std::make_pair( _ge, true ) );
  const auto n7 = cover.create_cover_node( { n5, n6 }, std::make_pair( _or, true ) );

  nodes.clear();
  cover.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  CHECK( nodes == std::vector{ c0, c1, a, b, n1, n2, n3, n4, n5, n6, n7 } );
  CHECK( cover.fanout_size( n7 ) == 0 );

  // substitute nodes
  cover.substitute_node( n4, n7 );

  CHECK( cover.size() == 11 );
  CHECK( cover.fanout_size( n4 ) == 0 );
  CHECK( cover.fanout_size( n7 ) == 1 );
  cover.foreach_po( [&]( auto f ) {
    CHECK( f == n7 );
    return false;
  } );
}

TEST_CASE( "structural properties of a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_size_v<cover_network> );
  CHECK( has_num_pis_v<cover_network> );
  CHECK( has_num_pos_v<cover_network> );
  CHECK( has_num_gates_v<cover_network> );
  CHECK( has_fanin_size_v<cover_network> );
  CHECK( has_fanout_size_v<cover_network> );

  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();

  const auto f1 = cover.create_and( x1, x2 );
  const auto f2 = cover.create_and( x2, x1 );

  cover.create_po( f1 );
  cover.create_po( f2 );

  CHECK( cover.size() == 6 );
  CHECK( cover.num_pis() == 2 );
  CHECK( cover.num_pos() == 2 );
  CHECK( cover.num_gates() == 2 );
  CHECK( cover.fanin_size( cover.get_node( x1 ) ) == 0 );
  CHECK( cover.fanin_size( cover.get_node( x2 ) ) == 0 );
  CHECK( cover.fanin_size( cover.get_node( f1 ) ) == 2 );
  CHECK( cover.fanin_size( cover.get_node( f2 ) ) == 2 );
  CHECK( cover.fanout_size( cover.get_node( x1 ) ) == 2 );
  CHECK( cover.fanout_size( cover.get_node( x2 ) ) == 2 );
  CHECK( cover.fanout_size( cover.get_node( f1 ) ) == 1 );
  CHECK( cover.fanout_size( cover.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in a cover network", "[cover]" )
{
  cover_network cover;

  CHECK( has_foreach_node_v<cover_network> );
  CHECK( has_foreach_pi_v<cover_network> );
  CHECK( has_foreach_po_v<cover_network> );
  CHECK( has_foreach_fanin_v<cover_network> );

  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();
  const auto f1 = cover.create_and( x1, x2 );
  const auto f2 = cover.create_and( x2, x1 );
  cover.create_po( f1 );
  cover.create_po( f2 );

  CHECK( cover.size() == 6 );

  /* iterate over nodes */
  uint32_t mask{ 0 }, counter{ 0 };
  cover.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 63 );
  CHECK( counter == 15 );

  mask = 0;
  cover.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 63 );

  mask = counter = 0;
  cover.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  cover.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  cover.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 12 );
  CHECK( counter == 1 );

  mask = 0;
  cover.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 12 );

  mask = counter = 0;
  cover.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 4 );
  CHECK( counter == 0 );

  mask = 0;
  cover.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 4 );

  /* iterate over POs */
  mask = counter = 0;
  cover.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << cover.get_node( s ) ); counter += i; } );
  CHECK( mask == 48 );
  CHECK( counter == 1 );

  mask = 0;
  cover.foreach_po( [&]( auto s ) { mask |= ( 1 << cover.get_node( s ) ); } );
  CHECK( mask == 48 );

  mask = counter = 0;
  cover.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << cover.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 16 );
  CHECK( counter == 0 );

  mask = 0;
  cover.foreach_po( [&]( auto s ) { mask |= ( 1 << cover.get_node( s ) ); return false; } );
  CHECK( mask == 16 );
}

TEST_CASE( "custom node values in cover networks", "[cover]" )
{
  cover_network cover;

  CHECK( has_clear_values_v<cover_network> );
  CHECK( has_value_v<cover_network> );
  CHECK( has_set_value_v<cover_network> );
  CHECK( has_incr_value_v<cover_network> );
  CHECK( has_decr_value_v<cover_network> );

  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();
  const auto f1 = cover.create_and( x1, x2 );
  const auto f2 = cover.create_and( x2, x1 );
  cover.create_po( f1 );
  cover.create_po( f2 );

  CHECK( cover.size() == 6 );

  cover.clear_values();
  cover.foreach_node( [&]( auto n ) {
    CHECK( cover.value( n ) == 0 );
    cover.set_value( n, static_cast<uint32_t>( n ) );
    CHECK( cover.value( n ) == n );
    CHECK( cover.incr_value( n ) == n );
    CHECK( cover.value( n ) == n + 1 );
    CHECK( cover.decr_value( n ) == n );
    CHECK( cover.value( n ) == n );
  } );
  cover.clear_values();
  cover.foreach_node( [&]( auto n ) {
    CHECK( cover.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in cover networks", "[cover]" )
{
  cover_network cover;

  CHECK( has_clear_visited_v<cover_network> );
  CHECK( has_visited_v<cover_network> );
  CHECK( has_set_visited_v<cover_network> );

  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();
  const auto f1 = cover.create_and( x1, x2 );
  const auto f2 = cover.create_and( x2, x1 );
  cover.create_po( f1 );
  cover.create_po( f2 );

  CHECK( cover.size() == 6 );

  cover.clear_visited();
  cover.foreach_node( [&]( auto n ) {
    CHECK( cover.visited( n ) == 0 );
    cover.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( cover.visited( n ) == n );
  } );
  cover.clear_visited();
  cover.foreach_node( [&]( auto n ) {
    CHECK( cover.visited( n ) == 0 );
  } );
}
