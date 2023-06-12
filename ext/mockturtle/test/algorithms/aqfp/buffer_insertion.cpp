#include <catch.hpp>

#include <lorina/lorina.hpp>
#include <mockturtle/algorithms/aqfp/buffer_insertion.hpp>
#include <mockturtle/algorithms/aqfp/buffer_verification.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "buffer_insertion simple test", "[buffer_insertion]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const e = mig.create_pi();

  auto const f1 = mig.create_maj( a, b, c );
  auto const f2 = mig.create_maj( d, e, f1 );
  auto const f3 = mig.create_maj( a, d, f1 );
  auto const f4 = mig.create_maj( f1, f2, f3 );
  mig.create_po( f4 );

  buffer_insertion_params ps;
  ps.assume.branch_pis = false;
  ps.assume.balance_pis = false;
  ps.assume.balance_pos = true;
  ps.assume.splitter_capacity = 4u;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( mig, ps );
  node_map<uint32_t, mig_network> levels{ mig };
  CHECK( buffering.dry_run( &levels ) == 2u );

  CHECK( levels[f1] == 1u );
  CHECK( levels[f2] == 3u );
  CHECK( levels[f3] == 3u );
  CHECK( levels[f4] == 4u );
  CHECK( buffering.depth() == 4u );
  CHECK( buffering.num_buffers( mig.get_node( f1 ) ) == 2u );
  CHECK( buffering.num_buffers( mig.get_node( f2 ) ) == 0u );
  CHECK( buffering.num_buffers( mig.get_node( f3 ) ) == 0u );
  CHECK( buffering.num_buffers( mig.get_node( f4 ) ) == 0u );
}

TEST_CASE( "two layers of splitters", "[buffer_insertion]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const e = mig.create_pi();
  auto const f = mig.create_pi();
  auto const g = mig.create_pi();
  auto const h = mig.create_pi();
  auto const i = mig.create_pi();
  auto const j = mig.create_pi();

  auto const f1 = mig.create_maj( a, b, c );
  auto const f2 = mig.create_maj( b, c, d );
  auto const f3 = mig.create_maj( d, e, f );
  auto const f4 = mig.create_maj( g, h, i );
  auto const f5 = mig.create_maj( h, i, j );

  auto const f6 = mig.create_maj( f3, f4, f5 );
  auto const f7 = mig.create_maj( a, f1, f2 );
  auto const f8 = mig.create_maj( f2, f3, g );
  auto const f9 = mig.create_maj( f7, f2, f8 );
  auto const f10 = mig.create_maj( f8, f2, f5 );
  auto const f11 = mig.create_maj( f2, f8, f6 );
  auto const f12 = mig.create_maj( f9, f10, f11 );
  mig.create_po( f12 );

  buffer_insertion_params ps;
  ps.assume.branch_pis = false;
  ps.assume.balance_pis = false;
  ps.assume.balance_pos = true;
  ps.assume.splitter_capacity = 4u;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( mig, ps );
  CHECK( buffering.dry_run() == 17u );

  CHECK( buffering.num_buffers( mig.get_node( f2 ) ) == 4u );
  CHECK( buffering.num_buffers( mig.get_node( f6 ) ) == 2u );
  CHECK( buffering.depth() == 7u );
}

TEST_CASE( "PO splitters, buffers and inverters", "[buffer_insertion]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();

  auto const f1 = mig.create_maj( a, b, c );
  auto const f2 = mig.create_maj( f1, c, d );
  mig.create_po( f1 );
  mig.create_po( !f1 );
  mig.create_po( f2 );
  mig.create_po( f2 );
  mig.create_po( !f2 );

  buffer_insertion_params ps;
  ps.assume.branch_pis = false;
  ps.assume.balance_pis = false;
  ps.assume.balance_pos = true;
  ps.assume.splitter_capacity = 4u;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( mig, ps );
  CHECK( buffering.dry_run() == 8u );

  CHECK( buffering.depth() == 5u );
  CHECK( buffering.num_buffers( mig.get_node( f1 ) ) == 5u );
  CHECK( buffering.num_buffers( mig.get_node( f2 ) ) == 3u );

  buffered_mig_network bufntk;
  buffering.dump_buffered_network( bufntk );
  CHECK( verify_aqfp_buffer( bufntk, ps.assume ) == true );
}

