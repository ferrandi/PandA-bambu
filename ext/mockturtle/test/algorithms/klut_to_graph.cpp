#include <catch.hpp>
#include <iostream>
#include <mockturtle/algorithms/klut_to_graph.hpp>
using namespace mockturtle;

/* NOTATION
 * AIG   : And Inverter Graph
 * XAG   : Xor And Graph
 * MIG   : Majority Inverter Graph
 * XMG   : Xor Majority Graph
 * DSD-R : Disjoint Support Decomposition based Resynthesis
 * SD-R  : Shannon Decomposition based Resynthesis
 * NPN-R : NPN based Resynthesis
 */

// Fully disjoint support decomposable functions
TEST_CASE( "AIG, XAG, MIG, XMG: DSD-R only ", "[klut_to_graph]" )
{
  kitty::dynamic_truth_table table( 6u );
  kitty::create_from_expression( table, "{(((ab)(cd))(ef))}" );

  klut_network kLUT_ntk;
  aig_network aig;
  xag_network xag;
  mig_network mig;
  xmg_network xmg;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();
  const auto x6 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5, x6 }, fn ) );

  aig = convert_klut_to_graph<aig_network>( kLUT_ntk );
  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );
  mig = convert_klut_to_graph<mig_network>( kLUT_ntk );
  xmg = convert_klut_to_graph<xmg_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( mig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == table );
}

// Partially decomposable functions. At a certain point the convert_klut_to_graph uses NPN-R
TEST_CASE( "AIG, XAG, MIG, XMG: DSD-R -fallback-> NPN-R", "[klut_to_graph]" )
{
  kitty::dynamic_truth_table table( 6u );
  kitty::create_from_expression( table, "((ab){((cd)(ef))((!c!d)(!e!f))})" );

  klut_network kLUT_ntk;
  aig_network aig;
  xag_network xag;
  mig_network mig;
  xmg_network xmg;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();
  const auto x6 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5, x6 }, fn ) );

  aig = convert_klut_to_graph<aig_network>( kLUT_ntk );
  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );
  mig = convert_klut_to_graph<mig_network>( kLUT_ntk );
  xmg = convert_klut_to_graph<xmg_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( mig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == table );
}

// No disjoint support. Start with Shannon Decomposition and then fallback to NPN based resynthesis
TEST_CASE( "AIG, XAG, MIG, XMG: SD-R -fallback-> NPN-R", "[klut_to_graph]" )
{
  kitty::dynamic_truth_table table( 5u );
  kitty::create_from_expression( table, "{(a((bc)(de)))(!a((!b!c)(!d!e)))}" );

  klut_network kLUT_ntk;
  aig_network aig;
  xag_network xag;
  mig_network mig;
  xmg_network xmg;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5 }, fn ) );

  aig = convert_klut_to_graph<aig_network>( kLUT_ntk );
  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );
  mig = convert_klut_to_graph<mig_network>( kLUT_ntk );
  xmg = convert_klut_to_graph<xmg_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( mig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == table );
}

// Combine all of the three resynthesis functions
TEST_CASE( "AIG, XAG, MIG, XMG: DSD-R -fallback-> SD-R -fallback-> NPN-R", "[klut_to_graph]" )
{

  kitty::dynamic_truth_table table( 6u );
  kitty::create_from_expression( table, "({(a((bc)(de)))(!a((!b!c)(!d!e)))}f)" );

  klut_network kLUT_ntk;
  aig_network aig;
  xag_network xag;
  mig_network mig;
  xmg_network xmg;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();
  const auto x6 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5, x6 }, fn ) );

  aig = convert_klut_to_graph<aig_network>( kLUT_ntk );
  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );
  mig = convert_klut_to_graph<mig_network>( kLUT_ntk );
  xmg = convert_klut_to_graph<xmg_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( mig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == table );
}

// Random 10 inputs LUT
TEST_CASE( "AIG,XAG,MIG,XMG: Random 10-LUT ", "[klut_to_graph]" )
{

  kitty::dynamic_truth_table table( 10u );
  kitty::create_random( table, 112358 );

  klut_network kLUT_ntk;
  aig_network aig;
  xag_network xag;
  mig_network mig;
  xmg_network xmg;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();
  const auto x6 = kLUT_ntk.create_pi();
  const auto x7 = kLUT_ntk.create_pi();
  const auto x8 = kLUT_ntk.create_pi();
  const auto x9 = kLUT_ntk.create_pi();
  const auto x10 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5, x6, x7, x8, x9, x10 }, fn ) );

  aig = convert_klut_to_graph<aig_network>( kLUT_ntk );
  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );
  mig = convert_klut_to_graph<mig_network>( kLUT_ntk );
  xmg = convert_klut_to_graph<xmg_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );

  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( mig, sim )[0] == table );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == table );
}

// additional test for XAG explicitly presenting a XAG
TEST_CASE( "XAG: DSD-R -fallback-> NPN-R", "[klut_to_graph]" )
{

  kitty::dynamic_truth_table table( 6u );
  kitty::create_from_expression( table, "((ab){{(ef)(cd)}([cd](!e!f))})" );

  klut_network kLUT_ntk;
  xag_network xag;

  const auto x1 = kLUT_ntk.create_pi();
  const auto x2 = kLUT_ntk.create_pi();
  const auto x3 = kLUT_ntk.create_pi();
  const auto x4 = kLUT_ntk.create_pi();
  const auto x5 = kLUT_ntk.create_pi();
  const auto x6 = kLUT_ntk.create_pi();

  auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
    return kLUT_ntk.create_node( children, remainder );
  };

  kLUT_ntk.create_po( dsd_decomposition( kLUT_ntk, table, { x1, x2, x3, x4, x5, x6 }, fn ) );

  xag = convert_klut_to_graph<xag_network>( kLUT_ntk );

  default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == table );
}
