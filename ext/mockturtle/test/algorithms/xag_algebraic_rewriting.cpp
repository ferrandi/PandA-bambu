#include <catch.hpp>

#include <mockturtle/algorithms/xag_algebraic_rewriting.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

TEST_CASE( "xag depth optimization with and associativity", "[xag_algebraic_rewriting]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto d = xag.create_pi();

  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( f1, c );
  const auto f3 = xag.create_and( f2, d );

  xag.create_po( f3 );

  depth_view depth_xag{ xag };

  CHECK( depth_xag.depth() == 3 );

  xag_algebraic_depth_rewriting( depth_xag );

  CHECK( depth_xag.depth() == 2 );
}

TEST_CASE( "xag depth optimization with xor associativity", "[xag_algebraic_rewriting]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto d = xag.create_pi();

  const auto f1 = xag.create_xor( a, b );
  const auto f2 = xag.create_xor( f1, c );
  const auto f3 = xag.create_xor( f2, d );

  xag.create_po( f3 );

  depth_view depth_xag{ xag };

  CHECK( depth_xag.depth() == 3 );

  xag_algebraic_depth_rewriting( depth_xag );

  CHECK( depth_xag.depth() == 2 );
}

TEST_CASE( "xag depth optimization with rare distributivity", "[xag_algebraic_rewriting]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( a, c );
  const auto f3 = xag.create_or( f1, f2 );

  xag.create_po( f3 );

  depth_view depth_xag{ xag };

  CHECK( depth_xag.depth() == 2 );
  CHECK( depth_xag.num_gates() == 3 );

  xag_algebraic_depth_rewriting_params ps;
  ps.allow_rare_rules = true;
  xag_algebraic_depth_rewriting( depth_xag, ps );

  CHECK( depth_xag.depth() == 2 );
  CHECK( depth_xag.num_gates() == 2 );
}

TEST_CASE( "xag depth optimization with and-or distributivity", "[xag_algebraic_rewriting]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto d = xag.create_pi();
  const auto e = xag.create_pi();

  const auto f1 = xag.create_xor( a, b );
  const auto f2 = xag.create_and( f1, c );
  const auto f3 = xag.create_or( f2, d );
  const auto f4 = xag.create_and( f3, e );

  xag.create_po( f4 );

  depth_view depth_xag{ xag };

  CHECK( depth_xag.depth() == 4 );
  CHECK( depth_xag.num_gates() == 4 );

  xag_algebraic_depth_rewriting( depth_xag );

  CHECK( depth_xag.depth() == 3 );
  CHECK( depth_xag.num_gates() == 5 );
}

TEST_CASE( "xag depth optimization with and-xor distributivity", "[xag_algebraic_rewriting]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto d = xag.create_pi();
  const auto e = xag.create_pi();

  const auto f1 = xag.create_xor( a, b );
  const auto f2 = xag.create_and( f1, c );
  const auto f3 = xag.create_xor( f2, d );
  const auto f4 = xag.create_and( f3, e );

  xag.create_po( f4 );

  depth_view depth_xag{ xag };

  CHECK( depth_xag.depth() == 4 );
  CHECK( depth_xag.num_gates() == 4 );

  xag_algebraic_depth_rewriting( depth_xag );

  CHECK( depth_xag.depth() == 3 );
  CHECK( depth_xag.num_gates() == 5 );
}
