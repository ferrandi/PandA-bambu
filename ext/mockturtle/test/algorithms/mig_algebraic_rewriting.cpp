#include <catch.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

TEST_CASE( "MIG depth optimization with associativity", "[mig_algebraic_rewriting]" )
{
  mig_network mig;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();

  const auto f1 = mig.create_and( a, b );
  const auto f2 = mig.create_and( f1, c );
  const auto f3 = mig.create_and( f2, d );

  mig.create_po( f3 );

  depth_view depth_mig{mig};

  CHECK( depth_mig.depth() == 3 );

  mig_algebraic_depth_rewriting( depth_mig );

  CHECK( depth_mig.depth() == 2 );
}

TEST_CASE( "MIG depth optimization with complemented associativity", "[mig_algebraic_rewriting]" )
{
  mig_network mig;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();

  const auto f1 = mig.create_and( a, b );
  const auto f2 = mig.create_and( f1, c );
  const auto f3 = mig.create_or( f2, d );

  mig.create_po( f3 );

  depth_view depth_mig{mig};

  CHECK( depth_mig.depth() == 3 );

  mig_algebraic_depth_rewriting( depth_mig );

  CHECK( depth_mig.depth() == 2 );
}

TEST_CASE( "MIG depth optimization with distributivity", "[mig_algebraic_rewriting]" )
{
  mig_network mig;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();
  const auto e = mig.create_pi();
  const auto f = mig.create_pi();
  const auto g = mig.create_pi();

  const auto f1 = mig.create_maj( e, f, g );
  const auto f2 = mig.create_maj( c, d, f1 );
  const auto f3 = mig.create_maj( a, b, f2 );

  mig.create_po( f3 );

  depth_view depth_mig{mig};

  CHECK( depth_mig.depth() == 3 );

  mig_algebraic_depth_rewriting( depth_mig );

  CHECK( depth_mig.depth() == 2 );
}
