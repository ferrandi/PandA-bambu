#include <catch.hpp>

#include <kitty/kitty.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

template<class Ntk>
void test_buffered_network()
{
  Ntk ntk;

  CHECK( has_create_buf_v<Ntk> );
  CHECK( has_is_buf_v<Ntk> );
  CHECK( has_is_not_v<Ntk> );

  auto x1 = ntk.create_pi();
  auto x2 = ntk.create_pi();
  auto b1 = ntk.create_buf( x1 );

  auto g1 = ntk.create_and( b1, x2 );
  auto b2 = ntk.create_buf( g1 );
  auto b3 = ntk.create_buf( g1 );
  auto b4 = ntk.create_buf( b3 );

  auto g2 = ntk.create_and( b1, b2 );
  ntk.create_po( b4 );
  ntk.create_po( g2 );

  /* properties */
  CHECK( ntk.is_pi( ntk.get_node( x1 ) ) );
  CHECK( !ntk.is_buf( ntk.get_node( x1 ) ) );
  CHECK( !ntk.is_not( ntk.get_node( x1 ) ) );
  CHECK( !ntk.is_ci( ntk.get_node( b1 ) ) );
  CHECK( ntk.is_buf( ntk.get_node( b1 ) ) );

  CHECK( ntk.num_pis() == 2 );
  CHECK( ntk.size() == 9 );
  CHECK( ntk.num_gates() == 2 );

  CHECK( ntk.fanout_size( ntk.get_node( x1 ) ) == 1 );
  CHECK( ntk.fanout_size( ntk.get_node( b1 ) ) == 2 );
  CHECK( ntk.fanin_size( ntk.get_node( b1 ) ) == 1 );

  /* foreach */
  uint32_t mask = 0;
  ntk.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 0x1ff );

  mask = 0;
  ntk.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 0x110 );

  mask = 0;
  ntk.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 0x006 );

  mask = 0;
  ntk.foreach_po( [&]( auto s ) { mask |= ( 1 << ntk.get_node( s ) ); } );
  CHECK( mask == 0x180 );

  /* simulation */
  auto const po_values = simulate_buffered<2>( ntk );

  CHECK( po_values[0]._bits == 8 );
  CHECK( po_values[1]._bits == 8 );
}

TEST_CASE( "buffered networks", "[buffered]" )
{
  test_buffered_network<buffered_aig_network>();
  test_buffered_network<buffered_mig_network>();
}

TEST_CASE( "is_buffered_network_type", "[buffered]" )
{
  CHECK( is_buffered_network_type_v<buffered_aig_network> );
  CHECK( is_buffered_network_type_v<buffered_mig_network> );
  CHECK( is_buffered_network_type_v<buffered_crossed_klut_network> );
  CHECK( is_buffered_network_type_v<buffered_aqfp_network> );

  CHECK( !is_buffered_network_type_v<aig_network> );
  CHECK( !is_buffered_network_type_v<mig_network> );
  CHECK( !is_buffered_network_type_v<klut_network> );
  CHECK( !is_buffered_network_type_v<aqfp_network> );
}
