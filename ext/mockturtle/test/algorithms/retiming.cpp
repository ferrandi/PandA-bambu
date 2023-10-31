#include <catch.hpp>

#include <cstdint>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <mockturtle/algorithms/retiming.hpp>
#include <mockturtle/networks/generic.hpp>
#include <mockturtle/views/binding_view.hpp>

using namespace mockturtle;

generic_network::signal create_register_box( generic_network& ntk, generic_network::signal const& a )
{
  auto const in_register = ntk.create_box_input( a );
  auto const node_register = ntk.create_register( in_register );
  auto const node_register_out = ntk.create_box_output( node_register );
  return node_register_out;
}

TEST_CASE( "Retime forward 1", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto b1 = create_register_box( ntk, a );
  const auto b2 = create_register_box( ntk, b );
  const auto f = ntk.create_and( b1, b2 );

  ntk.create_po( f );

  retime( ntk );
  CHECK( ntk.num_registers() == 1u );
}

TEST_CASE( "Retime backward 1", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto x1 = ntk.create_not( a );
  const auto x2 = ntk.create_buf( a );

  const auto b1 = create_register_box( ntk, x1 );
  const auto b2 = create_register_box( ntk, x2 );

  ntk.create_po( b1 );
  ntk.create_po( b2 );

  retime( ntk );
  CHECK( ntk.num_registers() == 1u );
}

TEST_CASE( "Zero retime forward", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto b1 = create_register_box( ntk, a );
  const auto b2 = create_register_box( ntk, b );

  const auto x1 = ntk.create_and( b1, b2 );
  const auto x2 = ntk.create_or( b1, b2 );

  ntk.create_po( x1 );
  ntk.create_po( x2 );

  retime( ntk );
  CHECK( ntk.num_registers() == 2u );
}

TEST_CASE( "Retime forward 2", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  const auto b1 = create_register_box( ntk, a );
  const auto b2 = create_register_box( ntk, b );
  const auto b3 = create_register_box( ntk, c );

  const auto x1 = ntk.create_and( b1, b2 );
  const auto x2 = ntk.create_or( x1, b3 );

  const auto b4 = create_register_box( ntk, x2 );

  ntk.create_po( b4 );

  retime( ntk );
  CHECK( ntk.num_registers() == 2u );
}

TEST_CASE( "Retime backward 2", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();

  const auto x1 = ntk.create_and( a, b );
  const auto x2 = ntk.create_not( a );
  const auto x3 = ntk.create_not( b );
  const auto x4 = ntk.create_not( x1 );
  const auto x5 = ntk.create_and( x1, x2 );

  const auto x6 = create_register_box( ntk, x3 );
  const auto x7 = create_register_box( ntk, x4 );
  const auto x8 = create_register_box( ntk, x5 );

  ntk.create_po( x6 );
  ntk.create_po( x7 );
  ntk.create_po( x8 );

  retime( ntk );
  CHECK( ntk.num_registers() == 2u );
}

TEST_CASE( "Retime forward 3", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  const auto b1 = create_register_box( ntk, a );
  const auto b2 = create_register_box( ntk, b );
  const auto b3 = create_register_box( ntk, c );

  const auto x1 = ntk.create_maj( b1, b2, b3 );

  const auto b4 = create_register_box( ntk, x1 );

  ntk.create_po( ntk.create_not( b1 ) );
  ntk.create_po( b4 );

  retime( ntk );
  CHECK( ntk.num_registers() == 3u );
}

TEST_CASE( "Retime backward 3", "[retime]" )
{
  generic_network ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  const auto d = ntk.create_pi();

  const auto x1 = ntk.create_and( a, b );
  const auto x2 = ntk.create_or( c, d );
  const auto x3 = ntk.create_not( a );
  const auto r1 = create_register_box( ntk, x3 );
  const auto r2 = create_register_box( ntk, b );
  const auto r3 = create_register_box( ntk, x1 );
  const auto r4 = create_register_box( ntk, r3 );
  const auto x4 = ntk.create_or( r2, x2 );
  const auto x5 = ntk.create_and( r1, x4 );
  const auto x6 = ntk.create_and( x5, r4 );

  ntk.create_po( x6 );

  retime( ntk );
  CHECK( ntk.num_registers() == 3u );
}