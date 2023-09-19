#include <catch.hpp>

#include <lorina/lorina.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_retiming.hpp>
#include <mockturtle/algorithms/aqfp/buffer_insertion.hpp>
#include <mockturtle/algorithms/aqfp/buffer_verification.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "aqfp retiming simple test", "[aqfp_retiming]" )
{
  buffered_aqfp_network aqfp;
  auto const a = aqfp.create_pi();
  auto const b = aqfp.create_pi();
  auto const c = aqfp.create_pi();

  auto const b1 = aqfp.create_buf( a );
  auto const b2 = aqfp.create_buf( b );
  auto const b3 = aqfp.create_buf( c );

  auto const f1 = aqfp.create_maj( b1, b2, b3 );
  aqfp.create_po( f1 );

  aqfp_assumptions aqfp_ps;
  aqfp_ps.splitter_capacity = 3;
  aqfp_ps.branch_pis = true;
  aqfp_ps.balance_pis = true;
  aqfp_ps.balance_pos = true;

  aqfp_retiming_params ps;
  aqfp_retiming_stats st;
  ps.aqfp_assumptions_ps = aqfp_ps;
  auto aqfp_ret = aqfp_retiming( aqfp, ps, &st );

  CHECK( st.buffers_pre == 3 );
  CHECK( st.buffers_post == 0 );
}

TEST_CASE( "aqfp retiming not retimeable", "[aqfp_retiming]" )
{
  buffered_aqfp_network aqfp;
  auto const a = aqfp.create_pi();
  auto const b = aqfp.create_pi();
  auto const c = aqfp.create_pi();

  auto const b1 = aqfp.create_buf( a );
  auto const b2 = aqfp.create_buf( b );
  auto const b3 = aqfp.create_buf( c );

  auto const f1 = aqfp.create_maj( b1, b2, b3 );
  auto const f2 = aqfp.create_and( b1, b2 );
  aqfp.create_po( f1 );
  aqfp.create_po( f2 );

  aqfp_assumptions aqfp_ps;
  aqfp_ps.splitter_capacity = 3;
  aqfp_ps.branch_pis = true;
  aqfp_ps.balance_pis = true;
  aqfp_ps.balance_pos = true;

  aqfp_retiming_params ps;
  aqfp_retiming_stats st;
  ps.aqfp_assumptions_ps = aqfp_ps;
  auto aqfp_ret = aqfp_retiming( aqfp, ps, &st );

  CHECK( st.buffers_pre == 3 );
  CHECK( st.buffers_post == 3 );
}

TEST_CASE( "aqfp retiming", "[aqfp_retiming]" )
{
  aqfp_network aqfp;
  auto const a = aqfp.create_pi();
  auto const b = aqfp.create_pi();
  auto const c = aqfp.create_pi();
  auto const d = aqfp.create_pi();
  auto const e = aqfp.create_pi();
  auto const f = aqfp.create_pi();
  auto const g = aqfp.create_pi();
  auto const h = aqfp.create_pi();
  auto const i = aqfp.create_pi();

  auto const f1 = aqfp.create_maj( a, b, c );
  auto const f2 = aqfp.create_maj( f1, c, d );
  auto const f3 = aqfp.create_maj( f1, f2, e );
  auto const f4 = aqfp.create_maj( f1, f2, f );
  auto const f5 = aqfp.create_maj( f1, f3, f4 );
  auto const f6 = aqfp.create_maj( f1, f5, f );
  auto const f7 = aqfp.create_maj( f1, f2, g );
  auto const f8 = aqfp.create_maj( f1, f7, h );
  auto const f9 = aqfp.create_maj( f1, f7, i );
  aqfp.create_po( f1 );
  aqfp.create_po( f1 );
  aqfp.create_po( f1 );
  aqfp.create_po( f1 );
  aqfp.create_po( f1 );
  aqfp.create_po( f6 );
  aqfp.create_po( f8 );
  aqfp.create_po( f9 );

  aqfp_assumptions asp;
  asp.splitter_capacity = 4u;
  asp.branch_pis = true;
  asp.balance_pis = true;
  asp.balance_pos = true;

  buffer_insertion_params ps;
  ps.assume = asp;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( aqfp, ps );
  buffered_aqfp_network buffered_aqfp;
  CHECK( buffering.run( buffered_aqfp ) == 57 );
  CHECK( buffering.depth() == 9u );

  aqfp_retiming_params rps;
  aqfp_retiming_stats rst;
  rps.aqfp_assumptions_ps = asp;
  auto aqfp_ret = aqfp_retiming( buffered_aqfp, rps, &rst );

  CHECK( rst.buffers_pre == 57 );
  CHECK( rst.buffers_post == 49 );
  CHECK( verify_aqfp_buffer( aqfp_ret, asp ) == true );
}
