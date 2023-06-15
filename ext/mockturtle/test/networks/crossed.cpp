#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/crossed.hpp>
#include <mockturtle/networks/klut.hpp>

using namespace mockturtle;

TEST_CASE( "type traits", "[crossed]" )
{
  CHECK( !is_crossed_network_type_v<klut_network> );
  CHECK( !has_create_crossing_v<klut_network> );
  CHECK( !has_insert_crossing_v<klut_network> );
  CHECK( !has_is_crossing_v<klut_network> );
  CHECK( !has_merge_into_crossing_v<klut_network> );

  CHECK( is_crossed_network_type_v<crossed_klut_network> );
  CHECK( has_create_crossing_v<crossed_klut_network> );
  CHECK( has_insert_crossing_v<crossed_klut_network> );
  CHECK( has_is_crossing_v<crossed_klut_network> );
  CHECK( is_crossed_network_type_v<buffered_crossed_klut_network> );
  CHECK( !has_merge_into_crossing_v<crossed_klut_network> );
  CHECK( has_merge_into_crossing_v<buffered_crossed_klut_network> );
}

TEST_CASE( "insert crossings in reversed topological order, then cleanup (topo-sort)", "[crossed]" )
{
  crossed_klut_network crossed;
  auto const x1 = crossed.create_pi();
  auto const x2 = crossed.create_pi();

  auto const n3 = crossed.create_and( x1, x2 );
  auto const n4 = crossed.create_or( x1, x2 );
  auto const n5 = crossed.create_xor( x1, x2 );

  crossed.create_po( n3 );
  crossed.create_po( n4 );
  crossed.create_po( n5 );

  auto const c6 = crossed.insert_crossing( x1, x2, crossed.get_node( n4 ), crossed.get_node( n3 ) );
  auto const c7 = crossed.insert_crossing( x1, x2, crossed.get_node( n5 ), crossed.get_node( n4 ) );
  auto const c8 = crossed.insert_crossing( x1, x2, c7, c6 );
  (void)c8;

  crossed = cleanup_dangling( crossed );

  crossed.foreach_po( [&]( auto const& po ) {
    crossed.foreach_fanin_ignore_crossings( crossed.get_node( po ), [&]( auto const& f, auto i ) {
      if ( i == 0 )
        CHECK( f == x1 );
      else
        CHECK( f == x2 );
    } );
  } );
}

TEST_CASE( "create crossings in topological order", "[crossed]" )
{
  crossed_klut_network crossed;
  auto const x1 = crossed.create_pi();
  auto const x2 = crossed.create_pi();

  auto const [c3x1, c3x2] = crossed.create_crossing( x1, x2 );
  auto const [c4x1, c4x2] = crossed.create_crossing( x1, c3x2 );
  auto const [c5x1, c5x2] = crossed.create_crossing( c3x1, x2 );

  auto const n6 = crossed.create_and( x1, c4x2 );
  auto const n7 = crossed.create_or( c4x1, c5x2 );
  auto const n8 = crossed.create_xor( c5x1, x2 );

  crossed.create_po( n6 );
  crossed.create_po( n7 );
  crossed.create_po( n8 );

  crossed.foreach_po( [&]( auto const& po ) {
    crossed.foreach_fanin_ignore_crossings( crossed.get_node( po ), [&]( auto const& f, auto i ) {
      if ( i == 0 )
        CHECK( f == x1 );
      else
        CHECK( f == x2 );
    } );
  } );
}

TEST_CASE( "transform from klut to crossed_klut", "[crossed]" )
{
  klut_network klut;

  auto const x1 = klut.create_pi();
  auto const x2 = klut.create_pi();

  auto const n3 = klut.create_and( x1, x2 );
  auto const n4 = klut.create_or( x1, x2 );
  auto const n5 = klut.create_xor( x1, x2 );

  klut.create_po( n3 );
  klut.create_po( n4 );
  klut.create_po( n5 );

  crossed_klut_network crossed = cleanup_dangling<klut_network, crossed_klut_network>( klut );
  CHECK( klut.size() == crossed.size() );
}

TEST_CASE( "merge buffers into a crossing cell", "[crossed]" )
{
  buffered_crossed_klut_network klut;

  auto const x1 = klut.create_pi();
  auto const x2 = klut.create_pi();

  auto const w1 = klut.create_buf( x1 );
  auto const w2 = klut.create_buf( x2 );
  auto const w3 = klut.create_buf( w1 );
  auto const w4 = klut.create_buf( w2 );

  auto const a1 = klut.create_and( w3, w4 );

  klut.create_po( a1 );

  auto const cx = klut.merge_into_crossing( klut.get_node( w1 ), klut.get_node( w2 ) );

  CHECK( klut.is_crossing( cx ) );

  klut.foreach_fanin( cx, [&]( auto const& f, auto i ) {
    if ( i == 0 )
      CHECK( f == x1 );
    else
      CHECK( f == x2 );
  } );

  CHECK( klut.size() == 10 );

  klut = cleanup_dangling( klut );

  CHECK( klut.size() == 8 );
}
