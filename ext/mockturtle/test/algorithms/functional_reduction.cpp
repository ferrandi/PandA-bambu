#include <catch.hpp>

#include <kitty/static_truth_table.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

TEST_CASE( "functional reduction on AIG", "[functional_reduction]" )
{
  aig_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();

  const auto f1 = ntk.create_and( a, !b );
  const auto f2 = ntk.create_and( !a, b );
  const auto f3 = ntk.create_and( !a, !b );
  const auto f4 = ntk.create_and( a, b );
  const auto f5 = ntk.create_or( f1, f2 );  // a ^ b
  const auto f6 = ntk.create_or( f3, f4 );  // a == b
  const auto f7 = ntk.create_and( f5, f6 ); // 0

  ntk.create_po( f5 );
  ntk.create_po( f6 );
  ntk.create_po( f7 );

  auto vals = simulate<kitty::static_truth_table<2>>( ntk );

  CHECK( ntk.size() == 10 );
  functional_reduction( ntk );
  ntk = cleanup_dangling( ntk );
  CHECK( ntk.size() == 6 );
  CHECK( vals == simulate<kitty::static_truth_table<2>>( ntk ) );
}

TEST_CASE( "functional reduction on XAG", "[functional_reduction]" )
{
  xag_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto f3 = ntk.create_and( a, !c );
  const auto f4 = ntk.create_and( f3, b ); // ab!c
  const auto f5 = ntk.create_and( !a, c );
  const auto f6 = ntk.create_and( !b, c );
  const auto f7 = ntk.create_or( f5, f6 ); // !ac + !bc
  const auto f8 = ntk.create_or( f4, f7 ); // ab!c + !ac + !bc = (ab) ^ c

  const auto f1 = ntk.create_and( a, b );
  const auto f2 = ntk.create_xor( f1, c );

  ntk.create_po( f2 );
  ntk.create_po( f8 );
  // f2 == f8

  auto vals = simulate<kitty::static_truth_table<3>>( ntk );

  CHECK( ntk.size() == 12 );
  functional_reduction( ntk );
  ntk = cleanup_dangling( ntk );
  CHECK( ntk.size() == 6 );
  CHECK( vals == simulate<kitty::static_truth_table<3>>( ntk ) );
}

TEST_CASE( "functional reduction on MIG", "[functional_reduction]" )
{
  mig_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  const auto d = ntk.create_pi();

  const auto f1 = ntk.create_maj( a, b, c );
  const auto f2 = ntk.create_maj( f1, b, d );

  const auto f3 = ntk.create_maj( d, b, c );
  const auto f4 = ntk.create_maj( f3, b, a );

  ntk.create_po( f2 );
  ntk.create_po( f4 );
  // f2 == f4

  auto vals = simulate<kitty::static_truth_table<4>>( ntk );

  CHECK( ntk.size() == 9 );
  functional_reduction( ntk );
  ntk = cleanup_dangling( ntk );
  CHECK( ntk.size() == 7 );
  CHECK( vals == simulate<kitty::static_truth_table<4>>( ntk ) );
}

TEST_CASE( "functional reduction on XMG", "[functional_reduction]" )
{
  xmg_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  const auto d = ntk.create_pi();

  const auto f1 = ntk.create_xor( a, b );

  const auto f2 = ntk.create_maj( c, f1, d );
  const auto f3 = ntk.create_maj( f2, a, b );

  const auto f4 = ntk.create_maj( a, b, c );
  const auto f5 = ntk.create_maj( a, b, d );
  const auto f6 = ntk.create_maj( f4, f5, f1 );

  ntk.create_po( f3 );
  ntk.create_po( f6 );
  // f3 == f6

  auto vals = simulate<kitty::static_truth_table<4>>( ntk );

  CHECK( ntk.size() == 11 );
  functional_reduction( ntk );
  ntk = cleanup_dangling( ntk );
  CHECK( ntk.size() == 9 );
  CHECK( vals == simulate<kitty::static_truth_table<4>>( ntk ) );
}
