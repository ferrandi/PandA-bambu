#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <mockturtle/algorithms/aig_balancing.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

TEST_CASE( "Balancing AND chain in AIG", "[aig_balancing]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  aig.create_po( aig.create_and( a, aig.create_and( b, aig.create_and( c, d ) ) ) );

  CHECK( depth_view{ aig }.depth() == 3u );

  aig_balance( aig );

  CHECK( depth_view{ aig }.depth() == 2u );
}

TEST_CASE( "Balance AND finding structural hashing", "[aig_balancing]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_and( f1, c );
  const auto f3 = aig.create_and( b, c );
  const auto f4 = aig.create_and( f3, d );

  aig.create_po( f2 );
  aig.create_po( f4 );

  CHECK( depth_view{ aig }.depth() == 2u );
  CHECK( aig.num_gates() == 4u );

  aig_balancing_params ps;
  ps.minimize_levels = false;
  aig_balance( aig, ps );

  CHECK( depth_view{ aig }.depth() == 2u );
  CHECK( aig.num_gates() == 3u );
}

TEST_CASE( "Balance AND tree that is constant 0", "[aig_balancing]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_and( !a, c );
  const auto f3 = aig.create_and( f1, f2 );

  aig.create_po( f3 );

  CHECK( depth_view{ aig }.depth() == 2u );
  CHECK( aig.num_gates() == 3u );

  aig_balance( aig );

  CHECK( depth_view{ aig }.depth() == 0u );
  CHECK( aig.num_gates() == 0u );
}

TEST_CASE( "Balance AND tree that has redundant leaves", "[aig_balancing]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_and( a, c );
  const auto f3 = aig.create_and( f1, f2 );

  aig.create_po( f3 );

  CHECK( depth_view{ aig }.depth() == 2u );
  CHECK( aig.num_gates() == 3u );

  aig_balance( aig );

  CHECK( depth_view{ aig }.depth() == 2u );
  CHECK( aig.num_gates() == 2u );
}
