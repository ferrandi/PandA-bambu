#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/experimental/boolean_optimization.hpp>
#include <mockturtle/algorithms/experimental/sim_resub.hpp>
#include <mockturtle/algorithms/experimental/window_resub.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>

using namespace mockturtle;
using namespace mockturtle::experimental;

TEST_CASE( "Enumerative 0-resub", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 0u );
}

TEST_CASE( "Enumerative 1-resub: AND", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Enumerative 1-resub: AND-INV", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Enumerative 1-resub: OR", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<2u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 1u );
}

TEST_CASE( "Enumerative 2-resub: OR-OR", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 2u );
}

TEST_CASE( "Enumerative 2-resub: AND-AND", "[aig_resyn]" )
{
  /* ( x0 * x1 ) * ( x0 * x2 ) ==> ( x0 * x1 ) * x2 */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto x2 = aig.create_pi();
  auto n0 = aig.create_and( x0, x1 );
  auto n1 = aig.create_and( x0, x2 );
  auto n2 = aig.create_and( n0, n1 );
  aig.create_po( n2 );

  const auto tt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt._bits == 0b10000000 );
  CHECK( aig.num_gates() == 3u );

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 2u );
}

TEST_CASE( "Enumerative 2-resub: OR-AND", "[aig_resyn]" )
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

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<3u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 2u );
}

TEST_CASE( "Enumerative 3-resub: AND-2OR", "[aig_resyn]" )
{
  /* ( (x0 * x2) + (x1 * x3) ) + ( (x0 * x3) + (x1 * x2) ) ==> ( x0 + x1 ) * (x2 + x3) */
  aig_network aig;
  auto x0 = aig.create_pi();
  auto x1 = aig.create_pi();
  auto x2 = aig.create_pi();
  auto x3 = aig.create_pi();
  auto n0 = aig.create_and( x0, x2 );
  auto n1 = aig.create_and( x1, x3 );
  auto n2 = aig.create_and( x0, x3 );
  auto n3 = aig.create_and( x1, x2 );
  auto n4 = aig.create_and( !n0, !n1 );
  auto n5 = aig.create_and( !n2, !n3 );
  auto n6 = aig.create_and( n4, n5 );
  aig.create_po( !n6 );

  const auto tt = simulate<kitty::static_truth_table<4u>>( aig )[0];
  CHECK( tt._bits == 0xeee0 );
  CHECK( aig.num_gates() == 7u );

  window_resub_params ps;
  ps.wps.max_inserts = 3;

  window_aig_enumerative_resub( aig, ps );
  aig = cleanup_dangling( aig );

  /* check equivalence */
  const auto tt_opt = simulate<kitty::static_truth_table<4u>>( aig )[0];
  CHECK( tt_opt._bits == tt._bits );
  CHECK( aig.num_gates() == 3u );
}
