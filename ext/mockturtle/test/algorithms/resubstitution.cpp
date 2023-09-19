#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/mig_resub.hpp>
#include <mockturtle/algorithms/sim_resub.hpp>
#include <mockturtle/algorithms/xag_resub_withDC.hpp>
#include <mockturtle/algorithms/xmg_resub.hpp>

#include <kitty/static_truth_table.hpp>

using namespace mockturtle;

TEST_CASE( "Resubstitution of AIGs", "[resubstitution]" )
{
  /* The test cases are (a,b)-pairs, where a is an index_list and b is
     the expected improvement after resubstitution. */
  using value_type = std::vector<uint32_t>;
  std::vector<std::pair<value_type, uint32_t>> const test_cases{
#include "aig_resubstitution.tc"
  };

  for ( const auto& tc : test_cases )
  {
    aig_network aig;
    decode( aig, xag_index_list{ tc.first } );

    uint64_t const size_before{ aig.size() };

    default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
    auto const tt = simulate<kitty::dynamic_truth_table>( aig, sim )[0];
    using view_t = depth_view<fanout_view<aig_network>>;
    fanout_view<aig_network> fanout_view{ aig };
    view_t resub_view{ fanout_view };

    aig_resubstitution( resub_view );
    aig = cleanup_dangling( aig );

    auto const tt_prime = simulate<kitty::dynamic_truth_table>( aig, sim )[0];

    CHECK( size_before - aig.size() == tc.second );
    CHECK( tt == tt_prime );
  }
}

