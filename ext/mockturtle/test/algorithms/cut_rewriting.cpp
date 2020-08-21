#include <catch.hpp>

#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_minmc2.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "In-place cut rewriting of bad MAJ", "[cut_rewriting]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  mig_npn_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "In-place cut rewriting with XMG3 4-input npn database", "[cut_rewriting]" )
{

  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  const auto h = xmg.create_xor3( a, xmg.create_maj( a, b, c ), c );

  xmg.create_po( h );
  xmg3_npn_resynthesis<xmg_network> resyn;
  cut_rewriting_with_compatibility_graph( xmg, resyn );

  xmg = cleanup_dangling( xmg );
  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 1 );
  CHECK( xmg.num_gates() == 1 );
}

TEST_CASE( "Cut rewriting with Akers synthesis", "[cut_rewriting]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  akers_resynthesis<mig_network> resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "In-place cut rewriting from constant", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( false ) );

  mig_npn_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( false ) );
  } );
}

TEST_CASE( "In-place cut rewriting from inverted constant", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( true ) );

  mig_npn_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( true ) );
  } );
}

TEST_CASE( "In-place cut rewriting from projection", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.create_pi() );

  mig_npn_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( !mig.is_complemented( f ) );
  } );
}

TEST_CASE( "In-place cut rewriting from inverted projection", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( !mig.create_pi() );

  mig_npn_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( mig.is_complemented( f ) );
  } );
}

TEST_CASE( "In-place cut rewriting with exact LUT synthesis", "[cut_rewriting]" )
{
  klut_network klut;
  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto d = klut.create_pi();

  klut.create_po( klut.create_and( a, klut.create_and( b, klut.create_and( c, d ) ) ) );

  CHECK( klut.num_pis() == 4u );
  CHECK( klut.num_pos() == 1u );
  CHECK( klut.num_gates() == 3u );

  exact_resynthesis resyn( 3u );
  cut_rewriting_with_compatibility_graph( klut, resyn );

  klut = cleanup_dangling( klut );

  CHECK( klut.num_pis() == 4u );
  CHECK( klut.num_pos() == 1u );
  CHECK( klut.num_gates() == 2u );
}

TEST_CASE( "In-place cut rewriting with alternative costs", "[cut_rewriting]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  const auto f = xag.create_or( xag.create_or( xag.create_and( a, b ), xag.create_and( b, c ) ), xag.create_and( a, c ) );
  xag.create_po( f );

  future::xag_minmc_resynthesis resyn;
  cut_rewriting_with_compatibility_graph( xag, resyn, {}, nullptr, mc_cost<xag_network>() );

  xag = cleanup_dangling( xag );

  CHECK( xag.size() == 8 );
  CHECK( xag.num_pis() == 3 );
  CHECK( xag.num_pos() == 1 );
  CHECK( *multiplicative_complexity( xag ) == 1 );
}

TEST_CASE( "Cut rewriting of bad MAJ", "[cut_rewriting]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  mig_npn_resynthesis resyn;
  mig = cut_rewriting( mig, resyn );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "Cut rewriting with XMG3 4-input npn database", "[cut_rewriting]" )
{

  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  const auto h = xmg.create_xor3( a, xmg.create_maj( a, b, c ), c );

  xmg.create_po( h );
  xmg3_npn_resynthesis<xmg_network> resyn;
  xmg = cut_rewriting( xmg, resyn );

  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 1 );
  CHECK( xmg.num_gates() == 1 );
}

TEST_CASE( "Cut rewriting from constant", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( false ) );

  mig_npn_resynthesis resyn;
  mig = cut_rewriting( mig, resyn );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( false ) );
  } );
}

TEST_CASE( "Cut rewriting from inverted constant", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( true ) );

  mig_npn_resynthesis resyn;
  mig = cut_rewriting( mig, resyn );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( true ) );
  } );
}

TEST_CASE( "Cut rewriting from projection", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( mig.create_pi() );

  mig_npn_resynthesis resyn;
  mig = cut_rewriting( mig, resyn );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( !mig.is_complemented( f ) );
  } );
}

TEST_CASE( "Cut rewriting from inverted projection", "[cut_rewriting]" )
{
  mig_network mig;
  mig.create_po( !mig.create_pi() );

  mig_npn_resynthesis resyn;
  mig = cut_rewriting( mig, resyn );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( mig.is_complemented( f ) );
  } );
}

