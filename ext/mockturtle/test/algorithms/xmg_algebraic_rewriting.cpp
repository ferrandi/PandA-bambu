#include <catch.hpp>

#include <mockturtle/algorithms/xmg_algebraic_rewriting.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

TEST_CASE( "xmg depth optimization with xor complementary associativity", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();
  const auto e = xmg.create_pi();

  const auto f1 = xmg.create_maj( a, b, c );
  const auto f2 = xmg.create_xor( !f1, d );
  const auto f3 = xmg.create_maj( f1, f2, e );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg depth optimization with xor associativity", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();
  const auto e = xmg.create_pi();

  const auto f1 = xmg.create_maj( a, b, c );
  const auto f2 = xmg.create_xor( f1, d );
  const auto f3 = xmg.create_xor( f2, e );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg depth optimization with associativity", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();

  const auto f1 = xmg.create_and( a, b );
  const auto f2 = xmg.create_and( f1, c );
  const auto f3 = xmg.create_and( f2, d );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg depth optimization with complemented associativity", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();

  const auto f1 = xmg.create_and( a, b );
  const auto f2 = xmg.create_and( f1, c );
  const auto f3 = xmg.create_or( f2, d );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg depth optimization with distributivity", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();
  const auto e = xmg.create_pi();
  const auto f = xmg.create_pi();
  const auto g = xmg.create_pi();

  const auto f1 = xmg.create_maj( e, f, g );
  const auto f2 = xmg.create_maj( c, d, f1 );
  const auto f3 = xmg.create_maj( a, b, f2 );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg selective depth optimization", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();
  const auto e = xmg.create_pi();
  const auto f = xmg.create_pi();
  const auto g = xmg.create_pi();

  const auto f1 = xmg.create_maj( e, f, g );
  const auto f2 = xmg.create_maj( c, d, f1 );
  const auto f3 = xmg.create_maj( a, b, f2 );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };
  xmg_algebraic_depth_rewriting_params ps;
  ps.strategy = xmg_algebraic_depth_rewriting_params::selective;

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg, ps );

  CHECK( depth_xmg.depth() == 2 );
}

TEST_CASE( "xmg aggressive depth optimization", "[xmg_algebraic_rewriting]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();
  const auto d = xmg.create_pi();
  const auto e = xmg.create_pi();
  const auto f = xmg.create_pi();
  const auto g = xmg.create_pi();

  const auto f1 = xmg.create_maj( e, f, g );
  const auto f2 = xmg.create_maj( c, d, f1 );
  const auto f3 = xmg.create_maj( a, b, f2 );

  xmg.create_po( f3 );

  depth_view depth_xmg{ xmg };
  xmg_algebraic_depth_rewriting_params ps;
  ps.strategy = xmg_algebraic_depth_rewriting_params::aggressive;

  CHECK( depth_xmg.depth() == 3 );

  xmg_algebraic_depth_rewriting( depth_xmg, ps );

  CHECK( depth_xmg.depth() == 2 );
}
