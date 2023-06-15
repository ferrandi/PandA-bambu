#include <catch.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/dont_care_view.hpp>
#include <mockturtle/algorithms/sim_resub.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/miter.hpp>

#include <vector>
#include <fstream>
#include <iostream>

using namespace mockturtle;

template<typename Ntk>
void test_create_dont_care_view()
{
  /* traits */
  CHECK( has_EXCDC_interface_v<Ntk> == false );
  CHECK( has_EXODC_interface_v<Ntk> == false );

  using dc_view_cdc = dont_care_view<Ntk, true, false>;
  using dc_view_odc = dont_care_view<Ntk, false, true>;
  using dc_view_both = dont_care_view<Ntk, true, true>;

  CHECK( has_EXCDC_interface_v<dc_view_cdc> == true );
  CHECK( has_EXODC_interface_v<dc_view_cdc> == false );

  CHECK( has_EXCDC_interface_v<dc_view_odc> == false );
  CHECK( has_EXODC_interface_v<dc_view_odc> == true );

  CHECK( has_EXCDC_interface_v<dc_view_both> == true );
  CHECK( has_EXODC_interface_v<dc_view_both> == true );

  /* creation and basic properties */
  Ntk ntk;
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_and( b, c );
  auto const f = ntk.create_and( t1, t2 );
  ntk.create_po( t1 );
  ntk.create_po( t2 );
  ntk.create_po( f );

  Ntk cdc;
  cdc.create_pi();
  cdc.create_pi();
  cdc.create_pi();
  cdc.create_po( cdc.create_and( cdc.create_and( a, b ), c ) );
  /* The input pattern a = b = c = 1 is EXCDC */

  std::vector<bool> pat1{1, 1, 1};
  std::vector<bool> pat2{1, 1, 0};
  std::vector<bool> pat3{0, 0, 0};

  kitty::cube cube1( "1--" );
  kitty::cube cube2( "1-0" );
  kitty::cube cube3( "1-1" );

  dc_view_cdc dc_ntk1( ntk, cdc );
  CHECK( dc_ntk1.pattern_is_EXCDC( pat1 ) );
  CHECK( !dc_ntk1.pattern_is_EXCDC( pat2 ) );

  dc_view_odc dc_ntk2( ntk );
  dc_ntk2.add_EXODC( cube1, 2 );
  /* The third PO (f) is EXODC whenever the first PO (t1) is 1 */
  CHECK( dc_ntk2.are_observably_equivalent( pat1, pat2 ) );
  CHECK( dc_ntk2.are_observably_equivalent( cube2, cube3 ) );

  dc_view_both dc_ntk3( ntk, cdc );
  dc_ntk3.add_EXODC( cube1, 2 );
  dc_ntk3.add_EXOEC_pair( pat1, pat3 );
  /* 100 = 101 and 110 = 111 by EXODC, 000 = 111 by EXOEC, thus 110 = 000 by transitivity */
  CHECK( dc_ntk3.are_observably_equivalent( pat1, pat2 ) );
  CHECK( dc_ntk3.are_observably_equivalent( pat2, pat3 ) );
  CHECK( dc_ntk3.are_observably_equivalent( cube2, cube3 ) );
  CHECK( dc_ntk3.pattern_is_EXCDC( pat1 ) );
  CHECK( !dc_ntk3.pattern_is_EXCDC( pat2 ) );
}

TEST_CASE( "create dont_care_view and test API", "[dont_care_view]" )
{
  test_create_dont_care_view<aig_network>();
  test_create_dont_care_view<mig_network>();
  test_create_dont_care_view<xag_network>();
  test_create_dont_care_view<xmg_network>();
  test_create_dont_care_view<klut_network>();
}

template<typename Ntk>
void test_optimize_with_EXDC()
{
  using dc_view = dont_care_view<Ntk, true, true>;

  Ntk ntk;
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const n1 = ntk.create_and( a, !b );
  auto const n2 = ntk.create_and( !a, b );
  auto const n3 = ntk.create_and( b, !c );
  auto const n4 = ntk.create_and( !n1, !n3 );
  auto const n5 = ntk.create_and( !n2, !c );
  ntk.create_po( n1 );
  ntk.create_po( c );
  ntk.create_po( n4 );
  ntk.create_po( n5 );

  Ntk ntk_ori = ntk.clone();

  /* The input pattern 110 is EXCDC */
  Ntk cdc;
  auto const a_ = cdc.create_pi();
  auto const b_ = cdc.create_pi();
  auto const c_ = cdc.create_pi();
  cdc.create_po( cdc.create_and( cdc.create_and( a_, b_ ), !c_ ) );

  /* The last PO is EXODC whenever the first or the second PO is 1 */
  dc_view exdc( ntk, cdc );
  exdc.add_EXODC( kitty::cube( "1---" ), 3 );
  exdc.add_EXODC( kitty::cube( "-1--" ), 3 );

  /* Optimize with sim_resub */
  resubstitution_params ps;
  ps.odc_levels = -1;
  sim_resubstitution( exdc, ps );
  ntk = cleanup_dangling( ntk );

  CHECK( ntk.num_gates() < 5 );
  CHECK( *equivalence_checking_bill( dc_view{*miter<Ntk>( exdc, ntk_ori ), cdc} ) );

  /* After cleanup_dangling, dont_care_view needs to be re-wrapped */
  dc_view exdc2( ntk, cdc );
  exdc2.add_EXODC( kitty::cube( "1---" ), 3 );
  exdc2.add_EXODC( kitty::cube( "-1--" ), 3 );
  /* This better optimization can't be found by the current version of sim_resub, but should be valid */
  fanout_view fanout{exdc2};
  circuit_validator<fanout_view<dc_view>, bill::solvers::bsat2, false, true, true> val( fanout, { ps.max_clauses, ps.odc_levels, ps.conflict_limit, ps.random_seed } );
  if ( *( val.validate( ntk.get_node( ntk.po_at( 3 ) ), ntk.po_at( 2 ) ) ) )
  {
    ntk.substitute_node( ntk.get_node( ntk.po_at( 3 ) ), ntk.po_at( 2 ) );
  }
  ntk = cleanup_dangling( ntk );

  CHECK( ntk.num_gates() == 3 );
  CHECK( *equivalence_checking_bill( dc_view{*miter<Ntk>( exdc, ntk_ori ), cdc} ) );
}

TEST_CASE( "optimize with external don't cares", "[dont_care_view]" )
{
  test_optimize_with_EXDC<aig_network>();
  test_optimize_with_EXDC<xag_network>();
}
