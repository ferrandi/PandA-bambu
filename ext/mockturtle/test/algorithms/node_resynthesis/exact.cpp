#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Exact AIG for MAJ", "[exact]" )
{
  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  std::vector<aig_network::signal> pis = {a, b, c};

  exact_aig_resynthesis<aig_network> resyn;
  resyn( aig, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
    aig.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( aig.num_pos() == 1u );
  CHECK( aig.num_gates() == 4u );
  CHECK( simulate<kitty::dynamic_truth_table>( aig, sim )[0] == maj );
}

TEST_CASE( "Exact XAG for MAJ", "[exact]" )
{
  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  std::vector<xag_network::signal> pis = {a, b, c};

  exact_aig_resynthesis<xag_network> resyn;
  resyn( xag, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
    xag.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( xag.num_pos() == 1u );
  CHECK( xag.num_gates() == 4u );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == maj );
}

TEST_CASE( "Exact XMG for MAJ", "[exact]" )
{
  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  std::vector<xmg_network::signal> pis = {a, b, c};

  exact_xmg_resynthesis<xmg_network> resyn;
  resyn( xmg, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
    xmg.create_po( f );
    return false;
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( xmg.num_pos() == 1u );
  CHECK( xmg.num_gates() == 1u );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == maj );
}

TEST_CASE( "Exact AIG for XOR", "[exact]" )
{
  kitty::dynamic_truth_table _xor( 2u );
  kitty::create_from_hex_string( _xor, "6" );

  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  std::vector<xag_network::signal> pis = {a, b};

  exact_aig_resynthesis<xag_network> resyn( false );
  resyn( xag, _xor, pis.begin(), pis.end(), [&]( auto const& f ) {
    xag.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 2u );
  CHECK( xag.num_pos() == 1u );
  CHECK( xag.num_gates() == 3u );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == _xor );
}

TEST_CASE( "Exact XAG for XOR", "[exact]" )
{
  kitty::dynamic_truth_table _xor( 2u );
  kitty::create_from_hex_string( _xor, "6" );

  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  std::vector<xag_network::signal> pis = {a, b};

  exact_aig_resynthesis<xag_network> resyn( true );
  resyn( xag, _xor, pis.begin(), pis.end(), [&]( auto const& f ) {
    xag.create_po( f );
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 2u );
  CHECK( xag.num_pos() == 1u );
  CHECK( xag.num_gates() == 1u );
  CHECK( simulate<kitty::dynamic_truth_table>( xag, sim )[0] == _xor );
}

TEST_CASE( "Exact XMG for XOR2", "[exact]" )
{
  kitty::dynamic_truth_table _xor( 2u );
  kitty::create_from_hex_string( _xor, "6" );

  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();

  std::vector<xmg_network::signal> pis = {a, b};

  exact_xmg_resynthesis<xmg_network> resyn;
  resyn( xmg, _xor, pis.begin(), pis.end(), [&]( auto const& f ) {
    xmg.create_po( f );
    return false;
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 2u );
  CHECK( xmg.num_pos() == 1u );
  CHECK( xmg.num_gates() == 1u );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == _xor );
}

TEST_CASE( "Exact XMG for XOR3", "[exact]" )
{
  kitty::dynamic_truth_table _xor( 3u );
  kitty::create_from_hex_string( _xor, "96" );

  xmg_network xmg;
  const auto a = xmg.create_pi();
  const auto b = xmg.create_pi();
  const auto c = xmg.create_pi();

  std::vector<xmg_network::signal> pis = {a, b, c};

  exact_xmg_resynthesis<xmg_network> resyn;
  resyn( xmg, _xor, pis.begin(), pis.end(), [&]( auto const& f ) {
    xmg.create_po( f );
    return false;
  } );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( xmg.num_pos() == 1u );
  CHECK( xmg.num_gates() == 1u );
  CHECK( simulate<kitty::dynamic_truth_table>( xmg, sim )[0] == _xor );
}