TEST_CASE( "chain of fanouts", "[buffer_insertion]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const e = mig.create_pi();
  auto const f = mig.create_pi();
  auto const g = mig.create_pi();
  auto const h = mig.create_pi();
  auto const i = mig.create_pi();

  auto const f1 = mig.create_maj( a, b, c );
  auto const f2 = mig.create_maj( f1, c, d );
  auto const f3 = mig.create_maj( f1, f2, e );
  auto const f4 = mig.create_maj( f1, f2, f );
  auto const f5 = mig.create_maj( f1, f3, f4 );
  auto const f6 = mig.create_maj( f1, f5, f );
  auto const f7 = mig.create_maj( f1, f2, g );
  auto const f8 = mig.create_maj( f1, f7, h );
  auto const f9 = mig.create_maj( f1, f7, i );
  mig.create_po( f1 );
  mig.create_po( f1 );
  mig.create_po( f1 );
  mig.create_po( f1 );
  mig.create_po( f1 );
  mig.create_po( f6 );
  mig.create_po( f8 );
  mig.create_po( f9 );

  buffer_insertion_params ps;
  ps.assume.branch_pis = false;
  ps.assume.balance_pis = false;
  ps.assume.balance_pos = true;
  ps.assume.splitter_capacity = 4u;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( mig, ps );
  CHECK( buffering.dry_run() == 11u );

  CHECK( buffering.num_buffers( mig.get_node( f1 ) ) == 9u );
  CHECK( buffering.depth() == 8u );
}

TEST_CASE( "branch but not balance PIs", "[buffer_insertion]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi(); // shared
  auto const c = mig.create_pi(); // shared
  auto const d = mig.create_pi();
  auto const e = mig.create_pi(); // shared at higher level
  auto const f = mig.create_pi(); // connects to two POs

  auto const f1 = mig.create_maj( a, b, c );
  auto const f2 = mig.create_maj( b, c, d );
  auto const f3 = mig.create_and( f1, e );
  auto const f4 = mig.create_and( f2, e );
  mig.create_po( f3 );
  mig.create_po( f4 );
  mig.create_po( f );
  mig.create_po( f );

  buffer_insertion_params ps;
  ps.assume.branch_pis = true;
  ps.assume.balance_pis = false;
  ps.assume.balance_pos = true;
  ps.assume.splitter_capacity = 4u;
  ps.scheduling = buffer_insertion_params::ALAP;
  ps.optimization_effort = buffer_insertion_params::none;

  buffer_insertion buffering( mig, ps );
  node_map<uint32_t, mig_network> levels{ mig };
  CHECK( buffering.dry_run( &levels ) == 4u );

  CHECK( buffering.level( mig.get_node( f1 ) ) == 2u );
  CHECK( buffering.level( mig.get_node( f2 ) ) == 2u );
  CHECK( buffering.level( mig.get_node( f3 ) ) == 3u );
  CHECK( buffering.level( mig.get_node( f4 ) ) == 3u );

  CHECK( buffering.level( mig.get_node( a ) ) == 1u );
  CHECK( buffering.level( mig.get_node( b ) ) == 0u );
  CHECK( buffering.level( mig.get_node( c ) ) == 0u );
  CHECK( buffering.level( mig.get_node( d ) ) == 1u );
  CHECK( buffering.level( mig.get_node( e ) ) == 1u );
  CHECK( buffering.level( mig.get_node( f ) ) == 2u );

  CHECK( buffering.depth() == 3u );
}