TEST_CASE( "Cut rewriting with exact LUT synthesis", "[cut_rewriting]" )
{
  klut_network klut;
  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto d = klut.create_pi();

  klut.create_po( klut.create_and( a, klut.create_and( b, klut.create_and( c, d ) ) ) );

  CHECK( klut.num_pis() == 4u );
  CHECK( klut.num_pos() == 1u );
  CHECK( klut.num_gates() == 3u );

  exact_resynthesis resyn( 3u );
  klut = cut_rewriting( klut, resyn );

  CHECK( klut.num_pis() == 4u );
  CHECK( klut.num_pos() == 1u );
  CHECK( klut.num_gates() == 2u );
}

TEST_CASE( "Cut rewriting with alternative costs", "[cut_rewriting]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  const auto f = xag.create_or( xag.create_or( xag.create_and( a, b ), xag.create_and( b, c ) ), xag.create_and( a, c ) );
  xag.create_po( f );

  future::xag_minmc_resynthesis resyn;
  xag = cut_rewriting<xag_network, decltype( resyn ), mc_cost<xag_network>>( xag, resyn, {}, nullptr );

  CHECK( xag.size() == 8 );
  CHECK( xag.num_pis() == 3 );
  CHECK( xag.num_pos() == 1 );
  CHECK( *multiplicative_complexity( xag ) == 1 );
}

TEST_CASE( "Cut rewriting should avoid cycles", "[cut_rewriting]" )
{
  aig_network aig;
  const auto x0 = aig.create_pi();
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto n0 = aig.create_and( x1, !x2 );
  const auto n1 = aig.create_and( !x0, n0 );
  const auto n2 = aig.create_and( x0, !n0 );
  const auto n3 = aig.create_and( !n1, !n2 );
  const auto n4 = aig.create_and( x1, x2 );
  const auto n5 = aig.create_and( x0, !n4 );
  const auto n6 = aig.create_and( !x0, n4 );
  const auto n7 = aig.create_and( !n5, !n6 );
  aig.create_po( n3 );
  aig.create_po( n7 );

  xag_npn_resynthesis<aig_network> resyn;
  cut_rewriting_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  aig = cut_rewriting( aig, resyn, ps );

  CHECK( aig.size() == 12 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 8 );
}

TEST_CASE( "Cut rewriting with stacked fanout-depth views", "[cut_rewriting]" )
{
  aig_network aig;
  const auto x0 = aig.create_pi();
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto n0 = aig.create_and( x1, !x2 );
  const auto n1 = aig.create_and( !x0, n0 );
  const auto n2 = aig.create_and( x0, !n0 );
  const auto n3 = aig.create_and( !n1, !n2 );
  const auto n4 = aig.create_and( x1, x2 );
  const auto n5 = aig.create_and( x0, !n4 );
  const auto n6 = aig.create_and( !x0, n4 );
  const auto n7 = aig.create_and( !n5, !n6 );
  aig.create_po( n3 );
  aig.create_po( n7 );

  /* cut_rewriting of fanout_view of depth_view of aig_network */
  using cost_fn = unit_cost<aig_network>;
  using resyn_fn = xag_npn_resynthesis<aig_network>;

  resyn_fn resyn;
  fanout_view fanout_aig{aig};
  depth_view<fanout_view<aig_network>, cost_fn> depth_aig{fanout_aig};

  cut_rewriting_params ps;
  cut_rewriting_stats st;
  aig = detail::cut_rewriting_impl<aig_network, decltype( depth_aig ), resyn_fn, cost_fn>( depth_aig, resyn, ps, st ).run();
  CHECK( aig.size() == 12 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 8 );
}

TEST_CASE( "Cut rewriting with stacked depth-fanout views", "[cut_rewriting]" )
{
  aig_network aig;
  const auto x0 = aig.create_pi();
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto n0 = aig.create_and( x1, !x2 );
  const auto n1 = aig.create_and( !x0, n0 );
  const auto n2 = aig.create_and( x0, !n0 );
  const auto n3 = aig.create_and( !n1, !n2 );
  const auto n4 = aig.create_and( x1, x2 );
  const auto n5 = aig.create_and( x0, !n4 );
  const auto n6 = aig.create_and( !x0, n4 );
  const auto n7 = aig.create_and( !n5, !n6 );
  aig.create_po( n3 );
  aig.create_po( n7 );

  /* cut_rewriting of fanout_view of depth_view of aig_network */
  using cost_fn = unit_cost<aig_network>;
  using resyn_fn = xag_npn_resynthesis<aig_network>;

  resyn_fn resyn;
  fanout_view fanout_aig{aig};
  depth_view<fanout_view<aig_network>, cost_fn> depth_aig{fanout_aig};

  cut_rewriting_params ps;
  cut_rewriting_stats st;
  aig = detail::cut_rewriting_impl<aig_network, decltype( depth_aig ), resyn_fn, cost_fn>( depth_aig, resyn, ps, st ).run();
  CHECK( aig.size() == 12 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 8 );
}