TEST_CASE( "Replace with constant in AIG", "[resubstitution]" )
{
  /* x1 * ( !x0 * !x1 ) ==> 0 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto n0 = aig.create_and( !x0, !x1 );
  auto n1 = aig.create_and( x1, n0 );
  aig.create_po( n1 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x0 );
  CHECK( aig.num_gates() == 2u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 0u );
}

TEST_CASE( "Merge equivalent nodes in AIG", "[resubstitution]" )
{
  /* x0 * !( !x0 * !x1 ) ==> x0 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto n0 = aig.create_and( !x0, !x1 );
  auto n1 = aig.create_and( x0, !n0 );
  aig.create_po( n1 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 10 );
  CHECK( aig.num_gates() == 2u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 0u );
}

TEST_CASE( "Substitute AND with positive divisor in AIG", "[resubstitution]" )
{
  /* x1 * ( x0 * x1 ) ==> x0 * x1 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto n0 = aig.create_and( x0, x1 );
  auto n1 = aig.create_and( x1, n0 );
  aig.create_po( n1 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x8 );
  CHECK( aig.num_gates() == 2u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Substitute AND with negative divisor in AIG", "[resubstitution]" )
{
  /* !x0 * !( !x0 * !x1 ) ==> !x0 * x1 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto n0 = aig.create_and( !x0, !x1 );
  auto n1 = aig.create_and( !x0, !n0 );
  aig.create_po( n1 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x4 );
  CHECK( aig.num_gates() == 2u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Substitute OR with positive divisors in AIG", "[resubstitution]" )
{
  /* !x1 * !( x0 * !x1 ) ==> ( !x0 * !x1 ) */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto n0 = aig.create_and( x0, !x1 );
  auto n1 = aig.create_and( !x1, !n0 );
  aig.create_po( n1 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x1 );
  CHECK( aig.num_gates() == 2u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Substitute OR-OR in AIG", "[resubstitution]" )
{
  /* !( x0 * !x2 ) * ( !x1 * !x2 ) ==> ( !x0 * !x1 ) * !x2 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto x2 = aig.create_pi();
  auto n0 = aig.create_and( x0, !x2 );
  auto n1 = aig.create_and( !x1, !x2 );
  auto n2 = aig.create_and( !n0, n1 );
  aig.create_po( n2 );

  const auto tt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt._bits == 0x1 );
  CHECK( aig.num_gates() == 3u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 2u );
}

TEST_CASE( "Substitute OR-AND in AIG", "[resubstitution]" )
{
  /* !x2 * ( !x2 * !( !x0 * x1 ) ) ==> !( !x0 * x1 ) * !x2 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto x2 = aig.create_pi();
  auto n0 = aig.create_and( !x0, x1 );
  auto n1 = aig.create_and( !x2, !n0 );
  auto n2 = aig.create_and( !x2, n1 );
  aig.create_po( n2 );

  const auto tt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt._bits == 0xb );
  CHECK( aig.num_gates() == 3u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  aig_resubstitution( resub_view );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 2u );
}

TEST_CASE( "Substitute AND-2OR in AIG", "[resubstitution]" )
{
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto x2 = aig.create_pi();
  auto x3 = aig.create_pi();
  auto n0 = aig.create_and( !x2, x3 );
  auto n1 = aig.create_and( !x2, n0 );
  auto n2 = aig.create_and( x3, !n1 );
  auto n3 = aig.create_and( x0, !x1 );
  auto n4 = aig.create_and( !n2, n3 );
  auto n5 = aig.create_and( x1, !n2 );
  auto n6 = aig.create_and( !n4, !n5 );
  auto n7 = aig.create_and( n1, n3 );
  aig.create_po( n6 );
  aig.create_po( n7 );

  const auto tt = simulate<kitty::static_truth_table<4u>>( aig );
  CHECK( tt[0]._bits == 0xf111 );
  CHECK( tt[1]._bits == 0x200 );
  CHECK( aig.num_gates() == 8u );

  using view_t = depth_view<fanout_view<aig_network>>;
  fanout_view<aig_network> fanout_view{ aig };
  view_t resub_view{ fanout_view };

  resubstitution_params ps;
  ps.max_inserts = 3;
  aig_resubstitution( resub_view, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<4u>>( aig );
  CHECK( tt_opt[0]._bits == tt[0]._bits );
  CHECK( tt_opt[1]._bits == tt[1]._bits );
  CHECK( aig.num_gates() == 6u );
}

TEST_CASE( "Resubstitution of MIG", "[resubstitution]" )
{
  mig_network mig;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  CHECK( mig.size() == 6 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 2 );

  const auto tt = simulate<kitty::static_truth_table<3u>>( mig )[0];
  CHECK( tt._bits == 0xe8 );

  using view_t = depth_view<fanout_view<mig_network>>;
  fanout_view<mig_network> fanout_view{ mig };
  view_t resub_view{ fanout_view };

  mig_resubstitution( resub_view );

  mig = cleanup_dangling( mig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( mig )[0];
  CHECK( tt_opt._bits == tt._bits );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "Resubstitution of XMG", "[resubstitution]" )
{
  xmg_network xmg;

  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  const auto g = xmg.create_maj( a, xmg.create_maj( a, b, c ), c );
  const auto h = xmg.create_xor3( a, xmg.create_xor3( a, b, c ), c );
  xmg.create_po( g );
  xmg.create_po( h );

  CHECK( xmg.size() == 8 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 2 );
  CHECK( xmg.num_gates() == 4 );

  const auto tt1 = simulate<kitty::static_truth_table<3u>>( xmg )[0];
  const auto tt2 = simulate<kitty::static_truth_table<3u>>( xmg )[1];
  CHECK( tt1._bits == 0xe8 );
  CHECK( tt2._bits == 0xcc );

  using view_t = depth_view<fanout_view<xmg_network>>;
  fanout_view<xmg_network> fanout_view{ xmg };
  view_t resub_view{ fanout_view };

  xmg_resubstitution( resub_view );

  xmg = cleanup_dangling( xmg );

  /* check equivalence */
  const auto tt_opt1 = simulate<kitty::static_truth_table<3u>>( xmg )[0];
  const auto tt_opt2 = simulate<kitty::static_truth_table<3u>>( xmg )[1];
  CHECK( tt_opt1._bits == tt1._bits );
  CHECK( tt_opt2._bits == tt2._bits );

  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 2 );
  CHECK( xmg.num_gates() == 1 );
}

TEST_CASE( "Resubstitution of XAG to minimize ANDs", "[resubstitution]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  const auto f = xag.create_xor( xag.create_or( xag.create_and( a, xag.create_not( b ) ), xag.create_and( b, xag.create_not( a ) ) ), c );
  xag.create_po( f );

  CHECK( xag.size() == 8 );
  CHECK( xag.num_pis() == 3 );
  CHECK( xag.num_pos() == 1 );
  CHECK( xag.num_gates() == 4 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( xag )[0];

  resubstitution_params ps;

  using view_t = depth_view<fanout_view<xag_network>>;
  fanout_view<xag_network> fanout_view{ xag };
  view_t resub_view{ fanout_view };
  resubstitution_minmc_withDC( resub_view, ps );

  xag = cleanup_dangling( xag );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( xag )[0];
  CHECK( tt_opt._bits == tt._bits );

  CHECK( xag.size() == 6 );
  CHECK( xag.num_pis() == 3 );
  CHECK( xag.num_pos() == 1 );
  CHECK( xag.num_gates() == 2 );
}

TEST_CASE( "Simulation-guided resubstitution", "[resubstitution]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f = aig.create_and( a, aig.create_and( b, a ) );
  aig.create_po( f );

  CHECK( aig.size() == 5 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 2 );

  const auto tt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt._bits == 0x8 );

  sim_resubstitution( aig );

  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );

  CHECK( aig.size() == 4 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 1 );
}