TEST_CASE( "various assumptions", "[buffer_insertion]" )
{
  aig_network aig;
  auto const a = aig.create_pi();
  auto const b = aig.create_pi();
  auto const c = aig.create_pi();
  auto const d = aig.create_pi();
  auto const e = aig.create_pi();

  auto const f1 = aig.create_and( c, d );
  auto const f2 = aig.create_or( c, d );
  auto const f3 = aig.create_and( d, e );
  auto const f4 = aig.create_and( f2, f3 );

  aig.create_po( aig.get_constant( false ) ); // const -- PO
  aig.create_po( a );                         // PI -- PO
  aig.create_po( b );
  aig.create_po( b );
  aig.create_po( b ); // PI -- buffer tree -- PO
  aig.create_po( f1 );
  aig.create_po( f3 );
  aig.create_po( f4 );

  aqfp_assumptions asp;
  asp.splitter_capacity = 2u;

  buffer_insertion_params ps;
  ps.scheduling = buffer_insertion_params::ASAP;
  ps.optimization_effort = buffer_insertion_params::none;

  /* branch PI, balance PI and PO */
  asp.branch_pis = true;
  asp.balance_pis = true;
  asp.balance_pos = true;
  ps.assume = asp;
  {
    buffer_insertion buffering( aig, ps );
    buffered_aig_network buffered;
    CHECK( buffering.run( buffered ) == 23u );
    CHECK( verify_aqfp_buffer( buffered, asp ) == true );
  }

  /* branch PI, balance only PI */
  asp.branch_pis = true;
  asp.balance_pis = true;
  asp.balance_pos = false;
  ps.assume = asp;
  {
    buffer_insertion buffering( aig, ps );
    buffered_aig_network buffered;
    CHECK( buffering.run( buffered ) == 11u );
    CHECK( verify_aqfp_buffer( buffered, asp ) == true );
  }

  /* branch PI, balance only PO */
  asp.branch_pis = true;
  asp.balance_pis = false;
  asp.balance_pos = true;
  ps.assume = asp;
  {
    ps.scheduling = buffer_insertion_params::ASAP;
    buffer_insertion buffering1( aig, ps );
    buffered_aig_network buffered1;
    CHECK( buffering1.run( buffered1 ) == 23u );
    CHECK( verify_aqfp_buffer( buffered1, asp ) == true );

    ps.scheduling = buffer_insertion_params::ALAP;
    buffer_insertion buffering2( aig, ps );
    buffered_aig_network buffered2;
    CHECK( buffering2.run( buffered2 ) == 11u );
    CHECK( verify_aqfp_buffer( buffered2, asp ) == true );

    ps.scheduling = buffer_insertion_params::ASAP_depth;
    buffer_insertion buffering3( aig, ps );
    buffered_aig_network buffered3;
    CHECK( buffering3.run( buffered3 ) == 17u );
    CHECK( verify_aqfp_buffer( buffered3, asp ) == true );

    ps.scheduling = buffer_insertion_params::ALAP_depth;
    buffer_insertion buffering4( aig, ps );
    buffered_aig_network buffered4;
    CHECK( buffering4.run( buffered4 ) == 10u );
    CHECK( verify_aqfp_buffer( buffered4, asp ) == true );
  }

  /* branch PI, balance neither */
  asp.branch_pis = true;
  asp.balance_pis = false;
  asp.balance_pos = false;
  ps.assume = asp;
  {
    ps.scheduling = buffer_insertion_params::ASAP;
    buffer_insertion buffering1( aig, ps );
    buffered_aig_network buffered1;
    CHECK( buffering1.run( buffered1 ) == 11u );
    CHECK( verify_aqfp_buffer( buffered1, asp ) == true );

    ps.scheduling = buffer_insertion_params::ALAP;
    buffer_insertion buffering2( aig, ps );
    buffered_aig_network buffered2;
    CHECK( buffering2.run( buffered2 ) == 9u );
    CHECK( verify_aqfp_buffer( buffered2, asp ) == true );

    ps.scheduling = buffer_insertion_params::ASAP_depth;
    buffer_insertion buffering3( aig, ps );
    buffered_aig_network buffered3;
    CHECK( buffering3.run( buffered3 ) == 8u );
    CHECK( verify_aqfp_buffer( buffered3, asp ) == true );

    ps.scheduling = buffer_insertion_params::ALAP_depth;
    buffer_insertion buffering4( aig, ps );
    buffered_aig_network buffered4;
    CHECK( buffering4.run( buffered4 ) == 8u );
    CHECK( verify_aqfp_buffer( buffered4, asp ) == true );
  }

  /* don't branch PI, balance PO */
  asp.branch_pis = false;
  asp.balance_pis = false;
  asp.balance_pos = true;
  ps.assume = asp;
  {
    ps.scheduling = buffer_insertion_params::ASAP;
    buffer_insertion buffering1( aig, ps );
    buffered_aig_network buffered1;
    CHECK( buffering1.run( buffered1 ) == 5u );
    CHECK( verify_aqfp_buffer( buffered1, asp ) == true );

    ps.scheduling = buffer_insertion_params::ASAP_depth;
    buffer_insertion buffering2( aig, ps );
    buffered_aig_network buffered2;
    CHECK( buffering2.run( buffered2 ) == 5u );
    CHECK( verify_aqfp_buffer( buffered2, asp ) == true );
  }

  /* don't branch PI, balance neither */
  asp.branch_pis = false;
  asp.balance_pis = false;
  asp.balance_pos = false;
  ps.assume = asp;
  {
    ps.scheduling = buffer_insertion_params::ASAP;
    buffer_insertion buffering1( aig, ps );
    buffered_aig_network buffered1;
    CHECK( buffering1.run( buffered1 ) == 2u );
    CHECK( verify_aqfp_buffer( buffered1, asp ) == true );

    ps.scheduling = buffer_insertion_params::ASAP_depth;
    buffer_insertion buffering2( aig, ps );
    buffered_aig_network buffered2;
    CHECK( buffering2.run( buffered2 ) == 2u );
    CHECK( verify_aqfp_buffer( buffered2, asp ) == true );
  }
}

#ifndef _MSC_VER
TEST_CASE( "optimization with chunked movement", "[buffer_insertion]" )
{
  aig_network aig_ntk;
  buffered_aig_network buffered_ntk;
  auto const read = lorina::read_aiger( fmt::format( "{}/c432.aig", BENCHMARKS_PATH ), aiger_reader( aig_ntk ) );
  CHECK( read == lorina::return_code::success );

  buffer_insertion_params ps;
  ps.scheduling = buffer_insertion_params::better;
  ps.optimization_effort = buffer_insertion_params::one_pass;
  buffer_insertion buffering( aig_ntk, ps );

  buffering.ASAP();
  buffering.count_buffers();
  auto const num_buf_asap = buffering.num_buffers();
  auto const num_buf_opt = buffering.run( buffered_ntk );

  CHECK( verify_aqfp_buffer( buffered_ntk, ps.assume ) == true );
  CHECK( num_buf_opt < num_buf_asap );
}
#endif
