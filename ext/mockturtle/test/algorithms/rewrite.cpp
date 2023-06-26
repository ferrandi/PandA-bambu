#include <catch.hpp>

#include <mockturtle/algorithms/rewrite.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/fanout_view.hpp>

using namespace mockturtle;

TEST_CASE( "Rewrite bad MAJ", "[rewrite]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  mig_npn_resynthesis resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<mig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( mig, exact_lib );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "Rewrite XMG3 using a 4-input npn database", "[rewrite]" )
{
  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  const auto h = xmg.create_xor3( a, xmg.create_maj( a, b, c ), c );

  xmg.create_po( h );

  xmg3_npn_resynthesis<xmg_network> resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<xmg_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( xmg, exact_lib );

  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_pis() == 3 );
  CHECK( xmg.num_pos() == 1 );
  CHECK( xmg.num_gates() == 1 );
}

TEST_CASE( "Rewrite constant", "[rewrite]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( false ) );

  mig_npn_resynthesis resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<mig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( mig, exact_lib );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( false ) );
  } );
}

TEST_CASE( "Rewrite inverted constant", "[rewrite]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( true ) );

  mig_npn_resynthesis resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<mig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( mig, exact_lib );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( true ) );
  } );
}

TEST_CASE( "Rewrite projection", "[rewrite]" )
{
  mig_network mig;
  mig.create_po( mig.create_pi() );

  mig_npn_resynthesis resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<mig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( mig, exact_lib );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( !mig.is_complemented( f ) );
  } );
}

TEST_CASE( "Rewrite inverted projection", "[rewrite]" )
{
  mig_network mig;
  mig.create_po( !mig.create_pi() );

  mig_npn_resynthesis resyn;
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<mig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( mig, exact_lib );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( mig.is_complemented( f ) );
  } );
}

TEST_CASE( "Rewrite should avoid cycles", "[rewrite]" )
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
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<aig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite( aig, exact_lib );

  CHECK( aig.size() == 11 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 7 );
}

TEST_CASE( "Rewrite depth-preserving", "[rewrite]" )
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
  exact_library_params eps;
  eps.np_classification = false;
  exact_library<aig_network, decltype( resyn )> exact_lib( resyn, eps );

  rewrite_params ps;
  ps.preserve_depth = true;
  rewrite( aig, exact_lib, ps );

  CHECK( aig.size() == 12 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 8 );
}
