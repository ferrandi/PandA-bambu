#include <catch.hpp>

#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

TEST_CASE( "check equivalence of two XAGs as XAG miter", "[miter]" )
{
  xag_network xag1;
  const auto x1 = xag1.create_pi();
  const auto x2 = xag1.create_pi();
  const auto f1 = xag1.create_nand( x1, x2 );
  const auto f2 = xag1.create_nand( x1, f1 );
  const auto f3 = xag1.create_nand( x2, f1 );
  const auto f4 = xag1.create_nand( f2, f3 );
  xag1.create_po( f4 );

  xag_network xag2;
  const auto y1 = xag2.create_pi();
  const auto y2 = xag2.create_pi();
  const auto g1 = xag2.create_xor( y1, y2 );
  xag2.create_po( g1 );

  auto miter_ntk = miter<xag_network>( xag1, xag2 );

  CHECK( miter_ntk );

  CHECK( simulate<kitty::static_truth_table<2u>>( *miter_ntk )[0]._bits == 0b0000 );
}

TEST_CASE( "check equivalence of AIG and XAG as XMG miter", "[miter]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  xag_network xag;
  const auto y1 = xag.create_pi();
  const auto y2 = xag.create_pi();
  const auto g1 = xag.create_xor( y1, y2 );
  xag.create_po( g1 );

  auto miter_ntk = miter<xmg_network>( aig, xag );

  CHECK( miter_ntk );

  CHECK( simulate<kitty::static_truth_table<2u>>( *miter_ntk )[0]._bits == 0b0000 );
}

TEST_CASE( "check equivalence of AIG and XAG as k-LUT miter", "[miter]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  xag_network xag;
  const auto y1 = xag.create_pi();
  const auto y2 = xag.create_pi();
  const auto g1 = xag.create_xor( y1, y2 );
  xag.create_po( g1 );

  auto miter_ntk = miter<klut_network>( aig, xag );

  CHECK( miter_ntk );

  CHECK( simulate<kitty::static_truth_table<2u>>( *miter_ntk )[0]._bits == 0b0000 );
}
